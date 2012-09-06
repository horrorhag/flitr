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


//"The output format is automatically guessed according to the file extension and bit depth.\n"
//"Raw images can also be output by using '%%d' in the filename\n"

FFmpegWriter::FFmpegWriter(std::string filename, const ImageFormat& image_format, const uint32_t frame_rate, VideoContainer container, VideoCodec codec, int32_t bit_rate) :
    Container_(container),
    Codec_(codec),
    AVCodec_(0),
    AVCodecContext_(0),
    ImageFormat_(image_format),
    SaveFileName_(filename),
    WrittenFrameCount_(0)
{
    FrameRate_.num=frame_rate;
    FrameRate_.den=1.0;

    av_register_all();

    //=== VideoEncodeBuffer_ for use with older avcodec_encode_video(...) ===
    // be safe with size
    VideoEncodeBufferSize_ = ImageFormat_.getWidth() * ImageFormat_.getHeight() * 4;
    if (VideoEncodeBufferSize_ < FF_MIN_BUFFER_SIZE) {
        VideoEncodeBufferSize_ = FF_MIN_BUFFER_SIZE;
    }
    VideoEncodeBuffer_ = (uint8_t *)av_malloc(VideoEncodeBufferSize_);

    InputFrameFormat_ = PixelFormatFLITrToFFmpeg(image_format.getPixelFormat());
    SaveFrameFormat_=PIX_FMT_NONE;//Chosen below.


    //=== OutputFormat_ ===
    OutputFormat_=0;

    if (Container_==FLITR_AVI_CONTAINER)
    {//Select AVI container if requested.
        OutputFormat_=av_guess_format("avi", NULL, NULL);
    } else
    if (Container_==FLITR_MKV_CONTAINER)
    {//Select MKV container if requested.
        OutputFormat_=av_guess_format("matroska", NULL, NULL);
    }

    if (!OutputFormat_)
    {//If nothing requested or request failed. Then guess from pixel format.
        if (InputFrameFormat_ == PIX_FMT_GRAY16LE) {
            OutputFormat_ = av_guess_format("matroska", NULL, NULL);
        } else {
            OutputFormat_ = av_guess_format("avi", NULL, NULL);
        }
    }

    if (!OutputFormat_)
    {//If still no valid output format. Then attempt to guess from file name.
        OutputFormat_ = av_guess_format(NULL, filename.c_str(), NULL);
    }

    if (!OutputFormat_) {
        logMessage(LOG_CRITICAL) << "Cannot set video format.\n";
        logMessage(LOG_CRITICAL).flush();
        throw FFmpegWriterException();
    }
    //=====================


    //=== FormatContext_ ===
    FormatContext_ = avformat_alloc_context();

    if (!FormatContext_) {
        logMessage(LOG_CRITICAL) << "Cannot allocate video format context.\n";
        logMessage(LOG_CRITICAL).flush();
        throw FFmpegWriterException();
    }

    FormatContext_->oformat = OutputFormat_;
    sprintf(FormatContext_->filename, "%s", SaveFileName_.c_str());



    //=== AVCodec_ ===
    if (Codec_== FLITR_NONE_CODEC)
    {//If the codec is unspecified then use the codec recommended by the OutputFormat_.
            Codec_=(flitr::VideoCodec)OutputFormat_->video_codec;
    }

    AVCodec_=avcodec_find_encoder((CodecID)Codec_);

    if (!AVCodec_) {
        logMessage(LOG_CRITICAL) << "Cannot find video codec.\n";
        logMessage(LOG_CRITICAL).flush();
        throw FFmpegWriterException();
    }

    if (AVCodec_->supported_framerates!=0)
    {//Only some framerates are supported. Find the closest to the one requested.
        AVRational const * codecFramerates=AVCodec_->supported_framerates;
        int closestFramerateIndex=0;
        int framerateIndex=0;

        std::cout << "Requested framerate of " << (FrameRate_.num / ((float)FrameRate_.den)) << ", but valid pixel framerates are: ";

        float requestedFramerate=FrameRate_.num / ((float)FrameRate_.den);

        while (((*codecFramerates).den!=0) && ((*codecFramerates).num!=0))
        {
            float closestFramerate=AVCodec_->supported_framerates[closestFramerateIndex].num /
                    ((float)AVCodec_->supported_framerates[closestFramerateIndex].den);

            float framerate=(*codecFramerates).num / ((float)(*codecFramerates).den);

            if ( fabs(framerate-requestedFramerate) <
                 fabs(closestFramerate-requestedFramerate))
            {
                closestFramerateIndex=framerateIndex;
            }

            std::cout << ((*codecFramerates).num / ((float)(*codecFramerates).den)) << " ";
            framerateIndex++;
            codecFramerates++;
        }

        FrameRate_=AVCodec_->supported_framerates[closestFramerateIndex];
        std::cout << "\n";
        std::cout << "   Using framerate: " << (FrameRate_.num / ((float)FrameRate_.den)) << "\n";
    }

    if (AVCodec_->pix_fmts!=0)
    {//Choose the input pixel format if the codec support it. Otherwise choose the first supported format.
        PixelFormat const * codecPixelFormats=AVCodec_->pix_fmts;

        std::cout << "Valid pixel formats are: ";
        while ((*codecPixelFormats)!=PIX_FMT_NONE)
        {
            std::cout << (*codecPixelFormats) << " ";

            if ((*codecPixelFormats)==InputFrameFormat_)
            {
                SaveFrameFormat_=InputFrameFormat_;
                break;
            }

            codecPixelFormats++;
        }

        if ((*codecPixelFormats)==PIX_FMT_NONE)
        {
            SaveFrameFormat_ = AVCodec_->pix_fmts[0];
        }

        std::cout << "\n";
        std::cout.flush();
    } else
    {//Valid pixel formats are unknown. Just choose the input format.
        SaveFrameFormat_ = InputFrameFormat_;
        std::cout << "Valid pixel formats are UNKNOWN.\n";
    }

    if ( (((CodecID)Codec_)==CODEC_ID_RAWVIDEO) && (SaveFrameFormat_==PIX_FMT_RGB24) )
    {//It seems that ffmpeg swaps the rgb components when using the raw codec.
        SaveFrameFormat_=PIX_FMT_BGR24;
    }


    std::cout << "    Using format: " << SaveFrameFormat_ <<"\n";
    std::cout.flush();


    //=== VideoStream_ ===
    VideoStream_ = av_new_stream(FormatContext_, 0);
    //VideoStream_ = avformat_new_stream(FormatContext_, AVCodec_);
    if (!VideoStream_) {
        logMessage(LOG_CRITICAL) << "Cannot allocate video stream.\n";
        logMessage(LOG_CRITICAL).flush();
        throw FFmpegWriterException();
    }


    //=== AVCodecContext_ ===
    AVCodecContext_=VideoStream_->codec;
    AVCodecContext_->codec_id=(CodecID)Codec_;

    if (bit_rate>0) AVCodecContext_->bit_rate = bit_rate;

    /* resolution must be a multiple of two */
    AVCodecContext_->width = ImageFormat_.getWidth();;
    AVCodecContext_->height = ImageFormat_.getHeight();
    AVCodecContext_->time_base.den= FrameRate_.num;
    AVCodecContext_->time_base.num= FrameRate_.den;
    AVCodecContext_->gop_size = 1;
    AVCodecContext_->pix_fmt = SaveFrameFormat_;

    AVCodecContext_->codec_type = AVMEDIA_TYPE_VIDEO;

    // experiment with coder type
    //AVCodecContext_->coder_type = FF_CODER_TYPE_AC; // faster for FFV1
    //AVCodecContext_->coder_type = FF_CODER_TYPE_VLC;
    //AVCodecContext_->coder_type = FF_CODER_TYPE_DEFLATE;
    //AVCodecContext_->coder_type = FF_CODER_TYPE_RAW;
    //AVCodecContext_->coder_type = FF_CODER_TYPE_RLE;

       /* Some formats want stream headers to be separate. */
         if(FormatContext_->oformat->flags & AVFMT_GLOBALHEADER) {
        AVCodecContext_->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }


    if (avcodec_open2(AVCodecContext_, AVCodec_, NULL) < 0) {
        logMessage(LOG_CRITICAL) << "Cannot open video codec.\n";
        logMessage(LOG_CRITICAL).flush();
        throw FFmpegWriterException();
    }


#if LIBAVFORMAT_VERSION_INT < ((54<<16) + (21<<8) + 100)
    if (av_set_parameters(FormatContext_, NULL) < 0) {
        logMessage(LOG_CRITICAL) << "Cannot set video parameters.\n";
        logMessage(LOG_CRITICAL).flush();
        throw FFmpegWriterException();
    }
#endif



    if (getLogMessageCategory() & LOG_INFO) {
#if LIBAVFORMAT_VERSION_INT >= ((54<<16) + (21<<8) + 100)
        av_dump_format(FormatContext_, 0, SaveFileName_.c_str(), 1);
#else
        dump_format(FormatContext_, 0, SaveFileName_.c_str(), 1);
#endif
    }


    InputFrame_ = allocFFmpegFrame(InputFrameFormat_, ImageFormat_.getWidth(), ImageFormat_.getHeight());
    SaveFrame_ = allocFFmpegFrame(SaveFrameFormat_, ImageFormat_.getWidth(), ImageFormat_.getHeight());


    if (!InputFrame_ || !SaveFrame_) {
        logMessage(LOG_CRITICAL) << "Cannot allocate memory for video storage buffers.\n";
        logMessage(LOG_CRITICAL).flush();
        throw FFmpegWriterException();
    }

#if defined FLITR_USE_SWSCALE
    ConvertToSaveCtx_ = sws_getContext(ImageFormat_.getWidth(), ImageFormat_.getHeight(), InputFrameFormat_,
                                       ImageFormat_.getWidth(), ImageFormat_.getHeight(), SaveFrameFormat_,
                                       SWS_BILINEAR, NULL, NULL, NULL);
#endif



#if LIBAVFORMAT_VERSION_INT >= ((54<<16) + (21<<8) + 100)
    if (avio_open(&FormatContext_->pb, SaveFileName_.c_str(), AVIO_FLAG_WRITE) < 0) {
#else
    if (url_fopen(&FormatContext_->pb, SaveFileName_.c_str(), URL_WRONLY) < 0) {
#endif
        logMessage(LOG_CRITICAL) << "Cannot open video file " << SaveFileName_ << "\n";
        logMessage(LOG_CRITICAL).flush();
        throw FFmpegWriterException();
    }


#if LIBAVFORMAT_VERSION_INT >= ((54<<16) + (21<<8) + 100)
    avformat_write_header(FormatContext_,NULL);
#else
    av_write_header(FormatContext_);
#endif

    WrittenFrameCount_ = 0;
}

FFmpegWriter::~FFmpegWriter()
{
    closeVideoFile();

    av_free(InputFrame_);
    av_free(SaveFrame_);
    
    av_free(VideoEncodeBuffer_);
}


bool FFmpegWriter::writeVideoFrame(uint8_t *in_buf)
{
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = NULL;    // packet data will be allocated by the encoder
    pkt.size = 0;

    // fill the incoming picture
    avpicture_fill((AVPicture *)InputFrame_, in_buf, InputFrameFormat_, ImageFormat_.getWidth(), ImageFormat_.getHeight());

#if defined FLITR_USE_SWSCALE
    //if (SaveFrameFormat_==PIX_FMT_BGR24)||(SaveFrameFormat_==PIX_FMT_RGB 24)
    {//Sometime the image has to be flipped.
        InputFrame_->data[0] += InputFrame_->linesize[0] * (AVCodecContext_->height-1);
        InputFrame_->linesize[0]*=-1;
    }

    //int *test = InputFrame_->linesize;
    //printf("%d %d %d %d\n", test[0], test[1], test[2], test[3]);
    int err = sws_scale(ConvertToSaveCtx_,
                        InputFrame_->data, InputFrame_->linesize, 0, AVCodecContext_->height,
                        SaveFrame_->data, SaveFrame_->linesize);
#else
    img_convert((AVPicture *)SaveFrame_, AVCodecContext_->pix_fmt,
                (AVPicture *)InputFrame_, InputFrameFormat_,
                AVCodecContext_->width, AVCodecContext_->height);
#endif


    /* encode the image */
//#if LIBAVFORMAT_VERSION_INT >= ((54<<16) + (21<<8) + 100)
//    int encode_ret = avcodec_encode_video2(AVCodecContext_, &pkt, SaveFrame_, &got_output);
//#else
    int encode_ret = avcodec_encode_video(AVCodecContext_, VideoEncodeBuffer_, VideoEncodeBufferSize_, SaveFrame_);
//#endif

    if (encode_ret<=0)
    {
        logMessage(LOG_CRITICAL) << "Failed to encode video frame. \n";
        logMessage(LOG_CRITICAL).flush();
        return false;
    }


    pkt.pts = av_rescale_q(AVCodecContext_->coded_frame->pts, AVCodecContext_->time_base, VideoStream_->time_base);
    pkt.dts = pkt.pts;

    if(AVCodecContext_->coded_frame->key_frame) {
        pkt.flags |= AV_PKT_FLAG_KEY;
    }

    pkt.stream_index= VideoStream_->index;
    pkt.data = VideoEncodeBuffer_;
    pkt.size = encode_ret;

    int write_ret = av_write_frame(FormatContext_, &pkt);
    if (write_ret<0)
    {
        logMessage(LOG_CRITICAL) << "Failed to write video frame. \n";
        logMessage(LOG_CRITICAL).flush();
        return false;
    }

    WrittenFrameCount_++;
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


