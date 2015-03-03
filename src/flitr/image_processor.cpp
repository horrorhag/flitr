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

#include <flitr/image_processor.h>
#include <sstream>

using namespace flitr;
using std::shared_ptr;

void ImageProcessorThread::run()
{
    while (true)
    {
        //IP_->triggerMutex_.lock();
        
        if (!IP_->trigger())//The processor work happens in IP_->trigger()!!!
        {
            //IP_->triggerMutex_.unlock();

            // wait a while for producers and consumers if trigger() method didn't do anything...
            Thread::microSleep(1000);
            //Thread::YieldCurrentThread();
        } else
        {
            ++IP_->frameNumber_;
            //IP_->triggerMutex_.unlock();
        }

        // check for exit
        if (ShouldExit_) {
            break;
        }
    }
}

ImageProcessor::ImageProcessor(ImageProducer& upStreamProducer,
                               uint32_t images_per_slot,
                               uint32_t buffer_size) :
    ImageConsumer(upStreamProducer),
    ImagesPerSlot_(images_per_slot),
    buffer_size_(buffer_size),
    Thread_(0),
    frameNumber_(0)
{
    std::stringstream stats_name;
    stats_name << " ImageProcessor::process";
    ProcessorStats_ = std::shared_ptr<StatsCollector>(new StatsCollector(stats_name.str()));

}

ImageProcessor::~ImageProcessor()
{
    stopTriggerThread();
}

bool ImageProcessor::init()
{
    // Allocate storage
    SharedImageBuffer_ = shared_ptr<SharedImageBuffer>(
                new SharedImageBuffer(*this, buffer_size_, ImagesPerSlot_));

    SharedImageBuffer_->initWithStorage(true);

    return true;
}

bool ImageProcessor::stopTriggerThread()
{
    if (Thread_)
    {
        Thread_->setExit();
        Thread_->join();
        
        return true;
    }

    return false;
}

bool ImageProcessor::startTriggerThread()
{
    if (Thread_==0)
    {//If thread not already started.
        Thread_ = new ImageProcessorThread(this);
        Thread_->startThread();
        
        return true;
    }
    
    return false;
}

