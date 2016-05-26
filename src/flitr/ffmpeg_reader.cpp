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
using std::shared_ptr;

FFmpegReader::FFmpegReader() noexcept :
    FrameRate_(FLITR_DEFAULT_VIDEO_FRAME_RATE),
    SingleFrameSource_(false),
    SingleFrameDone_(false)
{
    av_lockmgr_register(&lockManager);

    avcodec_register_all();
    av_register_all();
    avformat_network_init();

    FormatContext_ = NULL;

    CodecContext_ = NULL;
    Codec_ = NULL;

    std::map<std::string, std::string> options;
    options["codec_type"] = "AVMEDIA_TYPE_VIDEO";
    setDictionaryOptions(options);
}

FFmpegReader::FFmpegReader(std::string filename, ImageFormat::PixelFormat out_pix_fmt) :
    FFmpegReader()
{
    /* Call the open video function. Care must be taken that any virtual functions
     * will be called as if not virtual. See the constructor documentation for
     * additional information. */
    bool opened = openVideo(filename, out_pix_fmt);
    if(opened == false) {
        throw FFmpegReaderException();
    }
}

FFmpegReader::~FFmpegReader()
{
    if (DecodedFrame_) {
        av_free(DecodedFrame_);
    }
    avcodec_close(CodecContext_);

#if LIBAVFORMAT_VERSION_INT >= ((53<<16) + (21<<8) + 0)
    avformat_close_input(&FormatContext_);
#else
    av_close_input_file(FormatContext_);
#endif
}

