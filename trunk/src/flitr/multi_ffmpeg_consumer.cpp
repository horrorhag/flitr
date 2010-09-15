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

#include <flitr/multi_ffmpeg_consumer.h>
#include <flitr/image_producer.h>

using namespace flitr;

void MultiFFmpegConsumerThread::run() 
{
	uint32_t num_writers = Consumer_->ImagesPerSlot_;
	std::vector<Image**> imv;
	
	while (true) {
		// check if image available
		imv.clear();
		imv = Consumer_->reserveReadSlot();
		if (imv.size() >= num_writers) { // allow selection of some sources
			OpenThreads::ScopedLock<OpenThreads::Mutex> wlock(Consumer_->WritingMutex_);
			if (Consumer_->Writing_) {
				for (unsigned int i=0; i<num_writers; i++) {
					Image* im = *(imv[i]);
					Consumer_->FFmpegWriters_[i]->writeVideoFrame(im->data());
					Consumer_->MetadataWriters_[i]->writeFrame(*im);
				}
			} else {
				// just discard
			}
			// indicate we are done with the image/s
			Consumer_->releaseReadSlot();
		} else {
			// wait a while
			Thread::microSleep(10000);
		}
		// check for exit
		if (ShouldExit_) {
			break;
		}
	}
}

MultiFFmpegConsumer::MultiFFmpegConsumer(ImageProducer& producer,
										 uint32_t images_per_slot) :
	ImageConsumer(producer),
	ImagesPerSlot_(images_per_slot),
	Writing_(false)
{
    for (uint32_t i=0; i<images_per_slot; i++) {
        ImageFormat_.push_back(producer.getFormat(i));
    }
}

MultiFFmpegConsumer::~MultiFFmpegConsumer()
{
	Thread_->setExit();
	Thread_->join();
}

bool MultiFFmpegConsumer::init()
{
	FFmpegWriters_.resize(ImagesPerSlot_);
	MetadataWriters_.resize(ImagesPerSlot_);

	Thread_ = new MultiFFmpegConsumerThread(this);
	Thread_->startThread();
    
    return true;
}

bool MultiFFmpegConsumer::openFiles(std::string basename)
{
	for (unsigned int i=0; i<ImagesPerSlot_; i++) {
		char c_count[16];
		sprintf(c_count, "%02d", i+1);
		std::string new_base = basename + "_" + c_count;
		std::string video_filename(new_base + ".avi");
		std::string metadata_filename(new_base + ".meta");
		
		FFmpegWriters_[i] = new FFmpegWriter(video_filename, ImageFormat_[i]);
		MetadataWriters_[i] = new MetadataWriter(metadata_filename);
	}
    return true;
}

bool MultiFFmpegConsumer::startWriting()
{
	OpenThreads::ScopedLock<OpenThreads::Mutex> wlock(WritingMutex_);
	Writing_ = true;
    return true;
}

bool MultiFFmpegConsumer::stopWriting()
{
	OpenThreads::ScopedLock<OpenThreads::Mutex> wlock(WritingMutex_);
	Writing_ = false;
    return true;
}

bool MultiFFmpegConsumer::closeFiles()
{
    stopWriting();
    
	for (unsigned int i=0; i<ImagesPerSlot_; i++) {
        if (FFmpegWriters_[i] != 0) {
            delete FFmpegWriters_[i];
            FFmpegWriters_[i] = 0;
        }
        if (MetadataWriters_[i] != 0) {
            delete MetadataWriters_[i];
            MetadataWriters_[i] = 0;
        }
	}
    return true;
}
