#ifndef V4L2_PRODUCER_H
#define V4L2_PRODUCER_H 1


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

//#include "default_metadata.h"

#include <flitr/ffmpeg_utils.h>
#include <flitr/image_format.h>
#include <flitr/image_producer.h>
#include <flitr/log_message.h>
#include <flitr/high_resolution_time.h>

#include <thread>

namespace flitr {

struct vc_buffer {
    void * start;
    size_t length;
};

class V4L2Producer : public flitr::ImageProducer {
  public:
	V4L2Producer(flitr::ImageFormat::PixelFormat out_pix_fmt, 
                 std::string device_file,
                 uint32_t buffer_size = FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS,
                 int32_t width_to_set = -1,
                 int32_t height_to_set = -1,
                 int32_t channel_to_set = -1);
    
    ~V4L2Producer();
    bool init();
    
  private:
    void read_thread();

    std::thread readThread_;

    int xioctl(int fd, int request, void * arg);
    bool open_device();
    bool init_device();
    bool vc_init_mmap();
    bool start_capture();
    bool get_frame();

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

    bool ShouldExit_;

    uint32_t buffer_size_;
};

} // namespace flitr


#endif //V4L2_PRODUCER_H
