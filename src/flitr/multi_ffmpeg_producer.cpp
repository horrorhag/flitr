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

#include <flitr/multi_ffmpeg_producer.h>

using namespace flitr;
using std::shared_ptr;

void MultiFFmpegProducerThread::run()
{
    // wait for all to init
    Parent_->ProducerThreadBarrier_.block();

    while (true) {
        // if we reach this barrier, it means we must seek or exit
        Parent_->ProducerThreadBarrier_.block();
        if (ShouldExit_) {
            return;
        }

        // seek on our producer
        Parent_->SeekOK_[Index_] = Parent_->Producers_[Index_]->seek(Parent_->SeekPos_);
        if (Parent_->SeekOK_[Index_])
        {
            std::vector<Image**> in_imvec = Parent_->Consumers_[Index_]->reserveReadSlot();
			if (in_imvec.size()!=1) {
				// the producer failed
                Parent_->SeekOK_[Index_] = false;
			} else {
				// point the output buffer to the correct image
				// Image * = Image *
				*(Parent_->OutputImageVector_[Index_]) = *(in_imvec[0]);
			}
        }
        // indicate we are done
        Parent_->ProducerThreadBarrier_.block();
        if (ShouldExit_) {
            return;
        }
    }
}

MultiFFmpegProducer::MultiFFmpegProducer(std::vector<std::string> filenames, ImageFormat::PixelFormat out_pix_fmt, uint32_t buffer_size) :
    ProducerThreadBarrier_(filenames.size()+1),
  buffer_size_(buffer_size)
{
	ImagesPerSlot_ = filenames.size();
	NumImages_ = 0;
	Producers_.resize(ImagesPerSlot_);
	Consumers_.resize(ImagesPerSlot_);
    for (uint32_t i=0; i < ImagesPerSlot_; ++i)
    {
        Producers_[i] = shared_ptr<FFmpegProducer>(new FFmpegProducer(filenames[i], out_pix_fmt, buffer_size_));
		Producers_[i]->init();
		Consumers_[i] = shared_ptr<ImageConsumer>(new ImageConsumer(*Producers_[i]));
		Consumers_[i]->init();

        ImageFormat_.push_back(Producers_[i]->getFormat());

		uint32_t num_frames = Producers_[i]->getNumImages();
		if (num_frames > NumImages_) {
			NumImages_ = num_frames;
		}
	}
	CurrentImage_ = -1;
	
    ProducerThreads_.resize(ImagesPerSlot_);
    SeekOK_.resize(ImagesPerSlot_);    
}

bool MultiFFmpegProducer::setAutoLoadMetaData(std::shared_ptr<ImageMetadata> defaultMetadata)
{
    bool success=true;

    for (uint32_t i=0; i < ImagesPerSlot_; ++i)
    {
        success=success & Producers_[i]->setAutoLoadMetaData(defaultMetadata);
    }

    return success;
}

bool MultiFFmpegProducer::init()
{
    for (uint32_t i=0; i < ImagesPerSlot_; ++i) {
        if (Producers_[i]->getNumImages()==0)
        {
                logMessage(LOG_CRITICAL) << "Something is wrong.  Video " << i << " has zero frames.\n";
                return false;
        }
    }

	// Allocate storage
	SharedImageBuffer_ = shared_ptr<SharedImageBuffer>(
        new SharedImageBuffer(*this, buffer_size_, ImagesPerSlot_));
	SharedImageBuffer_->initWithoutStorage();
	
    for (uint32_t i=0; i < ImagesPerSlot_; ++i) {
        ProducerThreads_[i] = new MultiFFmpegProducerThread(this, i);
        ProducerThreads_[i]->startThread();
    }

    // wait for all threads to init
    ProducerThreadBarrier_.block();
        
	return true;
}

MultiFFmpegProducer::~MultiFFmpegProducer()
{
    int barrier_num_threads=ImagesPerSlot_+1;

    for (uint32_t i=0; i < ImagesPerSlot_; ++i) {
        if (ProducerThreads_[i])
        {
            ProducerThreads_[i]->setExit();
        }
        else
        {//The thread was never initialised so fake the sync/block.
            barrier_num_threads--;
        }
    }
    // trigger threads
    ProducerThreadBarrier_.block(barrier_num_threads);

    for (uint32_t i=0; i < ImagesPerSlot_; ++i) {
        if (ProducerThreads_[i]) ProducerThreads_[i]->join();
    }
}

bool MultiFFmpegProducer::trigger()
{
    uint32_t seek_to = CurrentImage_ + 1;
	return seek(seek_to);
}

bool MultiFFmpegProducer::seek(uint32_t position)
{
    SeekPos_ = position;
    
    OutputImageVector_ = reserveWriteSlot(); 
	if (OutputImageVector_.size() != ImagesPerSlot_) {
		// no storage available
		return false;
	}
	
	// do a seek on each of our producers
    // trigger threads
    ProducerThreadBarrier_.block();

    // now wait for all seeks to complete
    ProducerThreadBarrier_.block();

	bool all_seek_ok = true;
    for (uint32_t i=0; i < ImagesPerSlot_; ++i) {
        if (!SeekOK_[i]) { // SeekOK_ set in threads
            all_seek_ok = false;
        }
    }

	if (all_seek_ok) {
		releaseWriteSlot();
        CurrentImage_ = SeekPos_;
	} else {
        // \todo implement discard
        //discardWriteSlot();
    }

	return all_seek_ok;
}

void MultiFFmpegProducer::releaseReadSlotCallback()
{
//    std::cout << "releaseReadSlotCallback\n";
//    std::cout.flush();

	// all external consumers are done with the oldest slot
	for (uint32_t i=0; i < ImagesPerSlot_; ++i) {
		Consumers_[i]->releaseReadSlot();
	}
}
