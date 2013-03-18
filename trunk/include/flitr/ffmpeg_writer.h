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

#ifndef FFMPEG_WRITER_H
#define FFMPEG_WRITER_H 1

#ifndef FLITR_USE_SWSCALE
#define FLITR_USE_SWSCALE 1
#endif

#include <flitr/ffmpeg_utils.h>
#include <flitr/image.h>
#include <flitr/log_message.h>
#include <flitr/stats_collector.h>

//#if !defined INT64_C
//# define INT64_C(c) c ## LL
//#endif

extern "C" {
#if defined FLITR_USE_SWSCALE
# include <libavformat/avformat.h>
# include <libswscale/swscale.h>
# include <libavutil/mathematics.h>
# include <libavcodec/avcodec.h>	
#include <libavutil/pixdesc.h>
#else
# include <libavformat/avformat.h>
# include <libavutil/mathematics.h>
# include <libavcodec/avcodec.h>
#include <libavutil/pixdesc.h>
#endif
}

#undef PixelFormat

#include <boost/tr1/memory.hpp>

#include <iostream>
#include <sstream>

namespace flitr {

/// Class thrown when video errors occur.
struct FFmpegWriterException {
    FFmpegWriterException() {}
};

enum VideoContainer {
    FLITR_ANY_CONTAINER = 0,
    FLITR_AVI_CONTAINER,
    FLITR_MKV_CONTAINER  //Will cause the ffmpeg consumers to use .mkv extension.
};

enum VideoCodec {//NB: Still need to test all the codecs and only include the ones that work.
    FLITR_NONE_CODEC = AV_CODEC_ID_NONE,
    FLITR_RAWVIDEO_CODEC = AV_CODEC_ID_RAWVIDEO,
    FLITR_MPEG1VIDEO_CODEC = AV_CODEC_ID_MPEG1VIDEO,
    FLITR_MPEG2VIDEO_CODEC = AV_CODEC_ID_MPEG2VIDEO,
    FLITR_MSMPEG4V3_CODEC = AV_CODEC_ID_MSMPEG4V3,
    FLITR_MJPEG_CODEC = AV_CODEC_ID_MJPEG,
    FLITR_MJPEGB_CODEC = AV_CODEC_ID_MJPEGB,
    FLITR_LJPEG_CODEC = AV_CODEC_ID_LJPEG,
    FLITR_JPEGLS_CODEC = AV_CODEC_ID_JPEGLS,
    FLITR_FFV1_CODEC = AV_CODEC_ID_FFV1,
    FLITR_MPEG4_CODEC = AV_CODEC_ID_MPEG4,
    FLITR_HUFFYUV_CODEC = AV_CODEC_ID_HUFFYUV,
    FLITR_THEORA_CODEC = AV_CODEC_ID_THEORA,
    FLITR_DIRAC_CODEC = AV_CODEC_ID_DIRAC
};


class FLITR_EXPORT FFmpegWriter {
public:
    FFmpegWriter(std::string filename, const ImageFormat& image_format,
                 const uint32_t frame_rate=FLITR_DEFAULT_VIDEO_FRAME_RATE,
                 VideoContainer container=FLITR_ANY_CONTAINER,
                 VideoCodec codec=FLITR_RAWVIDEO_CODEC,
                 int32_t bit_rate=-1);

    ~FFmpegWriter();

    bool writeVideoFrame(uint8_t *in_buf);

private:

    VideoContainer Container_;
    VideoCodec Codec_;

    AVFormatContext *FormatContext_;
    AVStream *VideoStream_;
    AVCodec *AVCodec_;
    AVCodecContext *AVCodecContext_;

	/// Open video file and prepare codec.
    bool openVideoFile();

    /// Close video file.
    bool closeVideoFile();

	/// Holds the input frame.
    AVFrame *InputFrame_;
    AVPixelFormat InputFrameFormat_;

    /// Holds the frame converted to the format required by the codec.
    AVFrame* SaveFrame_;
    AVPixelFormat SaveFrameFormat_;

    /// Format of the output video
    AVOutputFormat *OutputFormat_;

#if defined FLITR_USE_SWSCALE
    struct SwsContext *ConvertToSaveCtx_;
#endif

	ImageFormat ImageFormat_;
    std::string SaveFileName_;
    AVRational FrameRate_;
    /// Holds the number of frames written to disk.
    uint64_t WrittenFrameCount_;

    std::tr1::shared_ptr<StatsCollector> WriteFrameStats_;

    uint8_t* VideoEncodeBuffer_;
    uint32_t VideoEncodeBufferSize_;
};

}

#endif //FFMPEG_WRITER_H
