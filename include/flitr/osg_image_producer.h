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


#include <flitr/metadata_reader.h>
#include <flitr/image_producer.h>


namespace osg
{
    class Image;
}

namespace flitr
{
    
    /**
     * Simple producer to read a osg image.
     */
    class FLITR_EXPORT OsgImageProducer : public ImageProducer
    {
    public:
        /**
         * Constructs the producer.
         *
         * \param filename Video or image file name.
         *
         */
        OsgImageProducer(const std::string& file_name, uint32_t buffer_size = FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS);
        
        ~OsgImageProducer();
        
        
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
     
    protected:
        uint32_t buffer_size_;
        std::string file_name_;
        osg::Image* osg_image_;
    };
    
}

#endif //LIBTIFF_PRODUCER_H
