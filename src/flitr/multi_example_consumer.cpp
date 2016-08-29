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

#include <flitr/multi_example_consumer.h>
#include <flitr/image_producer.h>

using namespace flitr;

//======
void MultiExampleConsumerThread::run()
{
    std::vector<Image**> imv;
    
    while (true) {
        // check if image available
        imv.clear();
        imv = _consumer->reserveReadSlot();
        
        if (imv.size() > 0)
        {
            for (int imNum=0; imNum<(int)_consumer->_imagesPerSlot; imNum++)
            {// Calculate the histogram.
                Image* im = *(imv[imNum]);
                
                //!Width of the flitr image.
                const uint32_t width=im->format()->getWidth();
                
                //!Height of the flitr image.
                const uint32_t height=im->format()->getHeight();
                
                //!Pixels per image.
                const uint32_t numPixels=width*height;
                
                //!Colour channels per pixel.
                const uint32_t componentsPerPixel=im->format()->getComponentsPerPixel();
                
                //!Pointer to flitr image pixel data. Only valid until _consumer->releaseReadSlot() below.
                unsigned char const * const data=(unsigned char const * const)im->data();
                
                //=============
                //Do something with the image here...
                std::cerr << "width=" << width << " " << "height=" << height << " " << "componentsPerPixel=" << componentsPerPixel << "\n";
                for (int j=0; j<1/*height*/; ++j)
                {
                    for (int i=0; i<width; ++i)
                    {
                        for (int c=0; c<componentsPerPixel; ++c)
                        {
                            std::cerr << int(data[j*(width*componentsPerPixel) + i*componentsPerPixel + c]) << " ";
                        }
                    }
                    std::cerr << "\n";
                }
                std::cerr.flush();
            }
            
            // indicate we are done with the image/s
            _consumer->releaseReadSlot();
        } else
        {
            // wait a while for producers.
            FThread::microSleep(1000);
        }
        // check for exit
        if (_shouldExit) {
            break;
        }
    }
}


//======
MultiExampleConsumer::MultiExampleConsumer(ImageProducer& producer,
                                         const uint32_t imagesPerSlot) :
ImageConsumer(producer),
_imagesPerSlot(imagesPerSlot)
{
    for (uint32_t i=0; i<imagesPerSlot; i++)
    {
        _imageFormatVec.push_back(producer.getFormat(i));
    }
}


//======
MultiExampleConsumer::~MultiExampleConsumer()
{
    _thread->setExit();
    _thread->join();
}


//======
bool MultiExampleConsumer::init()
{
    ImageConsumer::init();
    
    _thread = new MultiExampleConsumerThread(this);
    _thread->startThread();
    
    return true;
}


//======
bool MultiExampleConsumer::openConnection(const std::string &streamName)
{
    return false;
}


//======
bool MultiExampleConsumer::closeConnection()
{
    return false;
}


