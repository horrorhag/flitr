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

#include <thrust/device_vector.h>
#include <thrust/host_vector.h>
#include <thrust/sort.h>
#include <thrust/copy.h>
#include <thrust/binary_search.h>

__global__ 
void kernel_histeq(uchar4* trg, 
                   unsigned int imageWidth, unsigned int imageHeight, float scale, 
                   int* histTbl, 
                   unsigned char* src)
{
    // compute thread dimension
    unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
    unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;
    
    if ( x < imageWidth && y < imageHeight )
    {
        unsigned int idx = x + y*imageWidth;
        unsigned char in_pix = src[idx];
        unsigned char out_pix = 255 * ((float)(histTbl[in_pix]) / scale);
        
        trg[idx] = make_uchar4(out_pix, out_pix, out_pix, 1);
    }
}

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
               void* trgBuffer, void* srcBuffer,
               unsigned int imageWidth, unsigned int imageHeight)
{
    thrust::device_ptr<unsigned char> d_data( reinterpret_cast<unsigned char*>(srcBuffer) );
    int sz = imageWidth*imageHeight;
    //thrust::sort(d_data, d_data+sz);
 
    // A copy is needed to in-place sort, because we still need the unsorted later
    thrust::device_vector<unsigned char> d_dataCpy(sz);
    thrust::copy(d_data, d_data+sz, d_dataCpy.begin());

    thrust::sort(d_dataCpy.begin(), d_dataCpy.end() );

    const int num_bins = 256;
    thrust::device_vector<int> d_cumulative_histogram(num_bins);
    
    // find the end of each bin of values
    thrust::counting_iterator<int> search_begin(0);
    thrust::upper_bound(d_dataCpy.begin(),
                        d_dataCpy.end(),
                        search_begin,
                        search_begin + num_bins,
                        d_cumulative_histogram.begin());
  
    int *d_ptr = thrust::raw_pointer_cast(&d_cumulative_histogram[0]);
    kernel_histeq<<< blocks, threads >>>( 
        reinterpret_cast<uchar4*>(trgBuffer), 
        imageWidth, imageHeight, imageWidth*imageHeight, 
        d_ptr, 
        reinterpret_cast<unsigned char*>(srcBuffer));
}

extern "C"
void cu_contrast_stretch(const dim3& blocks, const dim3& threads, 
                         void* trgBuffer, void* srcBuffer,
                         unsigned int imageWidth, unsigned int imageHeight)
{
    thrust::device_ptr<unsigned char> s_data( reinterpret_cast<unsigned char*>(srcBuffer) );
    thrust::device_ptr<uchar4> t_data( reinterpret_cast<uchar4*>(trgBuffer) );

    int sz = imageWidth*imageHeight;

    thrust::pair<thrust::device_ptr<unsigned char>, thrust::device_ptr<unsigned char> > minmax = thrust::minmax_element(s_data, s_data + sz);
    unsigned char immin = *minmax.first;
    unsigned char immax = *minmax.second;
    int scale = immax - immin;
    
    contrast_functor f(immin, scale);

    thrust::transform(s_data, s_data+sz, t_data, f);
}


#endif // CUDA_AUTO_CONTRAST_CU
