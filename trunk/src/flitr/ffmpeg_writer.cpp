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

#include <flitr/ffmpeg_writer.h>
#include <flitr/ffmpeg_utils.h>

using namespace flitr;


FFmpegWriter::FFmpegWriter(std::string filename, const ImageFormat& image_format, const uint32_t frame_rate, VideoContainer container, VideoCodec codec) :
    ImageFormat_(image_format),
    SaveFileName_(filename),
    FrameRate_(frame_rate),
    WrittenFrameCount_(0)
{
    av_register_all();
    
    // be safe with size
    VideoEncodeBufferSize_ = ImageFormat_.getWidth() * ImageFormat_.getHeight() * 4;
    if (VideoEncodeBufferSize_ < FF_MIN_BUFFER_SIZE) {
        VideoEncodeBufferSize_ = FF_MIN_BUFFER_SIZE;
    }
    VideoEncodeBuffer_ = (uint8_t *)av_malloc(VideoEncodeBufferSize_);

    InputFrameFormat_ = PixelFormatFLITrToFFmpeg(image_format.getPixelFormat());
    InputFrame_ = allocFFmpegFrame(InputFrameFormat_, ImageFormat_.getWidth(), ImageFormat_.getHeight());

	// \todo use pix format expected by selected encoder
    // for now just use the same as input
    SaveFrameFormat_ = InputFrameFormat_;
    
    if (SaveFrameFormat_ == PIX_FMT_RGB24) {
        // for some strange reason ffmpeg defaults to upside down BGR24 whenever it finds raw colour video, so we also flip (colour and geom)
        SaveFrameFormat_ = PIX_FMT_BGR24;
    }

    //SaveFrameFormat_ = PIX_FMT_YUV422;
    
    SaveFrame_ = allocFFmpegFrame(SaveFrameFormat_, ImageFormat_.getWidth(), ImageFormat_.getHeight());

    if (!InputFrame_ || !SaveFrame_) {
		logMessage(LOG_CRITICAL) << "Cannot allocate memory for video storage buffers.\n";
		throw FFmpegWriterException();
    }

#if defined FLITR_USE_SWSCALE
    ConvertToSaveCtx_ = sws_getContext(ImageFormat_.getWidth(), ImageFormat_.getHeight(), InputFrameFormat_, 
                                       ImageFormat_.getWidth(), ImageFormat_.getHeight(), SaveFrameFormat_,					
                                       SWS_BILINEAR, NULL, NULL, NULL);
#endif

    // video file
    if (SaveFrameFormat_ == PIX_FMT_GRAY16LE) {
        OutputFormat_ = av_guess_format("matroska", NULL, NULL);
    } else {
    OutputFormat_ = av_guess_format("avi", NULL, NULL);
    }
    
    if (!OutputFormat_) {
		logMessage(LOG_CRITICAL) << "Cannot set video format.\n";
		throw FFmpegWriterException();
    }

    // open the file
    if (!openVideoFile()) {
		throw FFmpegWriterException();
    }
}

FFmpegWriter::~FFmpegWriter()
{
    closeVideoFile();

    av_free(InputFrame_);
    av_free(SaveFrame_);
    
    av_free(VideoEncodeBuffer_);
}

/// Create a video stream
AVStream *FFmpegWriter::addVideoStream(AVFormatContext *fc, int codec_id)
{
    AVCodecContext *cc;
    AVStream *st;

    st = av_new_stream(fc, 0); // stream 0 is the videoqU4kH5PJ7Yr7
    if (!st) {
		logMessage(LOG_CRITICAL) << "Cannot allocate video stream.\n";
        return NULL;
    }

    cc = st->codec;
    cc->codec_id = (CodecID)codec_id;
    cc->codec_type = AVMEDIA_TYPE_VIDEO;
	
	// experiment with coder type
	//cc->coder_type = FF_CODER_TYPE_AC; // faster for FFV1
	//cc->coder_type = FF_CODER_TYPE_VLC;
	//cc->coder_type = FF_CODER_TYPE_DEFLATE;
	//cc->coder_type = FF_CODER_TYPE_RAW;
	//cc->coder_type = FF_CODER_TYPE_RLE;
	
    if(fc->oformat->flags & AVFMT_GLOBALHEADER) {
        cc->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }

    /* put sample parameters */
    //cc->bit_rate = 4000000;
    //cc->bit_rate = 6000000;
    //cc->bit_rate = 100000000;
    
    /* resolution must be a multiple of two */
    cc->width = ImageFormat_.getWidth();
    cc->height = ImageFormat_.getHeight();

    cc->time_base.den = FrameRate_;

    cc->time_base.num = 1;
    cc->gop_size = 1;
    cc->pix_fmt = SaveFrameFormat_;
    return st;
}

