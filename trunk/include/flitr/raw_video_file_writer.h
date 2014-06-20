/* Framework for Live Image Transformation (FLITr)
 * Copyright (c) 2013 CSIR
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

#ifndef FFMPEG_RAW_VIDEO_FILE_WRITER_H
#define FFMPEG_RAW_VIDEO_FILE_WRITER_H 1

#include <flitr/image.h>
#include <flitr/log_message.h>
#include <flitr/stats_collector.h>
#include <flitr/raw_video_file_utils.h>

#include <iostream>
#include <sstream>

namespace flitr {


    /// Class thrown when video errors occur.
struct RawVideoFileWriterException {
    RawVideoFileWriterException() {}
};

/**
 * The RawVideoFileWriter class
 *
 * This is the writer for the FLITr custom video file. The custom video
 * recording does not make use of any standard video file container or
 * video encoding as with the flitr::FfmpegWriter class. The container
 * is a custom format defined by this writer.
 *
 * Each file begins with a standard header that contains some markers and
 * other information about the video that is contained in the video file.
 * This can then be read by a reader to reproduce the video from the file.
 * The header is described by the flitr::FileHeader structure.
 *
 * This writer along with the flitr::RawVideoFileReader class are designed
 * to be a simple way to write video to a file and then to read it from the
 * file at a later stage. Since no compression is done, the file can become
 * very large on large video streams. The custom format also means that only
 * the FLITr library will be able to make sense of the recorded video.
 */
class FLITR_EXPORT RawVideoFileWriter {
public:
    /**
     * Constructs the Writer.
     *
     * \param[in] filename Video file name that must be written to.
     * \param[in] image_format Image format of the images that will be
     *              written to the file.
     * \param[in] frame_rate Frame rate at which the frames will get written
     *              to the file.
     */
    RawVideoFileWriter(std::string filename,
                        const ImageFormat& image_format,
                        const uint32_t frame_rate=FLITR_DEFAULT_VIDEO_FRAME_RATE);

    ~RawVideoFileWriter();
    /**
     * Write a Video Frame to the video file.
     *
     * \param[in] in_buf The video frame that must be written. This image must be
     *              in the format of \a image_format passed to the constructor of
     *              this class.
     * \return True if the frame was written successfully.
     */
    bool writeVideoFrame(uint8_t *in_buf);
private:

    /// Open video file
    bool openVideoFile();
    /// Close video file.
    bool closeVideoFile();
    /// Write the file header to the file
    void writeFileHeader();


	ImageFormat ImageFormat_;
    std::string SaveFileName_;
    uint32_t FrameRate_;
    /// Holds the number of frames written to disk.
    uint64_t WrittenFrameCount_;

    std::shared_ptr<StatsCollector> WriteFrameStats_;

    uint32_t VideoFrameSize_;

    FILE* File_;
    FileHeader FileHeader_;
};

}

#endif //FFMPEG_RAW_VIDEO_FILE_WRITER_H
