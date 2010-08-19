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

#include <flitr/flitr_stdint.h>
#include <flitr/multi_osg_consumer.h>
#include <flitr/image_producer.h>

#ifdef _WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

using namespace flitr;

void MultiOSGConsumerThread::run() 
{
	uint32_t ims_per_slot = Consumer_->ImagesPerSlot_;
	
	std::vector<Image**> imv;
	
	while (true) {
		// check if image available
		// reading and popping must be synced for the consumer
		{
			OpenThreads::ScopedLock<OpenThreads::Mutex> buflock(Consumer_->BufferMutex_);
			uint32_t num_avail = Consumer_->getNumReadSlotsAvailable();
			// discard all but one
			if (num_avail > 1) {
				for (uint32_t i=0; i<num_avail-1; i++) {
					imv = Consumer_->reserveReadSlot();
					if (imv.size() >= ims_per_slot) {
						Consumer_->releaseReadSlot();
					}
				}
			}
		}
		// wait a while
		Thread::microSleep(10000);
		// check for exit
		if (ShouldExit_) {
			break;
		}
	}
}

MultiOSGConsumer::MultiOSGConsumer(ImageProducer& producer,
								   uint32_t images_per_slot,
                                   uint32_t images_in_history) :
	ImageConsumer(producer),
	ImagesPerSlot_(images_per_slot),
    ImagesInHistory_(images_in_history),
    HistoryWritePos_(0)
{
    for (uint32_t i=0; i<images_per_slot; i++) {
        ImageFormat_.push_back(producer.getFormat(i));
    }
}

MultiOSGConsumer::~MultiOSGConsumer()
{
    if (Thread_->isRunning()) {
        Thread_->setExit();
        Thread_->join();
    }
}

bool MultiOSGConsumer::init()
{
    OSGImages_.resize(ImagesInHistory_);
    OutputTextures_.resize(ImagesInHistory_);

    // create dummy images so the textures have an initial valid image
    DummyImages_.resize(ImagesPerSlot_);
    for (uint32_t i=0; i<ImagesPerSlot_; i++) {
        DummyImages_[i] = new osg::Image();
        switch (ImageFormat_[i].getPixelFormat()) {
          case ImageFormat::FLITR_PIX_FMT_Y_8:
            DummyImages_[i]->allocateImage(ImageFormat_[i].getWidth(),
                                           ImageFormat_[i].getHeight(),
                                           1, 
                                           GL_LUMINANCE, GL_UNSIGNED_BYTE);
            break;
          case ImageFormat::FLITR_PIX_FMT_RGB_8:
            DummyImages_[i]->allocateImage(ImageFormat_[i].getWidth(),
                                           ImageFormat_[i].getHeight(),
                                           1, 
                                           GL_RGB, GL_UNSIGNED_BYTE);
            break;
          default:
            // \todo report error
            break;
        }
    }

    for (uint32_t h=0; h<ImagesInHistory_; h++) {
        for (uint32_t i=0; i<ImagesPerSlot_; i++) {
            OSGImages_[h].push_back(new osg::Image());
            OSGImages_[h][i]->setPixelBufferObject(new osg::PixelBufferObject(OSGImages_[h][i].get()));
            switch (ImageFormat_[i].getPixelFormat()) {
              case ImageFormat::FLITR_PIX_FMT_Y_8:
                OSGImages_[h][i]->setImage(ImageFormat_[i].getWidth(),
                                           ImageFormat_[i].getHeight(),
                                           1, GL_LUMINANCE, GL_LUMINANCE, GL_UNSIGNED_BYTE, 
                                           DummyImages_[i]->data(),
                                           osg::Image::NO_DELETE);
                break;
              case ImageFormat::FLITR_PIX_FMT_RGB_8:
                OSGImages_[h][i]->setImage(ImageFormat_[i].getWidth(),
                                           ImageFormat_[i].getHeight(),
                                           1, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, 
                                           DummyImages_[i]->data(),
                                           osg::Image::NO_DELETE);
                break;
              default:
                // \todo report error
                break;
            }

            OutputTextures_[h].push_back(new osg::TextureRectangle());
            OutputTextures_[h][i]->setImage(OSGImages_[h][i].get());
            OutputTextures_[h][i]->setTextureWidth(ImageFormat_[i].getWidth());
            OutputTextures_[h][i]->setTextureHeight(ImageFormat_[i].getHeight());
            //OutputTextures_[h][i]->setFilter(osg::TextureRectangle::MIN_FILTER,osg::TextureRectangle::NEAREST);
            //OutputTextures_[h][i]->setFilter(osg::TextureRectangle::MAG_FILTER,osg::TextureRectangle::NEAREST);
            //OutputTextures_[h][i]->setFilter(osg::TextureRectangle::MIN_FILTER,osg::TextureRectangle::LINEAR);
            //OutputTextures_[h][i]->setFilter(osg::TextureRectangle::MAG_FILTER,osg::TextureRectangle::LINEAR);
        }	
    }
	Thread_ = std::tr1::shared_ptr<MultiOSGConsumerThread>(
        new MultiOSGConsumerThread(this));

	return true;
}

