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
# include <avformat.h>
# include <swscale.h>
# include <libavutil/mathematics.h>
# include <libavcodec/avcodec.h>	
#else
# include <avformat.h>
# include <libavutil/mathematics.h>
# include <libavcodec/avcodec.h>
#endif
}

#include <boost/tr1/memory.hpp>

#include <iostream>
#include <sstream>

namespace flitr {

/// Class thrown when video errors occur.
struct FFmpegWriterException {
    FFmpegWriterException() {}
};

enum VideoContainer {
    FLITR_AVI_CONTAINER = 0,
    FLITR_MKV_CONTAINER = 1
};

enum VideoCodec {
    FLITR_RAWVIDEO_CODEC = 0,
    FLITR_FFV1_CODEC = 1
};

class FLITR_EXPORT FFmpegWriter {
public:
    FFmpegWriter(std::string filename, const ImageFormat& image_format,
                 const uint32_t frame_rate=FLITR_DEFAULT_VIDEO_FRAME_RATE,
                 VideoContainer container=FLITR_AVI_CONTAINER,
                 VideoCodec codec=FLITR_RAWVIDEO_CODEC);

    ~FFmpegWriter();

    bool writeVideoFrame(uint8_t *in_buf);

private:
    /// Create a video stream
    AVStream *addVideoStream(AVFormatContext *fc, int codec_id);
	
	/// Open video file and prepare codec.
    bool openVideoFile();

    /// Close video file.
    bool closeVideoFile();

	/// Holds the input frame.
    AVFrame *InputFrame_;
    PixelFormat InputFrameFormat_;

    /// Holds the frame converted to the format required by the codec.
    AVFrame* SaveFrame_;
    PixelFormat SaveFrameFormat_;

    /// Format of the output video
    AVOutputFormat *OutputFormat_;
    AVFormatContext *FormatContext_;
    AVStream *VideoStream_;

#if defined FLITR_USE_SWSCALE
    struct SwsContext *ConvertToSaveCtx_;
#endif

	ImageFormat ImageFormat_;
    std::string SaveFileName_;
    uint32_t FrameRate_;
    /// Holds the number of frames written to disk.
    uint32_t WrittenFrameCount_;

    uint8_t* VideoEncodeBuffer_;
    uint32_t VideoEncodeBufferSize_;
};

}

#endif //FFMPEG_WRITER_H
