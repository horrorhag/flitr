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

#define IPF_USE_SWSCALE 1
extern "C" {
#if defined IPF_USE_SWSCALE
# include <libavformat/avformat.h>
# include <libswscale/swscale.h>
#else
# include <avformat.h>
#endif
}

#undef PixelFormat

#include <cassert>

// V4L stuff
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include <linux/videodev2.h>

#include <flitr/ffmpeg_utils.h>
#include <flitr/image_format.h>
#include <flitr/image_producer.h>
#include <flitr/log_message.h>
#include <flitr/high_resolution_time.h>

using std::shared_ptr;
using namespace flitr;

#define CLEAR(x) memset (&(x), 0, sizeof (x))


namespace flitr
{
struct vc_buffer {
    void * start;
    size_t length;
};

struct VideoParam
{
    flitr::ImageFormat::PixelFormat OutputPixelFormat_;
    AVPixelFormat InputFFmpegPixelFormat_;
    AVPixelFormat FinalFFmpegPixelFormat_;

    std::string DeviceFile_;
    int DeviceFD_;
    int32_t DeviceWidth_;
    int32_t DeviceHeight_;
    int32_t DeviceChannel_;
    struct vc_buffer *DeviceBuffers_;
    int DeviceNumBuffers_;

    /// The frame as received from the device.
    AVFrame* InputV42LFrame_;
    
    /// The frame in the format the user requested.
    AVFrame* FinalFrame_;
    
    /// Convert from the input format to the final.
    struct SwsContext *ConvertInToFinalCtx_;

    uint32_t buffer_size_;
};
}

namespace
{
    static flitr::VideoParam* gVid = 0;

    const std::string DEFAULT_VIDEO_DEVICE = "/dev/video0";
    const uint32_t DEFAULT_BUFFER_SIZE = FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS;
    const int32_t DEFAULT_CHANNEL = -1;
}

int xioctl(int fd, int request, void * arg);
bool open_device();
bool init_device();
bool vc_init_mmap();
bool start_capture();

/**************************************************************
* Thread
***************************************************************/
void VideoProducer::VideoProducerThread::run()
{
    while (!shouldExit)
    {
        struct v4l2_buffer buf;
        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        
        if (-1 == xioctl(gVid->DeviceFD_, VIDIOC_DQBUF, &buf)) {
            switch (errno) {
              case EAGAIN:
                continue;
              case EIO:
                /* Could ignore EIO, see spec. */
                /* fall through */
              default:
                continue;
            }
        }

        // we have a frame
        
        // create an input picture from the buffer
        avpicture_fill((AVPicture *)gVid->InputV42LFrame_, 
                       (uint8_t *)gVid->DeviceBuffers_[buf.index].start, 
                       gVid->InputFFmpegPixelFormat_,
                       gVid->DeviceWidth_,
                       gVid->DeviceHeight_);
        
        // convert to the final format
        // \todo deinterlace here
        
        // Get address of memory to write image to.
        std::vector<Image**> imvec = producer->reserveWriteSlot(); 
        if (imvec.size() == 0) {
            // no storage available
            logMessage(LOG_CRITICAL) 
                << ": dropping frames - no space in buffer\n";
            xioctl (gVid->DeviceFD_, VIDIOC_QBUF, &buf);
            continue;
        }

        Image& out_image = *(*(imvec[0]));
        // Point final frame to sharedImageBuffer data
        gVid->FinalFrame_->data[0] = out_image.data();

        //Convert image to final format
        sws_scale(gVid->ConvertInToFinalCtx_, 
                  gVid->InputV42LFrame_->data, gVid->InputV42LFrame_->linesize, 0, gVid->DeviceHeight_,
                  gVid->FinalFrame_->data, gVid->FinalFrame_->linesize);

        // timestamp
         //if (CreateMetadataFunction_) {
            //out_image.setMetadata(CreateMetadataFunction_());
        //} else {
            ////shared_ptr<DefaultMetadata> meta(new DefaultMetadata());
            ////meta->PCTimeStamp_ = currentTimeNanoSec();
            ////out_image.setMetadata(meta);
        //}

        // Notify that new data has been written to buffer.
        producer->releaseWriteSlot();

        // enqueue again
        if (-1 == xioctl (gVid->DeviceFD_, VIDIOC_QBUF, &buf)) {
            continue;
        }  
    }   
}

/**************************************************************
* Producer
***************************************************************/

