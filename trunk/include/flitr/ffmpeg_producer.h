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

#ifndef FFMPEG_PRODUCER_H
#define FFMPEG_PRODUCER_H 1

#include <boost/tr1/memory.hpp>

#include <flitr/ffmpeg_reader.h>
#include <flitr/image_producer.h>

namespace flitr {

/**
 * Simple producer to read images from FFmpeg supported video files or
 * still images.
 * 
 */
class FLITR_EXPORT FFmpegProducer : public ImageProducer {
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
    FFmpegProducer(std::string filename, ImageFormat::PixelFormat out_pix_fmt);
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
    uint32_t getNumImages() { return NumFrames_; }
  private:
    /// The reader to do the actual reading.
    std::tr1::shared_ptr<FFmpegReader> Reader_;
    /// Number of frames in the file.
    uint32_t NumFrames_;
    /// Frame that would be read on next trigger.
    uint32_t CurrentFrame_;
};

}

#endif //FFMPEG_PRODUCER_H
