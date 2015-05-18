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

#include <flitr/modules/flitr_image_processors/median/fip_median.h>

#include <iostream>

using namespace flitr;
using std::shared_ptr;

FIPMedian::FIPMedian(ImageProducer& upStreamProducer, uint32_t filterSize,
                     uint32_t images_per_slot,
                     uint32_t buffer_size) :
ImageProcessor(upStreamProducer, images_per_slot, buffer_size),
filterSize_(filterSize>=1 ? filterSize : 1)
{
    //Setup image format being produced to downstream.
    for (uint32_t i=0; i<images_per_slot; i++) {
        //ImageFormat(uint32_t w=0, uint32_t h=0, PixelFormat pix_fmt=FLITR_PIX_FMT_Y_8, bool flipV = false, bool flipH = false):
        ImageFormat downStreamFormat(upStreamProducer.getFormat().getWidth(), upStreamProducer.getFormat().getHeight(),
                                     upStreamProducer.getFormat().getPixelFormat());
        
        ImageFormat_.push_back(downStreamFormat);
    }
}

FIPMedian::~FIPMedian()
{
}

bool FIPMedian::init()
{
    bool rValue=ImageProcessor::init();
    //Note: SharedImageBuffer of downstream producer is initialised with storage in ImageProcessor::init.
    
    return rValue;
}

bool FIPMedian::trigger()
{
    if ((getNumReadSlotsAvailable())&&(getNumWriteSlotsAvailable()))
    {//There are images to consume and the downstream producer has space to produce.
        std::vector<Image**> imvRead=reserveReadSlot();
        std::vector<Image**> imvWrite=reserveWriteSlot();
        
        //Start stats measurement event.
        ProcessorStats_->tick();
        
        for (size_t imgNum=0; imgNum<1; ++imgNum)//For now, only process one image in each slot.
        {
            Image const * const imRead = *(imvRead[imgNum]);
            Image * const imWrite = *(imvWrite[imgNum]);
            
            uint8_t const * const dataRead=imRead->data();
            uint8_t * const dataWrite=imWrite->data();
            
            const ImageFormat imFormat=getDownstreamFormat(imgNum);
            
            const int32_t width=imFormat.getWidth();
            const int32_t height=imFormat.getHeight();
            
            memset(dataWrite, 0, width*height);//Clear the downstream image.
            
            const int32_t border=filterSize_-1;
            const int32_t widthMinusBorder=width - border;
            const int32_t heightMinusBorder=height - border;
            
            for (int32_t y=border; y<heightMinusBorder; ++y)
            {
                const int32_t lineOffset=y * width;
                
                for (int32_t x=border; x<widthMinusBorder; ++x)
                {
                    uint8_t minPixelValue;
                    uint8_t maxMinValue=0;
                    
                    //=== TopLeft ===//
                    minPixelValue=255;
                    for (int32_t dy=-border; dy<=0; ++dy)
                    {
                        for (int32_t dx=-border; dx<=0; ++dx)
                        {
                            const uint8_t value=dataRead[lineOffset + x + dx + dy*width];
                            
                            if (value<minPixelValue)
                            {
                                minPixelValue=value;
                            }
                        }
                    }
                    //=== ===//
                    
                    if (minPixelValue>maxMinValue)
                    {
                        maxMinValue=minPixelValue;
                    }
                    
                    //=== TopRight ===//
                    minPixelValue=255;
                    for (int32_t dy=-border; dy<=0; ++dy)
                    {
                        for (int32_t dx=0; dx<=border; ++dx)
                        {
                            const uint8_t value=dataRead[lineOffset + x + dx + dy*width];
                            
                            if (value<minPixelValue)
                            {
                                minPixelValue=value;
                            }
                        }
                    }
                    //=== ===//
                    
                    if (minPixelValue>maxMinValue)
                    {
                        maxMinValue=minPixelValue;
                    }
                    
                    //=== BottomLeft ===//
                    minPixelValue=255;
                    for (int32_t dy=0; dy<=border; ++dy)
                    {
                        for (int32_t dx=-border; dx<=0; ++dx)
                        {
                            const uint8_t value=dataRead[lineOffset + x + dx + dy*width];
                            
                            if (value<minPixelValue)
                            {
                                minPixelValue=value;
                            }
                        }
                    }
                    //=== ===//
                    
                    if (minPixelValue>maxMinValue)
                    {
                        maxMinValue=minPixelValue;
                    }
                    
                    //=== BottomRight ===//
                    minPixelValue=255;
                    for (int32_t dy=0; dy<=border; ++dy)
                    {
                        for (int32_t dx=0; dx<=border; ++dx)
                        {
                            const uint8_t value=dataRead[lineOffset + x + dx + dy*width];
                            
                            if (value<minPixelValue)
                            {
                                minPixelValue=value;
                            }
                        }
                    }
                    //=== ===//
                    
                    if (minPixelValue>maxMinValue)
                    {
                        maxMinValue=minPixelValue;
                    }

                    dataWrite[lineOffset + x]=maxMinValue;
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

