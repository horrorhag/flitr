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

#ifndef SMULTI_FFMPEG_PRODUCER_H
#define SMULTI_FFMPEG_PRODUCER_H 1

#include <flitr/ffmpeg_reader.h>
#include <flitr/image_producer.h>

namespace flitr {

/**
 * Simple producer to read images from FFmpeg supported video files or
 * still images.
 * 
 */
class FLITR_EXPORT SMultiFFmpegProducer : public ImageProducer {
  public:
    /** 
     * Constructs the producer.
     * 
     * \param filename Video or image file name.
     * \param out_pix_fmt The pixel format of the output. Whatever the
     * actual input format, it will be converted to this requested
     * format.
     * 
     */
    SMultiFFmpegProducer(std::vector<std::string> filenames, ImageFormat::PixelFormat out_pix_fmt, uint32_t buffer_size=FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS);
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
     * Returns the number of frames in a file.
     * 
     * \return Number of frames.
     */
    uint32_t getNumImages() { return NumImages_; }
    /** 
     * Returns the number of the last correctly read frames.
     * 
     * \return 0 to getNumImages()-1 for valid frames or -1 when not
     * successfully triggered.
     */
    int32_t getCurrentImage() { return CurrentImage_; }

    /**
     * Get the frame rate of the video at position imageNum in the slot.
     *
     * \return Frame rate.
     */
    uint32_t getFrameRate(const int imageNum) const {return Readers_[imageNum]->getFrameRate();}

  private:
    /// The readers to do the actual reading.
    std::vector<std::shared_ptr<FFmpegReader> > Readers_;
    uint32_t ImagesPerSlot_;
    std::vector<std::string> Filenames_;

     ImageFormat::PixelFormat out_pix_fmt_;

    /// Number of frames in the file.
    uint32_t NumImages_;
    /// Last correctly read frame.
    int32_t CurrentImage_;

    uint32_t buffer_size_;
};

}

#endif //FFMPEG_PRODUCER_H
