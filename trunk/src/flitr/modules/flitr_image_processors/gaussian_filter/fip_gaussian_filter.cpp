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

#include <flitr/modules/flitr_image_processors/gaussian_filter/fip_gaussian_filter.h>


using namespace flitr;
using std::shared_ptr;

FIPGaussianFilter::FIPGaussianFilter(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                                     const float filterRadius,
                                     const size_t kernelWidth,
                                     uint32_t buffer_size) :
ImageProcessor(upStreamProducer, images_per_slot, buffer_size),
xFiltData_(nullptr),
kernel1D_(nullptr),
filterRadius_(filterRadius),
kernelWidth_(kernelWidth|1)//Make sure the kernel width is odd.
{
    //Setup image format being produced to downstream.
    for (uint32_t i=0; i<images_per_slot; i++) {
        ImageFormat downStreamFormat=upStreamProducer.getFormat();
        
        ImageFormat_.push_back(downStreamFormat);
    }
}

FIPGaussianFilter::~FIPGaussianFilter()
{
    delete [] xFiltData_;
    delete [] kernel1D_;
}

void FIPGaussianFilter::updateKernel1D()
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

void FIPGaussianFilter::setFilterRadius(const float filterRadius)
{
    filterRadius_=filterRadius;
    updateKernel1D();
}

void FIPGaussianFilter::setKernelWidth(const int kernelWidth)
{
    kernelWidth_=kernelWidth|1;
    updateKernel1D();
}

bool FIPGaussianFilter::init()
{
    bool rValue=ImageProcessor::init();
    //Note: SharedImageBuffer of downstream producer is initialised with storage in ImageProcessor::init.
    
    size_t maxXFiltDataSize=0;
    
    for (uint32_t i=0; i<ImagesPerSlot_; i++)
    {
        const ImageFormat imFormat=getUpstreamFormat(i);
        
        const size_t width=imFormat.getWidth();
        const size_t height=imFormat.getHeight();
        
        const size_t componentsPerPixel=imFormat.getComponentsPerPixel();
        
        const size_t xFiltDataSize = width * height * componentsPerPixel;
        
        if (xFiltDataSize>maxXFiltDataSize)
        {
            maxXFiltDataSize=xFiltDataSize;
        }
    }
    
    //Allocate a buffer big enough for any of the image slots.
    xFiltData_=new float[maxXFiltDataSize];
    
    updateKernel1D();
    
    return rValue;
}

bool FIPGaussianFilter::trigger()
{
    if ((getNumReadSlotsAvailable())&&(getNumWriteSlotsAvailable()))
    {
        std::vector<Image**> imvRead=reserveReadSlot();
        
        std::vector<Image**> imvWrite=reserveWriteSlot();
        
        //Start stats measurement event.
        ProcessorStats_->tick();
        
        for (size_t imgNum=0; imgNum<ImagesPerSlot_; ++imgNum)
        {
            Image const * const imReadUS = *(imvRead[imgNum]);
            Image * const imWriteDS = *(imvWrite[imgNum]);
            
            const ImageFormat imFormat=getDownstreamFormat(imgNum);//down stream and up stream formats are the same.
            
            const size_t width=imFormat.getWidth();
            const size_t height=imFormat.getHeight();
            
            const size_t widthMinusKernel=imFormat.getWidth()-(kernelWidth_-1);
            const size_t heightMinusKernel=imFormat.getHeight()-(kernelWidth_-1);
            
            const size_t halfKernelWidth=(kernelWidth_>>1);//Always even!
            const size_t widthMinusHalfKernel=imFormat.getWidth()-halfKernelWidth;
            
            if (imFormat.getPixelFormat()==ImageFormat::FLITR_PIX_FMT_Y_F32)
            {
                float const * const dataReadUS=(float const * const)imReadUS->data();
                float * const dataWriteDS=(float * const)imWriteDS->data();
                
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
                        
                        xFiltData_[lineOffsetFS + x]=xFiltValue;
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
                            filtValue += xFiltData_[(lineOffsetFS + x) + j*width] * kernel1D_[j];
                            //Performance note: The XCode profiler shows that the above multiply by width has little performance overhead compared to the loop in x above.
                            //                  Is the code memory bandwidth limited?
                        }
                        
                        dataWriteDS[lineOffsetDS + x]=filtValue;
                    }
                }
            }
        }
        
        //Stop stats measurement event.
        ProcessorStats_->tock();
        
        releaseWriteSlot();
        releaseReadSlot();
        
        return true;
    }
    
    return false;
}

