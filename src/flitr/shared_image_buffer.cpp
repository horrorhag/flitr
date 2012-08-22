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

#include <flitr/shared_image_buffer.h>
#include <flitr/image_consumer.h>
#include <flitr/image_producer.h>

using namespace flitr;

SharedImageBuffer::SharedImageBuffer(ImageProducer& my_producer, uint32_t num_slots, uint32_t images_per_slot) :
	ImageProducer_(&my_producer),
	NumSlots_(num_slots+1),
	ImagesPerSlot_(images_per_slot),
	WriteTail_(0),
	WriteHead_(0),
	NumWriteReserved_(0),
	HasStorage_(false)
{
}

SharedImageBuffer::~SharedImageBuffer()
{
	if (HasStorage_) {
		for (uint32_t i=0; i<NumSlots_; i++) {
			for (uint32_t j=0; j<ImagesPerSlot_; j++) {
				delete Buffer_[i][j];
			}
		}
	}
}

bool SharedImageBuffer::initWithStorage()
{
    // assert producer has all formats

	// create images
	Buffer_.clear();
	Buffer_.resize(NumSlots_);
	for (uint32_t i=0; i<NumSlots_; i++) {
		Buffer_[i].reserve(ImagesPerSlot_);
		for (uint32_t j=0; j<ImagesPerSlot_; j++) {
			Buffer_[i].push_back(new Image(ImageProducer_->getFormat(j)));
		}
	}
	HasStorage_=true;
	return true;
}

bool SharedImageBuffer::initWithoutStorage()
{
	Buffer_.clear();
	Buffer_.resize(NumSlots_);
	for (uint32_t i=0; i<NumSlots_; i++) {
		Buffer_[i].resize(ImagesPerSlot_);
	}
	return true;
}

bool SharedImageBuffer::isFull() const
{
    // only allow up to -1, to diff between full and empty cases
    // assert fill > (NumSlots_-1)
    return (getFill() == (NumSlots_-1));
}

uint32_t SharedImageBuffer::getFill() const
{
    typedef std::map< const ImageConsumer*, uint32_t >::const_iterator map_it;
    uint32_t max_fill=0;

    for (map_it i = ReadTails_.begin(); i != ReadTails_.end(); ++i) {
        uint32_t read_tail = i->second;
        uint32_t fill = (WriteHead_ + NumSlots_ - read_tail) % NumSlots_;
        if (fill > max_fill) {
            max_fill = fill;
        }
    }
    // also check writer
    uint32_t fill = (WriteHead_ + NumSlots_ - WriteTail_) % NumSlots_;
    if (fill > max_fill) {
        max_fill = fill;
    }

    // assert max_fill > (NumSlots_-1)
    return max_fill;
}

bool SharedImageBuffer::tailPopped(const ImageConsumer& consumer)
{
    // the consumer that just popped is passed in
    // check the gap between its tail and write tail
    uint32_t popped_tail = ReadTails_[&consumer];
    uint32_t popped_gap = (WriteTail_ + NumSlots_ - popped_tail) % NumSlots_;

    // if there is a larger gap, this slot is still busy
    typedef std::map< const ImageConsumer*, uint32_t >::iterator map_it;
	for (map_it i = ReadTails_.begin(); i != ReadTails_.end(); ++i) {
		uint32_t read_tail = i->second;
		uint32_t gap = (WriteTail_ + NumSlots_ - read_tail) % NumSlots_;
		if (gap > popped_gap) {
            return false;
		}
	}
    return true;
}

uint32_t SharedImageBuffer::numAvailable(const ImageConsumer& consumer)
{
	// caller should lock
	uint32_t read_head = ReadHeads_[&consumer];
	return (WriteTail_ + NumSlots_ - read_head) % NumSlots_;
}

bool SharedImageBuffer::addConsumer(ImageConsumer& consumer)
{
	OpenThreads::ScopedLock<OpenThreads::Mutex> buflock(BufferMutex_);

    // init both to the current write tail
    ReadTails_[&consumer] = WriteTail_;
	ReadHeads_[&consumer] = WriteTail_;

	consumer.setSharedImageBuffer(*this);

	return true;
}

uint32_t SharedImageBuffer::getNumWriteSlotsAvailable() const
{
    //OpenThreads::ScopedLock<OpenThreads::Mutex> buflock(BufferMutex_);
    // only allow up to -1, to diff between full and empty cases
   return std::max<int32_t>( ( ((int32_t)NumSlots_) - 1 ) - getFill(), 0);
}

std::vector<Image**> SharedImageBuffer::reserveWriteSlot()
{
	OpenThreads::ScopedLock<OpenThreads::Mutex> buflock(BufferMutex_);
	
	std::vector<Image**> v;
	
	if (isFull()) {
		// we cannot write more, dropping images
		return v;
	}
	
	for (uint32_t i=0; i<ImagesPerSlot_; i++) {
		v.push_back(&(Buffer_[WriteHead_][i]));
	}
	
	WriteHead_ = (WriteHead_ + 1)  % NumSlots_;
	NumWriteReserved_++;
	
	return v;
}

void SharedImageBuffer::releaseWriteSlot()
{
	OpenThreads::ScopedLock<OpenThreads::Mutex> buflock(BufferMutex_);
	// assert !filled
	// assert NumWriteReserved_>0
	WriteTail_ = (WriteTail_ + 1)  % NumSlots_;
	NumWriteReserved_--;
}

uint32_t SharedImageBuffer::getLeastNumReadSlotsAvailable()
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> buflock(BufferMutex_);
    typedef std::map< const ImageConsumer*, uint32_t >::iterator map_it;
	uint32_t least_num = NumSlots_;
    for (map_it i = ReadTails_.begin(); i != ReadTails_.end(); ++i) {
        const ImageConsumer* c = i->first;
        uint32_t num_avail = numAvailable(*c); 
        if (num_avail < least_num) {
            least_num = num_avail;
        }
    }
    return least_num;
}

uint32_t SharedImageBuffer::getNumReadSlotsAvailable(const ImageConsumer& consumer)
{
	OpenThreads::ScopedLock<OpenThreads::Mutex> buflock(BufferMutex_);
	return numAvailable(consumer);
}

uint32_t SharedImageBuffer::getNumReadSlotsReserved(const ImageConsumer& consumer)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> buflock(BufferMutex_);
    return NumReadReserved_[&consumer];
}

std::vector<Image**> SharedImageBuffer::reserveReadSlot(const ImageConsumer& consumer)
{
	OpenThreads::ScopedLock<OpenThreads::Mutex> buflock(BufferMutex_);
	
	std::vector<Image**> v;

	if (numAvailable(consumer) == 0) {
		return v;
	}
	
	uint32_t read_head = ReadHeads_[&consumer];
	for (uint32_t i=0; i<ImagesPerSlot_; i++) {
		v.push_back(&(Buffer_[read_head][i]));
	}
	
	ReadHeads_[&consumer] = (ReadHeads_[&consumer] + 1)  % NumSlots_;
	NumReadReserved_[&consumer]++;

	return v;
}

void SharedImageBuffer::releaseReadSlot(const ImageConsumer& consumer)
{
	// assert readreserved > 0
	bool do_notify = false;
	{
		OpenThreads::ScopedLock<OpenThreads::Mutex> buflock(BufferMutex_);
		// assert numAvailable > 0
		ReadTails_[&consumer] = (ReadTails_[&consumer] + 1) % NumSlots_;
		NumReadReserved_[&consumer]--;
		do_notify = tailPopped(consumer);
	}
	if (do_notify) {
		ImageProducer_->releaseReadSlotCallback();
	}
}
