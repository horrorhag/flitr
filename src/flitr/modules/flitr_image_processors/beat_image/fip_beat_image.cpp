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

#include <flitr/modules/flitr_image_processors/beat_image/fip_beat_image.h>

using namespace flitr;
using std::shared_ptr;

FIPBeatImage::FIPBeatImage(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                           uint8_t base2WindowLength,
                           uint32_t buffer_size) :
ImageProcessor(upStreamProducer, images_per_slot, buffer_size),
base2WindowLength_(base2WindowLength),
windowLength_((uint32_t)(powf(2.0f, base2WindowLength_)+0.5f)),
oldestHistorySlot_(0)
{
    //Setup image format being produced to downstream.
    for (uint32_t i=0; i<images_per_slot; i++) {
        ImageFormat_.push_back(upStreamProducer.getFormat(i));//Output format is same as input format.
    }
}

FIPBeatImage::~FIPBeatImage()
{
    for (uint32_t i=0; i<ImagesPerSlot_; ++i)
    {
        delete [] historyImageVec_[i];
        delete [] beatImageVec_[i];
    }
}

bool FIPBeatImage::init()
{
    bool rValue=ImageProcessor::init();
    //Note: SharedImageBuffer of downstream producer is initialised with storage in ImageProcessor::init.
    
    for (uint32_t i=0; i<ImagesPerSlot_; ++i)
    {
        const ImageFormat imFormat=getUpstreamFormat(i);//Downstream format is same as upstream format.
        
        const size_t width=imFormat.getWidth();
        const size_t height=imFormat.getHeight();
        
        const size_t componentsPerPixel=imFormat.getComponentsPerPixel();
        
        historyImageVec_.push_back(new float[width*height*componentsPerPixel * windowLength_]);
        memset(historyImageVec_[i], 0, width*height*componentsPerPixel * windowLength_ * sizeof(float));
        
        beatImageVec_.push_back(new float[width*height*componentsPerPixel * windowLength_*2]);
        memset(beatImageVec_[i], 0, width*height*componentsPerPixel * windowLength_*2 * sizeof(float));
    }
    
    return rValue;
}



bool FIPBeatImage::trigger()
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
            
            const ImageFormat imFormat=getUpstreamFormat(imgNum);//Downstream format is same as upstream format.
            
            const size_t width=imFormat.getWidth();
            const size_t height=imFormat.getHeight();
            const size_t windowLength_Minus1=windowLength_-1;
            
            
            if (imFormat.getPixelFormat()==ImageFormat::FLITR_PIX_FMT_Y_F32)
            {
                float const * const dataRead = (float const * const)imRead->data();
                float * const dataWrite = (float * const)imWrite->data();
                
                float * const historyImage = historyImageVec_[imgNum];
                float * const beatImage = beatImageVec_[imgNum];
                
                for (size_t y=0; y<height; ++y)
                {
                    const size_t imgOffset=y * width;
                    size_t histOffset=y * width * windowLength_;
                    size_t beatOffset=y * width * (windowLength_<<1);
                    
                    for (size_t x=0; x<width; ++x)
                    {
                        //Update history ring buffer.
                        historyImage[histOffset + oldestHistorySlot_]=dataRead[imgOffset + x];
                        
                        
                        //Setup beat input...
                        //* Unring the history ring buffer.
                        //* beatImage will then be modified in place.
                        for (size_t n=0; n<windowLength_; ++n)
                        {
                            beatImage[beatOffset + (n << 1)]     = historyImage[histOffset + ((n+oldestHistorySlot_) & windowLength_Minus1)];
                            beatImage[beatOffset + (n << 1) + 1] = 0.0f;
                        }
                        
                        
                        //Calculculate the beat...
                        four1(beatImage+beatOffset-1, windowLength_, +1);
                        
                        
                        //Update the output image.
                        const size_t beatNum=4;//0,2,4,...
                        const float vx=beatImage[beatOffset+beatNum];
                        const float vi=beatImage[beatOffset+beatNum+1];
                        
                        dataWrite[imgOffset + x]=(vx*vx+vi*vi)*2.625f+0.5f;
                        
                        histOffset+=windowLength_;
                        beatOffset+=windowLength_<<1;
                    }
                }
            } else
                if (imFormat.getPixelFormat()==ImageFormat::FLITR_PIX_FMT_RGB_F32)
                {
                    float const * const dataRead = (float const * const)imRead->data();
                    float * const dataWrite = (float * const)imWrite->data();
                    
                }
            
        }
        
        oldestHistorySlot_=(oldestHistorySlot_+1) % windowLength_;
        
        //Stop stats measurement event.
        ProcessorStats_->tock();
        
        releaseWriteSlot();
        releaseReadSlot();
        
        return true;
    }
    
    return false;
}

