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

#include <flitr/modules/flitr_image_processors/motion_detect/fip_motion_detect.h>


using namespace flitr;
using std::shared_ptr;

FIPMotionDetect::FIPMotionDetect(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                                 const bool showOverlays, const bool produceOnlyMotionImages, const int motionThreshold,
                                 uint32_t buffer_size) :
ImageProcessor(upStreamProducer, images_per_slot, buffer_size),
upStreamFrameCount_(0),
scratchData_(nullptr),
intImageScratchData_(nullptr),
boxFilter_(20),
showOverlays_(showOverlays),
produceOnlyMotionImages_(produceOnlyMotionImages),
motionThreshold_(motionThreshold)
{
    //Setup image format being produced to downstream.
    for (uint32_t i=0; i<images_per_slot; i++) {
        ImageFormat downStreamFormat=upStreamProducer.getFormat();
        
        ImageFormat_.push_back(downStreamFormat);
    }
}

FIPMotionDetect::~FIPMotionDetect()
{
    delete [] scratchData_;
    delete [] intImageScratchData_;
}

bool FIPMotionDetect::init()
{
    bool rValue=ImageProcessor::init();
    //Note: SharedImageBuffer of downstream producer is initialised with storage in ImageProcessor::init.
    
    size_t maxScratchDataSize=0;
    size_t maxScratchDataValues=0;
    
    for (uint32_t i=0; i<ImagesPerSlot_; i++)
    {
        const ImageFormat imFormat=getUpstreamFormat(i);
        
        const size_t width=imFormat.getWidth();
        const size_t height=imFormat.getHeight();
        
        const size_t bytesPerPixel=imFormat.getBytesPerPixel();
        const size_t componentsPerPixel=imFormat.getComponentsPerPixel();
        
        const size_t scratchDataSize = width * height * bytesPerPixel;
        const size_t scratchDataValues = width * height * componentsPerPixel;
        
        if (scratchDataSize>maxScratchDataSize)
        {
            maxScratchDataSize=scratchDataSize;
        }
        
        if (scratchDataValues>maxScratchDataValues)
        {
            maxScratchDataValues=scratchDataValues;
        }
    }
    
    //Allocate a buffer big enough for any of the image slots.
    scratchData_=new uint8_t[maxScratchDataSize];
    memset(scratchData_, 0, maxScratchDataSize);
    
    intImageScratchData_=new double[maxScratchDataValues];
    memset(intImageScratchData_, 0, maxScratchDataValues*sizeof(double));
    
    
    currentFrame_=new uint8_t[maxScratchDataSize];
    memset(currentFrame_, 0, maxScratchDataSize);
    
    previousFrame_=new uint8_t[maxScratchDataSize];
    memset(previousFrame_, 0, maxScratchDataSize);
    
    return rValue;
}

bool FIPMotionDetect::trigger()
{
    if ((getNumReadSlotsAvailable())&&(getNumWriteSlotsAvailable()))
    {
        std::vector<Image**> imvRead=reserveReadSlot();
        
        //The write slot will be reserved later.
        
        const int intImgApprox=2;
        
        //Start stats measurement event.
        ProcessorStats_->tick();
        
        ++upStreamFrameCount_;
        
        
        size_t imgNum=0;
        {
            Image const * const imReadUS = *(imvRead[imgNum]);
            const ImageFormat imFormat=getDownstreamFormat(imgNum);//down stream and up stream formats are the same.
            const int width=imFormat.getWidth();
            const int height=imFormat.getHeight();
            //const int components=width * imFormat.getComponentsPerPixel();
            
            
            if (imFormat.getPixelFormat()==ImageFormat::FLITR_PIX_FMT_RGB_8)
            {
                uint8_t const * const dataReadUS=(uint8_t const * const)imReadUS->data();
                
                memcpy(previousFrame_, currentFrame_, width*height*3);
                
                boxFilter_.filterRGB(currentFrame_, dataReadUS, width, height,
                                     intImageScratchData_, true);
                
                for (int i=1; i<intImgApprox; ++i)
                {
                    memcpy(scratchData_, currentFrame_, width*height*sizeof(uint8_t)*3);
                    
                    boxFilter_.filterRGB(currentFrame_, scratchData_, width, height,
                                         intImageScratchData_, true);
                }
                
                
                int64_t M=0;
                
                for (int y=0; y<height; ++y)
                {
                    const int lineOffset=y*width*3;
                    
                    for (int x=0; x<width; ++x)
                    {
                        const int offset=lineOffset + x*3;
                        
                        M+=std::abs(currentFrame_[offset+1] - previousFrame_[offset+1]);
                    }
                }
                
                const bool frameMotion=(M / (width*height/30)) > (motionThreshold_);
                
                if ((!produceOnlyMotionImages_) || frameMotion)
                {//Add motion blob overlay.
                    std::vector<Image**> imvWrite=reserveWriteSlot();
                    Image * const imWriteDS = *(imvWrite[imgNum]);
                    uint8_t * const dataWriteDS=(uint8_t * const)imWriteDS->data();
                    
                    if ((frameMotion) && (showOverlays_))
                    {
                        for (int y=0; y<height; ++y)
                        {
                            const int lineOffset=y*width*3;
                            
                            for (int x=0; x<width; ++x)
                            {
                                const int offset=lineOffset + x*3;
                                
                                const int mv=std::abs(currentFrame_[offset+1] - previousFrame_[offset+1]);
                                
                                dataWriteDS[offset + 0] = dataReadUS[offset + 0];
                                dataWriteDS[offset + 1] = mv > motionThreshold_ ? (128+(dataReadUS[offset + 1]>>1)) : dataReadUS[offset + 1];
                                dataWriteDS[offset + 2] = dataReadUS[offset + 2];
                            }
                        }
                    } else
                    {//Don't add motion overlay. Just copy the data from the input.
                        memcpy(dataWriteDS, dataReadUS, width*height*3);
                    }
                    
                    releaseWriteSlot();
                }
            }
        }
        
        //Stop stats measurement event.
        ProcessorStats_->tock();
        
        releaseReadSlot();
        
        return true;
    }
    
    return false;
}

