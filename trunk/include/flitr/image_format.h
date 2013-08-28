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

#ifndef FLITR_IMAGE_FORMAT_H
#define FLITR_IMAGE_FORMAT_H 1

#include <flitr/flitr_stdint.h>
#include <flitr/flitr_export.h>

namespace flitr {

/**
 * This class contains information about the format (width, height,
 * pixel type) of an image.
 */
class FLITR_EXPORT ImageFormat {
public:
    /// Do not change enum values
    enum PixelFormat {
        FLITR_PIX_FMT_ANY = 0,
        FLITR_PIX_FMT_Y_8 = 1,
        FLITR_PIX_FMT_RGB_8 = 2,
        FLITR_PIX_FMT_Y_16 = 3,
        FLITR_PIX_FMT_UNDF = 4,
		FLITR_PIX_FMT_BGR = 5,
		FLITR_PIX_FMT_BGRA
    };
    
    ImageFormat(uint32_t w=0, uint32_t h=0, PixelFormat pix_fmt=FLITR_PIX_FMT_Y_8):
        Width_(w),
        Height_(h),
        PixelFormat_(pix_fmt)
    {
        setBytesPerPixel();
        setComponentsPerPixel();
    }
    inline uint32_t getWidth() const { return Width_; }
    inline uint32_t getHeight() const { return Height_; }
    inline PixelFormat getPixelFormat() const { return PixelFormat_; }

    inline uint32_t getComponentsPerPixel() const { return ComponentsPerPixel_; }
    inline uint32_t getBytesPerPixel() const { return BytesPerPixel_; }
    inline uint32_t getBytesPerImage() const { return Width_ * Height_ * BytesPerPixel_; }

    inline void setWidth(uint32_t w) { Width_ = w; }
    inline void setHeight(uint32_t h) { Height_ = h; }
    inline void setPixelFormat(PixelFormat pix_fmt)
    {
        PixelFormat_ = pix_fmt;
        setBytesPerPixel();
        setComponentsPerPixel();
    }
    
    inline void cnvrtPixelFormat(uint8_t const * const inData, uint8_t * const outData, const PixelFormat outFormat) const
    {
        switch (PixelFormat_)
        {
        case FLITR_PIX_FMT_Y_8:
            switch (outFormat)
            {
            case FLITR_PIX_FMT_Y_8:
            {
                *outData=*inData;
            }
                break;
            case FLITR_PIX_FMT_RGB_8:
            {
                *(outData+0)=*inData;
                *(outData+1)=*inData;
                *(outData+2)=*inData;
            }
                break;
            case FLITR_PIX_FMT_Y_16:
            {
                *(outData+0)=0;
                *(outData+1)=*inData;
            }
                break;
            default:
                break;
            }
            break;
        case FLITR_PIX_FMT_RGB_8:
            switch (outFormat)
            {
            case FLITR_PIX_FMT_Y_8:
            {
                *outData=( ((uint16_t)(*(inData+0)))+
                           ((uint16_t)(*(inData+1)))+
                           ((uint16_t)(*(inData+2))) )/3;
            }
                break;
            case FLITR_PIX_FMT_RGB_8:
            {
                *(outData+0)=*(inData+0);
                *(outData+1)=*(inData+1);
                *(outData+2)=*(inData+2);
            }
                break;
            case FLITR_PIX_FMT_Y_16:
            {
                uint16_t y16=( ( ((uint32_t)(*(inData+0)))+
                                 ((uint32_t)(*(inData+1)))+
                                 ((uint32_t)(*(inData+2))) ) << 8 ) / 3;
                *(outData+0)=y16 & 0xFF;
                *(outData+1)=(y16 >> 8) & 0xFF;
            }
                break;
            default:
                break;
            }
            break;
        case FLITR_PIX_FMT_Y_16:
            switch (outFormat)
            {
            case FLITR_PIX_FMT_Y_8:
            {
                *outData=*(inData+1);
            }
                break;
            case FLITR_PIX_FMT_RGB_8:
            {
                *(outData+0)=*(inData+1);
                *(outData+1)=*(inData+1);
                *(outData+2)=*(inData+1);
            }
                break;
            case FLITR_PIX_FMT_Y_16:
            {
                *(outData+0)=*(inData+0);
                *(outData+1)=*(inData+1);
            }
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }
    }

private:
    inline void setBytesPerPixel()
    {
        switch (PixelFormat_)
        {
        case FLITR_PIX_FMT_Y_8:
            BytesPerPixel_ = 1;
            break;
        case FLITR_PIX_FMT_RGB_8:
            BytesPerPixel_ = 3;
            break;
		case FLITR_PIX_FMT_BGR:
            BytesPerPixel_ = 3;
            break;
		case FLITR_PIX_FMT_BGRA:
            BytesPerPixel_ = 4;
            break;
        case FLITR_PIX_FMT_Y_16:
            BytesPerPixel_ = 2;
            break;
        default:
            // \todo maybe return error
            BytesPerPixel_ = 1;
        }
    }

    inline void setComponentsPerPixel()
    {
        switch (PixelFormat_)
        {
        case FLITR_PIX_FMT_Y_8:
            ComponentsPerPixel_ = 1;
            break;
        case FLITR_PIX_FMT_RGB_8:
            ComponentsPerPixel_ = 3;
            break;
		case FLITR_PIX_FMT_BGR:
            ComponentsPerPixel_ = 3;
            break;
		case FLITR_PIX_FMT_BGRA:
            ComponentsPerPixel_ = 4;
            break;
        case FLITR_PIX_FMT_Y_16:
            ComponentsPerPixel_ = 1;
            break;
        default:
            // \todo maybe return error
            ComponentsPerPixel_ = 1;
        }
    }

    uint32_t Width_;
    uint32_t Height_;
    PixelFormat PixelFormat_;
    uint32_t BytesPerPixel_;
    uint32_t ComponentsPerPixel_;
};

}

#endif //FLITR_IMAGE_FORMAT_H
