#include <flitr/v4l2_producer.h>

#define CLEAR(x) memset (&(x), 0, sizeof (x))

flitr::V4L2Producer::V4L2Producer(ImageFormat::PixelFormat out_pix_fmt,
                           std::string device_file, uint32_t buffer_size,
                           int32_t width_to_set,
                           int32_t height_to_set,
                           int32_t channel_to_set) :
    OutputPixelFormat_(out_pix_fmt),
    DeviceFile_(device_file),
    DeviceFD_(-1),
    DeviceWidth_(width_to_set),
    DeviceHeight_(height_to_set),
    DeviceChannel_(channel_to_set),
    ShouldExit_(false),
    buffer_size_(buffer_size)
{
    av_register_all();
    
    // \todo match v4l and ffmpeg pix formats
    InputFFmpegPixelFormat_ = PIX_FMT_YUYV422;
    FinalFFmpegPixelFormat_ = PixelFormatFLITrToFFmpeg(out_pix_fmt);
}

flitr::V4L2Producer::~V4L2Producer()
{
    ShouldExit_=true;

    if (readThread_.joinable() == true)
    {
        readThread_.join();
    }

    //first free allocated memory
    for (int n = 0; n < DeviceNumBuffers_; n++)
    {
        vc_buffer buf = DeviceBuffers_[n];
        munmap(buf.start, buf.length);
    }

    if (DeviceFD_ != -1)
    {
        //turn off camera streaming
        v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        xioctl(DeviceFD_, VIDIOC_STREAMOFF, &type);
        close(DeviceFD_);
    }
}

bool flitr::V4L2Producer::init()
{

    if (!open_device()) {
        return false;
    }
    if (!init_device()) {
        return false;
    }

    int w(DeviceWidth_);
    int h(DeviceHeight_);

    // now we know the format
    ImageFormat_.push_back(ImageFormat(w, h, OutputPixelFormat_));

    // Allocate storage
    SharedImageBuffer_ = std::shared_ptr<SharedImageBuffer>(new SharedImageBuffer(*this, buffer_size_, 1));
    SharedImageBuffer_->initWithStorage();

    InputV42LFrame_ = allocFFmpegFrame(InputFFmpegPixelFormat_, w, h);
    FinalFrame_ = allocFFmpegFrame(FinalFFmpegPixelFormat_, w, h);
    if (!FinalFrame_ || !InputV42LFrame_) {
        logMessage(LOG_CRITICAL)
                << DeviceFile_
                << ": cannot allocate memory for video storage buffers.\n";
        return false;
    }
    
    ConvertInToFinalCtx_ = sws_getContext(
                ImageFormat_[0].getWidth(), ImageFormat_[0].getHeight(), InputFFmpegPixelFormat_,
            ImageFormat_[0].getWidth(), ImageFormat_[0].getHeight(), FinalFFmpegPixelFormat_,
            SWS_BILINEAR, NULL, NULL, NULL);

    if (!start_capture())
    {
        return false;
    }

    std::thread t(&V4L2Producer::read_thread, this);
    readThread_.swap(t);

    return true;
}

int flitr::V4L2Producer::xioctl(int fd, int request, void * arg)
{
    int r;
    do {
        r = ioctl (fd, request, arg);
    } while (-1 == r && EINTR == errno);
    
    return r;
}

bool flitr::V4L2Producer::open_device()
{
    struct v4l2_capability cap;

    DeviceFD_ = open(DeviceFile_.c_str(), O_RDWR, 0);
    //DeviceFD_ = open(dev_name, O_RDWR | O_NONBLOCK, 0);

    if (-1 == DeviceFD_) {
        logMessage(LOG_CRITICAL)
                << "Cannot open video device "
                << DeviceFile_
                << ": error " << errno << ", "
                << std::string(strerror(errno)) << "\n";
        return false;
    }
    
    if (-1 == xioctl(DeviceFD_, VIDIOC_QUERYCAP, &cap)) {
        if (EINVAL == errno) {
            logMessage(LOG_CRITICAL)
                    << DeviceFile_
                    << ": is not a V4L2 device\n";
            close(DeviceFD_);
            DeviceFD_ = -1;
            return false;
        } else {
            close(DeviceFD_);
            DeviceFD_ = -1;
            return false;
        }
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        logMessage(LOG_CRITICAL)
                << DeviceFile_
                << ": is not a capture device\n";
        close(DeviceFD_);
        DeviceFD_ = -1;
        return false;
    }
    
    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        logMessage(LOG_CRITICAL)
                << DeviceFile_
                << ": does not support streaming I/O\n";
        close(DeviceFD_);
        DeviceFD_ = -1;
        return false;
    }

    return true;
}

