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

#ifndef FFMPEG_READER_H
#define FFMPEG_READER_H 1

#ifndef FLITR_USE_SWSCALE
#define FLITR_USE_SWSCALE 1
#endif

#include <flitr/image.h>
#include <flitr/log_message.h>
#include <flitr/stats_collector.h>

extern "C" {
#if defined FLITR_USE_SWSCALE
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/mathematics.h>
#include <libavutil/error.h>
#include <libavcodec/avcodec.h>
#include <libavutil/pixdesc.h>
#include <libavutil/pixfmt.h>
#else
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>
#include <libavutil/error.h>
#include <libavcodec/avcodec.h>
#include <libavutil/pixdesc.h>
#include <libavutil/pixfmt.h>
#endif
}

#undef PixelFormat

#include <iostream>
#include <sstream>

namespace flitr {

/// Class thrown when video errors occur.
struct FFmpegReaderException {
    FFmpegReaderException() {}
};

/**
 * Class to encapsulate the reading of images from video files using FFmpeg.
 */
class FLITR_EXPORT FFmpegReader {
  public:
    /** 
     * Creates a reader for a video file.
     * 
     * \param filename Input video file name.
     * \param out_pix_fmt The format the image data would be converted.
     * to prior to output.
     *
     */
    FFmpegReader(std::string filename, ImageFormat::PixelFormat out_pix_fmt);
    ~FFmpegReader();

    /** 
     * Reads the specified frame from the video file.
     * 
     * \param out_image Image to receive the read data.
     * \param im_number Image or frame number in the video (0-based).
     * 
     * \return True if an image was successfully read.
     */
    bool getImage(Image &out_image, int im_number);

    /** 
     * Obtain the number of images/frames present in the video file.
     * 
     * \return Number of images/frames. 
     */
    uint32_t getNumImages() { return NumImages_; }

    /** 
     * Get the number of the last successfully decoded image.
     * 
     * \return 0 to getNumImages()-1 for valid frames or -1 when not
     * successfully triggered.
     */
    int32_t getCurrentImage() { return CurrentImage_; }

    /** 
     * Get the format that would be used for output.
     * 
     * \return Image format of the output data.
     */
    ImageFormat getFormat() { return ImageFormat_; }

    /**
     * Get the frame rate of the video.
     *
     * \return Frame rate.
     */
    uint32_t getFrameRate() const { return FrameRate_; }

  private:
    /// Used for multi-threading access.
    static int lockManager(void **mutex, enum AVLockOp op);

    AVFormatContext *FormatContext_;
    //AVFormatParameters FormatParameters_;
    AVCodecContext *CodecContext_;
    AVCodec *Codec_;
    int VideoStreamIndex_;
    /// Frame to receive the internal format decoded video.
    AVFrame* DecodedFrame_;
    /// Frame containing the image data converted to output format.
    AVFrame* ConvertedFrame_;

    uint32_t FrameRate_;
    /// Output format.
    ImageFormat ImageFormat_;
    /// Number of frames/images in the video.
    uint32_t NumImages_;
    /// -1 if nothing decoded yet, otherwise the number of the last
    /// -decoded frame.
    int32_t CurrentImage_;
    std::string FileName_;
    /// True if e.g. reading from jpg or png files.
    bool SingleFrameSource_;
    bool SingleFrameDone_;
    std::shared_ptr<Image> SingleImage_;

    std::shared_ptr<StatsCollector> SwscaleStats_;
    std::shared_ptr<StatsCollector> GetImageStats_;


#if defined FLITR_USE_SWSCALE
    struct SwsContext *ConvertFormatCtx_;
#endif
};

}

#endif //FFMPEG_READER_H
