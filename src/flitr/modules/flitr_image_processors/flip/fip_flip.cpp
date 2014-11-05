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

#include <flitr/modules/flitr_image_processors/flip/fip_flip.h>

using namespace flitr;
using std::shared_ptr;

FIPFlip::FIPFlip(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                 const std::vector<bool> flipLeftRightVect,
                 const std::vector<bool> flipTopBottomVect,
                 uint32_t buffer_size) :
    ImageProcessor(upStreamProducer, images_per_slot, buffer_size),
    flipLeftRightVect_(flipLeftRightVect),
    flipTopBottomVect_(flipTopBottomVect)
{
    
    //Setup image format being produced to downstream.
    for (uint32_t i=0; i<images_per_slot; i++)
    {
        ImageFormat_.push_back(upStreamProducer.getFormat(i));
    }
    
}

FIPFlip::~FIPFlip()
{
}

bool FIPFlip::init()
{
    bool rValue=ImageProcessor::init();
    //Note: SharedImageBuffer of downstream producer is initialised with storage in ImageProcessor::init.
    
    return rValue;
}

bool FIPFlip::trigger()
{
    if ((getNumReadSlotsAvailable())&&(getNumWriteSlotsAvailable()))
    {//There are images to consume and the downstream producer has space to produce.
        std::vector<Image**> imvRead=reserveReadSlot();
        std::vector<Image**> imvWrite=reserveWriteSlot();
        
        //Start stats measurement event.
        ProcessorStats_->tick();
        
        for (size_t imgNum=0; imgNum<ImagesPerSlot_; ++imgNum)
        {
            Image const * const imRead = *(imvRead[imgNum]);
            Image * const imWrite = *(imvWrite[imgNum]);
            
            uint8_t const * const dataRead=(uint8_t const * const)imRead->data();
            uint8_t * const dataWrite=(uint8_t * const )imWrite->data();
            
            const ImageFormat imFormat=getUpstreamFormat(imgNum);
            
            const int width=imFormat.getWidth();
            const int height=imFormat.getHeight();

            const int bytesPerPixel=imFormat.getBytesPerPixel();


            if ((!flipLeftRightVect_[imgNum]) && (!flipTopBottomVect_[imgNum]))
            {
                memcpy(dataWrite, dataRead, width*height*bytesPerPixel);
            } else
                if ((flipLeftRightVect_[imgNum]) && (!flipTopBottomVect_[imgNum]))
                {
                    //=== Flip left-right ===//
                    for (int y=0; y<height; ++y)
                    {
                        int readOffset=(y*width)*bytesPerPixel;
                        int writeOffset=(y*width+width-1)*bytesPerPixel;

                        for (int x=0; x<width; ++x)
                        {
                            memcpy(dataWrite+writeOffset, dataRead+readOffset, bytesPerPixel);
                            readOffset+=bytesPerPixel;
                            writeOffset-=bytesPerPixel;
                        }
                    }
                    //=======================//
                } else
                    if ((!flipLeftRightVect_[imgNum]) && (flipTopBottomVect_[imgNum]))
                    {
                        //=== Flip top-bottom ===//
                        for (int y=0; y<height; ++y)
                        {
                            int readOffset=(y*width)*bytesPerPixel;
                            int writeOffset=((height-y-1)*width)*bytesPerPixel;

                            for (int x=0; x<width; ++x)
                            {
                                memcpy(dataWrite+writeOffset, dataRead+readOffset, bytesPerPixel);
                                readOffset+=bytesPerPixel;
                                writeOffset+=bytesPerPixel;
                            }
                        }
                        //=======================//
                    } else
                        if ((flipLeftRightVect_[imgNum]) && (flipTopBottomVect_[imgNum]))
                        {
                            //=== Flip left-right and top-bottom===//
                            for (int y=0; y<height; ++y)
                            {
                                int readOffset=(y*width)*bytesPerPixel;
                                int writeOffset=((height-y-1)*width+width-1)*bytesPerPixel;

                                for (int x=0; x<width; ++x)
                                {
                                    memcpy(dataWrite+writeOffset, dataRead+readOffset, bytesPerPixel);
                                    readOffset+=bytesPerPixel;
                                    writeOffset-=bytesPerPixel;
                                }
                            }
                            //=======================//
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



