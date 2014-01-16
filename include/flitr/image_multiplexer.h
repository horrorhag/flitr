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

#ifndef IMAGE_MULTIPLEXER_H
#define IMAGE_MULTIPLEXER_H 1

#include <flitr/image.h>
#include <flitr/image_consumer.h>
#include <flitr/image_producer.h>
#include <flitr/stats_collector.h>

#include <OpenThreads/Thread>
#include <OpenThreads/Mutex>

namespace flitr {

class ImageMultiplexer;

/*! Helper/Service thread class for ImageProcessor that consumes and produces images as they become available from the upstream producer.*/
class ImageMultiplexerThread : public OpenThreads::Thread
{
public:

    /*! Constructor given a pointer to the ImageMultiplexer object.*/
    ImageMultiplexerThread(ImageMultiplexer *im) :
        IM_(im),
        ShouldExit_(false) {}

    /*! The thread's run method.*/
    void run();

    /*! Method to notify the thread to exit.*/
    void setExit() { ShouldExit_ = true; }

private:
    /*! A pointer to the ImageProcessor object being serviced.*/
    ImageMultiplexer *IM_;

    /*! Boolean flag set to true by ImageProcessorThread::setExit.
     *@sa ImageMultiplexerThread::setExit */
    bool ShouldExit_;
};

/*! A multiplexer class inheriting from ImageProducer, but having a consumer member per each of n upstream producers.*/
class FLITR_EXPORT ImageMultiplexer : public ImageProducer
{
    friend class ImageMultiplexerThread;
public:

    /*! Constructor given the upstream producer.
     *@param images_per_slot The number of images per image slot from the upstream producer and that is produced down stream.
     *@param buffer_size The size of the shared image buffer of the downstream producer.*/
    ImageMultiplexer(uint32_t w, uint32_t h, ImageFormat::PixelFormat pix_fmt,
                     uint32_t images_per_slot,
                   uint32_t buffer_size=FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS);

    /*! Virtual destructor */
    virtual ~ImageMultiplexer();

    /*! Method to add multiplexer to upstream image producer.
     *@param upStreamProducer The upstream image producer.*/
    void addUpstreamProducer(ImageProducer& upStreamProducer);

    /*! Method to initialise the object.
     *@return Boolean result flag. True indicates successful initialisation.*/
    virtual bool init();

    /*!Method to start the asynchronous trigger thread. trigger() has to be called synchronously/manually if thread not started.
     *@sa ImageProcessor::trigger*/
    virtual bool startTriggerThread();
    virtual bool isTriggerThreadStarted() const {return Thread_!=0;}

    /*!Synchronous trigger method. Called automatically by the trigger thread if started.
     *@sa ImageProcessor::startTriggerThread*/
    virtual bool trigger();

    /*! Get the image format being consumed from the upstream producer.*/
    virtual ImageFormat getUpstreamFormat(const uint32_t consumer_index, const uint32_t img_index = 0) const {return ImageConsumerVec_[consumer_index]->getFormat(img_index);}


    /*! Get the image format being produced to the downstream consumers.*/
    virtual ImageFormat getDownstreamFormat(const uint32_t img_index = 0) const {return ImageProducer::getFormat(img_index);}

    uint32_t getNumSources()
    {
        return ImageConsumerVec_.size();
    }
    void setSingleSource(int32_t source)
    {
        PlexerSource_=source;
    }
    int32_t getSource()
    {
        return PlexerSource_;
    }
    void useAllSources()
    {
        PlexerSource_=-1;
    }

protected:
    const uint32_t ImagesPerSlot_;
    const uint32_t buffer_size_;

    /*! StatsCollector member to measure the time taken by the processor.*/
    std::tr1::shared_ptr<StatsCollector> ProcessorStats_;

private:
    ImageMultiplexerThread *Thread_;

    int32_t PlexerSource_;

    std::vector<std::tr1::shared_ptr<ImageConsumer> > ImageConsumerVec_;
    uint32_t ConsumerIndex_;

    uint32_t DownstreamWidth_;
    uint32_t DownstreamHeight_;
    ImageFormat::PixelFormat DownstreamPixFmt_;
};


}

#endif //IMAGE_MULTIPLEXER_H