void MultiOSGConsumer::startDiscardThread()
{
    Thread_->startThread();
}

osg::Image* MultiOSGConsumer::getOSGImage(uint32_t im_number, uint32_t im_age)
{
	// \todo check input num and input age

    int32_t read_pos = (HistoryWritePos_ - 1) - im_age;
    if (read_pos < 0) {
        read_pos += ImagesInHistory_;
    }
	return OSGImages_[read_pos][im_number].get();
}

osg::TextureRectangle* MultiOSGConsumer::getOutputTexture(uint32_t tex_number, uint32_t tex_age)
{
    return OutputTextures_[tex_age][tex_number].get();
}

bool MultiOSGConsumer::getNext()
{
    // if we are busy with all images, release one
    if (getNumReadSlotsReserved() == ImagesInHistory_) {
        releaseReadSlot();
    }

	OpenThreads::ScopedLock<OpenThreads::Mutex> buflock(BufferMutex_);
	std::vector<Image**> imv = reserveReadSlot();
	if (imv.size() >= ImagesPerSlot_) {
        // connect osg images to buffered data
        for (uint32_t i=0; i<ImagesPerSlot_; i++) {
			Image *im = *(imv[i]);
            switch (ImageFormat_[i].getPixelFormat()) {
              case ImageFormat::FLITR_PIX_FMT_Y_8:
                OSGImages_[HistoryWritePos_][i]->setImage(ImageFormat_[i].getWidth(),
                                                          ImageFormat_[i].getHeight(),
                                                          1, GL_LUMINANCE, GL_LUMINANCE, GL_UNSIGNED_BYTE, 
                                                          im->data(),
                                                          osg::Image::NO_DELETE);
                break;
              case ImageFormat::FLITR_PIX_FMT_RGB_8:
                OSGImages_[HistoryWritePos_][i]->setImage(ImageFormat_[i].getWidth(),
                                                          ImageFormat_[i].getHeight(),
                                                          1, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, 
                                                          im->data(),
                                                          osg::Image::NO_DELETE);
                break;
              default:
                // \todo report error
                break;
            }
			OSGImages_[HistoryWritePos_][i]->dirty();
		}
        // rewire the output textures
        for (uint32_t h=0; h<ImagesInHistory_; h++) {
            for (uint32_t i=0; i<ImagesPerSlot_; i++) {
                int32_t read_pos = HistoryWritePos_ - h;
                if (read_pos < 0) {
                    read_pos += ImagesInHistory_;
                }
                OutputTextures_[h][i]->setImage(OSGImages_[read_pos][i].get());
            }
        }
        HistoryWritePos_ = (HistoryWritePos_ + 1) % ImagesInHistory_;
		return true;
	}
	return false;
}
