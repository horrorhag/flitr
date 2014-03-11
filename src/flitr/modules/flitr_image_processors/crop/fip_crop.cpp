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

#include <flitr/modules/flitr_image_processors/crop/fip_crop.h>

using namespace flitr;
using std::tr1::shared_ptr;

FIPCrop::FIPCrop(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                 size_t startX,
                 size_t startY,
                 size_t width,
                 size_t height,
                 uint32_t buffer_size) :
ImageProcessor(upStreamProducer, images_per_slot, buffer_size),
startX_(startX),
startY_(startY),
width_(width),
height_(height)
{
    
    //Setup image format being produced to downstream.
    for (uint32_t i=0; i<images_per_slot; i++)
    {
        ImageFormat dsFormat=upStreamProducer.getFormat(i);
        
        dsFormat.setWidth(width_);
        dsFormat.setHeight(height_);
        
        ImageFormat_.push_back(dsFormat);
    }
    
}

FIPCrop::~FIPCrop()
{
}

bool FIPCrop::init()
{
    bool rValue=ImageProcessor::init();
    //Note: SharedImageBuffer of downstream producer is initialised with storage in ImageProcessor::init.
    
    return rValue;
}

bool FIPCrop::trigger()
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
            
            const ImageFormat imFormatUS=getUpstreamFormat(imgNum);
            const ImageFormat imFormatDS=getDownstreamFormat(imgNum);
            
            const size_t widthUS=imFormatUS.getWidth();
            
            const size_t widthDS=imFormatDS.getWidth();
            const size_t heightDS=imFormatDS.getHeight();
            
            for (size_t yDS=0; yDS<heightDS; yDS++)
            {
                const size_t lineOffsetUS=(yDS+startY_) * widthUS + startX_;
                const size_t lineOffsetDS=yDS * widthDS;
                
                memcpy(dataWrite+lineOffsetDS, dataRead+lineOffsetUS, widthDS*sizeof(float));
            }
        }
        
        
        //Stop stats measurement event.
        ProcessorStats_->tock();
        
        releaseWriteSlot();
        releaseReadSlot();
        
        return true;
    }
    
    OpenThreads::Thread::YieldCurrentThread();
    return false;
}



