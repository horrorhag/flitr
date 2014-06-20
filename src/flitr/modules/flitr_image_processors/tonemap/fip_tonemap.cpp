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

#include <flitr/modules/flitr_image_processors/tonemap/fip_tonemap.h>

using namespace flitr;
using std::shared_ptr;

FIPTonemap::FIPTonemap(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                       float power,
                       uint32_t buffer_size) :
ImageProcessor(upStreamProducer, images_per_slot, buffer_size),
power_(power)
{
    
    //Setup image format being produced to downstream.
    for (uint32_t i=0; i<images_per_slot; i++) {
        ImageFormat_.push_back(upStreamProducer.getFormat(i));//Output format is same as input format.
    }
    
}

FIPTonemap::~FIPTonemap()
{
}

bool FIPTonemap::init()
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

bool FIPTonemap::trigger()
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
            
            const size_t width=imFormat.getWidth();
            const size_t height=imFormat.getHeight();
            
            size_t y=0;
            {
                {
                    for (y=0; y<height; ++y)
                    {
                        const size_t lineOffset=y * width;
                        
                        for (size_t x=1; x<width; ++x)
                        {
                            dataWrite[lineOffset + x]=powf(dataRead[lineOffset + x], power_);
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



