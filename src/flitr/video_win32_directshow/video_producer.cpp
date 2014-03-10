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

#include <flitr/video_producer.h>
#include <flitr/image.h>
#include <flitr/ffmpeg_utils.h>
#include <stdlib.h>
#include "comutil.h"
#include "DSVL.h"
#include "tinyxml.h"
#include <sstream>

#include <boost/lexical_cast.hpp>

#define IPF_USE_SWSCALE 1
extern "C" {
#if defined IPF_USE_SWSCALE
# include <libavformat/avformat.h>
# include <libswscale/swscale.h>
#else
# include <avformat.h>
#endif
}

using namespace flitr;

namespace flitr
{
struct VideoParam
{
    DSVL_VideoSource* graphManager;
    MemoryBufferHandle g_Handle;
    bool bufferCheckedOut;
    __int64 g_Timestamp;


    flitr::ImageFormat::PixelFormat OutputPixelFormat_;
    AVPixelFormat InputFFmpegPixelFormat_;
    AVPixelFormat FinalFFmpegPixelFormat_;

    AVFrame* InputFrame_;
    AVFrame* FinalFrame_;
    
    struct SwsContext *ConvertInToFinalCtx_;

    int32_t DeviceWidth_;
    int32_t DeviceHeight_;
};
}

namespace
{
    static flitr::VideoParam* gVid = 0;
    const std::string config_default = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><dsvl_input><camera show_format_dialog=\"false\" friendly_name=\"PGR\" frame_width=\"1280\" frame_height=\"720\"><pixel_format><YUY2 flip_h=\"false\" flip_v=\"false\"/></pixel_format></camera></dsvl_input>";

    void SetConfigSetting(std::string& config, const std::string& attribute, const std::string& newValue)
    {
        TiXmlDocument doc;
        doc.Parse((const char*)config.c_str(), 0, TIXML_ENCODING_UTF8);

        TiXmlElement *e = doc.FirstChildElement("dsvl_input")->FirstChildElement("camera");
        if (e)
        {
            e->SetAttribute(attribute.c_str(), newValue.c_str());
        }
        std::stringstream s;
        s <<doc;
        config = s.str();
    }
}

/**************************************************************
* Thread
***************************************************************/
void VideoProducer::VideoProducerThread::run()
{
    std::vector<Image**> aImages;
    //std::vector<uint8_t> rgb(producer->ImageFormat_.back().getBytesPerImage());

    while (!shouldExit)
    {
        // wait for video frame
        bool ready = true;

        //
        //std::vector<uint8_t> rgb(producer->ImageFormat_.back().getBytesPerImage());
        do
        {
            if (gVid->bufferCheckedOut) 
            {
                if (FAILED(gVid->graphManager->CheckinMemoryBuffer(gVid->g_Handle)))
                {
                    ready = false;
                }
                else
                {
                    gVid->bufferCheckedOut = false;
                }
            }
            ready = false;
            if (gVid->graphManager->WaitForNextSample(0L) == WAIT_OBJECT_0)
            {
                unsigned char* pixelBuffer;
                if (!FAILED(gVid->graphManager->CheckoutMemoryBuffer(&(gVid->g_Handle), &pixelBuffer, NULL, NULL, NULL, &(gVid->g_Timestamp))))
                {
                    gVid->bufferCheckedOut = true;
                    ready = true;
                    //uint8_t* buffer_vid = static_cast<uint8_t*>(pixelBuffer);
                    //std::copy(buffer_vid, buffer_vid + producer->ImageFormat_.back().getBytesPerImage(), rgb.begin());

                    // create an input picture from the buffer
                    avpicture_fill((AVPicture *)gVid->InputFrame_, 
                    (uint8_t *)pixelBuffer, 
                    gVid->InputFFmpegPixelFormat_,
                    gVid->DeviceWidth_,
                    gVid->DeviceHeight_);
                }
            }
            
            if (!ready)
            {
                Thread::microSleep(1000);
            }

        } while (!ready && !shouldExit);

        if (gVid->bufferCheckedOut) 
        {
            if (!FAILED(gVid->graphManager->CheckinMemoryBuffer(gVid->g_Handle, true)))
            {
                gVid->bufferCheckedOut = false;
            }
        }

        // wait until there is space in the write buffer so that we can get a valid image pointer
        do
        {
            aImages = producer->reserveWriteSlot();
            if (aImages.size() != 1)
            {
                Thread::microSleep(1000);
            }
        } while ((aImages.size() != 1) && !shouldExit);

        if (!shouldExit)
        {
            //Image* img = *(aImages[0]);
            Image& out_image = *(*(aImages[0]));
            // Point final frame to sharedImageBuffer data
            gVid->FinalFrame_->data[0] = out_image.data();

            //Convert image to final format
            sws_scale(gVid->ConvertInToFinalCtx_, 
            gVid->InputFrame_->data, gVid->InputFrame_->linesize, 0, gVid->DeviceHeight_,
            gVid->FinalFrame_->data, gVid->FinalFrame_->linesize);
            //uint8_t* buffer = img->data();
            //std::copy(rgb.begin(), rgb.end(), buffer);
        }

        producer->releaseWriteSlot();

        Thread::microSleep(1000);
    }
}

