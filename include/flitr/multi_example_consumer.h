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

#ifndef MULTI_EXAMPLE_CONSUMER_H
#define MULTI_EXAMPLE_CONSUMER_H 1

#include <flitr/ffmpeg_utils.h>
#include <flitr/image_consumer.h>
#include <flitr/ffmpeg_writer.h>
#include <flitr/metadata_writer.h>

#include <OpenThreads/Thread>
#include <OpenThreads/Mutex>

namespace flitr
{
    class MultiExampleConsumer;
    
    class MultiExampleConsumerThread : public OpenThreads::Thread
    {
    public:
        MultiExampleConsumerThread(MultiExampleConsumer *consumer) :
        _consumer(consumer),
        _shouldExit(false) {}
        void run();
        void setExit() { _shouldExit = true; }
    private:
        
        MultiExampleConsumer *_consumer;
        bool _shouldExit;
    };
    
    class FLITR_EXPORT MultiExampleConsumer : public ImageConsumer
    {
        friend class MultiExampleConsumerThread;
    public:
        
        MultiExampleConsumer(ImageProducer& producer, const uint32_t imagesPerSlot);
        
        virtual ~MultiExampleConsumer();
        
        //!Calls the base class initialiser and starts up the consumer thread.
        bool init();
        
        //!Open the connection to some image sink.
        bool openConnection(const std::string &streamName);
        
        //!Close the connection.
        bool closeConnection();
        
        //!Example of using a scoped lock.
        int getterThatMightRequireAScopedLock(const uint32_t imNumber)
        {
            OpenThreads::ScopedLock<OpenThreads::Mutex> wlock(*_accessMutex);
            return 0;
        }
        
    private:
        //!Vector of image formats of the images in the slot.
        std::vector<ImageFormat> _imageFormatVec;
        
        //!Number of images per image slot.
        const uint32_t _imagesPerSlot;
        
        //!Consumer thread that processes the FLITr video stream.
        MultiExampleConsumerThread *_thread;
        
        //!Example of mutex that may be used for access control if required.
        std::shared_ptr<OpenThreads::Mutex> _accessMutex;
    };
    
}

#endif //MULTI_EXAMPLE_CONSUMER_H
