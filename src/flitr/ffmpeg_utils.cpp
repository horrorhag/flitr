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

#include <flitr/ffmpeg_utils.h>

#include <iostream>

AVPixelFormat flitr::PixelFormatFLITrToFFmpeg(flitr::ImageFormat::PixelFormat in_fmt)
{
    switch (in_fmt) {
      case ImageFormat::FLITR_PIX_FMT_Y_8:
        return AV_PIX_FMT_GRAY8;
        break;
      case ImageFormat::FLITR_PIX_FMT_RGB_8:
        return AV_PIX_FMT_RGB24;
        break;
      case ImageFormat::FLITR_PIX_FMT_Y_16:
        return AV_PIX_FMT_GRAY16LE;
        break;
      case ImageFormat::FLITR_PIX_FMT_BGRA:
          return AV_PIX_FMT_BGRA;
          break;
      default:
        // \todo maybe return error for unhandled FLITr pix format!
        return AV_PIX_FMT_NONE;
    }
}

flitr::ImageFormat::PixelFormat flitr::PixelFormatFFmpegToFLITr(AVPixelFormat in_fmt)
{
    switch (in_fmt) {
      case AV_PIX_FMT_GRAY8:
        return ImageFormat::FLITR_PIX_FMT_Y_8;
        break;
      case AV_PIX_FMT_RGB24:
        return ImageFormat::FLITR_PIX_FMT_RGB_8;
        break;
      case AV_PIX_FMT_GRAY16LE:
        return ImageFormat::FLITR_PIX_FMT_Y_16;
        break;
      default:
        return ImageFormat::FLITR_PIX_FMT_UNDF;
    }
}

AVFrame* flitr::allocFFmpegFrame(AVPixelFormat pix_fmt, int width, int height)
{
    AVFrame *picture;
    uint8_t *picture_buf;
    int size;

    picture = avcodec_alloc_frame();
    if (!picture)
        return NULL;

    size = avpicture_get_size(pix_fmt, width, height);
    picture_buf = (uint8_t *)av_malloc(size);
    if (!picture_buf) {
        av_free(picture);
        return NULL;
    }       

    picture->width=width;
    picture->height=height;
    picture->format=int(pix_fmt);

    avpicture_fill((AVPicture *)picture, picture_buf,
                   pix_fmt, width, height);

    return picture;
}
