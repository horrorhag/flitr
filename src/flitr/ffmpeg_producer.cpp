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

#include <flitr/ffmpeg_producer.h>

using namespace flitr;
using std::tr1::shared_ptr;

FFmpegProducer::FFmpegProducer(std::string filename, ImageFormat::PixelFormat out_pix_fmt)
{
	Reader_ = shared_ptr<FFmpegReader>(new FFmpegReader(filename, out_pix_fmt));
	NumFrames_ = Reader_->getNumImages();
	CurrentFrame_ = 0;
	ImageFormat_.push_back(Reader_->getFormat());
}

bool FFmpegProducer::init()
{
	// Allocate storage
	SharedImageBuffer_ = shared_ptr<SharedImageBuffer>(new SharedImageBuffer(*this, 32, 1));
	SharedImageBuffer_->initWithStorage();
	return true;
}

bool FFmpegProducer::trigger()
{
	bool seek_ok = seek(CurrentFrame_);
	if (seek_ok) {
		CurrentFrame_ = (CurrentFrame_ + 1) % NumFrames_;
		return true;
	}
	return false;
}

bool FFmpegProducer::seek(uint32_t position)
{
	std::vector<Image**> imvec = reserveWriteSlot(); 
	if (imvec.size() == 0) {
		// no storage available
		return false;
	}
	bool seek_result = Reader_->getImage(*(*(imvec[0])), position % NumFrames_);
	releaseWriteSlot();
	return seek_result;
}