bool flitr::V4L2Producer::init_device()
{
    struct v4l2_format cap_fmt;
    CLEAR(cap_fmt);
    cap_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (DeviceWidth_ == -1 || DeviceHeight_ == -1) {
        // query current settings
        if (-1 == xioctl(DeviceFD_, VIDIOC_G_FMT, &cap_fmt))
        {
            logMessage(LOG_CRITICAL)
                    << DeviceFile_
                    << ": failure in VIDIOC_G_FMT\n";
            close(DeviceFD_);
            DeviceFD_ = -1;
            return false;
        }
        DeviceWidth_ = cap_fmt.fmt.pix.width;
        DeviceHeight_ = cap_fmt.fmt.pix.height;
    }

    // Set image format.
    CLEAR(cap_fmt);

    cap_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    cap_fmt.fmt.pix.width = DeviceWidth_;
    cap_fmt.fmt.pix.height = DeviceHeight_;
    // \todo query formats
    cap_fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    cap_fmt.fmt.pix.field = V4L2_FIELD_ANY;


    if (-1 == xioctl(DeviceFD_, VIDIOC_S_FMT, &cap_fmt))
    {
        logMessage(LOG_CRITICAL)
                << DeviceFile_
                << ": failure in VIDIOC_S_FMT\n";
        close(DeviceFD_);
        DeviceFD_ = -1;
        return false;
    }

    // use the dimensions as received back
    DeviceWidth_ = cap_fmt.fmt.pix.width;
    DeviceHeight_ = cap_fmt.fmt.pix.height;

    // set channel
    if (DeviceChannel_ != -1)
    {
        if (-1 == xioctl(DeviceFD_, VIDIOC_S_INPUT, &DeviceChannel_))
        {
            logMessage(LOG_CRITICAL)
                    << DeviceFile_
                    << ": cannot set channel\n";
            close(DeviceFD_);
            DeviceFD_ = -1;
            return false;
        }
    }

    /*
    v4l2_std_id pal_std = V4L2_STD_PAL;
    if(-1 == xioctl(DeviceFD_, VIDIOC_S_STD, &pal_std))
    {
        logMessage(LOG_CRITICAL)
                << DeviceFile_
                << ": cannot set video standard to PAL\n";
        close(DeviceFD_);
            DeviceFD_ = -1;
        return false;
    }
*/

    //initlaize memory map.
    if (!vc_init_mmap()) {
        close(DeviceFD_);
        DeviceFD_ = -1;
        return false;
    }

    return true;
}

