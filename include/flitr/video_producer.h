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
#include <flitr/flitr_thread.h>

#ifdef _WIN32
#  include <flitr/video_win32_directshow.h>
#endif

#ifdef __linux
#	include <flitr/video_linux_v4l.h>
#endif

#ifdef __APPLE__
//#  include <flitr/video_mac_osx.h>
#endif

namespace flitr 
{

/**
 * Generic cross-platform video producer
 * 
 * This class provides cross-platform video input. Supported platforms:
 *  - Windows: with Microsoft DirectShow.
 *  - Linux: with Video4Linux.
 */
class FLITR_EXPORT VideoProducer : public ImageProducer 
{
    /*!
    *   Updates of the producer image buffer
    */
    class VideoProducerThread : public FThread
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
    VideoProducer(flitr::ImageFormat::PixelFormat pixelFormat = flitr::ImageFormat::FLITR_PIX_FMT_Y_8, unsigned int imageSlots = FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS, const std::string& deviceName = "", int frameWidth = 1280, int frameHeight = 720);
    VideoProducer(const std::string& config, flitr::ImageFormat::PixelFormat pixelFormat = flitr::ImageFormat::FLITR_PIX_FMT_Y_8, unsigned int imageSlots = FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS);
    ~VideoProducer();
    bool init();

    //unsigned char* GetPixelBuffer();

private:
    VideoProducerThread* thread_;
    flitr::ImageFormat::PixelFormat pixelFormat_;
    unsigned int imageSlots_;  
    std::string config_;
};
}
#endif // VIDEO_PRODUCER_H