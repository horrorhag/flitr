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

#include <flitr/modules/flitr_image_processors/cnvrt_to_float/fip_cnvrt_to_rgb_f32.h>

using namespace flitr;
using std::shared_ptr;

FIPConvertToRGBF32::FIPConvertToRGBF32(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                                 uint32_t buffer_size) :
    ImageProcessor(upStreamProducer, images_per_slot, buffer_size)
  , Title_(std::string("Convert To RGB f32"))
{
    
    //Setup image format being produced to downstream.
    for (uint32_t i=0; i<images_per_slot; i++) {
        //ImageFormat(uint32_t w=0, uint32_t h=0, PixelFormat pix_fmt=FLITR_PIX_FMT_Y_8, bool flipV = false, bool flipH = false):
        ImageFormat downStreamFormat(upStreamProducer.getFormat().getWidth(), upStreamProducer.getFormat().getHeight(),
                                     ImageFormat::FLITR_PIX_FMT_RGB_F32);
        
        ImageFormat_.push_back(downStreamFormat);
    }
    
}

FIPConvertToRGBF32::~FIPConvertToRGBF32()
{
}

bool FIPConvertToRGBF32::init()
{
    bool rValue=ImageProcessor::init();
    //Note: SharedImageBuffer of downstream producer is initialised with storage in ImageProcessor::init.
    
    return rValue;
}

bool FIPConvertToRGBF32::trigger()
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
            
            uint8_t const * const dataRead=imRead->data();
            
            float * const dataWrite=(float *)imWrite->data();
            
            const ImageFormat imFormatUS=getUpstreamFormat(imgNum);
            
            const size_t width=imFormatUS.getWidth();
            const size_t height=imFormatUS.getHeight();

            if (imFormatUS.getPixelFormat()==flitr::ImageFormat::FLITR_PIX_FMT_Y_8)
            {
                for (size_t y=0; y<height; ++y)
                {
                    const size_t lineOffset=y * width;
                    size_t writeOffset=lineOffset*3;

                    for (size_t x=0; x<width; ++x)
                    {
                        const float fc=((float)dataRead[lineOffset + x]) * 0.00390625f; // /256.0
                        dataWrite[writeOffset + 0]=fc;
                        dataWrite[writeOffset + 1]=fc;
                        dataWrite[writeOffset + 2]=fc;
                        writeOffset+=3;
                    }
                }
            } else
                if (imFormatUS.getPixelFormat()==flitr::ImageFormat::FLITR_PIX_FMT_RGB_8)
                {
                    for (size_t y=0; y<height; ++y)
                    {
                        size_t offset=(y * width) * 3;

                        for (size_t x=0; x<width; ++x)
                        {
                            dataWrite[offset + 0]=((float)dataRead[offset+0]) * 0.00390625f; // /256.0
                            dataWrite[offset + 1]=((float)dataRead[offset+1]) * 0.00390625f; // /256.0
                            dataWrite[offset + 2]=((float)dataRead[offset+2]) * 0.00390625f; // /256.0
                            offset+=3;
                        }
                    }
            } else
                if (imFormatUS.getPixelFormat()==flitr::ImageFormat::FLITR_PIX_FMT_BGRA)
                {
                    for (size_t y=0; y<height; ++y)
                    {
                        size_t offset=(y * width) * 4;
                        size_t offset_rgb = (y * width) * 3;

                        for (size_t x=0; x<width; ++x)
                        {
                            dataWrite[offset_rgb + 2]=((float)dataRead[offset+0]) * 0.00390625f; // /256.0
                            dataWrite[offset_rgb + 1]=((float)dataRead[offset+1]) * 0.00390625f; // /256.0
                            dataWrite[offset_rgb + 0]=((float)dataRead[offset+2]) * 0.00390625f; // /256.0
                            offset+=4;
                            offset_rgb+=3;
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

