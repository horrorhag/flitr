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
#ifdef WIN32
#define av_pix_fmt_descriptors av_pix_fmt_descriptors_foo
#endif

#include <libavutil/pixdesc.h>

#ifdef WIN32
#undef av_pix_fmt_descriptors
    extern __declspec(dllimport) const AVPixFmtDescriptor av_pix_fmt_descriptors[];
#endif
#else
# include <libavformat/avformat.h>
# include <libavutil/mathematics.h>
# include <libavcodec/avcodec.h>
#ifdef WIN32
#define av_pix_fmt_descriptors av_pix_fmt_descriptors_foo
#endif

#include <libavutil/pixdesc.h>

#ifdef WIN32
#undef av_pix_fmt_descriptors
    extern __declspec(dllimport) const AVPixFmtDescriptor av_pix_fmt_descriptors[];
#endif
#endif
}

#undef PixelFormat

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
    FLITR_MKV_CONTAINER,  //Will cause the ffmpeg consumers to use .mkv extension.
    FLITR_RTSP_CONTAINER
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

/** \brief The FFmpegWriter class.
 *
 * Write data to a file or RTSP stream.
 *
 * After the writer is constructed, the writeVideoFrame() function must be called manually
 * to write the data to the output.
 *
 * \section RTSP Streaming.
 * The FFmpeg Writer supports writing video out over the network
 * as a RTSP client. For this the \a filename must be an RTSP URL in the
 * following format: <tt>rtsp://hostname[:port]/path</tt>.
 * To stream to localhost over port 8554 with path 'mystream' the following
 * \a filename is expected:
 * \code
 * rtsp://127.0.0.1:8554/mystream.sdp
 * \endcode
 *
 * When this is done this writer will be have as an RTSP client. This requires
 * that an RTSP server must be running when the writer is created, otherwise
 * creation will fail with an "Connection Refused" message generated by FFmpeg.
 * The <tt>ffplay</tt> application can be used as a test FFmpeg server using the
 * following command given the above \a filename:
 * \code
 * $ ffplay -rtsp_flags listen -i rtsp://127.0.0.1:8554/mystream.sdp
 * \endcode
 *
 * When streaming as an RTSP client and the connection to the server is lost, a
 * new instance of this class must be created to re-establish streaming to the
 * server. Currently the is no functionality in place to recover from a lost
 * connection.
 *
 * The following example can be used to create a RTSP Writer:
 * \code
 * flitr::FFmpegWriter* writer = new flitr::FFmpegWriter("rtsp://127.0.0.1:8554/mystream.sdp"
 *                                                       , inputFormat // From producer
 *                                                       , 20 // Frame rate
 *                                                       , flitr::FLITR_RTSP_CONTAINER);
 * \endcode
 */
class FLITR_EXPORT FFmpegWriter {
public:
    /** \brief Construct the FFmpegWriter
     *
     * \param[in] filename Name of the file or URL of the RTSP stream server.
     * \param[in] image_format Input image format of the data that will be written
     *          using writeVideoFrame() on this writer.
     * \param[in] frame_rate Frame rate that the data must be written with.
     * \param[in] container Video Container format that must be used. This must
     *          be set to flitr::FLITR_RTSP_CONTAINER if RTSP streaming is required.
     * \param[in] codec The codec that must be used. For RTSP it is recommended to use
     *          flitr::FLITR_NONE_CODEC so that the codec will be chosen by the application.
     * \param[in] bit_rate Bit rate that must be used for writing the data.
     */
    FFmpegWriter(std::string filename, const ImageFormat& image_format,
                 const uint32_t frame_rate=FLITR_DEFAULT_VIDEO_FRAME_RATE,
                 VideoContainer container=FLITR_ANY_CONTAINER,
                 VideoCodec codec=FLITR_RAWVIDEO_CODEC,
                 int32_t bit_rate=-1);

    ~FFmpegWriter();

    /** \brief Write the video frame to the FFmpeg output stream.
     *
     * The \a in_buf must be set up according to the \a image_format set during
     * construction of this object. If the \a width or \a height passed to the
     * constructor is not the same as the image format, this function will do the
     * resizing of the frame automatically.
     *
     * \return True if the frame was successfully written. When doing RTSP
     * streaming this function will fail if the connection to the server was lost.
     */
    bool writeVideoFrame(uint8_t *in_buf);

    /** 
     * Returns the number of frames written in the file.
     * 
     * \return Number of frames.
     */
    uint64_t getNumImages() const { return WrittenFrameCount_; }

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

    std::shared_ptr<StatsCollector> WriteFrameStats_;

    uint8_t* VideoEncodeBuffer_;
    uint32_t VideoEncodeBufferSize_;

    uint32_t SaveFrameWidth_;
    uint32_t SaveFrameHeight_;
};

}

#endif //FFMPEG_WRITER_H
