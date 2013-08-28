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

#ifndef VIDEO_PRODUCER_H
#define VIDEO_PRODUCER_H 1

#include <flitr/image_producer.h>

#include <OpenThreads/Thread>

#ifdef _WIN32
#  include <flitr/video_win32_directshow.h>
#endif

#ifdef __linux
#	include <flitr/video_linux_v4l.h>
#endif

#ifdef __APPLE__
#  include <flitr/video_mac_osx.h>
#endif

namespace flitr 
{

/**
 * Generic cross-platform video producer
 * 
 * This class provides cross-platform video input. Supported platforms:
 *  - Windows: with Microsoft DirectShow.
 *  - Linux: with Video4Linux.
 *  - Macintosh: with QuickTime.
 */
class FLITR_EXPORT VideoProducer : public ImageProducer 
{
	/*!
    *   Updates of the producer image buffer
    */
    class VideoProducerThread : public OpenThreads::Thread
    {
    public:
        VideoProducerThread(VideoProducer* _producer) : producer(_producer), shouldExit(false) {}
        void run();
        void setExit() { shouldExit = true; }

    private:
        VideoProducer* producer;
        bool shouldExit;
    };

public:
	VideoProducer();
    ~VideoProducer();
    bool init();

private:
	VideoProducerThread* thread;
    uint32_t imageSlots;

};
}
#endif // VIDEO_PRODUCER_H