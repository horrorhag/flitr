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

#include <flitr/modules/flitr_image_processors/rotate/fip_rotate.h>

using namespace flitr;
using std::shared_ptr;

FIPRotate::FIPRotate(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                     const int multipleOf90,
                     uint32_t buffer_size) :
    ImageProcessor(upStreamProducer, images_per_slot, buffer_size),
    multipleOf90_(multipleOf90)
{
    while (multipleOf90_<0) multipleOf90_+=4;
    while (multipleOf90_>=4) multipleOf90_-=4;


    //Setup image format being produced to downstream.
    for (uint32_t i=0; i<images_per_slot; i++)
    {
        const size_t widthUS=upStreamProducer.getFormat(i).getWidth();
        const size_t heightUS=upStreamProducer.getFormat(i).getHeight();

        const size_t widthDS=((multipleOf90_%2) == 0) ? widthUS : heightUS;
        const size_t heightDS=((multipleOf90_%2) == 0) ? heightUS : widthUS;

        ImageFormat dsFormat=upStreamProducer.getFormat(i);
        dsFormat.setWidth(widthDS);
        dsFormat.setHeight(heightDS);
        
        ImageFormat_.push_back(dsFormat);
    }
    
}

FIPRotate::~FIPRotate()
{
}

bool FIPRotate::init()
{
    bool rValue=ImageProcessor::init();
    //Note: SharedImageBuffer of downstream producer is initialised with storage in ImageProcessor::init.
    
    return rValue;
}

bool FIPRotate::trigger()
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
            
            const ImageFormat imFormatUS=getUpstreamFormat(imgNum);
            const ImageFormat imFormatDS=getDownstreamFormat(imgNum);
            
            const size_t widthUS=imFormatUS.getWidth();
            const size_t heightUS=imFormatUS.getHeight();

            const size_t widthDS=imFormatDS.getWidth();
            const size_t heightDS=imFormatDS.getHeight();

            const size_t bytesPerPixel=imFormatUS.getBytesPerPixel();

            if (multipleOf90_ == 0)
            {
                memcpy(dataWrite, dataRead, widthUS*heightUS*bytesPerPixel);
            } else
                if (multipleOf90_ == 1)
                {
                    int readOffset=0;
                    int writeOffset=0;
                    //=== Rotate by 90 deg ===//
                    for (int y=0; y<heightUS; ++y)
                    {
                        readOffset=(y*widthUS+0)*bytesPerPixel;
                        writeOffset=(widthDS-y-1)*bytesPerPixel;

                        for (int x=0; x<widthUS; ++x)
                        {
                            memcpy(dataWrite+writeOffset, dataRead+readOffset, bytesPerPixel);
                            readOffset+=bytesPerPixel;
                            writeOffset+=bytesPerPixel*widthDS;
                        }
                    }
                    //=======================//
                } else
                    if (multipleOf90_ == 2)
                    {
                        int readOffset=0;
                        int writeOffset=0;
                        //=== Rotate by 180 deg ===//
                        for (int y=0; y<heightUS; ++y)
                        {
                            readOffset=(y*widthUS+0)*bytesPerPixel;
                            writeOffset=((heightDS-y-1)*widthDS + widthDS-1)*bytesPerPixel;

                            for (int x=0; x<widthUS; ++x)
                            {
                                memcpy(dataWrite+writeOffset, dataRead+readOffset, bytesPerPixel);
                                readOffset+=bytesPerPixel;
                                writeOffset-=bytesPerPixel;
                            }
                        }
                        //=======================//
                    } else
                        if (multipleOf90_ == 3)
                        {
                            int readOffset=0;
                            int writeOffset=0;
                            //=== Rotate by 270 deg ===//
                            for (int y=0; y<heightUS; ++y)
                            {
                                readOffset=(y*widthUS+0)*bytesPerPixel;
                                writeOffset=((heightDS-1)*widthDS + y)*bytesPerPixel;

                                for (int x=0; x<widthUS; ++x)
                                {
                                    memcpy(dataWrite+writeOffset, dataRead+readOffset, bytesPerPixel);
                                    readOffset+=bytesPerPixel;
                                    writeOffset-=bytesPerPixel*widthDS;
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



