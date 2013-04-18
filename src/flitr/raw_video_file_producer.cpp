/* Framework for Live Image Transformation (FLITr) 
 * Copyright (c) 2013 CSIR
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

#include <flitr/raw_video_file_producer.h>

using namespace flitr;
using std::tr1::shared_ptr;


RawVideoFileProducer::RawVideoFileProducer(std::string filename, uint32_t buffer_size) :
    filename_(filename),
    buffer_size_(buffer_size)
{
    Reader_ = shared_ptr<RawVideoFileReader>(new RawVideoFileReader(filename_));
    NumImages_ = Reader_->getNumImages();
    CurrentImage_ = -1;
    ImageFormat_.push_back(Reader_->getFormat());
}

bool RawVideoFileProducer::setAutoLoadMetaData(std::tr1::shared_ptr<ImageMetadata> defaultMetadata)
{
    DefaultMetadata_=std::tr1::shared_ptr<ImageMetadata>(defaultMetadata->clone());

    size_t posOfDot=filename_.find_last_of('.');
    if (posOfDot!=std::string::npos)
    {
        std::string metadataFilename=".meta";
        metadataFilename.insert(0,filename_,0,posOfDot);

        MetadataReader_=std::tr1::shared_ptr<MetadataReader>(new MetadataReader(metadataFilename));

        return true;
    }
    else
    {
        return false;
    }
}

bool RawVideoFileProducer::init()
{
    if (NumImages_==0)
    {
        logMessage(LOG_CRITICAL) << "Something is wrong. This video has zero frames.\n";
        return false;
    }

    // Allocate storage
    SharedImageBuffer_ = shared_ptr<SharedImageBuffer>(new SharedImageBuffer(*this, buffer_size_, 1));
    SharedImageBuffer_->initWithStorage();
    return true;
}

bool RawVideoFileProducer::trigger()
{
    uint32_t seek_to = CurrentImage_ + 1;
    return seek(seek_to);
}

bool RawVideoFileProducer::seek(uint32_t position)
{
    std::vector<Image**> imv = reserveWriteSlot();
    if (imv.size() != 1) {
        // no storage available
        return false;
    }

    Image *image=*(imv[0]);

    uint32_t seek_to = position % NumImages_;

    bool seek_result = Reader_->getImage(*image, seek_to);
    //The seek result should be true because were only seeking within the video.
    //ToDo: Add an assert to flag if seek_result is not true.

    CurrentImage_ = Reader_->getCurrentImage();

    // If there is a create meta data function, then use it to stay backwards compatible with uses before the setAutoLoadMetaData method was added.
    // Otherwise use the MetaDataReader_ as set up by setAutoLoadMetaData(...)
    if (CreateMetadataFunction_)
    {
        image->setMetadata(CreateMetadataFunction_());
    } else
        if ((MetadataReader_)&&(DefaultMetadata_))
        {
            // set default meta data which also gives access to the desired metadata class' readFromStream method.
            image->setMetadata(std::tr1::shared_ptr<ImageMetadata>(DefaultMetadata_->clone()));

            // update with the meta data's virtual readFromStream() method via the meta data reader.
            MetadataReader_->readFrame(*image, seek_to);
        }

    releaseWriteSlot();

    return true;//Return true because we've reserved and released a write slot.
}
