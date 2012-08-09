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

#include <flitr/ffmpeg_reader.h>
#include <flitr/ffmpeg_utils.h>

#include <OpenThreads/Mutex>

#include <sstream>

using namespace flitr;
using std::tr1::shared_ptr;


FFmpegReader::FFmpegReader(std::string filename, ImageFormat::PixelFormat out_pix_fmt) :
    FileName_(filename),
    SingleFrameSource_(false),
    SingleFrameDone_(false),
    FrameRate_(FLITR_DEFAULT_VIDEO_FRAME_RATE)
{
    std::stringstream sws_stats_name;
    sws_stats_name << filename << " swscale";
    SwscaleStats_ = shared_ptr<StatsCollector>(new StatsCollector(sws_stats_name.str()));

    av_lockmgr_register(&lockManager);

    av_register_all();
    
    FormatContext_ = NULL;

    FormatContext_ = avformat_alloc_context();

    CodecContext_ = NULL;
    Codec_ = NULL;

    //int err = av_open_input_file(&FormatContext_, FileName_.c_str(), NULL, 0, &FormatParameters_);
#if LIBAVFORMAT_VERSION_INT >= ((54<<16) + (21<<8) + 100)
    int err = avformat_open_input(&FormatContext_, FileName_.c_str(), NULL, NULL);
#else
    int err = av_open_input_file(&FormatContext_, FileName_.c_str(), NULL, 0, NULL);
#endif

    if (err < 0) {
        logMessage(LOG_CRITICAL) << "av_open_input_file failed on " << filename.c_str() << " with code " << err << "\n";
        throw FFmpegReaderException();
    }
    
    av_find_stream_info(FormatContext_);
    
    // look for video streams
    for(uint32_t i=0; i < FormatContext_->nb_streams; i++) {
        CodecContext_ = FormatContext_->streams[i]->codec;
        if(CodecContext_->codec_type == AVMEDIA_TYPE_VIDEO) {
            // look for codec
            Codec_ = avcodec_find_decoder(CodecContext_->codec_id);
            if (Codec_) {
                VideoStreamIndex_ = i;
                break; // no need to search further
            }
        }
    }
    
    if (!Codec_) {
        logMessage(LOG_CRITICAL) << "Cannot find video codec for " << filename.c_str() << "\n";
        throw FFmpegReaderException();
    }

    if (avcodec_open(CodecContext_, Codec_) < 0) {
        logMessage(LOG_CRITICAL) << "Cannot open video codec for " << filename.c_str() << "\n";
        throw FFmpegReaderException();
    }


    // everything should be ready for decoding video
    
    if (getLogMessageCategory() & LOG_INFO) {
#if LIBAVFORMAT_VERSION_INT >= ((54<<16) + (21<<8) + 100)
        av_dump_format(FormatContext_, 0, FileName_.c_str(), 0);
#else
        dump_format(FormatContext_, 0, FileName_.c_str(), 0);
#endif
    }
    
    double duration_seconds = double(FormatContext_->duration) / 1e6;
    FrameRate_ = double(FormatContext_->streams[VideoStreamIndex_]->r_frame_rate.num) / double(FormatContext_->streams[VideoStreamIndex_]->r_frame_rate.den);
    NumImages_ = 0.5 + (duration_seconds * FrameRate_);
    logMessage(LOG_DEBUG) << "FFmpegReader gets duration: " << duration_seconds << " frame rate: " << FrameRate_ << " total frames: " << NumImages_ << "\n";


    //=== convert or reset IP format to ffmpeg format ===//
    PixelFormat out_ffmpeg_pix_fmt;

    if (out_pix_fmt!=ImageFormat::FLITR_PIX_FMT_ANY)
    {
        out_ffmpeg_pix_fmt=PixelFormatFLITrToFFmpeg(out_pix_fmt);
    } else
    {
        out_ffmpeg_pix_fmt=CodecContext_->pix_fmt;
        out_pix_fmt=PixelFormatFFmpegToFLITr(out_ffmpeg_pix_fmt);

        if (out_pix_fmt==ImageFormat::FLITR_PIX_FMT_UNDF)
        {//FLITr doesn't understand the ffmpeg pixel format and the user selected ImageFormat::FLITR_PIX_FMT_ANY.
         //Therefore, make a output pixelformat selection for the user.
            out_pix_fmt=ImageFormat::FLITR_PIX_FMT_RGB_8;
            out_ffmpeg_pix_fmt=PixelFormatFLITrToFFmpeg(out_pix_fmt);

            std::cout << "FFmpeg pixel format " << CodecContext_->pix_fmt << " is FLITR_PIX_FMT_UNDF. Defaulting to FLITR_PIX_FMT_RGB_8.\n";
        }
    }

    ImageFormat_ = ImageFormat(CodecContext_->width, CodecContext_->height, out_pix_fmt);
    //=== ===//

    // Allocate the image for the single frame sources
    // it will be reused
    SingleImage_ = shared_ptr<Image>(new Image(ImageFormat_));


        // create scaler to convert from video format to user required format
#if defined FLITR_USE_SWSCALE
        ConvertFormatCtx_ = sws_getContext(
                    ImageFormat_.getWidth(), ImageFormat_.getHeight(), CodecContext_->pix_fmt,
                    ImageFormat_.getWidth(), ImageFormat_.getHeight(), (PixelFormat)out_ffmpeg_pix_fmt,
                    SWS_BILINEAR, NULL, NULL, NULL);
#endif
    
    DecodedFrame_ = avcodec_alloc_frame();
    if (!DecodedFrame_) {
        logMessage(LOG_CRITICAL) << "Cannot allocate memory for decoded frame.\n";
        throw FFmpegReaderException();
    }

    ConvertedFrame_ = allocFFmpegFrame(out_ffmpeg_pix_fmt, ImageFormat_.getWidth(), ImageFormat_.getHeight());
    if (!ConvertedFrame_) {
        logMessage(LOG_CRITICAL) << "Cannot allocate memory for video storage buffers.\n";
        throw FFmpegReaderException();
    }

    if (NumImages_ == 1) {
        // Mostly when images used as input
        SingleFrameSource_ = true;
    }

    CurrentImage_ = -1;
}

