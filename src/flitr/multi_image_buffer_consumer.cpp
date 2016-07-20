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

#include <flitr/multi_image_buffer_consumer.h>
#include <flitr/image_producer.h>

using namespace flitr;

//======
void MultiImageBufferConsumerThread::run()
{
    std::vector<Image**> imv;
    
    while (true)
    {
        // check if image available
        imv.clear();
        imv = _consumer->reserveReadSlot();
        
        if (imv.size() > 0)
        {
            OpenThreads::ScopedLock<OpenThreads::Mutex> wlock(*_consumer->_accessMutex);
            
            if (!_consumer->_buffersHold)
            {
                for (int imNum=0; imNum<(int)_consumer->_imagesPerSlot; imNum++)
                {// Calculate the histogram.
                    Image* im = *(imv[imNum]);
                    
                    if (im->format()->getPixelFormat() == flitr::ImageFormat::FLITR_PIX_FMT_RGB_8)
                    {
                        const uint32_t width=im->format()->getWidth();
                        const uint32_t height=im->format()->getHeight();
                        const uint32_t componentsPerPixel=im->format()->getComponentsPerPixel();
                        
                        //!Pointer to flitr image pixel data. Only valid until _consumer->releaseReadSlot() below.
                        unsigned char const * const data=(unsigned char const * const)im->data();
                        
                        if ((imNum < _consumer->_bufferVec.size()) && (_consumer->_bufferVec[imNum]!=nullptr))
                        {
                            memcpy(_consumer->_bufferVec[imNum], data, width*height*componentsPerPixel);
                        }
                    }
                }
            }
            
            // indicate we are done with the image/s
            _consumer->releaseReadSlot();
        } else
        {
            // wait a while for producers.
            Thread::microSleep(1000);
        }
        // check for exit
        if (_shouldExit) {
            break;
        }
    }
}


//======
MultiImageBufferConsumer::MultiImageBufferConsumer(ImageProducer& producer,
                                                   const uint32_t imagesPerSlot) :
ImageConsumer(producer),
_imagesPerSlot(imagesPerSlot),
_buffersHold(false)
{
    for (uint32_t i=0; i<imagesPerSlot; i++)
    {
        _imageFormatVec.push_back(producer.getFormat(i));
        _bufferVec.push_back(nullptr);
    }
}


//======
MultiImageBufferConsumer::~MultiImageBufferConsumer()
{
    _thread->setExit();
    _thread->join();
}


//======
bool MultiImageBufferConsumer::init()
{
    ImageConsumer::init();
    
    _thread = new MultiImageBufferConsumerThread(this);
    _thread->startThread();
    
    return true;
}


void MultiImageBufferConsumer::setBufferVec(const std::vector<uint8_t *> bufferVec)
{
    _bufferVec=bufferVec;
}


void MultiImageBufferConsumer::setBufferHold(const bool hold)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> wlock(*_accessMutex);
    
    _buffersHold=hold;
}

