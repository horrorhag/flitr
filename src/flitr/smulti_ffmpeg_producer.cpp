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

#include <flitr/smulti_ffmpeg_producer.h>

using namespace flitr;
using std::tr1::shared_ptr;

SMultiFFmpegProducer::SMultiFFmpegProducer(std::vector<std::string> filenames, ImageFormat::PixelFormat out_pix_fmt)
{
    Filenames_=filenames;
    out_pix_fmt_=out_pix_fmt;

    ImagesPerSlot_=Filenames_.size();

    NumImages_=0;
    CurrentImage_ = -1;
}

bool SMultiFFmpegProducer::init()
{
    int numFiles=Filenames_.size();
    for (int i=0; i<numFiles; i++)
    {
        Readers_.push_back(shared_ptr<FFmpegReader>(new FFmpegReader(Filenames_[i], out_pix_fmt_)));
        ImageFormat_.push_back(Readers_[i]->getFormat());
    }

    //check that all videos have the same format and number of frames.

    NumImages_ = Readers_[0]->getNumImages();

    CurrentImage_ = -1;

    // Allocate storage
    SharedImageBuffer_ = shared_ptr<SharedImageBuffer>(new SharedImageBuffer(*this, FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS, Readers_.size()));
    SharedImageBuffer_->initWithStorage();

    return true;
}

bool SMultiFFmpegProducer::trigger()
{
    uint32_t seek_to = CurrentImage_ + 1;
    return seek(seek_to);
}

bool SMultiFFmpegProducer::seek(uint32_t position)
{
    std::vector<Image**> imvec = reserveWriteSlot();
    if (imvec.size()<ImagesPerSlot_) {
        // too little storage available
        return false;
    }

    uint32_t seek_to = position % NumImages_;

    bool seek_result = true;

    int numReaders=Readers_.size();
    for (int i=0; i<numReaders; i++)
    {
        seek_result&=(Readers_[i]->getImage(*(*(imvec[i])), seek_to));
    }

    CurrentImage_ = Readers_[0]->getCurrentImage();

    if (CreateMetadataFunction_)
    {
        (*(imvec[0]))->setMetadata(CreateMetadataFunction_());
    }

    releaseWriteSlot();

    return seek_result;
}
