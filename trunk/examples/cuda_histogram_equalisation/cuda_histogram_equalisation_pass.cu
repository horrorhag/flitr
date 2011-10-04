#ifndef CUDA_HISTOGRAM_EQUALIZATION_CU
#define CUDA_HISTOGRAM_EQUALIZATION_CU 1

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

///////////////////////////////////////////////////////////////////////////////////////////
// HOST FUNCTIONS /////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
extern "C"
void cu_histeq(const dim3& blocks, const dim3& threads, 
               void* trgBuffer, void* srcArray,
               unsigned int imageWidth, unsigned int imageHeight)
{
    thrust::device_ptr<unsigned char> d_data( reinterpret_cast<unsigned char*>(srcArray) );
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
        reinterpret_cast<unsigned char*>(srcArray));
}

#endif // CUDA_HISTOGRAM_EQUALIZATION_CU
