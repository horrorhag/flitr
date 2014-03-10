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

#include <flitr/modules/flitr_image_processors/gradient_image/fip_gradient_image.h>

#ifdef _OPENMP
#include <omp.h>
#endif

using namespace flitr;
using std::tr1::shared_ptr;

FIPGradientXImage::FIPGradientXImage(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                                     uint32_t buffer_size) :
ImageProcessor(upStreamProducer, images_per_slot, buffer_size)
{
    
    //Setup image format being produced to downstream.
    for (uint32_t i=0; i<images_per_slot; i++) {
        ImageFormat_.push_back(upStreamProducer.getFormat(i));//Output format is same as input format.
    }
    
}

FIPGradientXImage::~FIPGradientXImage()
{
}

bool FIPGradientXImage::init()
{
    bool rValue=ImageProcessor::init();
    //Note: SharedImageBuffer of downstream producer is initialised with storage in ImageProcessor::init.
    
    size_t maxHeight=0;
    
    for (uint32_t i=0; i<ImagesPerSlot_; i++)
    {
        const ImageFormat imFormat=getUpstreamFormat(i);//Downstream format is same as upstream format.
        //const ImageFormat::PixelFormat pixelFormat=imFormat.getPixelFormat();
        
        //const size_t width=imFormat.getWidth();
        const size_t height=imFormat.getHeight();
        
        //const size_t componentsPerPixel=imFormat.getComponentsPerPixel();
        //const size_t bytesPerPixel=imFormat.getBytesPerPixel();
        
        if (height>maxHeight)
        {
            maxHeight=height;
        }
    }
    
    return rValue;
}

bool FIPGradientXImage::trigger()
{
    if ((getNumReadSlotsAvailable())&&(getNumWriteSlotsAvailable()))
    {//There are images to consume and the downstream producer has space to produce.
        std::vector<Image**> imvRead=reserveReadSlot();
        std::vector<Image**> imvWrite=reserveWriteSlot();
        
        //Start stats measurement event.
        ProcessorStats_->tick();
        
        for (size_t imgNum=0; imgNum<ImagesPerSlot_; imgNum++)
        {
            Image const * const imRead = *(imvRead[imgNum]);
            Image * const imWrite = *(imvWrite[imgNum]);
            
            float const * const dataRead=(float const * const)imRead->data();
            float * const dataWrite=(float * const )imWrite->data();
            
            const ImageFormat imFormat=getUpstreamFormat(imgNum);//Downstream format is same as upstream format.
            
            const ssize_t width=imFormat.getWidth();
            const ssize_t height=imFormat.getHeight();
            
            ssize_t y=0;
            {
#ifdef _OPENMP
                //omp_set_num_threads(4);//There might be some blocking operation or mem bandwidth limited code??? because the parallel for seems to work best with much more threads than CPU cores.
#endif
                
#ifdef _OPENMP
#pragma omp parallel private(y)
#endif
                {
#ifdef _OPENMP
#pragma omp for nowait
#endif
                    for (y=0; y<height; y++)
                    {
                        const ssize_t lineOffset=y * width;
                        
                        for (ssize_t x=1; x<width; x++)
                        {
                            const ssize_t offset=lineOffset + x;
                            
                            const float v1=dataRead[offset-width-1];
                            const float v3=dataRead[offset-width+1];
                            const float v4=dataRead[offset-1];
                            const float v6=dataRead[offset+1];
                            const float v7=dataRead[offset+width-1];
                            const float v9=dataRead[offset+width+1];
                            
                            //Use Scharr operator for image gradient.
                            dataWrite[offset]=(v3-v1)*(3.0f/32.0f) + (v6-v4)*(10.0f/32.0f) + (v9-v7)*(3.0f/32.0f);
                        }
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



FIPGradientYImage::FIPGradientYImage(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                                     uint32_t buffer_size) :
ImageProcessor(upStreamProducer, images_per_slot, buffer_size)
{
    
    //Setup image format being produced to downstream.
    for (uint32_t i=0; i<images_per_slot; i++) {
        ImageFormat_.push_back(upStreamProducer.getFormat(i));//Output format is same as input format.
    }
    
}

FIPGradientYImage::~FIPGradientYImage()
{
}

bool FIPGradientYImage::init()
{
    bool rValue=ImageProcessor::init();
    //Note: SharedImageBuffer of downstream producer is initialised with storage in ImageProcessor::init.
    
    size_t maxHeight=0;
    
    for (uint32_t i=0; i<ImagesPerSlot_; i++)
    {
        const ImageFormat imFormat=getUpstreamFormat(i);//Downstream format is same as upstream format.
        //const ImageFormat::PixelFormat pixelFormat=imFormat.getPixelFormat();
        
        //const size_t width=imFormat.getWidth();
        const size_t height=imFormat.getHeight();
        
        //const size_t componentsPerPixel=imFormat.getComponentsPerPixel();
        //const size_t bytesPerPixel=imFormat.getBytesPerPixel();
        
        if (height>maxHeight)
        {
            maxHeight=height;
        }
    }
    
    return rValue;
}

bool FIPGradientYImage::trigger()
{
    if ((getNumReadSlotsAvailable())&&(getNumWriteSlotsAvailable()))
    {//There are images to consume and the downstream producer has space to produce.
        std::vector<Image**> imvRead=reserveReadSlot();
        std::vector<Image**> imvWrite=reserveWriteSlot();
        
        //Start stats measurement event.
        ProcessorStats_->tick();
        
        for (size_t imgNum=0; imgNum<ImagesPerSlot_; imgNum++)
        {
            Image const * const imRead = *(imvRead[imgNum]);
            Image * const imWrite = *(imvWrite[imgNum]);
            
            float const * const dataRead=(float const * const)imRead->data();
            float * const dataWrite=(float * const )imWrite->data();
            
            const ImageFormat imFormat=getUpstreamFormat(imgNum);//Downstream format is same as upstream format.
            
            const ssize_t width=imFormat.getWidth();
            const ssize_t height=imFormat.getHeight();
            
            ssize_t y=0;
            {
#ifdef _OPENMP
                //omp_set_num_threads(4);//There might be some blocking operation or mem bandwidth limited code??? because the parallel for seems to work best with much more threads than CPU cores.
#endif
                
#ifdef _OPENMP
#pragma omp parallel private(y)
#endif
                {
#ifdef _OPENMP
#pragma omp for nowait
#endif
                    for (y=1; y<height; y++)
                    {
                        const ssize_t lineOffset=y * width;
                        
                        for (size_t x=0; x<width; x++)
                        {
                            const ssize_t offset=lineOffset + x;
                            
                            const float v1=dataRead[offset-width-1];
                            const float v2=dataRead[offset-width];
                            const float v3=dataRead[offset-width+1];
                            const float v7=dataRead[offset+width-1];
                            const float v8=dataRead[offset+width];
                            const float v9=dataRead[offset+width+1];
                            
                            //Use Scharr operator for image gradient.
                            dataWrite[offset]=(v7-v1)*(3.0f/32.0f) + (v8-v2)*(10.0f/32.0f) + (v9-v3)*(3.0f/32.0f);
                        }
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

