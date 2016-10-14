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

#include <sstream>

#include <flitr/flitr_thread.h>

#include <flitr/image_diff_and_scale.h>

using namespace flitr;
using std::shared_ptr;

void ImageDiffAndScaleThread::run()
{
    while (true)
    {
        if (!IDS_->trigger()) FThread::microSleep(1000);
        
        // check for exit
        if (ShouldExit_) {
            break;
        }
    }
}

ImageDiffAndScale::ImageDiffAndScale(ImageProducer& upStreamProducerA,
                                     ImageProducer& upStreamProducerB,
                                     float scale,
                                     uint32_t images_per_slot,
                                     uint32_t buffer_size) :
ImagesPerSlot_(images_per_slot),
buffer_size_(buffer_size),
scale_(scale),
Thread_(nullptr)
{
    std::stringstream stats_name;
    stats_name << " ImageDiffAndScale::process";
    ProcessorStats_ = std::shared_ptr<StatsCollector>(new StatsCollector(stats_name.str()));
    
    ImageConsumerVec_.push_back(std::shared_ptr<ImageConsumer>(new ImageConsumer(upStreamProducerA)));
    ImageConsumerVec_.push_back(std::shared_ptr<ImageConsumer>(new ImageConsumer(upStreamProducerB)));
    
    const uint32_t w=upStreamProducerA.getFormat().getWidth();
    const uint32_t h=upStreamProducerA.getFormat().getHeight();
    const ImageFormat::PixelFormat pix_fmt=upStreamProducerA.getFormat().getPixelFormat();
    
    //Setup image format being produced to downstream.
    for (uint32_t i=0; i<images_per_slot; i++)
    {
        ImageFormat_.push_back(ImageFormat(w, h, pix_fmt));
    }
}

ImageDiffAndScale::~ImageDiffAndScale()
{
    stopTriggerThread();
}

bool ImageDiffAndScale::init()
{
    if (ImageConsumerVec_[0]->getFormat() != ImageConsumerVec_[1]->getFormat())
    {
        logMessage(LOG_CRITICAL) << "Error: The producers must have the same image format " << __FILE__ <<":"<<__LINE__<<".\n";
        logMessage(LOG_CRITICAL).flush();
        return false;
    }
    
    // Allocate storage
    SharedImageBuffer_ = shared_ptr<SharedImageBuffer>(
                                                       new SharedImageBuffer(*this, buffer_size_, ImagesPerSlot_));
    
    SharedImageBuffer_->initWithStorage();
    
    return true;
}

bool ImageDiffAndScale::stopTriggerThread()
{
    if (Thread_)
    {
        Thread_->setExit();
        Thread_->join();
        
        return true;
    }
    
    return false;
}

bool ImageDiffAndScale::startTriggerThread()
{
    if (Thread_==0)
    {//If thread not already started.
        Thread_ = new ImageDiffAndScaleThread(this);
        Thread_->startThread();
        
        return true;
    }
    
    return false;
}

bool ImageDiffAndScale::trigger()
{
    if (getNumWriteSlotsAvailable())
    {
        if ((ImageConsumerVec_[0]->getNumReadSlotsAvailable()) && (ImageConsumerVec_[1]->getNumReadSlotsAvailable()))
        {
            ProcessorStats_->tick();
            
            std::vector<Image**> imvReadA=ImageConsumerVec_[0]->reserveReadSlot();
            std::vector<Image**> imvReadB=ImageConsumerVec_[1]->reserveReadSlot();
            
            std::vector<Image**> imvWrite=reserveWriteSlot();
            
            unsigned int i=0;
            for (i=0; i<ImagesPerSlot_; i++)
            {
                Image const * const imReadA = *(imvReadA[i]);
                Image const * const imReadB = *(imvReadB[i]);
                
                Image * const imWrite = *(imvWrite[i]);
                
                const ImageFormat imFormat=getDownstreamFormat(i);
                const uint32_t componentsPerLine=imFormat.getWidth() * imFormat.getComponentsPerPixel();
                const uint32_t height=imFormat.getHeight();
                
                
                if (imFormat.getDataType() == ImageFormat::FLITR_PIX_DT_FLOAT32)
                {
                    float const * const dataReadA=(float *)imReadA->data();
                    float const * const dataReadB=(float *)imReadB->data();
                    
                    float * const dataWrite=(float *)imWrite->data();
                    
                    for (uint32_t y=0; y<height; y++)
                    {
                        const uint32_t lineOffset=y * componentsPerLine;
                        
                        for (uint32_t i=0; i<componentsPerLine; i++)
                        {
                            const uint32_t offset = lineOffset + i;
                            
                            dataWrite[offset]=fabsf((dataReadA[offset]-dataReadB[offset]) * scale_);
                        }
                    }
                }
                
            }
            
            releaseWriteSlot();
            
            ImageConsumerVec_[0]->releaseReadSlot();
            ImageConsumerVec_[1]->releaseReadSlot();
            
            ProcessorStats_->tock();
        }
    }
    
    return true;
}