bool FFmpegReader::openVideo(const std::string& filename, ImageFormat::PixelFormat out_pix_fmt)
{
    FileName_ = filename;
    /* Register the StatsCollector handlers. */
    std::stringstream sws_stats_name;
    sws_stats_name << filename << " FFmpegReader::getImage,swscale";
    SwscaleStats_ = shared_ptr<StatsCollector>(new StatsCollector(sws_stats_name.str()));

    std::stringstream getimage_stats_name;
    getimage_stats_name << filename << " FFmpegReader::getImage";
    GetImageStats_ = shared_ptr<StatsCollector>(new StatsCollector(getimage_stats_name.str()));

    /* Get the dictionary options before opening the stream. */
    AVDictionary *options = NULL;
    std::map<std::string, std::string> dictionaryOptions = getDictionaryOptions();
    for(const std::pair<std::string, std::string>& option: dictionaryOptions) {
        av_dict_set(&options, option.first.c_str(), option.second.c_str(), 0);
    }

#if LIBAVFORMAT_VERSION_INT >= ((53<<16) + (21<<8) + 0)
    int err = avformat_open_input(&FormatContext_, FileName_.c_str(), NULL, &options);
#else
    int err = av_open_input_file(&FormatContext_, FileName_.c_str(), NULL, 0, NULL);
#endif

    if (err < 0) {
        char errorBuffer[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_strerror(err, errorBuffer, AV_ERROR_MAX_STRING_SIZE);
        logMessage(LOG_CRITICAL) << "av_open_input_file failed on " << filename.c_str() << " with code " << err << ": " << errorBuffer << "\n";
        return false;
    }

#if 0
    /* Print out the dictionary options that were not used during the call to
     * avformat_open_input(). This is for debugging only. */
    AVDictionaryEntry *dictionaryEntry = NULL;
    while(true) {
        dictionaryEntry = av_dict_get(options, "", dictionaryEntry, AV_DICT_IGNORE_SUFFIX);
        if(dictionaryEntry == nullptr) {
            break;
        }
        logMessage(LOG_DEBUG) << "Header dictionary option not found: " << dictionaryEntry->key << " " << dictionaryEntry->value << "\n";
    }
#endif /* 0 */
    av_dict_free(&options);

    err=avformat_find_stream_info(FormatContext_, NULL);
    if (err < 0) {
        char errorBuffer[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_strerror(err, errorBuffer, AV_ERROR_MAX_STRING_SIZE);
        logMessage(LOG_CRITICAL) << "avformat_find_stream_info failed on " << filename.c_str() << " with code " << err << ": " << errorBuffer << "\n";
        return false;
    }


    //=== Find video stream ===//
    VideoStreamIndex_=av_find_best_stream(FormatContext_, AVMEDIA_TYPE_VIDEO, -1, -1, &Codec_, 0);
    CodecContext_ = FormatContext_->streams[VideoStreamIndex_]->codec;
    //Codec_ = avcodec_find_decoder(CodecContext_->codec_id);
/*
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
*/
    //=== ===//


    //CodecContext_->workaround_bugs = 1;
    //CodecContext_ = avcodec_alloc_context3(Codec_);

    if (!Codec_) {
        logMessage(LOG_CRITICAL) << "Cannot find video codec for " << filename.c_str() << "\n";
        return false;
    }

    if (avcodec_open2(CodecContext_, Codec_, NULL) < 0) {
        logMessage(LOG_CRITICAL) << "Cannot open video codec for " << filename.c_str() << "\n";
        return false;
    }


    // everything should be ready for decoding video

    if (getLogMessageCategory() & LOG_INFO) {
#if LIBAVFORMAT_VERSION_INT >= ((53<<16) + (21<<8) + 0)
        av_dump_format(FormatContext_, 0, FileName_.c_str(), 0);
#else
        dump_format(FormatContext_, 0, FileName_.c_str(), 0);
#endif
    }

    double duration_seconds = double(FormatContext_->duration) / AV_TIME_BASE;
    FrameRate_ = double(FormatContext_->streams[VideoStreamIndex_]->r_frame_rate.num) / double(FormatContext_->streams[VideoStreamIndex_]->r_frame_rate.den);
    NumImages_ = 0.5 + (duration_seconds * FrameRate_);
    logMessage(LOG_DEBUG) << "FFmpegReader gets duration: " << duration_seconds << " frame rate: " << FrameRate_ << " total frames: " << NumImages_ << "\n";


    //=== convert or reset IP format to ffmpeg format ===//
    AVPixelFormat out_ffmpeg_pix_fmt;

    if (out_pix_fmt!=ImageFormat::FLITR_PIX_FMT_ANY)
    {//The user specified an image format so use it.
        out_ffmpeg_pix_fmt=PixelFormatFLITrToFFmpeg(out_pix_fmt);
    } else
    {//Select the image format automatically.
        out_ffmpeg_pix_fmt=CodecContext_->pix_fmt;
        out_pix_fmt=PixelFormatFFmpegToFLITr(out_ffmpeg_pix_fmt);

        if (out_pix_fmt==ImageFormat::FLITR_PIX_FMT_UNDF)
        {//FLITr doesn't understand the ffmpeg pixel format specified by the codec.
            //Therefore, make an output pixelformat selection for the user.

            //ToDo: Choose the FLITr format smartly.
            out_pix_fmt=ImageFormat::FLITR_PIX_FMT_RGB_8;


            out_ffmpeg_pix_fmt=PixelFormatFLITrToFFmpeg(out_pix_fmt);

            logMessage(LOG_INFO) << "FFmpeg codec pixel format " << av_get_pix_fmt_name(CodecContext_->pix_fmt) << " is not known to FLITr (FLITR_PIX_FMT_UNDF). Defaulting to " << av_get_pix_fmt_name(out_ffmpeg_pix_fmt) << ".\n";
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
                ImageFormat_.getWidth(), ImageFormat_.getHeight(), (AVPixelFormat)out_ffmpeg_pix_fmt,
                SWS_BILINEAR, NULL, NULL, NULL);
#endif

    DecodedFrame_ = avcodec_alloc_frame();
    if (!DecodedFrame_) {
        logMessage(LOG_CRITICAL) << "Cannot allocate memory for decoded frame.\n";
        return false;
    }

    ConvertedFrame_ = allocFFmpegFrame(out_ffmpeg_pix_fmt, ImageFormat_.getWidth(), ImageFormat_.getHeight());
    if (!ConvertedFrame_) {
        logMessage(LOG_CRITICAL) << "Cannot allocate memory for video storage buffers.\n";
        return false;
    }

    if (NumImages_ == 1) {
        // Mostly when images used as input
        SingleFrameSource_ = true;
    }

    CurrentImage_ = -1;
    return true;
}

bool FFmpegReader::getImage(Image &out_image, int im_number)
{
    GetImageStats_->tick();

    // convert frame number to time
    int64_t seek_time = ((int64_t)im_number * AV_TIME_BASE * FormatContext_->streams[VideoStreamIndex_]->r_frame_rate.den) / (FormatContext_->streams[VideoStreamIndex_]->r_frame_rate.num);

    //std::cout << "Seektime = " << seek_time << "\n";
    //std::cout << "FrameNum = " << im_number << "\n";
    //std::cout << "RateNum  = " << FormatContext_->streams[VideoStreamIndex_]->r_frame_rate.num << "\n";
    //std::cout << "RateDen  = " << FormatContext_->streams[VideoStreamIndex_]->r_frame_rate.den << "\n";
    //std::cout.flush();

    // Do not seek if we have a single frame source
    // Assume we are looking for frame 0, XXX assert maybe
    if (SingleFrameSource_) {
        // if we already have a frame, just copy it, do not read again
        if (SingleFrameDone_) {
            out_image = *SingleImage_;
            GetImageStats_->tock();
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
            //std::cout.flush();
        }
    }

    // decode
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data=NULL;
    pkt.size=0;
    int gotframe;
    avcodec_get_frame_defaults(DecodedFrame_);

    int loopcount=0;
    while(1) {

        int read_err=0;
        if ( (read_err=av_read_frame(FormatContext_, &pkt)) < 0 ) {
            //XXX
            if (read_err==AVERROR_EOF)
            {
                logMessage(LOG_DEBUG) << "EOF while reading frame " << im_number << "/" << NumImages_ <<  "\n";
                //GetImageStats_->tock(); We'll only keep stats of the good frames.
                return false;
            } else
            {
                char errbuf[AV_ERROR_MAX_STRING_SIZE];
                av_strerror(read_err, errbuf, AV_ERROR_MAX_STRING_SIZE);
                logMessage(LOG_DEBUG) << "Error ("<< errbuf <<") while reading frame " << im_number << "/" << NumImages_ <<  "\n";
                //GetImageStats_->tock(); We'll only keep stats of the good frames.
                return false;
            }
        }
        if (pkt.stream_index == VideoStreamIndex_) {
            int decoded_len = avcodec_decode_video2(CodecContext_, DecodedFrame_, &gotframe, &pkt);
            if (decoded_len < 0) {
                logMessage(LOG_DEBUG) << "Error " << decoded_len << " while decoding video\n";
                //GetImageStats_->tock(); We'll only keep stats of the good frames.
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
                //  std::cout.flush();
                //}

                if (pkt_pts_scaled >= seek_time ||
                        pkt_pts_scaled + pkt_duration_scaled > seek_time) {
                    // we are done
                    //fprintf(stderr, "LOOP %d\n", loopcount);
                    //fflush(stderr);
                    break;
                }
            }
            loopcount++;
        }
        av_free_packet(&pkt); // allocated in read_frame
    }

    if (!gotframe) {
        av_free_packet(&pkt);
        //GetImageStats_->tock(); We'll only keep stats of the good frames.
        return false;
    }

    SwscaleStats_->tick();
#if defined FLITR_USE_SWSCALE
    ConvertedFrame_->data[0] = out_image.data(); // save a memcpy

    sws_scale(ConvertFormatCtx_,
              DecodedFrame_->data, DecodedFrame_->linesize, 0, CodecContext_->height,
              ConvertedFrame_->data, ConvertedFrame_->linesize);

    //printf("%d %d\n", DecodedFrame_->linesize, ConvertedFrame_->linesize);
    //fflush(stdout);
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
    GetImageStats_->tock();
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
