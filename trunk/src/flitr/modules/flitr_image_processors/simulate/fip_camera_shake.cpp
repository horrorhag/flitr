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

#include <flitr/modules/flitr_image_processors/simulate/fip_camera_shake.h>
#include <random>
#include <iostream>

using namespace flitr;
using std::shared_ptr;

FIPCameraShake::FIPCameraShake(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                               const float sd,
                               const size_t kernelWidth,
                               uint32_t buffer_size) :
ImageProcessor(upStreamProducer, images_per_slot, buffer_size),
randGen_(randDev_()),
randNormDist_(0.0f, 1.0f),
kernel2D_(nullptr),
sd_(sd),
kernelWidth_(kernelWidth | 1),//Ensure kernel width is odd.
currentX_(randNormDist_(randGen_)*sd_),
currentY_(randNormDist_(randGen_)*sd_),
oldX_(randNormDist_(randGen_)*sd_),
oldY_(randNormDist_(randGen_)*sd_),
latestHx_(0.0),
latestHy_(0.0),
latestHFrameNumber_(0)
{
    //Setup image format being produced to downstream.
    for (uint32_t i=0; i<images_per_slot; i++) {
        ImageFormat_.push_back(upStreamProducer.getFormat(i));//Output format is same as input format.
    }
}

FIPCameraShake::~FIPCameraShake()
{
    if (kernel2D_) delete [] kernel2D_;
}

void FIPCameraShake::updateKernel()
{
    oldX_=currentX_;
    oldY_=currentY_;
    
    currentX_=randNormDist_(randGen_)*sd_;
    currentY_=randNormDist_(randGen_)*sd_;
    
    const float kernelSD=2.0f;
    
    float kernelSum=0.0f;
    
    if (kernel2D_)
    {
        for (size_t y=0; y<kernelWidth_; ++y)
        {
            for (size_t x=0; x<kernelWidth_; ++x)
            {
                kernel2D_[x+y*kernelWidth_]=0.0f;
            }
        }

        for (float t=0.5f; t<1.5f; t+=0.005f)
        //float t=1.0f;
        {
            const float tX=oldX_*(1.0f-t) + currentX_*t;
            const float tY=oldY_*(1.0f-t) + currentY_*t;
            
            for (size_t y=0; y<kernelWidth_; ++y)
            {
                const float b=y - (kernelWidth_*0.5f + tY);
                
                for (size_t x=0; x<kernelWidth_; ++x)
                {
                    const float a=x - (kernelWidth_*0.5f + tX);
                    
                    const float w=(1.0f/(2.0f*float(M_PI)*kernelSD*kernelSD)) * exp(-(a*a+b*b)/(2.0f*kernelSD*kernelSD));
                    kernel2D_[x+y*kernelWidth_] += w;
                    kernelSum += w;
                }
            }
        }
        
        //Normalise the kernel.
        if (kernelSum>0.0f)
        {
            const float recipKernelSum=1.0f/kernelSum;
            
            for (size_t y=0; y<kernelWidth_; ++y)
            {
                for (size_t x=0; x<kernelWidth_; ++x)
                {
                    kernel2D_[x+y*kernelWidth_]*=recipKernelSum;
                }
            }
        }
    }
}

void FIPCameraShake::setShakeSD(const float sd)
{
    //std::lock_guard<std::mutex> scopedLock(triggerMutex_);
    
    sd_=sd;
    updateKernel();
}

bool FIPCameraShake::init()
{
    bool rValue=ImageProcessor::init();
    //Note: SharedImageBuffer of downstream producer is initialised with storage in ImageProcessor::init.
    
    kernel2D_=new float[kernelWidth_ * kernelWidth_];
    
    updateKernel();
    
    return rValue & (kernel2D_!=nullptr);
}

bool FIPCameraShake::trigger()
{
    if ((getNumReadSlotsAvailable())&&(getNumWriteSlotsAvailable()))
    {//There are images to consume and the downstream producer has space to produce.
        std::vector<Image**> imvRead=reserveReadSlot();
        std::vector<Image**> imvWrite=reserveWriteSlot();
        
        //Start stats measurement event.
        ProcessorStats_->tick();
        
        updateKernel();
        
        for (size_t imgNum=0; imgNum<ImagesPerSlot_; imgNum++)
        {
            const ImageFormat imFormat=getUpstreamFormat(imgNum);//Downstream format is same as upstream format.
            
            const size_t width=imFormat.getWidth();
            const size_t height=imFormat.getHeight();
            
            const size_t widthMinusKernel=width - kernelWidth_ + 1;
            const size_t heightMinusKernel=height - kernelWidth_ + 1;
            const size_t halfKernalWidth=kernelWidth_>>1;
            
            Image const * const imRead = *(imvRead[imgNum]);
            Image * const imWrite = *(imvWrite[imgNum]);
            
            if (imFormat.getPixelFormat()==flitr::ImageFormat::FLITR_PIX_FMT_Y_F32)
            {//Image format float32.
                float const * const dataRead=(float const * const)imRead->data();
                float * const dataWrite=(float * const )imWrite->data();
                
                for (size_t y=0; y<heightMinusKernel; ++y)
                {
                    const size_t lineOffsetDS=(y+halfKernalWidth) * width;
                    const size_t lineOffsetUS=y * width;
                    
                    for (size_t x=0; x<widthMinusKernel; ++x)
                    {
                        float ws=0.0f;
                        
                        for (size_t j=0; j<kernelWidth_; ++j)
                        {
                            const size_t lineOffsetUS2=lineOffsetUS + j*width + x;
                            const size_t lineOffsetKernel=j*kernelWidth_;
                            
                            for (size_t i=0; i<kernelWidth_; ++i)
                            {
                                ws+=dataRead[lineOffsetUS2 + i] * kernel2D_[lineOffsetKernel + i];
                            }
                        }
                        
                        dataWrite[lineOffsetDS + x + halfKernalWidth]=ws;
                    }
                }
            }
            
        }
        
        
        //A race condition lay here...
        
        //Save latest H vector.
        latestHx_=currentX_ - oldX_;
        latestHy_=currentY_ - oldY_;
        latestHFrameNumber_=frameNumber_;
        
        //Stop stats measurement event.
        ProcessorStats_->tock();
        
        releaseWriteSlot();
        releaseReadSlot();
        
        return true;
    }
    
    return false;
}



