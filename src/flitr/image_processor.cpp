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

using namespace flitr;
using std::tr1::shared_ptr;

void ImageProcessorThread::run()
{
    while (true)
    {
        if (!IP_->trigger())
        {
            // wait a while for producers and consumers.
            Thread::microSleep(1000);
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
    Thread_(0)
{
    std::stringstream stats_name;
    stats_name << " ImageProcessor::process";
    ProcessorStats_ = std::tr1::shared_ptr<StatsCollector>(new StatsCollector(stats_name.str()));

}

ImageProcessor::~ImageProcessor()
{
    if (Thread_)
    {
        Thread_->setExit();
        Thread_->join();
    }
}

bool ImageProcessor::init()
{
    // Allocate storage
    SharedImageBuffer_ = shared_ptr<SharedImageBuffer>(
                new SharedImageBuffer(*this, buffer_size_, ImagesPerSlot_));

    SharedImageBuffer_->initWithStorage();

    return true;
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

