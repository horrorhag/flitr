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

#ifndef TEST_PATTERN_PRODUCER_H
#define TEST_PATTERN_PRODUCER_H 1

#include <boost/tr1/memory.hpp>

#include <flitr/metadata_reader.h>
#include <flitr/image_producer.h>

namespace flitr {

/**
 * Simple producer to create a checkerboard test pattern.
 * 
 */
class FLITR_EXPORT TestPatternProducer : public ImageProducer {
  public:
    /** 
     * Constructs the producer.
     * 
     * \param out_pix_fmt The pixel format of the output. Whatever the
     * actual input format, it will be converted to this requested
     * format.
     * 
     */
    TestPatternProducer(uint32_t width, uint32_t height, ImageFormat::PixelFormat out_pix_fmt,
                        float patternSpeed,
                        uint32_t buffer_size=FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS);

    /**
     * The init method is used after construction to be able to return
     * success or failure.
     * 
     * \return True if successful.
     */
    bool init();

    /** 
     * Generates the next frame and make it available.
     *
     * \return True if a frame was successfully read.
     */
    bool trigger();


  private:
    uint32_t buffer_size_;
    uint32_t numFramesDone_;
    float patternSpeed_;
};

}

#endif //TEST_PATTERN_PRODUCER_H
