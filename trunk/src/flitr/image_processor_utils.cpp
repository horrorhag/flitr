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

#define _USE_MATH_DEFINES
#include <cmath>

#include <flitr/image_processor_utils.h>
#include <sstream>

using namespace flitr;
using std::shared_ptr;

//=========== GaussianFilter ==========//

GaussianFilter::GaussianFilter(const float filterRadius,
                               const size_t kernelWidth) :
kernel1D_(nullptr),
filterRadius_(filterRadius),
kernelWidth_(kernelWidth|1)//Make sure the kernel width is odd.
{
    updateKernel1D();
}

GaussianFilter::~GaussianFilter()
{
    delete [] kernel1D_;
}

void GaussianFilter::updateKernel1D()
{
    if (kernel1D_!=nullptr) delete [] kernel1D_;
    
    kernel1D_=new float[kernelWidth_];
    
    const float kernelCentre=kernelWidth_ * 0.5f;
    const float sigma=filterRadius_ * 0.5f;
    
    float kernelSum=0.0f;
    
    for (size_t i=0; i<kernelWidth_; ++i)
    {
        const float r=((i + 0.5f) - kernelCentre);
        kernel1D_[i]=(1.0f/(sqrt(2.0f*float(M_PI))*sigma)) * exp(-(r*r)/(2.0f*sigma*sigma));
        kernelSum+=kernel1D_[i];
    }
    
    //=== Ensure that the kernel is normalised!
    for (size_t i=0; i<kernelWidth_; ++i)
    {
        kernel1D_[i]/=kernelSum;
    }
}

void GaussianFilter::setFilterRadius(const float filterRadius)
{
    filterRadius_=filterRadius;
    updateKernel1D();
}

void GaussianFilter::setKernelWidth(const int kernelWidth)
{
    kernelWidth_=kernelWidth|1;
    updateKernel1D();
}

bool GaussianFilter::filter(float * const dataWriteDS, float const * const dataReadUS,
                            const size_t width, const size_t height,
                            float * const dataScratch)
{
    const size_t widthMinusKernel=width-(kernelWidth_-1);
    const size_t heightMinusKernel=height-(kernelWidth_-1);
    
    const size_t halfKernelWidth=(kernelWidth_>>1);
    const size_t widthMinusHalfKernel=width-halfKernelWidth;
    
    //if (pixelFormat==ImageFormat::FLITR_PIX_FMT_Y_F32)
    {
        for (size_t y=0; y<height; ++y)
        {
            const size_t lineOffsetFS=y * width + halfKernelWidth;
            const size_t lineOffsetUS=y * width;
            
            for (size_t x=0; x<widthMinusKernel; ++x)
            {
                float xFiltValue=0.0f;
                
                for (size_t j=0; j<kernelWidth_; ++j)
                {
                    xFiltValue += dataReadUS[(lineOffsetUS + x) + j] * kernel1D_[j];
                }
                
                dataScratch[lineOffsetFS + x]=xFiltValue;
            }
        }
        
        for (size_t y=0; y<heightMinusKernel; ++y)
        {
            const size_t lineOffsetDS=(y + halfKernelWidth) * width;
            const size_t lineOffsetFS=y * width;
            
            for (size_t x=halfKernelWidth; x<widthMinusHalfKernel; ++x)
            {
                float filtValue=0.0f;
                
                for (size_t j=0; j<kernelWidth_; ++j)
                {
                    filtValue += dataScratch[(lineOffsetFS + x) + j*width] * kernel1D_[j];
                    //Performance note: The XCode profiler shows that the above multiply by width has little performance overhead compared to the loop in x above.
                    //                  Is the code memory bandwidth limited?
                }
                
                dataWriteDS[lineOffsetDS + x]=filtValue;
            }
        }
    }
    
    return true;
}

//=========================================//



//=========== GaussianDownsample ==========//
GaussianDownsample::GaussianDownsample(const float filterRadius,
                                       const size_t kernelWidth) :
kernel1D_(nullptr),
filterRadius_(filterRadius),
kernelWidth_((kernelWidth>>1)<<1)//Make sure the kernel width is even.
{
    updateKernel1D();
}

GaussianDownsample::~GaussianDownsample()
{
    delete [] kernel1D_;
}

void GaussianDownsample::updateKernel1D()
{
    if (kernel1D_!=nullptr) delete [] kernel1D_;
    
    kernel1D_=new float[kernelWidth_];
    
    const float kernelCentre=kernelWidth_ * 0.5f;
    const float sigma=filterRadius_ * 0.5f;
    
    float kernelSum=0.0f;
    
    for (size_t i=0; i<kernelWidth_; ++i)
    {
        const float r=((i + 0.5f) - kernelCentre);
        kernel1D_[i]=(1.0f/(sqrt(2.0f*float(M_PI))*sigma)) * exp(-(r*r)/(2.0f*sigma*sigma));
        kernelSum+=kernel1D_[i];
    }
    
    //=== Ensure that the kernel is normalised!
    for (size_t i=0; i<kernelWidth_; ++i)
    {
        kernel1D_[i]/=kernelSum;
    }
}

void GaussianDownsample::setFilterRadius(const float filterRadius)
{
    filterRadius_=filterRadius;
    updateKernel1D();
}

void GaussianDownsample::setKernelWidth(const int kernelWidth)
{
    kernelWidth_=(kernelWidth>>1)<<1;
    updateKernel1D();
}

bool GaussianDownsample::downsample(float * const dataWriteUS, float const * const dataReadUS,
                                    const size_t widthUS, const size_t heightUS,
                                    float * const dataScratch)
{
    const size_t widthDS=widthUS>>1;
    //const size_t heightDS=widthUS>>1;
    
    const size_t widthUSMinusKernel=widthUS-(kernelWidth_-1);
    const size_t heightUSMinusKernel=heightUS-(kernelWidth_-1);
    
    const size_t halfKernelWidth=(kernelWidth_>>1);//kernelWidth_ is even!
    
    //if (pixelFormat==ImageFormat::FLITR_PIX_FMT_Y_F32)
    {
        for (size_t y=0; y<heightUS; ++y)
        {
            const size_t lineOffsetFS=y * widthDS + (halfKernelWidth>>1);
            const size_t lineOffsetUS=y * widthUS;
            
            for (size_t xUS=0; xUS<widthUSMinusKernel; xUS+=2)
            {
                float xFiltValue=0.0f;
                
                for (size_t j=0; j<kernelWidth_; ++j)
                {
                    xFiltValue+=dataReadUS[(lineOffsetUS + xUS) + j] * kernel1D_[j];
                }
                
                dataScratch[lineOffsetFS + (xUS>>1)]=xFiltValue;
            }
        }
        
        for (size_t y=0; y<heightUSMinusKernel; y+=2)
        {
            const size_t lineOffsetDS=((y>>1) + (halfKernelWidth>>1)) * widthDS;
            const size_t lineOffsetFS=y * widthDS;
            
            for (size_t xFS=0; xFS<widthDS; ++xFS)
            {
                float filtValue=0.0f;
                
                for (size_t j=0; j<kernelWidth_; ++j)
                {
                    filtValue+=dataScratch[(lineOffsetFS + xFS) + j*widthDS] * kernel1D_[j];
                    //Performance note: The XCode profiler shows that the above multiply by width has little performance overhead compared to the loop in x above.
                    //                  Is the code memory bandwidth limited?
                }
                
                dataWriteUS[lineOffsetDS + xFS]=filtValue;
            }
        }
    }
    
    return true;
}
//=========================================//