FFmpegReader::~FFmpegReader()
{
    if (DecodedFrame_) {
        av_free(DecodedFrame_);
    }
    avcodec_close(CodecContext_);
    av_close_input_file(FormatContext_);
}

bool FFmpegReader::getImage(Image &out_image, int im_number)
{
    // convert frame number to time
    int64_t seek_time = ((int64_t)im_number * 1000000 * FormatContext_->streams[VideoStreamIndex_]->r_frame_rate.den) / (FormatContext_->streams[VideoStreamIndex_]->r_frame_rate.num);
    
    //std::cout << "Seektime = " << seek_time << "\n";
    //std::cout << "FrameNum = " << im_number << "\n";
    //std::cout << "RateNum  = " << FormatContext_->streams[VideoStreamIndex_]->r_frame_rate.num << "\n";
    //std::cout << "RateDen  = " << FormatContext_->streams[VideoStreamIndex_]->r_frame_rate.den << "\n";

    // Do not seek if we have a single frame source
    // Assume we are looking for frame 0, XXX assert maybe
    if (SingleFrameSource_) {
        // if we already have a frame, just copy it, do not read again
        if (SingleFrameDone_) {
            out_image = *SingleImage_;
            return true;
        }
    } else {
        // only seek if we are not playing or restarting
        if ((CurrentImage_ <= 0) || ((im_number - CurrentImage_) != 1)  ) {
            //MS VS does not seem to allow initialisation lists used in the AV_TIME_BASE_Q macro.
            //int seekret = av_seek_frame(FormatContext_, VideoStreamIndex_, av_rescale_q(seek_time, AV_TIME_BASE_Q, FormatContext_->streams[VideoStreamIndex_]->time_base), AVSEEK_FLAG_BACKWARD);
            AVRational avTimeBaseQ;
            avTimeBaseQ.num=1;
            avTimeBaseQ.den=AV_TIME_BASE;
            int seekret = av_seek_frame(FormatContext_, VideoStreamIndex_, av_rescale_q(seek_time, avTimeBaseQ , (FormatContext_->streams[VideoStreamIndex_])->time_base), AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_ANY);
            if (seekret < 0) {
                // start from beginning
                seekret = av_seek_frame(FormatContext_, -1, 0, AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_ANY);
            }
            avcodec_flush_buffers(CodecContext_);
            //std::cout << "Seekret = " << seekret << "\n";
            //std::cout << "Rescale = " << av_rescale_q(seek_time, AV_TIME_BASE_Q, FormatContext_->streams[VideoStreamIndex_]->time_base) << "\n";
            //std::cout << "Norm    = " << seek_time << "\n";
        }
    }

    // decode
    AVPacket pkt;
    int gotframe;
    avcodec_get_frame_defaults(DecodedFrame_);

    int loopcount=0;
    while(1) {
        if (av_read_frame(FormatContext_, &pkt) < 0 ) {
            //XXX
            return false;
        }
        if (pkt.stream_index == VideoStreamIndex_) {
            int decoded_len = avcodec_decode_video2(CodecContext_, DecodedFrame_, &gotframe, &pkt);
            if (decoded_len < 0) {
                logMessage(LOG_DEBUG) << "Error while decoding video\n";
                return false;
            }
            if (gotframe) {
                //MS VS does not seem to allow initialisation lists used in the AV_TIME_BASE_Q macro.
                //int64_t pkt_pts_scaled = av_rescale_q(pkt.pts, FormatContext_->streams[VideoStreamIndex_]->time_base, AV_TIME_BASE_Q);
                //int64_t pkt_duration_scaled = av_rescale_q(pkt.duration, FormatContext_->streams[VideoStreamIndex_]->time_base, AV_TIME_BASE_Q);
                AVRational avTimeBaseQ;
                avTimeBaseQ.num=1;
                avTimeBaseQ.den=AV_TIME_BASE;
                int64_t pkt_pts_scaled = av_rescale_q(pkt.pts, FormatContext_->streams[VideoStreamIndex_]->time_base, avTimeBaseQ);
                int64_t pkt_duration_scaled = av_rescale_q(pkt.duration, FormatContext_->streams[VideoStreamIndex_]->time_base, avTimeBaseQ);

                //if (pkt_pts_scaled > 0 && loopcount == 0) {
                //  std::cout << "seekt   = " << seek_time << "\n";
                //  std::cout << "PTS     = " << pkt_pts_scaled << "\n";
                //  std::cout << "Dur     = " << pkt_duration_scaled << "\n";
                //}

                if (pkt_pts_scaled >= seek_time ||
                        pkt_pts_scaled + pkt_duration_scaled > seek_time) {
                    // we are done
                    //fprintf(stderr, "LOOP %d\n", loopcount);
                    break;
                }
            }
            loopcount++;
        }
        av_free_packet(&pkt); // allocated in read_frame
    }

    if (!gotframe) {
        av_free_packet(&pkt);
        return false;
    }

    SwscaleStats_->tick();
#if defined FLITR_USE_SWSCALE
    ConvertedFrame_->data[0] = out_image.data(); // save a memcpy
    
    int err = sws_scale(ConvertFormatCtx_,
                        DecodedFrame_->data, DecodedFrame_->linesize, 0, CodecContext_->height,
                        ConvertedFrame_->data, ConvertedFrame_->linesize);
    
    //printf("%d %d\n", DecodedFrame_->linesize, ConvertedFrame_->linesize);
#else
    img_convert((AVPicture *)ConvertedFrame_, PIX_FMT_GRAY8,
                (AVPicture *)DecodedFrame_, CodecContext_->pix_fmt,
                CodecContext_->width, CodecContext_->height);
#endif
    SwscaleStats_->tock();

    /*
    // only needed if we cannot directly scale into the target memory
    memcpy(&(out_image.Data_[0]), ((AVPicture *)ConvertedFrame_)->data[0],
           ImageFormat_.getBytesPerImage());
    */

    if (SingleFrameSource_ && !SingleFrameDone_) {
        *SingleImage_ = out_image;
        SingleFrameDone_ = true;
    }

    av_free_packet(&pkt);

    CurrentImage_ = im_number;
    return true;
}

int FFmpegReader::lockManager(void **mutex, enum AVLockOp op)
{
    OpenThreads::Mutex **m = (OpenThreads::Mutex**)mutex;
    if (op==AV_LOCK_CREATE) {
        *m = new OpenThreads::Mutex;
        return !*m;
    }
    else if (op==AV_LOCK_DESTROY) {
        delete *m;
        return 0;
    }
    else if (op==AV_LOCK_OBTAIN) {
        (*m)->lock();
        return 0;
    }
    else if (op==AV_LOCK_RELEASE) {
        (*m)->unlock();
        return 0;
    } else {
        return -1;
    }
}