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

#include <OpenThreads/Thread>

#include <flitr/image_multiplexer.h>

using namespace flitr;
using std::tr1::shared_ptr;

void ImageMultiplexerThread::run()
{
    while (true)
    {
        IM_->trigger();

        Thread::microSleep(1000);

        // check for exit
        if (ShouldExit_) {
            break;
        }
    }
}

ImageMultiplexer::ImageMultiplexer(uint32_t w, uint32_t h, ImageFormat::PixelFormat pix_fmt,
                                   uint32_t images_per_slot,
                                   uint32_t buffer_size) :
    ImagesPerSlot_(images_per_slot),
    buffer_size_(buffer_size),
    Thread_(0),
    PlexerSource_(-1),
    ConsumerIndex_(0),
    DownstreamWidth_(w),
    DownstreamHeight_(h),
    DownstreamPixFmt_(pix_fmt)
{
    std::stringstream stats_name;
    stats_name << " ImageMultiplexer::process";
    ProcessorStats_ = std::tr1::shared_ptr<StatsCollector>(new StatsCollector(stats_name.str()));

    //Setup image format being produced to downstream.
    for (uint32_t i=0; i<images_per_slot; i++) {
        ImageFormat_.push_back(ImageFormat(DownstreamWidth_, DownstreamHeight_, DownstreamPixFmt_));
    }

}

ImageMultiplexer::~ImageMultiplexer()
{
    stopTriggerThread();
}

void ImageMultiplexer::addUpstreamProducer(ImageProducer& upStreamProducer)
{
    ImageConsumerVec_.push_back(std::tr1::shared_ptr<ImageConsumer>(new ImageConsumer(upStreamProducer)));
}

bool ImageMultiplexer::init()
{
    // Allocate storage
    SharedImageBuffer_ = shared_ptr<SharedImageBuffer>(
                new SharedImageBuffer(*this, buffer_size_, ImagesPerSlot_));

    SharedImageBuffer_->initWithStorage();

    return true;
}

bool ImageMultiplexer::stopTriggerThread()
{
    if (Thread_)
    {
        Thread_->setExit();
        Thread_->join();
        
        return true;
    }
    
    return false;
}

bool ImageMultiplexer::startTriggerThread()
{
    if (Thread_==0)
    {//If thread not already started.
        Thread_ = new ImageMultiplexerThread(this);
        Thread_->startThread();

        return true;
    }

    return false;
}

bool ImageMultiplexer::trigger()
{    
    std::vector<Image**> imvWrite;
    std::vector<Image**> imvRead;

    //std::cout << getNumWriteSlotsAvailable() << "@write\n";
    //std::cout.flush();

    int32_t ThreadPlexerSource=PlexerSource_;

    if (getNumWriteSlotsAvailable())
    {
        /*
        for (size_t i=0; i<ImageConsumerVec_.size(); i++)
        {
            std::cout << ImageConsumerVec_[i]->getNumReadSlotsAvailable() << "@" << i << "\n";
            std::cout.flush();
        }

        std::cout << "Currently" << "@" << ConsumerIndex_ << "\n";
        std::cout.flush();
        */
        if (ImageConsumerVec_[ConsumerIndex_]->getNumReadSlotsAvailable())
        {
            ProcessorStats_->tick();

            imvRead=ImageConsumerVec_[ConsumerIndex_]->reserveReadSlot();
            
            if (imvRead.size()==ImagesPerSlot_)
            {

                if ((ThreadPlexerSource<0)||(ConsumerIndex_==ThreadPlexerSource))
                {
                    imvWrite=reserveWriteSlot();
                    if (imvWrite.size()==ImagesPerSlot_)
                    {
                        unsigned int i=0;
                        for (i=0; i<ImagesPerSlot_; i++)
                        {
                            Image const * const imRead = *(imvRead[i]);
                            Image * const imWrite = *(imvWrite[i]);

                            const ImageFormat imReadFormat=getUpstreamFormat(ConsumerIndex_, i);
                            //const ImageFormat::PixelFormat pixelReadFormat=imReadFormat.getPixelFormat();

                            const ImageFormat imWriteFormat=getDownstreamFormat(i);
                            const ImageFormat::PixelFormat pixelWriteFormat=imWriteFormat.getPixelFormat();

                            const uint32_t readWidth=imReadFormat.getWidth();
                            const uint32_t readHeight=imReadFormat.getHeight();
                            const uint32_t readBytesPerPixel=imReadFormat.getBytesPerPixel();

                            const uint32_t writeWidth=imWriteFormat.getWidth();
                            const uint32_t writeHeight=imWriteFormat.getHeight();
                            const uint32_t writeBytesPerPixel=imWriteFormat.getBytesPerPixel();

                            uint8_t const * const dataRead=imRead->data();
                            uint8_t * const dataWrite=imWrite->data();

                            uint32_t writeOffset=0;
                            const float readXOffset_delta_float=((float)readWidth)/writeWidth;

                            //Do image copy and format conversion here...

                            for (uint32_t y=0; y<writeHeight; y++)
                            {
                                const uint32_t readOffset=((uint32_t)((((float)y)/writeHeight) * readHeight)) * readWidth;

                                float readXOffset_float=0.0;

                                for (uint32_t x=0; x<writeWidth; x++)
                                {
                                    imReadFormat.cnvrtPixelFormat(dataRead+((uint32_t)(readOffset+readXOffset_float))*readBytesPerPixel,
                                                                  dataWrite+writeOffset,
                                                                  pixelWriteFormat);

                                    writeOffset+=writeBytesPerPixel;

                                    readXOffset_float+=readXOffset_delta_float;
                                }
                            }


                        }

                        releaseWriteSlot();
                    }
                }

                ImageConsumerVec_[ConsumerIndex_]->releaseReadSlot();

                ConsumerIndex_=(ConsumerIndex_+1)%ImageConsumerVec_.size();
            }

            ProcessorStats_->tock();
        }
    }


    return true;
}


