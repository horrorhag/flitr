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

#ifndef IMAGE_DIFF_AND_SCALE_H
#define IMAGE_DIFF_AND_SCALE_H 1

#include <flitr/image.h>
#include <flitr/image_consumer.h>
#include <flitr/image_producer.h>
#include <flitr/stats_collector.h>

#include <flitr/flitr_thread.h>

namespace flitr {
    
    class ImageDiffAndScale;
    
    /*! Helper/Service thread class for ImageDiffAndScale - Takes the scaled difference from two producers to one down stream.*/
    class ImageDiffAndScaleThread : public FThread
    {
    public:
        
        /*! Constructor given a pointer to the ImageMultiplexer object.*/
        ImageDiffAndScaleThread(ImageDiffAndScale *ids) :
        IDS_(ids),
        ShouldExit_(false) {}
        
        /*! The thread's run method.*/
        void run();
        
        /*! Method to notify the thread to exit.*/
        void setExit() { ShouldExit_ = true; }
        
    private:
        /*! A pointer to the ImageDiffAndScale object being serviced.*/
        ImageDiffAndScale *IDS_;
        
        /*! Boolean flag set to true by ImageDiffAndScaleThread::setExit.
         *@sa ImageDiffAndScaleThread::setExit */
        bool ShouldExit_;
    };
    
    /*! A processor class that takes the scaled difference from two producers - Inherits from ImageProducer, but having two consumer members.*/
    class FLITR_EXPORT ImageDiffAndScale : public ImageProducer
    {
        friend class ImageDiffAndScaleThread;
    public:
        
        /*! Constructor.
         *@param upStreamProducerA The first upstream proWducer.
         *@param upStreamProducerB The second upstream producer.
         *@param images_per_slot The number of images per image slot from the upstream producer and that is produced down stream.
         *@param buffer_size The size of the shared image buffer of the downstream producer.*/
        ImageDiffAndScale(ImageProducer& upStreamProducerA,
                          ImageProducer& upStreamProducerB,
                          float scale,
                          uint32_t images_per_slot,
                          uint32_t buffer_size=FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS);
        
        /*! Virtual destructor */
        virtual ~ImageDiffAndScale();
        
        /*! Method to initialise the object.
         *@return Boolean result flag. True indicates successful initialisation.*/
        virtual bool init();
        
        /*!Method to start the asynchronous trigger thread. trigger() has to be called synchronously/manually if thread not started.
         *@sa ImageProcessor::trigger*/
        virtual bool startTriggerThread();
        virtual bool stopTriggerThread();
        virtual bool isTriggerThreadStarted() const {return Thread_!=0;}
        
        /*!Synchronous trigger method. Called automatically by the trigger thread if started.
         *@sa ImageProcessor::startTriggerThread*/
        virtual bool trigger();
        
        /*! Get the image format being consumed from the upstream producer.*/
        virtual ImageFormat getUpstreamFormat(const uint32_t consumer_index, const uint32_t img_index = 0) const {return ImageConsumerVec_[consumer_index]->getFormat(img_index);}
        
        
        /*! Get the image format being produced to the downstream consumers.*/
        virtual ImageFormat getDownstreamFormat(const uint32_t img_index = 0) const {return ImageProducer::getFormat(img_index);}
        
    protected:
        const uint32_t ImagesPerSlot_;
        const uint32_t buffer_size_;
        const float scale_;
        
        /*! StatsCollector member to measure the time taken by the processor.*/
        std::shared_ptr<StatsCollector> ProcessorStats_;
        
    private:
        ImageDiffAndScaleThread *Thread_;
        
        std::vector<std::shared_ptr<ImageConsumer> > ImageConsumerVec_;
    };
}

#endif //IMAGE_DIFF_AND_SCALE_H