//unsigned char* VideoProducer::GetPixelBuffer()
//{
//    if (gVid->graphManager->WaitForNextSample(0L) == WAIT_OBJECT_0)
//    {
//        unsigned char* pixelBuffer;
//        if (!FAILED(gVid->graphManager->CheckoutMemoryBuffer(&(gVid->g_Handle), &pixelBuffer, NULL, NULL, NULL, &(gVid->g_Timestamp))))
//        {
//            gVid->bufferCheckedOut = true;
//            return (pixelBuffer);
//        }
//    }
//    return 0;
//}

/**************************************************************
* Producer
***************************************************************/

VideoProducer::VideoProducer(flitr::ImageFormat::PixelFormat pixelFormat /*= flitr::ImageFormat::FLITR_PIX_FMT_Y_8*/, unsigned int imageSlots /*= FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS*/, const std::string& deviceName /*= ""*/, int frameWidth /*= 1280*/, int frameHeight /*= 720*/)
    : thread_(0)
    , imageSlots_(imageSlots)
    , pixelFormat_(pixelFormat)
    , config_(config_default)
{
    SetConfigSetting(config_, "frame_width", boost::lexical_cast<std::string>(frameWidth));
    SetConfigSetting(config_, "frame_height", boost::lexical_cast<std::string>(frameHeight));
    if (!deviceName.empty())
    {
        SetConfigSetting(config_, "device_name", deviceName);
    }
}

VideoProducer::VideoProducer(const std::string& config, flitr::ImageFormat::PixelFormat pixelFormat /*= flitr::ImageFormat::FLITR_PIX_FMT_Y_8*/, unsigned int imageSlots /*= FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS*/)
    : thread_(0)
    , imageSlots_(imageSlots)
    , pixelFormat_(pixelFormat)
    , config_(config)
{
}

VideoProducer::~VideoProducer()
{
    if (!gVid && !gVid->graphManager) return;
    
    if (gVid->bufferCheckedOut) 
    {
        gVid->graphManager->CheckinMemoryBuffer(gVid->g_Handle, true);
        gVid->bufferCheckedOut = false;
    }

    // PRL 2005-09-21: Commented out due to issue where stopping the
    // media stream cuts off glut's periodic tasks, including functions
    // registered with glutIdleFunc() and glutDisplayFunc();
    //if(FAILED(vid->graphManager->Stop())) return (-1);

    if (gVid->bufferCheckedOut) 
        gVid->graphManager->CheckinMemoryBuffer(gVid->g_Handle, true);

    thread_->setExit();
    thread_->join();

    gVid->graphManager->Stop();
    delete gVid->graphManager;
    gVid->graphManager = NULL;
    free(gVid);
    gVid = 0;

    // COM should be closed down in the same context
    CoUninitialize();
}

bool VideoProducer::init()
{
    gVid = new flitr::VideoParam;
    gVid->bufferCheckedOut = false;

    gVid->InputFFmpegPixelFormat_ = PIX_FMT_YUYV422;
    gVid->FinalFFmpegPixelFormat_ = PixelFormatFLITrToFFmpeg(pixelFormat_);
    gVid->OutputPixelFormat_ = pixelFormat_;

    CoInitialize(NULL);

    gVid->graphManager = new DSVL_VideoSource();
    if (config_.empty() || config_.find("<?xml") != 0) 
    {
        config_ = config_default;
    } 
    
    char* c = new char[config_.size() + 1];
    std::copy(config_.begin(), config_.end(), c);
    c[config_.size()] = '\0';
    if (FAILED(gVid->graphManager->BuildGraphFromXMLString(c))) return false;
    if (FAILED(gVid->graphManager->EnableMemoryBuffer())) return false;

    
    if (gVid == 0) return false;
    if (gVid->graphManager == 0) return false;

    long frame_width;
    long frame_height;
    gVid->graphManager->GetCurrentMediaFormat(&frame_width, &frame_height, 0, 0);

    gVid->DeviceWidth_ = frame_width;
    gVid->DeviceHeight_ = frame_height;

    ImageFormat_.push_back(ImageFormat(frame_width, frame_height, pixelFormat_));

    SharedImageBuffer_ = std::tr1::shared_ptr<SharedImageBuffer>(new SharedImageBuffer(*this, imageSlots_, 1));
    SharedImageBuffer_->initWithStorage();


    int w(frame_width);
    int h(frame_height);
    gVid->InputFrame_ = allocFFmpegFrame(gVid->InputFFmpegPixelFormat_, w, h);
    gVid->FinalFrame_ = allocFFmpegFrame(gVid->FinalFFmpegPixelFormat_, w, h);
    if (!gVid->FinalFrame_ || !gVid->InputFrame_) {
        logMessage(LOG_CRITICAL)
            //<< gVid->DeviceFile_
            << ": cannot allocate memory for video storage buffers.\n";
        return false;
    }
    
    gVid->ConvertInToFinalCtx_ = sws_getContext(
        ImageFormat_[0].getWidth(), ImageFormat_[0].getHeight(), gVid->InputFFmpegPixelFormat_, 
        ImageFormat_[0].getWidth(), ImageFormat_[0].getHeight(), gVid->FinalFFmpegPixelFormat_,
        SWS_BILINEAR, NULL, NULL, NULL);



    if (FAILED(gVid->graphManager->Run())) false;
        


    thread_ = new VideoProducerThread(this);
    thread_->startThread();

    return true;
}