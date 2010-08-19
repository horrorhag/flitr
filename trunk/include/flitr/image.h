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

#ifndef FLITR_IMAGE_H
#define FLITR_IMAGE_H 1

#include <flitr/image_format.h>
#include <flitr/image_metadata.h>

extern "C" {
#include <avformat.h>
}

#include <boost/tr1/memory.hpp>

namespace flitr {

class Image {
  public:
    Image(const ImageFormat& image_format) :
        Format_(image_format)
    {
        Data_ = (uint8_t*)av_malloc(Format_.getBytesPerImage());
        if (!Data_) {
            // \todo what to do on malloc fail
        }
    };
    ~Image()
    {
        av_free(Data_);
    }
    /// Copy constructor
    Image(const Image& rh)
    {
        Data_ = (uint8_t*)av_malloc(rh.Format_.getBytesPerImage());
        deepCopy(rh);
    }
    /// Assignment
    Image& operator=(const Image& rh)
    {
        if (this == &rh) {
            return *this;
        }
        
        av_free(Data_);
        Data_ = (uint8_t*)av_malloc(rh.Format_.getBytesPerImage());
        if (!Data_) {
            // \todo what to do on malloc fail
        }
        deepCopy(rh);
        
        return *this;
    }
    ImageFormat* format() { return &Format_; }
    const std::tr1::shared_ptr<ImageMetadata> metadata() { return Metadata_; }
    void setMetadata(std::tr1::shared_ptr<ImageMetadata> md) { Metadata_ = md; } 
    uint8_t* data() { return &(Data_[0]); }

  private:
    void deepCopy(const Image& rh)
    {
        Format_ = rh.Format_;
        // assume metadata copyable
        if (rh.Metadata_) {
            *Metadata_ = *(rh.Metadata_);
        }
        // assume allocation was done
        memcpy(Data_, rh.Data_, Format_.getBytesPerImage());
    }

    ImageFormat Format_;
    std::tr1::shared_ptr<ImageMetadata> Metadata_;
    uint8_t* Data_;
};

}

#endif //FLITR_IMAGE_H
