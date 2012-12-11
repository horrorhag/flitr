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

#include <flitr/image_consumer.h>
#include <flitr/image_producer.h>
#include <flitr/stats_collector.h>

#include <OpenThreads/Thread>
#include <OpenThreads/Mutex>

namespace flitr {

class ImageProcessor;

/*! Helper/Service thread class for ImageProcessor that consumes and produces images as they become available from the upstream producer.*/
class ImageProcessorThread : public OpenThreads::Thread {
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

/*! A processor class inheriting from both ImageConsumer and ImageProducer. Consumes flitr images as input and then produces flitr images as output.*/
class FLITR_EXPORT ImageProcessor : public ImageConsumer, public ImageProducer {
    friend class ImageProcessorThread;
public:

    /*! Constructor given the upstream producer.
     *@param producer The upstream image producer.
     *@param images_per_slot The number of images per image slot from the upstream producer.
     *@param buffer_size The size of the shared image buffer of the downstream producer.*/
    ImageProcessor(ImageProducer& producer, uint32_t images_per_slot, uint32_t buffer_size=FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS);

    /*! Virtual destructor */
    virtual ~ImageProcessor();

    /*! Method to initialise the object.
     *@return Boolean result flag. True indicates successful initialisation.*/
    bool init();

private:
    uint32_t ImagesPerSlot_;
    uint32_t buffer_size_;

    ImageProcessorThread *Thread_;

    /*! StatsCollector member to measure the time taken by the processor.*/
    std::tr1::shared_ptr<StatsCollector> ProcessorStats_;
};

}

#endif //IMAGE_PROCESSOR_H
