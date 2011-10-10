/* Framework for Live Image Transformation (FLITr) 
 * Copyright (c) 2010 CSIR
 * 
 * This file is part of FLITr.
 *
 * FLITr is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 * 
 * FLITr is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with FLITr. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef CUDA_AUTO_CONTRAST_CU
#define CUDA_AUTO_CONTRAST_CU 1

#include "pitched_2d_range.h"

#include <thrust/device_vector.h>
#include <thrust/host_vector.h>
#include <thrust/sort.h>
#include <thrust/copy.h>
#include <thrust/binary_search.h>

__global__ 
void kernel_histeq(uchar4* trg, int trg_pitch,
                   unsigned char* src, int src_pitch,
                   unsigned int imageWidth, unsigned int imageHeight, 
                   float scale, 
                   int* histTbl)
{
    // compute thread dimension
    unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
    unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;
    
    if ( x < imageWidth && y < imageHeight )
    {
        unsigned int src_idx = x + (y * (src_pitch / sizeof(unsigned char)));
        unsigned int trg_idx = x + (y * (trg_pitch / sizeof(uchar4)));

        unsigned char in_pix = src[src_idx];
        unsigned char out_pix = 255 * ((float)(histTbl[in_pix]) / scale);
        
        trg[trg_idx] = make_uchar4(out_pix, out_pix, out_pix, 1);
    }
}

/// From count determine if we are in 2d bounds
struct in_2d_bound
{
    const int width;
    const int padded_width;
    const bool no_padding;

    in_2d_bound(int w, int pw) :
        width(w), padded_width(pw), no_padding(width == padded_width) {}

    __host__ __device__
    bool operator()(const int x)
    {
        if (no_padding) return true;

        return (x % padded_width) < width;
    }
};

/// Functor to do contrast stretch
struct contrast_functor
{
    const int min;
    const float scale;

    contrast_functor(int im_min, int im_scale) : 
        min(im_min),
        scale(im_scale) {}

    __host__ __device__
    uchar4 operator()(const unsigned char& pix) const {
        unsigned char new_pix = ((float)(pix - min) / (float)scale) * 255;
        return make_uchar4(new_pix, new_pix, new_pix, 1);
    }
};

///////////////////////////////////////////////////////////////////////////////////////////
// HOST FUNCTIONS /////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
extern "C"
void cu_histeq(const dim3& blocks, const dim3& threads, 
               void* trgBuffer, int trgPitch, 
               void* srcBuffer, int srcPitch,
               unsigned int imageWidth, unsigned int imageHeight)
{
    thrust::device_ptr<unsigned char> src_data( reinterpret_cast<unsigned char*>(srcBuffer) );
    
    int src_padded_size = srcPitch/sizeof(unsigned char) * imageHeight;
    int src_size = imageWidth * imageHeight;
    
    //thrust::sort(src_data, src_data+src_padded_size);
    // A copy is needed to in-place sort, because we still need the unsorted later
    // we must cater for padded 2d input data, but pack it tightly for sort
    thrust::device_vector<unsigned char> src_data_copy(src_size);
    in_2d_bound copy_pred(imageWidth, srcPitch/sizeof(unsigned char));
    thrust::copy_if(src_data, src_data+src_padded_size, 
                    thrust::counting_iterator<int>(0),
                    src_data_copy.begin(),
                    copy_pred);
    
    thrust::sort(src_data_copy.begin(), src_data_copy.end());

    const int num_bins = 256;
    thrust::device_vector<int> d_cumulative_histogram(num_bins);
    
    // find the end of each bin of values
    thrust::counting_iterator<int> search_begin(0);
    thrust::upper_bound(src_data_copy.begin(),
                        src_data_copy.end(),
                        search_begin,
                        search_begin + num_bins,
                        d_cumulative_histogram.begin());
  
    int* histogram_ptr = thrust::raw_pointer_cast(&d_cumulative_histogram[0]);
    kernel_histeq<<< blocks, threads >>>( 
        reinterpret_cast<uchar4*>(trgBuffer), trgPitch,
        reinterpret_cast<unsigned char*>(srcBuffer), srcPitch,
        imageWidth, imageHeight, 
        imageWidth*imageHeight, 
        histogram_ptr
        );
}

extern "C"
void cu_contrast_stretch(const dim3& blocks, const dim3& threads, 
                         void* trgBuffer, int trgPitch, 
                         void* srcBuffer, int srcPitch,
                         unsigned int imageWidth, unsigned int imageHeight)
{
    thrust::device_ptr<unsigned char> s_data( reinterpret_cast<unsigned char*>(srcBuffer) );
    thrust::device_ptr<uchar4> t_data( reinterpret_cast<uchar4*>(trgBuffer) );

    int src_padded_size = srcPitch/sizeof(unsigned char) * imageHeight;
    int trg_padded_size = trgPitch/sizeof(uchar4) * imageHeight;

    // create iterators to skip padded elements
    typedef thrust::device_vector<unsigned char>::iterator src_it_type;
    pitched_2d_range<src_it_type> 
        src_it(s_data, s_data + src_padded_size, 
               imageWidth, srcPitch/sizeof(unsigned char));

    typedef thrust::device_vector<uchar4>::iterator trg_it_type;
    pitched_2d_range<trg_it_type> 
        trg_it(t_data, t_data + trg_padded_size, imageWidth, trgPitch/sizeof(uchar4));

    thrust::pair<pitched_2d_range<src_it_type>::iterator, 
                 pitched_2d_range<src_it_type>::iterator> minmax = thrust::minmax_element(src_it.begin(), src_it.end());
    unsigned char immin = *minmax.first;
    unsigned char immax = *minmax.second;
    int scale = immax - immin;
    
    contrast_functor f(immin, scale);

    thrust::transform(src_it.begin(), src_it.end(), trg_it.begin(), f);
}


#endif // CUDA_AUTO_CONTRAST_CU
