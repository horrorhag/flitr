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

#ifndef LIBTIFF_PRODUCER_H
#define LIBTIFF_PRODUCER_H 1

#include <tiffio.h>

#include <flitr/metadata_reader.h>
#include <flitr/image_producer.h>

namespace flitr {

/**
 * Simple producer to read multipage tiff files.
 */
class FLITR_EXPORT LibTiffProducer : public ImageProducer {
  public:
    /** 
     * Constructs the producer.
     * 
     * \param filename Video or image file name.
     *
     */
    LibTiffProducer(std::string filename, uint32_t buffer_size=FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS);

    ~LibTiffProducer();

    bool setAutoLoadMetaData(std::shared_ptr<ImageMetadata> defaultMetadata);

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
    uint32_t getNumImages();
    
    /** 
     * Returns the number of the last correctly read image.
     * 
     * \return 0 to getNumImages()-1 for valid frames or -1 when not
     * successfully triggered.
     */
    int32_t getCurrentImage() {return currentImage_;}

    
  protected:
    //!Read current tiff image and write to shared image buffer.
    bool readImage();

    const std::string filename_;

    TIFF* tif_;
    uint8_t* tifScanLine_;

    
    //!Current directory of tif_;
    uint16_t currentDir_;
    
    //!Last correctly read frame.
    int32_t currentImage_;

    std::shared_ptr<MetadataReader> MetadataReader_;
    std::shared_ptr<ImageMetadata> DefaultMetadata_;

    uint32_t buffer_size_;
};

}

#endif //LIBTIFF_PRODUCER_H
