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

#ifndef MULTILIBTIFF_PRODUCER_H
#define MULTILIBTIFF_PRODUCER_H 1

#include <tiffio.h>

#include <flitr/metadata_reader.h>
#include <flitr/image_producer.h>

namespace flitr
{
    
    /**
     * Simple producer to read multipage tiff files.
     */
    class FLITR_EXPORT MultiLibTiffProducer : public ImageProducer
    {
    public:
        /**
         * Constructs the producer.
         *
         * \param filename Video or image file name.
         *
         */
        MultiLibTiffProducer(const std::vector<std::string> &fileVec, uint32_t buffer_size=FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS);
        
        ~MultiLibTiffProducer();
        
        bool setAutoLoadMetaData(std::shared_ptr<ImageMetadata> defaultMetadata);
        
        /**
         * The init method is used after construction to be able to return
         * success or failure of opening the file.
         *
         * \return True if successful.
         */
        bool init();
        
        /**
         * Reads the next page from the file and make it available. This producer does not loop back to the beginning!
         *
         * \return True if a page was successfully read.
         */
        bool trigger();
        
        /**
         * Reads a specified page from the files.
         *
         * \param position Page number to read (0 based).
         *
         * \return True if a page was successfully read.
         */
        bool seek(uint32_t position);
        
        /**
         * Returns the number of pages in the first tif file. THIS IS A SLOW OPERATION THAT FOLLOWS THE DIRECTORY CHAINS THROUGH THE TIFF FILES TO COUNT THE NUMBER OF PAGES.
         *
         * \return Number of pages.
         */
        uint32_t getNumImages(const size_t fileNum=0);
        
        /**
         * Returns the number of the last correctly read page.
         *
         * \return 0 to getNumImages()-1
         */
        int32_t getCurrentImage() {return currentImage_;}
        
        
    protected:
        //!Read current (already open) tiff directory images/pages and write to shared image buffer.
        bool readSlot();
        
        const std::vector<std::string> fileVec_;
        
        std::vector<TIFF*> tifVec_;
        std::vector<uint8_t*> tifScanLineVec_;
        
        //!Current directory of tif_;
        uint32_t currentPage_;
        
        //!Last correctly read frame.
        int32_t currentImage_;
        
        std::shared_ptr<MetadataReader> MetadataReader_;
        std::shared_ptr<ImageMetadata> DefaultMetadata_;
        
        uint32_t buffer_size_;
    };
    
}

#endif //LIBTIFF_PRODUCER_H
