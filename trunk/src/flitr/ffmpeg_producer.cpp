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
	NumImages_ = Reader_->getNumImages();
	CurrentImage_ = -1;
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
    uint32_t seek_to = CurrentImage_ + 1;
	return seek(seek_to);
}

bool FFmpegProducer::seek(uint32_t position)
{
	std::vector<Image**> imvec = reserveWriteSlot(); 
	if (imvec.size() == 0) {
		// no storage available
		return false;
	}

    uint32_t seek_to = position % NumImages_;
	bool seek_result = Reader_->getImage(*(*(imvec[0])), seek_to);
    CurrentImage_ = Reader_->getCurrentImage();
	
    if (CreateMetadataFunction_) 
    {
      (*(imvec[0]))->setMetadata(CreateMetadataFunction_());
    }

    releaseWriteSlot();
	
    return seek_result;
}