bool flitr::V4L2Producer::vc_init_mmap()
{
    struct v4l2_requestbuffers req;
    CLEAR (req);
    DeviceNumBuffers_ = 4;
    req.count = DeviceNumBuffers_;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl (DeviceFD_, VIDIOC_REQBUFS, &req)) {
        logMessage(LOG_CRITICAL)
                << DeviceFile_
                << ": failure in VIDIOC_REQBUFS\n";
        close(DeviceFD_);
        DeviceFD_ = -1;
        return false;
    }
    
    //printf("req count = %d\n", req.count);
    if (req.count < 2) {
        logMessage(LOG_CRITICAL)
                << DeviceFile_
                << ": insufficient buffer memory\n";
        close(DeviceFD_);
        DeviceFD_ = -1;
        return false;
    }

    DeviceNumBuffers_ = req.count;

    //fprintf(stderr,"sizeof %d", sizeof(*DeviceBuffers_));

    DeviceBuffers_ = (vc_buffer *)calloc(req.count, sizeof(struct vc_buffer));
    if (!DeviceBuffers_) {
        logMessage(LOG_CRITICAL)
                << DeviceFile_
                << ": insufficient buffer memory\n";
        return false;
    }

    for (int n = 0; n < (int)(req.count); n++) {
        struct v4l2_buffer buf;
        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n;

        if (-1 == xioctl (DeviceFD_, VIDIOC_QUERYBUF, &buf)) {
            logMessage(LOG_CRITICAL)
                    << DeviceFile_
                    << ": failure in VIDIOC_QUERYBUF\n";
            close(DeviceFD_);
            DeviceFD_ = -1;
            return false;
        }
        
        DeviceBuffers_[n].length = buf.length;
        DeviceBuffers_[n].start = mmap (NULL /* start anywhere */,
                                        buf.length,
                                        PROT_READ | PROT_WRITE /* required */,
                                        MAP_SHARED /* recommended */,
                                        DeviceFD_, buf.m.offset);
        
        if (MAP_FAILED == DeviceBuffers_[n].start) {
            logMessage(LOG_CRITICAL)
                    << DeviceFile_
                    << ": failure in mmap\n";
            close(DeviceFD_);
            DeviceFD_ = -1;
            return false;
        }
    }

    return true;
}

bool flitr::V4L2Producer::start_capture()
{
    
    for (int n = 0; n < DeviceNumBuffers_; n++) {
        struct v4l2_buffer buf;
        CLEAR (buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n;
        
        if (-1 == xioctl(DeviceFD_, VIDIOC_QBUF, &buf)) {
            logMessage(LOG_CRITICAL)
                    << DeviceFile_
                    << ": failure in VIDIOC_QBUF\n";
            return false;
        }
    }

    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl(DeviceFD_, VIDIOC_STREAMON, &type)) {
        logMessage(LOG_CRITICAL)
                << DeviceFile_
                << ": failure in VIDIOC_STREAMON\n";
        return false;
    }

    //fprintf(stderr,"started\n");
    return true;
}

bool flitr::V4L2Producer::get_frame(void)
{	

    struct v4l2_buffer buf;
    CLEAR(buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    
    if (-1 == xioctl(DeviceFD_, VIDIOC_DQBUF, &buf)) {
        switch (errno) {
        case EAGAIN:
            return false;
        case EIO:
            /* Could ignore EIO, see spec. */
            /* fall through */
        default:
            return false;
        }
    }

    // we have a frame
    
    // create an input picture from the buffer
    avpicture_fill((AVPicture *)InputV42LFrame_,
                   (uint8_t *)DeviceBuffers_[buf.index].start,
            InputFFmpegPixelFormat_,
            DeviceWidth_,
            DeviceHeight_);
    
    // convert to the final format
    // \todo deinterlace here

    // Get address of memory to write image to.
    std::vector<Image**> imvec = reserveWriteSlot();
    if (imvec.size() == 0) {
        // no storage available
        logMessage(LOG_CRITICAL)
                << ": dropping frames - no space in buffer\n";
        xioctl (DeviceFD_, VIDIOC_QBUF, &buf);
        return false;
    }

    Image& out_image = *(*(imvec[0]));
    // Point final frame to sharedImageBuffer data
    FinalFrame_->data[0] = out_image.data();

    //Convert image to final format
    sws_scale(ConvertInToFinalCtx_,
              InputV42LFrame_->data, InputV42LFrame_->linesize, 0, DeviceHeight_,
              FinalFrame_->data, FinalFrame_->linesize);

    /*
    // timestamp
    if (CreateMetadataFunction_) {
        out_image.setMetadata(CreateMetadataFunction_());
    } else {
        std::shared_ptr<DefaultMetadata> meta(new DefaultMetadata());
        meta->PCTimeStamp_ = currentTimeNanoSec();
        out_image.setMetadata(meta);
    }
*/

    // Notify that new data has been written to buffer.
    releaseWriteSlot();

    // enqueue again
    if (-1 == xioctl (DeviceFD_, VIDIOC_QBUF, &buf)) {
        return false;
    }
    return true;
}

void flitr::V4L2Producer::read_thread()
{
    while(!ShouldExit_) {
        get_frame();
    }
}

