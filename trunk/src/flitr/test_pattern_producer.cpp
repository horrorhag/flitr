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

#include <flitr/test_pattern_producer.h>
#include <cassert>

using namespace flitr;
using std::shared_ptr;


TestPatternProducer::TestPatternProducer(const uint32_t width, const uint32_t height, const ImageFormat::PixelFormat out_pix_fmt,
                                         const float patternSpeed, const uint8_t patternScale,
                                         uint32_t buffer_size) :
    buffer_size_(buffer_size), numFramesDone_(0),
    patternSpeed_(patternSpeed), patternScale_(patternScale)
{
    ImageFormat_.push_back(ImageFormat(width, height, out_pix_fmt));
}

bool TestPatternProducer::init()
{
    // Allocate storage
    SharedImageBuffer_ = shared_ptr<SharedImageBuffer>(new SharedImageBuffer(*this, buffer_size_, 1));
    SharedImageBuffer_->initWithStorage();
    return true;
}

bool TestPatternProducer::trigger()
{
    std::vector<Image**> imv = reserveWriteSlot();
    if (imv.size() != 1) {
        // no storage available
        return false;
    }

    Image *image=*(imv[0]);

    {
        const float testPatternFrame=numFramesDone_*patternSpeed_;
        unsigned char * const data=image->data();

        unsigned long offset=0;
        const double patternOffsetX=/*100.0*sin(testPatternFrame*0.05)+10.0*sin(testPatternFrame*2.5) +*/ 100000.5;
        const double patternOffsetY=/*100.0*cos(testPatternFrame*0.05)+10.0*cos(testPatternFrame*2.5) +*/ 100000.5;

        const double l=cos(sin(testPatternFrame*0.01)*0.45);
        const double m=sin(sin(testPatternFrame*0.01)*0.45);
        const double n=-m;
        const double o=l;

        const int32_t halfWidth=ImageFormat_[0].getWidth() >> 1;
        const int32_t halfHeight=ImageFormat_[0].getHeight() >> 1;



        if (ImageFormat_[0].getPixelFormat()==ImageFormat::FLITR_PIX_FMT_Y_8)
        {
            for (int32_t y=-halfHeight; y<halfHeight; y++)
            {
                double fix=-halfWidth*l+y*n+patternOffsetX;
                double fiy=-halfWidth*m+y*o+patternOffsetY;

                for (int32_t x=-halfWidth; x<halfWidth; x++)
                {
                    const uint8_t patternColour=( ( (((int32_t)fix)>>patternScale_)+(((int32_t)fiy)>>patternScale_) ) % 2 ) * 0xFF;

                    data[offset]=patternColour;

                    offset++;
                    fix+=l;
                    fiy+=m;
                }
            }
        } else
            if (ImageFormat_[0].getPixelFormat()==ImageFormat::FLITR_PIX_FMT_Y_16)
            {
                for (int32_t y=-halfHeight; y<halfHeight; y++)
                {
                    double fix=-halfWidth*l+y*n+patternOffsetX;
                    double fiy=-halfWidth*m+y*o+patternOffsetY;

                    for (int32_t x=-halfWidth; x<halfWidth; x++)
                    {
                        const uint8_t patternColour=( ( (((int32_t)fix)>>patternScale_)+(((int32_t)fiy)>>patternScale_) ) % 2 ) * 0xFF;

                        data[offset]=data[offset+1]=patternColour;

                        offset+=2;
                        fix+=l;
                        fiy+=m;
                    }
                }
            } else
                if (ImageFormat_[0].getPixelFormat()==ImageFormat::FLITR_PIX_FMT_Y_F32)
                {
                    for (int32_t y=-halfHeight; y<halfHeight; y++)
                    {
                        double fix=-halfWidth*l+y*n+patternOffsetX;
                        double fiy=-halfWidth*m+y*o+patternOffsetY;

                        for (int32_t x=-halfWidth; x<halfWidth; x++)
                        {
                            const uint8_t patternColour=( ( (((int32_t)fix)>>patternScale_)+(((int32_t)fiy)>>patternScale_) ) % 2 ) * 0xFF;

                            *((float*)(data+offset))=(patternColour>127) ? 1.0f : 0.0f;

                            offset+=4;
                            fix+=l;
                            fiy+=m;
                        }
                    }
                } else
                if (ImageFormat_[0].getPixelFormat()==ImageFormat::FLITR_PIX_FMT_RGB_8)
                {
                    for (int32_t y=-halfHeight; y<halfHeight; y++)
                    {
                        double fix=-halfWidth*l+y*n+patternOffsetX;
                        double fiy=-halfWidth*m+y*o+patternOffsetY;

                        for (int32_t x=-halfWidth; x<halfWidth; x++)
                        {
                            const uint8_t patternColour=( ( (((int32_t)fix)>>patternScale_)+(((int32_t)fiy)>>patternScale_) ) % 2 ) * 0xFF;

                            data[offset]=data[offset+1]=data[offset+2]=patternColour;

                            offset+=3;
                            fix+=l;
                            fiy+=m;
                        }
                    }
                } else
                {
                    std::cout << "TestPatternProducer::trigger(), unknown output pixel format.\n";
                    std::cout.flush();
                    assert(0);
                }
    }

    numFramesDone_++;
    releaseWriteSlot();

    return true;//Return true because we've reserved and released a write slot.
}
