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

#ifndef FFMPEG_UTILS_H
#define FFMPEG_UTILS_H 1

#include <flitr/image_format.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/pixfmt.h>
}

#undef PixelFormat

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(52, 64, 0)
 #ifndef AVMEDIA_TYPE_VIDEO
  #define AVMEDIA_TYPE_VIDEO CODEC_TYPE_VIDEO
 #endif
 #ifndef AV_PKT_FLAG_KEY
  #define AV_PKT_FLAG_KEY PKT_FLAG_KEY
 #endif
#endif

#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(52, 39, 9)
 #define avformat_alloc_context av_alloc_format_context
#endif

#define FLITR_DEFAULT_VIDEO_FRAME_RATE 20

namespace flitr {

/** 
 * Converts from FLITr pixel format to FFmpeg format.
 * 
 * \param in_fmt FLITr pixel format.
 * 
 * \return FFmpeg pixel format.
 */
AVPixelFormat PixelFormatFLITrToFFmpeg(ImageFormat::PixelFormat in_fmt);

ImageFormat::PixelFormat PixelFormatFFmpegToFLITr(AVPixelFormat in_fmt);

/**
 * Utility function to allocate and fill an FFmpeg frame.
 * 
 * \param pix_fmt FFmpeg pixel format.
 * \param width Width in pixels.
 * \param height Height in pixels.
 * 
 * \return Pointer to a created AVFrame if successful, otherwise NULL.
 */
AVFrame *allocFFmpegFrame(AVPixelFormat pix_fmt, int width, int height);

}

#endif //FFMPEG_UTILS_H