VideoProducer::VideoProducer(flitr::ImageFormat::PixelFormat pixelFormat /*= flitr::ImageFormat::FLITR_PIX_FMT_Y_8*/, unsigned int imageSlots /*= FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS*/, const std::string& deviceName /*= ""*/, int frameWidth /*= 1280*/, int frameHeight /*= 720*/)
    : thread_(0)
    , imageSlots_(imageSlots)
    , pixelFormat_(pixelFormat)
    , config_()
{
    av_register_all();
    // \todo match v4l and ffmpeg pix formats

    gVid = new flitr::VideoParam;

    gVid->InputFFmpegPixelFormat_ = PIX_FMT_YUYV422;
    gVid->FinalFFmpegPixelFormat_ = PixelFormatFLITrToFFmpeg(pixelFormat_);
    gVid->OutputPixelFormat_ = pixelFormat_;
    gVid->DeviceFile_ = DEFAULT_VIDEO_DEVICE;
    if (!deviceName.empty())
    {
        gVid->DeviceFile_ = deviceName;
    }
    gVid->DeviceWidth_ = frameWidth;
    gVid->DeviceHeight_ = frameHeight;
    gVid->DeviceChannel_ = DEFAULT_CHANNEL;
    gVid->buffer_size_ = DEFAULT_BUFFER_SIZE;
}

VideoProducer::VideoProducer(const std::string& config, flitr::ImageFormat::PixelFormat pixelFormat /*= flitr::ImageFormat::FLITR_PIX_FMT_Y_8*/, unsigned int imageSlots /*= FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS*/)
    : thread_(0)
    , imageSlots_(imageSlots)
    , pixelFormat_(pixelFormat)
    , config_()
{
    assert(0);
    // not implemented
}

VideoProducer::~VideoProducer()
{
    thread_->setExit();
    thread_->join();

    delete gVid;
    gVid = 0;
}

bool VideoProducer::init()
{
    if (!open_device()) {
        return false;
    }
    if (!init_device()) {
        return false;
    }

    int w(gVid->DeviceWidth_);
    int h(gVid->DeviceHeight_);

    // now we know the format
    ImageFormat_.push_back(ImageFormat(w, h, gVid->OutputPixelFormat_));

    // Allocate storage
    SharedImageBuffer_ = std::shared_ptr<SharedImageBuffer>(new SharedImageBuffer(*this, gVid->buffer_size_, 1));
    SharedImageBuffer_->initWithStorage();

    gVid->InputV42LFrame_ = allocFFmpegFrame(gVid->InputFFmpegPixelFormat_, w, h);
    gVid->FinalFrame_ = allocFFmpegFrame(gVid->FinalFFmpegPixelFormat_, w, h);
    if (!gVid->FinalFrame_ || !gVid->InputV42LFrame_) {
        logMessage(LOG_CRITICAL)
            << gVid->DeviceFile_
            << ": cannot allocate memory for video storage buffers.\n";
        return false;
    }
    
    gVid->ConvertInToFinalCtx_ = sws_getContext(
        ImageFormat_[0].getWidth(), ImageFormat_[0].getHeight(), gVid->InputFFmpegPixelFormat_, 
        ImageFormat_[0].getWidth(), ImageFormat_[0].getHeight(), gVid->FinalFFmpegPixelFormat_,
        SWS_BILINEAR, NULL, NULL, NULL);

    if (!start_capture())
    {
        return false;
    }
    // start thread
    thread_ = new VideoProducerThread(this);
    thread_->startThread();

    return true;
}

int xioctl(int fd, int request, void * arg)
{
    int r;
    do {
        r = ioctl (fd, request, arg);
    } while (-1 == r && EINTR == errno);
    
    return r;
}

bool open_device()
{
    struct v4l2_capability cap;

    gVid->DeviceFD_ = open(gVid->DeviceFile_.c_str(), O_RDWR, 0);
    //DeviceFD_ = open(dev_name, O_RDWR | O_NONBLOCK, 0);

    if (-1 == gVid->DeviceFD_) {
        logMessage(LOG_CRITICAL) 
            << "Cannot open video device " 
            << gVid->DeviceFile_
            << ": error " << errno << ", "
            << std::string(strerror(errno)) << "\n";
        return false;
    }
    
    if (-1 == xioctl(gVid->DeviceFD_, VIDIOC_QUERYCAP, &cap)) {
        if (EINVAL == errno) {
            logMessage(LOG_CRITICAL)
                << gVid->DeviceFile_
                << ": is not a V4L2 device\n";
            close(gVid->DeviceFD_);
            return false;
        } else {
            close(gVid->DeviceFD_);
            return false;
        }
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        logMessage(LOG_CRITICAL)
            << gVid->DeviceFile_
            << ": is not a capture device\n";
        close(gVid->DeviceFD_);
        return false;
    }
    
    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        logMessage(LOG_CRITICAL)
            << gVid->DeviceFile_
            << ": does not support streaming I/O\n";
        close(gVid->DeviceFD_);
        return false;
    }

    return true;
}

