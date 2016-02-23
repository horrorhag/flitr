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
                                     const short intImgApprox,
                                     uint32_t buffer_size) :
ImageProcessor(upStreamProducer, images_per_slot, buffer_size),
intImgApprox_(intImgApprox),
scratchData_(nullptr),
intImageScratchData_(nullptr),
gaussianFilter_(filterRadius, kernelWidth),
boxFilter_(kernelWidth),
KernelWidth_(kernelWidth),
FilterRadius_(filterRadius),
Title_(std::string("Gaussian Filter"))
{
    //Setup image format being produced to downstream.
    for (uint32_t i=0; i<images_per_slot; i++) {
        ImageFormat downStreamFormat=upStreamProducer.getFormat();
        
        ImageFormat_.push_back(downStreamFormat);
    }
}

FIPGaussianFilter::~FIPGaussianFilter()
{
    delete [] scratchData_;
    delete [] intImageScratchData_;
}

void FIPGaussianFilter::setFilterRadius(const float filterRadius)
{
    gaussianFilter_.setFilterRadius(filterRadius);
}

void FIPGaussianFilter::setKernelWidth(const int kernelWidth)
{
    gaussianFilter_.setKernelWidth(kernelWidth);
    boxFilter_.setKernelWidth(kernelWidth);
}

bool FIPGaussianFilter::init()
{
    bool rValue=ImageProcessor::init();
    //Note: SharedImageBuffer of downstream producer is initialised with storage in ImageProcessor::init.
    
    size_t maxScratchDataSize=0;
    size_t maxScratchDataValues=0;
    
    for (uint32_t i=0; i<ImagesPerSlot_; i++)
    {
        const ImageFormat imFormat=getUpstreamFormat(i);
        
        const size_t width=imFormat.getWidth();
        const size_t height=imFormat.getHeight();
        
        const size_t bytesPerPixel=imFormat.getBytesPerPixel();
        const size_t componentsPerPixel=imFormat.getComponentsPerPixel();
        
        const size_t scratchDataSize = width * height * bytesPerPixel;
        const size_t scratchDataValues = width * height * componentsPerPixel;
        
        if (scratchDataSize>maxScratchDataSize)
        {
            maxScratchDataSize=scratchDataSize;
        }
        
        if (scratchDataValues>maxScratchDataValues)
        {
            maxScratchDataValues=scratchDataValues;
        }
    }
    
    //Allocate a buffer big enough for any of the image slots.
    scratchData_=new uint8_t[maxScratchDataSize];
    intImageScratchData_=new double[maxScratchDataValues];
    
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
            
            
            if (imFormat.getPixelFormat()==ImageFormat::FLITR_PIX_FMT_Y_F32)
            {
                float const * const dataReadUS=(float const * const)imReadUS->data();
                float * const dataWriteDS=(float * const)imWriteDS->data();
                
                if (intImgApprox_==0)
                {
                    gaussianFilter_.filter(dataWriteDS, dataReadUS, width, height, (float *)scratchData_);
                } else
                {
                    boxFilter_.filter(dataWriteDS, dataReadUS, width, height,
                                      intImageScratchData_, true);
                    
                    for (short i=1; i<intImgApprox_; ++i)
                    {
                        memcpy(scratchData_, dataWriteDS, width*height*sizeof(float));
                        
                        boxFilter_.filter(dataWriteDS, (float *)scratchData_, width, height,
                                          intImageScratchData_, true);
                    }
                }
            } else
                if (imFormat.getPixelFormat()==ImageFormat::FLITR_PIX_FMT_Y_8)
                {
                    uint8_t const * const dataReadUS=(uint8_t const * const)imReadUS->data();
                    uint8_t * const dataWriteDS=(uint8_t * const)imWriteDS->data();
                    
                    if (intImgApprox_==0)
                    {
                        gaussianFilter_.filter(dataWriteDS, dataReadUS, width, height, (uint8_t *)scratchData_);
                    } else
                    {
                        boxFilter_.filter(dataWriteDS, dataReadUS, width, height,
                                          intImageScratchData_, true);
                        
                        for (short i=1; i<intImgApprox_; ++i)
                        {
                            memcpy(scratchData_, dataWriteDS, width*height*sizeof(uint8_t));
                            
                            boxFilter_.filter(dataWriteDS, (uint8_t *)scratchData_, width, height,
                                              intImageScratchData_, true);
                        }
                    }
                } else
                if (imFormat.getPixelFormat()==ImageFormat::FLITR_PIX_FMT_RGB_F32)
                {
                    float const * const dataReadUS=(float const * const)imReadUS->data();
                    float * const dataWriteDS=(float * const)imWriteDS->data();
                    
                    if (intImgApprox_==0)
                    {
                        gaussianFilter_.filterRGB(dataWriteDS, dataReadUS, width, height, (float *)scratchData_);
                    } else
                    {
                        boxFilter_.filter(dataWriteDS, dataReadUS, width, height,
                                          intImageScratchData_, true);
                        
                        for (short i=1; i<intImgApprox_; ++i)
                        {
                            memcpy(scratchData_, dataWriteDS, width*height*sizeof(float)*3);
                            
                            boxFilter_.filterRGB(dataWriteDS, (float *)scratchData_, width, height,
                                                 intImageScratchData_, true);
                        }
                    }
                } else
                    if (imFormat.getPixelFormat()==ImageFormat::FLITR_PIX_FMT_RGB_8)
                    {
                        uint8_t const * const dataReadUS=(uint8_t const * const)imReadUS->data();
                        uint8_t * const dataWriteDS=(uint8_t * const)imWriteDS->data();
                        
                        if (intImgApprox_==0)
                        {
                            gaussianFilter_.filterRGB(dataWriteDS, dataReadUS, width, height, scratchData_);
                        } else
                        {
                            boxFilter_.filterRGB(dataWriteDS, dataReadUS, width, height,
                                                 intImageScratchData_, true);
                            
                            for (short i=1; i<intImgApprox_; ++i)
                            {
                                memcpy(scratchData_, dataWriteDS, width*height*sizeof(uint8_t)*3);
                                
                                boxFilter_.filterRGB(dataWriteDS, scratchData_, width, height,
                                                     intImageScratchData_, true);
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

