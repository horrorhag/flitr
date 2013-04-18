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

#ifndef RAW_VIDEO_FILE_PRODUCER_H
#define RAW_VIDEO_FILE_PRODUCER_H 1

#include <boost/tr1/memory.hpp>

#include <flitr/metadata_reader.h>
#include <flitr/image_producer.h>
#include <flitr/raw_video_file_reader.h>

namespace flitr {

/**
 * Simple producer to read images from Custom FLITr video recording file.
 * 
 * This class reads the custom FLITr recording and reproduces the image stream
 * from the file using the flitr::RawVideoFileReader class.
 */
class FLITR_EXPORT RawVideoFileProducer : public ImageProducer {
  public:
    /** 
     * Constructs the producer.
     * 
     * \param[in] filename Video file name.
     * \param[in] buffer_size Buffer size to use for the video images read
     *              from the file
     */
    RawVideoFileProducer(std::string filename, uint32_t buffer_size=FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS);

    bool setAutoLoadMetaData(std::tr1::shared_ptr<ImageMetadata> defaultMetadata);

    /** 
     * The init method is used after construction to be able to return
     * success or failure of opening the file.
     * 
     * \return True if successful.
     */
    bool init();
    /** 
     * Reads the next frame from the file and make it available. If
     * the file reached the end, reading is resumed from the start.
     * 
     * \return True if a frame was successfully read.
     */
    bool trigger();
    /** 
     * Reads a specified frame from the file.
     *
     * \param position Frame number to read (0 based).
     * 
     * \return True if a frame was successfully read.
     */
    bool seek(uint32_t position);
    /** 
     * Returns the number of frames in the file.
     * 
     * \return Number of frames.
     */
    uint32_t getNumImages() { return NumImages_; }
    /** 
     * Returns the number of the last correctly read image.
     * 
     * \return 0 to getNumImages()-1 for valid frames or -1 when not
     * successfully triggered.
     */
    int32_t getCurrentImage() { return CurrentImage_; }

    /**
     * Get the frame rate of the video.
     *
     * \return Frame rate.
     */
    uint32_t getFrameRate() const {return Reader_->getFrameRate();}

  protected:
    std::string filename_;

    /// The reader to do the actual reading.
    std::tr1::shared_ptr<RawVideoFileReader> Reader_;

    std::tr1::shared_ptr<MetadataReader> MetadataReader_;
    std::tr1::shared_ptr<ImageMetadata> DefaultMetadata_;

    /// Number of frames in the file.
    uint32_t NumImages_;
    /// Last correctly read frame.
    int32_t CurrentImage_;
    /// Buffer size
    uint32_t buffer_size_;
};

}

#endif //RAW_VIDEO_FILE_PRODUCER_H
