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
using std::tr1::shared_ptr;

void MultiFFmpegProducerThread::run()
{
    // wait for all to init
    Parent_->ProducerThreadBarrier_.block(Parent_->ImagesPerSlot_+1);

    while (true) {
        // if we reach this barrier, it means we must seek or exit
        Parent_->ProducerThreadBarrier_.block(Parent_->ImagesPerSlot_+1);
        if (ShouldExit_) {
            return;
        }

        // seek on our producer
        Parent_->SeekOK_[Index_] = Parent_->Producers_[Index_]->seek(Parent_->SeekPos_);
        if (Parent_->SeekOK_[Index_]) {
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
        Parent_->ProducerThreadBarrier_.block(Parent_->ImagesPerSlot_+1);
        if (ShouldExit_) {
            return;
        }
    }
}

MultiFFmpegProducer::MultiFFmpegProducer(std::vector<std::string> filenames, ImageFormat::PixelFormat out_pix_fmt)
{
	ImagesPerSlot_ = filenames.size();
	NumFrames_ = 0;
	Producers_.resize(ImagesPerSlot_);
	Consumers_.resize(ImagesPerSlot_);
	for (uint32_t i=0; i < ImagesPerSlot_; ++i) {
		Producers_[i] = shared_ptr<FFmpegProducer>(new FFmpegProducer(filenames[i], out_pix_fmt));
		Producers_[i]->init();
		Consumers_[i] = shared_ptr<ImageConsumer>(new ImageConsumer(*Producers_[i]));
		Consumers_[i]->init();

        ImageFormat_.push_back(Producers_[i]->getFormat());

		uint32_t num_frames = Producers_[i]->getNumImages();
		if (num_frames > NumFrames_) {
			NumFrames_ = num_frames;
		}
	}
	CurrentFrame_ = 0;
	
    ProducerThreads_.resize(ImagesPerSlot_);
    SeekOK_.resize(ImagesPerSlot_);
    
}

bool MultiFFmpegProducer::init()
{
	// Allocate storage
	SharedImageBuffer_ = shared_ptr<SharedImageBuffer>(
        new SharedImageBuffer(*this, 32, ImagesPerSlot_));
	SharedImageBuffer_->initWithoutStorage();
	
    for (uint32_t i=0; i < ImagesPerSlot_; ++i) {
        ProducerThreads_[i] = new MultiFFmpegProducerThread(this, i);
        ProducerThreads_[i]->startThread();
    }

    // wait for all threads to init
    ProducerThreadBarrier_.block(ImagesPerSlot_+1);
        
	return true;
}

MultiFFmpegProducer::~MultiFFmpegProducer()
{
    for (uint32_t i=0; i < ImagesPerSlot_; ++i) {
        ProducerThreads_[i]->setExit();
    }
    // trigger threads
    ProducerThreadBarrier_.block(ImagesPerSlot_+1);

    for (uint32_t i=0; i < ImagesPerSlot_; ++i) {
        ProducerThreads_[i]->join();
    }
}

bool MultiFFmpegProducer::trigger()
{
	bool seek_ok = seek(CurrentFrame_);
	if (seek_ok) {
		CurrentFrame_ = (CurrentFrame_ + 1) % NumFrames_;
		return true;
	}
	return false;
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
    ProducerThreadBarrier_.block(ImagesPerSlot_+1);

    // now wait for all seeks to complete
    ProducerThreadBarrier_.block(ImagesPerSlot_+1);

	bool all_seek_ok = true;
    for (uint32_t i=0; i < ImagesPerSlot_; ++i) {
        if (!SeekOK_[i]) { // SeekOK_ set in threads
            all_seek_ok = false;
        }
    }

	if (all_seek_ok) {
		releaseWriteSlot();
	} else {
        // \todo implement discard
        //discardWriteSlot();
    }

	return all_seek_ok;
}

void MultiFFmpegProducer::releaseReadSlotCallback()
{
	// all external consumers are done with the oldest slot
	for (uint32_t i=0; i < ImagesPerSlot_; ++i) {
		Consumers_[i]->releaseReadSlot();
	}
}
