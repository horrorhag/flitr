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

#ifndef IMAGE_PROCESSOR_H
#define IMAGE_PROCESSOR_H 1

#include <flitr/modules/parameters/parameters.h>
#include <flitr/image_consumer.h>
#include <flitr/image_producer.h>
#include <flitr/stats_collector.h>

#include <flitr/flitr_thread.h>

#include <mutex>

namespace flitr {
    
    class ImageProcessor;
    
    /*! Helper/Service thread class for ImageProcessor that consumes and produces images as they become available from the upstream producer.*/
    class ImageProcessorThread : public FThread
    {
    public:
        
        /*! Constructor given a pointer to the ImageProcessor object.*/
        ImageProcessorThread(ImageProcessor *ip) :
        IP_(ip),
        ShouldExit_(false) {}
        
        /*! The thread's run method.*/
        void run();
        
        /*! Method to notify the thread to exit.*/
        void setExit() { ShouldExit_ = true; }
        
    private:
        /*! A pointer to the ImageProcessor object being serviced.*/
        ImageProcessor *IP_;
        
        /*! Boolean flag set to true by ImageProcessorThread::setExit.
         *@sa ImageProcessorThread::setExit */
        bool ShouldExit_;
    };
    
    /*! A processor class inheriting from both ImageConsumer and ImageProducer. Consumes flitr images as input and then produces flitr images as output.
     *
     * Derived classes should make sure to pass metadata from the upstream images to the
     * downstream image. The PassMetadataFunction_ member can be used when the passing
     * function should be defined by the user. */
    class FLITR_EXPORT ImageProcessor : public ImageConsumer, public ImageProducer, virtual public Parameters
    {
        friend class ImageProcessorThread;
    public:
        
        /*! Constructor given the upstream producer.
         *@param upStreamProducer The upstream image producer.
         *@param images_per_slot The number of images per image slot from the upstream producer and that is produced down stream.
         *@param buffer_size The size of the shared image buffer of the downstream producer.*/
        ImageProcessor(ImageProducer& upStreamProducer,
                       uint32_t images_per_slot,
                       uint32_t buffer_size=FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS);
        
        /*! Virtual destructor */
        virtual ~ImageProcessor();
        
        /*! Method to initialise the object.
         *@return Boolean result flag. True indicates successful initialisation.*/
        virtual bool init();
        
        /*! Method to start the asynchronous trigger thread.
         *
         * trigger() has to be called synchronously/manually if thread not started.
         *@sa ImageProcessor::trigger()
         *
         * The optional argument @a cpu_affinity can be used to specify the
         * CPU affinity for the trigger thread. See the FThread::startThread() and
         * FThread::applyAffinity() functions for more information. */
        virtual bool startTriggerThread(int32_t cpu_affinity = -1);
        virtual bool stopTriggerThread();
        virtual bool isTriggerThreadStarted() const {return Thread_!=0;}
        
        /*! Synchronous trigger method. Called automatically by the trigger thread if started.
         *@sa ImageProcessor::startTriggerThread() */
        virtual bool trigger() = 0;
        
        /*! Get the image format being consumed from the upstream producer.*/
        virtual ImageFormat getUpstreamFormat(const uint32_t img_index = 0) const { return ImageConsumer::getFormat(img_index);}
        
        /*! Get the image format being produced to the downstream consumers.*/
        virtual ImageFormat getDownstreamFormat(const uint32_t img_index = 0) const {return ImageProducer::getFormat(img_index);}
        
        /*! Get number of frames processed. */
        virtual size_t getFrameNumber()
        {
            return frameNumber_;
        }

        /*! Get the processor target for this image processor */
        virtual flitr::Parameters::EPassType getPassType() { return flitr::Parameters::CPU_PASS; }

        /*! Set a function that will pass the input image metadata to the output image.
         *
         * By default the PassMetadataFunction on the processor will set the metadata on the
         * input image to the metadata on the output image. It will not clone the metadata
         * but only copy the pointer. Due to this care must be taken when changing the metadata
         * in the pass since it will affect the metadata of images before the pass. If this is not
         * desired a pass can be set that clones the metadata.
         *
         * The default pass is implemented as follow:
         * @code
         * processor->setPassMetadataFunction([](std::shared_ptr<ImageMetadata> readMetadata){ return readMetadata; });
         * @endcode
         *
         * If the metadata must be cloned the following code can be used:
         * @code
         * processor->setPassMetadataFunction([](std::shared_ptr<ImageMetadata> readMetadata){ return readMetadata; });
         * @endcode
         *
         * It is recommended to call this function before startTriggerThread() is called
         * to avoid potential threading issues. */
        virtual void setPassMetadataFunction(PassMetadataFunction f)
        {
            PassMetadataFunction_ = f;
        }

    protected:
        const uint32_t ImagesPerSlot_;
        const uint32_t buffer_size_;
        
        /*! StatsCollector member to measure the time taken by the processor.*/
        std::shared_ptr<StatsCollector> ProcessorStats_;
        /*! Function to pass metadata from the upstream image to the downstream image.
         *
         * Can be used as follow in derived classes:
         * @code
         * if(PassMetadataFunction_ != nullptr)
         * {
         *     imWrite->setMetadata(PassMetadataFunction_(imRead->metadata()));
         * }
         * @endcode
         *
         * @sa setPassMetadataFunction() */
        PassMetadataFunction PassMetadataFunction_;
        
    protected:
        ImageProcessorThread *Thread_;
        
    protected:
        mutable std::mutex triggerMutex_;
        
        size_t frameNumber_;
    };
    
    
}

#endif //IMAGE_PROCESSOR_H
