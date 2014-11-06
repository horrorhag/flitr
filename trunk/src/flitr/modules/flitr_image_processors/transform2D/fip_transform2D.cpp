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

#include <flitr/modules/flitr_image_processors/transform2D/fip_transform2D.h>

using namespace flitr;
using std::shared_ptr;

FIPTransform2D::FIPTransform2D(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                               const std::vector<M2D> transformVect,
                               uint32_t buffer_size) :
ImageProcessor(upStreamProducer, images_per_slot, buffer_size),
transformVect_(transformVect)
{
    
    //Setup image format being produced to downstream.
    for (uint32_t i=0; i<images_per_slot; i++)
    {
        ImageFormat_.push_back(upStreamProducer.getFormat(i));
    }
    
}

FIPTransform2D::~FIPTransform2D()
{
}

bool FIPTransform2D::init()
{
    bool rValue=ImageProcessor::init();
    //Note: SharedImageBuffer of downstream producer is initialised with storage in ImageProcessor::init.
    
    return rValue;
}

bool FIPTransform2D::trigger()
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
            
            const int widthUS=imFormatUS.getWidth();
            const int heightUS=imFormatUS.getHeight();
            const int widthDS=imFormatDS.getWidth();
            const int heightDS=imFormatDS.getHeight();
            
            const float halfWidthDS=widthDS * 0.5f;
            const float halfHeightDS=heightDS * 0.5f;
            
            const M2D transform=transformVect_[imgNum];
            
            const int bytesPerPixel=imFormatUS.getBytesPerPixel();
            
            for (int y=0; y<heightDS; ++y)
            {
                int writeOffset=(y*widthDS)*bytesPerPixel;
                
                for (int x=0; x<widthDS; ++x)
                {
                    const float cx = x-halfWidthDS;
                    const float cy = y-halfHeightDS;
                    
                    const float s=(cx*transform.a_) + (cy*transform.b_) + halfWidthDS;
                    const float t=(cx*transform.c_) + (cy*transform.d_) + halfHeightDS;
                    
                    const int readOffset=(int(s+0.5f) + int(t+0.5f)*widthUS)*bytesPerPixel;
                    
                    memcpy(dataWrite+writeOffset, dataRead+readOffset, bytesPerPixel);
                    writeOffset+=bytesPerPixel;
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