bool init_device()
{
    struct v4l2_format cap_fmt;
    CLEAR(cap_fmt);
    cap_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (gVid->DeviceWidth_ == -1 || gVid->DeviceHeight_ == -1) {
        // query current settings
        if (-1 == xioctl(gVid->DeviceFD_, VIDIOC_G_FMT, &cap_fmt))
        {
            logMessage(LOG_CRITICAL)
                << gVid->DeviceFile_
                << ": failure in VIDIOC_G_FMT\n";
            close(gVid->DeviceFD_);
            return false;
        }
        gVid->DeviceWidth_ = cap_fmt.fmt.pix.width;
        gVid->DeviceHeight_ = cap_fmt.fmt.pix.height;
    }

    // Set image format.
    CLEAR(cap_fmt);
    cap_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    cap_fmt.fmt.pix.width = gVid->DeviceWidth_;
    cap_fmt.fmt.pix.height = gVid->DeviceHeight_;
    // \todo query formats
    cap_fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;

    if (-1 == xioctl(gVid->DeviceFD_, VIDIOC_S_FMT, &cap_fmt))
    {
        logMessage(LOG_CRITICAL)
            << gVid->DeviceFile_
            << ": failure in VIDIOC_S_FMT\n";
        close(gVid->DeviceFD_);
        return false;
    }

    // use the dimensions as received back
    gVid->DeviceWidth_ = cap_fmt.fmt.pix.width;
    gVid->DeviceHeight_ = cap_fmt.fmt.pix.height;

    // set channel
    if (gVid->DeviceChannel_ != -1) {
        if (-1 == xioctl(gVid->DeviceFD_, VIDIOC_S_INPUT, &gVid->DeviceChannel_)) {
            logMessage(LOG_CRITICAL)
                << gVid->DeviceFile_
                << ": cannot set channel\n";
            close(gVid->DeviceFD_);
            return false;
        }
        v4l2_std_id pal_std = V4L2_STD_PAL;
        if(-1 == xioctl(gVid->DeviceFD_, VIDIOC_S_STD, &pal_std)) {
            logMessage(LOG_CRITICAL)
                << gVid->DeviceFile_
                << ": cannot set video standard to PAL\n";
            close(gVid->DeviceFD_);
            return false;
        }
    }
    
    //initlaize memory map.
    if (!vc_init_mmap()) {
        close(gVid->DeviceFD_);
        return false;
    }

    return true;
}

bool vc_init_mmap()
{
    struct v4l2_requestbuffers req;
    CLEAR (req);
    gVid->DeviceNumBuffers_ = 4;
    req.count = gVid->DeviceNumBuffers_;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl (gVid->DeviceFD_, VIDIOC_REQBUFS, &req)) {
        logMessage(LOG_CRITICAL)
            << gVid->DeviceFile_
            << ": failure in VIDIOC_REQBUFS\n";
        close(gVid->DeviceFD_);
        return false;
    }
    
    //printf("req count = %d\n", req.count);
    if (req.count < 2) {
        logMessage(LOG_CRITICAL)
            << gVid->DeviceFile_
            << ": insufficient buffer memory\n";
        close(gVid->DeviceFD_);
        return false;
    }

    gVid->DeviceNumBuffers_ = req.count;

    //fprintf(stderr,"sizeof %d", sizeof(*DeviceBuffers_));

    gVid->DeviceBuffers_ = (vc_buffer *)calloc(req.count, sizeof(struct vc_buffer));
    if (!gVid->DeviceBuffers_) {
        logMessage(LOG_CRITICAL)
            << gVid->DeviceFile_
            << ": insufficient buffer memory\n";
        return false;
    }

    for (int n = 0; n < (int)(req.count); n++) {
        struct v4l2_buffer buf;
        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n;
    
        if (-1 == xioctl (gVid->DeviceFD_, VIDIOC_QUERYBUF, &buf)) {
            logMessage(LOG_CRITICAL)
                << gVid->DeviceFile_
                << ": failure in VIDIOC_QUERYBUF\n";
            close(gVid->DeviceFD_);
            return false;
        }
        
        gVid->DeviceBuffers_[n].length = buf.length;
        gVid->DeviceBuffers_[n].start = mmap (NULL /* start anywhere */,
                                        buf.length,
                                        PROT_READ | PROT_WRITE /* required */,
                                        MAP_SHARED /* recommended */,
                                        gVid->DeviceFD_, buf.m.offset);
        
        if (MAP_FAILED == gVid->DeviceBuffers_[n].start) {
            logMessage(LOG_CRITICAL)
                << gVid->DeviceFile_
                << ": failure in mmap\n";
            close(gVid->DeviceFD_);
            return false;
        }
    }

    return true;
}

bool start_capture()
{
    
    for (int n = 0; n < gVid->DeviceNumBuffers_; n++) {
        struct v4l2_buffer buf;
        CLEAR (buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n;
        
        if (-1 == xioctl(gVid->DeviceFD_, VIDIOC_QBUF, &buf)) {
            logMessage(LOG_CRITICAL)
                << gVid->DeviceFile_
                << ": failure in VIDIOC_QBUF\n";
            return false;
        }
    }

    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl(gVid->DeviceFD_, VIDIOC_STREAMON, &type)) {
        logMessage(LOG_CRITICAL)
                << gVid->DeviceFile_
                << ": failure in VIDIOC_STREAMON\n";
            return false;
    }

    //fprintf(stderr,"started\n");
    return true;
}
