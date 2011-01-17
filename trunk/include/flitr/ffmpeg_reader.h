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
# include <avformat.h>
# include <swscale.h>
#else
# include <avformat.h>
#endif
}

#include <boost/tr1/memory.hpp>

#include <iostream>
#include <sstream>

namespace flitr {

/// Class thrown when video errors occur.
struct FFmpegReaderException {
    FFmpegReaderException() {}
};

class FLITR_EXPORT FFmpegReader {
  public:
    FFmpegReader(std::string filename, ImageFormat::PixelFormat out_pix_fmt);
    ~FFmpegReader();

    bool getImage(Image &out_image, int im_number);
    uint32_t getNumImages() { return NumImages_; }
    /** 
     * The last successfully decoded image.
     * 
     * \return 0 to getNumImages()-1 for valid frames or -1 when not successfully triggered.  
     */
    int32_t getCurrentImage() { return CurrentImage_; }
	ImageFormat getFormat() { return ImageFormat_; }

  private:
    static int lockManager(void **mutex, enum AVLockOp op);

    AVFormatContext *FormatContext_;
    AVFormatParameters FormatParameters_;
    AVCodecContext *CodecContext_;
    AVCodec *Codec_;
    int VideoStreamIndex_;
	AVFrame* ConvertedFrame_;

	ImageFormat ImageFormat_;
    uint32_t NumImages_;
	int32_t CurrentImage_;
    std::string FileName_;
    bool SingleFrameSource_;
    bool SingleFrameDone_;
    std::tr1::shared_ptr<Image> SingleImage_;

    std::tr1::shared_ptr<StatsCollector> SwscaleStats_;

#if defined FLITR_USE_SWSCALE
    struct SwsContext *ConvertFormatCtx_;
#endif
};

}

#endif //FFMPEG_READER_H
