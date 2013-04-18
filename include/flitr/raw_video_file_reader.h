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

#ifndef RAW_VIDEO_FILE_READER_H
#define RAW_VIDEO_FILE_READER_H 1

#include <flitr/image.h>
#include <flitr/log_message.h>
#include <flitr/stats_collector.h>

#include <boost/tr1/memory.hpp>

#include <iostream>
#include <sstream>

namespace flitr {

/// Class thrown when video errors occur.
struct RawVideoFileReaderException {
    RawVideoFileReaderException() {}
};

/**
 * Class to encapsulate the reading of images from the FLITr Custom video file.
 *
 * The reader opens the custom FLITr video file and checks that the header
 * of the video file is correct and supported. If it supported it sets up the
 * file for reading to produce the images from the file.
 *
 * Using the reader the images can then be read from the file. The reader will
 * only read images if the start and end bytes of the image is correct.
 *
 * For more information of the recorded video look at flitr::RawVideoFileWriter.
 */
class FLITR_EXPORT RawVideoFileReader {
  public:
    /**
     * Constructs the Reader.
     *
     * \param[in] filename Video file name that must be read from.
     */
    RawVideoFileReader(std::string filename);
    ~RawVideoFileReader();

    /** 
     * Reads the specified frame from the video file.
     * 
     * \param[out] out_image Image to receive the read data.
     * \param[in] im_number Image or frame number in the video (0-based).
     * 
     * \return True if an image was successfully read.
     */
    bool getImage(Image &out_image, int im_number);

    /** 
     * Obtain the number of images/frames present in the video file.
     * 
     * \return Number of images/frames. 
     */
    uint32_t getNumImages() { return NumImages_; }

    /** 
     * Get the number of the last successfully decoded image.
     * 
     * \return 0 to getNumImages()-1 for valid frames or -1 when not
     * successfully triggered.
     */
    int32_t getCurrentImage() { return CurrentImage_; }

    /** 
     * Get the format that would be used for output.
     * 
     * \return Image format of the output data.
     */
    ImageFormat getFormat() { return ImageFormat_; }

    /**
     * Get the frame rate of the video.
     *
     * \return Frame rate.
     */
    uint32_t getFrameRate() const { return FrameRate_; }

  private:
    bool openVideoFile();

    int VideoStreamIndex_;
    uint32_t FrameRate_;
    ImageFormat ImageFormat_;
    uint32_t BytesPerImage_;
    uint32_t NumImages_;
    int32_t CurrentImage_;
    std::string FileName_;
    uint32_t FirstFramePos_;
    std::tr1::shared_ptr<Image> SingleImage_;
    FILE* File_;

    std::tr1::shared_ptr<StatsCollector> GetImageStats_;
};

}

#endif //RAW_VIDEO_FILE_READER_H
