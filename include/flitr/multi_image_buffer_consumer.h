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

#ifndef MULTI_WEBRTC_CONSUMER_H
#define MULTI_WEBRTC_CONSUMER_H 1

#include <flitr/ffmpeg_utils.h>
#include <flitr/image_consumer.h>
#include <flitr/ffmpeg_writer.h>
#include <flitr/metadata_writer.h>

#include <flitr/flitr_thread.h>

namespace flitr
{
    class MultiImageBufferConsumer;
    
    class MultiImageBufferConsumerThread : public FThread
    {
    public:
        MultiImageBufferConsumerThread(MultiImageBufferConsumer *consumer) :
        _consumer(consumer),
        _shouldExit(false) {}
        void run();
        void setExit() { _shouldExit = true; }
    private:
        
        MultiImageBufferConsumer *_consumer;
        bool _shouldExit;
    };
    
    class FLITR_EXPORT MultiImageBufferConsumer : public ImageConsumer
    {
        friend class MultiImageBufferConsumerThread;
    public:
        
        MultiImageBufferConsumer(ImageProducer& producer, const uint32_t imagesPerSlot);
        
        virtual ~MultiImageBufferConsumer();
        
        //!Calls the base class initialiser and starts up the consumer thread.
        bool init();
        
        //!Set the buffers to user defined memory location.
        void setBufferVec(const std::vector<uint8_t *> bufferVec, const std::vector<uint64_t *> bufferSeqNumberVec);
        
        //!Set buffer hold.
        void setBufferHold(const bool hold);
        
        
    private:
        //!Vector of image formats of the images in the slot.
        std::vector<ImageFormat> _imageFormatVec;
        
        //!Vector of user defined buffer locations.
        std::vector<uint8_t *> _bufferVec;
        
        //!Vector of user defined buffer seq numbers.
        std::vector<uint64_t *> _bufferSeqNumberVec;
        
        //!Indicates if buffers are on hold i.e. not being updated!
        bool _buffersHold;
        
        //!Number of images per image slot.
        const uint32_t _imagesPerSlot;
        
        //!Consumer thread that processes the FLITr video stream.
        MultiImageBufferConsumerThread *_thread;
        
        //!Mutex used with setBufferHold(...).
        std::mutex _buffersHoldMutex;
    };
    
}

#endif //MULTI_WEBRTC_CONSUMER_H