bool FFmpegWriter::writeVideoFrame(uint8_t *in_buf)
{
    // fill the incoming picture
    avpicture_fill((AVPicture *)InputFrame_, in_buf, InputFrameFormat_, ImageFormat_.getWidth(), ImageFormat_.getHeight());

    int out_size, ret;
    AVCodecContext *cc;
    cc = VideoStream_->codec;

#if defined FLITR_USE_SWSCALE
    //if (SaveFrameFormat_ == PIX_FMT_BGR24) {
    //    // flip upside down
    //    InputFrame_->data[0] += InputFrame_->linesize[0] * (cc->height-1);
    //    InputFrame_->linesize[0] *= -1;
    //}
    
    //int *test = InputFrame_->linesize;
    //printf("%d %d %d %d\n", test[0], test[1], test[2], test[3]);
    int err = sws_scale(ConvertToSaveCtx_, 
						InputFrame_->data, InputFrame_->linesize, 0, cc->height,
						SaveFrame_->data, SaveFrame_->linesize);
#else
    img_convert((AVPicture *)SaveFrame_, cc->pix_fmt,
				(AVPicture *)InputFrame_, InputFrameFormat_,
				cc->width, cc->height);
#endif

    out_size = avcodec_encode_video(cc, VideoEncodeBuffer_, VideoEncodeBufferSize_, SaveFrame_);

    /* if zero size, it means the image was buffered */
    if (out_size > 0) {
		AVPacket pkt;
		av_init_packet(&pkt);
	
		pkt.pts = av_rescale_q(cc->coded_frame->pts, cc->time_base, VideoStream_->time_base);
		if(cc->coded_frame->key_frame) {
			pkt.flags |= AV_PKT_FLAG_KEY;
		}
		pkt.stream_index= VideoStream_->index;
		pkt.data = VideoEncodeBuffer_;
		pkt.size = out_size;
		ret = av_write_frame(FormatContext_, &pkt);
    } else {
		ret = 0;
    }
    if (ret != 0) {
		logMessage(LOG_CRITICAL) << "Error while writing video frame.\n";
		return false;
    }

    WrittenFrameCount_++;
    return true;
}

bool FFmpegWriter::openVideoFile()
{
    FormatContext_ = avformat_alloc_context();

    if (!FormatContext_) {
        logMessage(LOG_CRITICAL) << "Cannot allocate video format context.\n";
        return false;
    }
    FormatContext_->oformat = OutputFormat_;
    //snprintf(FormatContext_->filename, sizeof(FormatContext_->filename), "%s", SaveFileName_.c_str());
    sprintf(FormatContext_->filename, "%s", SaveFileName_.c_str());

    //VideoStream_ = addVideoStream(FormatContext_, OutputFormat_->video_codec);
    //VideoStream_ = addVideoStream(FormatContext_, CODEC_ID_FFV1);
    //VideoStream_ = addVideoStream(FormatContext_, CODEC_ID_FFVHUFF);
	//VideoStream_ = addVideoStream(FormatContext_, CODEC_ID_LJPEG);
	//VideoStream_ = addVideoStream(FormatContext_, CODEC_ID_MJPEG);
	VideoStream_ = addVideoStream(FormatContext_, CODEC_ID_RAWVIDEO);
    //VideoStream_ = addVideoStream(FormatContext_, CODEC_ID_JPEGLS);
    //VideoStream_ = addVideoStream(FormatContext_, CODEC_ID_BMP);

    if (VideoStream_ == NULL) {
		return false;
    }

#if LIBAVFORMAT_VERSION_INT < ((54<<16) + (21<<8) + 100)
    if (av_set_parameters(FormatContext_, NULL) < 0) {
        logMessage(LOG_CRITICAL) << "Cannot set video parameters.\n";
        return false;
    }
#endif

    if (getLogMessageCategory() & LOG_INFO) { 
#if LIBAVFORMAT_VERSION_INT >= ((54<<16) + (21<<8) + 100)
        av_dump_format(FormatContext_, 0, SaveFileName_.c_str(), 1);
#else
        dump_format(FormatContext_, 0, SaveFileName_.c_str(), 1);
#endif
    }
    
    // open video codec
    AVCodecContext *vcc = VideoStream_->codec;
    AVCodec *vcodec = avcodec_find_encoder(vcc->codec_id);

    if (!vcodec) {
		logMessage(LOG_CRITICAL) << "Cannot find video codec.\n";
		return false;
    }
    
    if (avcodec_open(vcc, vcodec) < 0) {
		logMessage(LOG_CRITICAL) << "Cannot open video codec.\n";
		return false;
    }

#if LIBAVFORMAT_VERSION_INT >= ((54<<16) + (21<<8) + 100)
    if (avio_open(&FormatContext_->pb, SaveFileName_.c_str(), AVIO_FLAG_WRITE) < 0) {
#else
    if (url_fopen(&FormatContext_->pb, SaveFileName_.c_str(), URL_WRONLY) < 0) {
#endif
		logMessage(LOG_CRITICAL) << "Cannot open video file " << SaveFileName_ << "\n";
		return false;
    }
    
#if LIBAVFORMAT_VERSION_INT >= ((54<<16) + (21<<8) + 100)
    avformat_write_header(FormatContext_,NULL);
#else
    av_write_header(FormatContext_);
#endif
    WrittenFrameCount_ = 0;
    
    return true;
}

bool FFmpegWriter::closeVideoFile()
{
    av_write_trailer(FormatContext_);
    
// check for lib version > 52.1.0
   #if LIBAVFORMAT_VERSION_INT >= ((54<<16) + (21<<8) + 100)
       avio_close(FormatContext_->pb);
   #elif LIBAVFORMAT_VERSION_INT > ((52<<16) + (1<<8) + 0)
       url_fclose(FormatContext_->pb);
   #else
       url_fclose(&FormatContext_->pb);
   #endif

    avcodec_close(VideoStream_->codec);
    
    for(unsigned int i=0; i < FormatContext_->nb_streams; i++) {
		av_freep(&FormatContext_->streams[i]->codec);
		av_freep(&FormatContext_->streams[i]);
    }

    av_free(FormatContext_);
	
	return true;
}


