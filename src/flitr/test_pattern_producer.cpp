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

using namespace flitr;
using std::tr1::shared_ptr;


TestPatternProducer::TestPatternProducer(uint32_t width, uint32_t height, ImageFormat::PixelFormat out_pix_fmt,
                                         float patternSpeed,
                                         uint32_t buffer_size) :
    buffer_size_(buffer_size), numFramesDone_(0), patternSpeed_(patternSpeed)
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
        double testPatternFrame=numFramesDone_*patternSpeed_;
        unsigned char *inputData=image->data();

        unsigned long offset=0;
        double patternOffsetX=100.0*sin(testPatternFrame*0.05)+10.0*sin(testPatternFrame*2.5);
        double patternOffsetY=100.0*cos(testPatternFrame*0.05)+10.0*cos(testPatternFrame*2.5);
        double l=cos(sin(testPatternFrame*0.01*1.0)*0.45);
        double m=sin(sin(testPatternFrame*0.01*1.0)*0.45);
        double rotCentreX=ImageFormat_[0].getWidth()*0.5;
        double rotCentreY=ImageFormat_[0].getHeight()*0.5;

        for (long y=0; y<ImageFormat_[0].getHeight(); y++)
        {
            for (long x=0; x<ImageFormat_[0].getWidth(); x++)
            {

                unsigned long ix=(long)((x-rotCentreX)*l+(y-rotCentreY)*(-m)+patternOffsetX + 100000.5);
                unsigned long iy=(long)((x-rotCentreX)*m+(y-rotCentreY)*( l)+patternOffsetY + 100000.5);

                if (ImageFormat_[0].getPixelFormat()==ImageFormat::FLITR_PIX_FMT_Y_8)
                {
                  inputData[offset]=( ( (ix>>6)+(iy>>6) ) %2 )*255;
                  offset+=1;
                } else
                    if (ImageFormat_[0].getPixelFormat()==ImageFormat::FLITR_PIX_FMT_Y_16)
                    {
                      inputData[offset]=( ( ( (ix>>6)+(iy>>6) ) %2 )*65535 ) & 0xFF;
                      inputData[offset+1]=( ( ( (ix>>6)+(iy>>6) ) %2 )*65535 ) >> 8;
                      offset+=2;
                    } else
                        if (ImageFormat_[0].getPixelFormat()==ImageFormat::FLITR_PIX_FMT_RGB_8)
                        {
                          inputData[offset]=( ( (ix>>6)+(iy>>6) ) %2 )*255;
                          inputData[offset+1]=( ( (ix>>6)+(iy>>6) ) %2 )*255;
                          inputData[offset+2]=( ( (ix>>6)+(iy>>6) ) %2 )*255;
                          offset+=3;
                        } else
                        {
                            std::cout << "TestPatternProducer::trigger(), unknown output pixel format.\n";
                            std::cout.flush();
                            assert(0);
                        }
            }
        }
    }

    numFramesDone_++;
    releaseWriteSlot();

    return true;//Return true because we've reserved and released a write slot.
}
