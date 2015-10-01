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

#include <flitr/modules/flitr_image_processors/simulate/fip_test_pattern.h>
#include <random>
#include <iostream>

using namespace flitr;
using std::shared_ptr;

FIPTestPattern::FIPTestPattern(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                               uint32_t buffer_size) :
ImageProcessor(upStreamProducer, images_per_slot, buffer_size),
latestHFrameNumber_(0)
{
    //Setup image format being produced to downstream.
    for (uint32_t i=0; i<images_per_slot; i++) {
        ImageFormat_.push_back(upStreamProducer.getFormat(i));//Output format is same as input format.
    }
}

FIPTestPattern::~FIPTestPattern()
{
}

bool FIPTestPattern::init()
{
    bool rValue=ImageProcessor::init();
    //Note: SharedImageBuffer of downstream producer is initialised with storage in ImageProcessor::init.
    
    return true;
}

bool FIPTestPattern::trigger()
{
    if ((getNumReadSlotsAvailable())&&(getNumWriteSlotsAvailable()))
    {//There are images to consume and the downstream producer has space to produce.
        std::vector<Image**> imvRead=reserveReadSlot();
        std::vector<Image**> imvWrite=reserveWriteSlot();
        
        //Start stats measurement event.
        ProcessorStats_->tick();
        
        for (size_t imgNum=0; imgNum<ImagesPerSlot_; imgNum++)
        {
            const ImageFormat imFormat=getUpstreamFormat(imgNum);//Downstream format is same as upstream format.
            
            const size_t width=imFormat.getWidth();
            const size_t height=imFormat.getHeight();
            
            const float cx = width * 0.5f;
            const float cy = height * 0.5f;
            
            Image const * const imRead = *(imvRead[imgNum]);
            Image * const imWrite = *(imvWrite[imgNum]);
            
            if (imFormat.getPixelFormat()==flitr::ImageFormat::FLITR_PIX_FMT_Y_F32)
            {//Image format float32.
                float const * const dataRead=(float const * const)imRead->data();
                float * const dataWrite=(float * const )imWrite->data();
                
                for (size_t y=0; y<height; ++y)
                {
                    const size_t lineOffset=y * width;
                    const size_t ky = y;
                    
                    for (size_t x=0; x<width; ++x)
                    {
                        const size_t kx = x;
                        
                        dataWrite[lineOffset + x] =0.5f + sinf((kx*0.125+frameNumber_)*powf(float(ky)/(height), 1.00f) * 3.25f)*0.5f;
                    }
                }
            }
            
        }
        
        
        latestHFrameNumber_=frameNumber_;
        std::cout << frameNumber_ << "\n";
        std::cout.flush();
        
        //Stop stats measurement event.
        ProcessorStats_->tock();
        
        releaseWriteSlot();
        releaseReadSlot();
        
        return true;
    }
    
    return false;
}



