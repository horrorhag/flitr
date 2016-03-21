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

#include <flitr/libtiff_producer.h>

using namespace flitr;
using std::shared_ptr;


LibTiffProducer::LibTiffProducer(const std::string filename, const uint32_t buffer_size) :
filename_(filename),
buffer_size_(buffer_size)
{
    tif_ = TIFFOpen(filename_.c_str(), "r");
    
    currentDir_=0;
    currentImage_=-1;
    tifScanLine_=nullptr;
    
    if (tif_)
    {
        uint32 width, height;
        
        TIFFGetField(tif_, TIFFTAG_IMAGEWIDTH, &width);
        TIFFGetField(tif_, TIFFTAG_IMAGELENGTH, &height);
        
        tifScanLine_ = (uint8_t*) _TIFFmalloc(TIFFScanlineSize(tif_));
        
        ImageFormat imf(width, height, flitr::ImageFormat::PixelFormat::FLITR_PIX_FMT_Y_8);
        ImageFormat_.push_back(imf);
        
        TIFFClose(tif_);
    }
}

LibTiffProducer::~LibTiffProducer()
{
    _TIFFfree(tifScanLine_);
}

bool LibTiffProducer::setAutoLoadMetaData(std::shared_ptr<ImageMetadata> defaultMetadata)
{
    DefaultMetadata_=std::shared_ptr<ImageMetadata>(defaultMetadata->clone());
    
    const size_t posOfDot=filename_.find_last_of('.');
    if (posOfDot!=std::string::npos)
    {
        std::string metadataFilename=".meta";
        metadataFilename.insert(0,filename_,0,posOfDot);
        
        MetadataReader_=std::shared_ptr<MetadataReader>(new MetadataReader(metadataFilename));
        
        return true;
    }
    else
    {
        return false;
    }
}

bool LibTiffProducer::init()
{
    tif_ = TIFFOpen(filename_.c_str(), "r");
    if (!tif_) return false;
    TIFFClose(tif_);
    
    // Allocate storage
    SharedImageBuffer_ = shared_ptr<SharedImageBuffer>(new SharedImageBuffer(*this, buffer_size_, 1));
    SharedImageBuffer_->initWithStorage();
    
    return true;
}

bool LibTiffProducer::trigger()
{
    return seek(currentDir_ + 1);
}

bool LibTiffProducer::seek(uint32_t position)
{
    tif_ = TIFFOpen(filename_.c_str(), "r");
    
    const uint16_t numDirs=TIFFNumberOfDirectories(tif_);
    const uint16_t dirPosition=position % numDirs;
    
    TIFFSetDirectory(tif_, dirPosition);
    currentDir_=dirPosition;
    
    bool rvalue=readImage();
    
    TIFFClose(tif_);
    
    return rvalue;
}

uint32_t LibTiffProducer::getNumImages()
{
    tif_ = TIFFOpen(filename_.c_str(), "r");
    
    const uint16_t numDirs=TIFFNumberOfDirectories(tif_);
    
    TIFFClose(tif_);
    
    return numDirs;
}


bool LibTiffProducer::readImage()
{
    std::vector<Image**> imv = reserveWriteSlot();
    
    if (imv.size() != 1)
    {
        // no storage available
        return false;
    }
    
    Image * const image=*(imv[0]);
    const uint32_t width=ImageFormat_[0].getWidth();
    const uint32_t height=ImageFormat_[0].getHeight();
    
    
    {
        currentImage_=currentDir_;
        uint8_t * const downStreamData=image->data();
        
        for (size_t y=0; y<height; ++y)
        {
            TIFFReadScanline(tif_, tifScanLine_, y);
            const size_t dsLineOffset=y*width;
            
            for (size_t x=0; x<width; ++x)
            {
                downStreamData[dsLineOffset + x] = (((uint16_t *)tifScanLine_)[x]) >> 0;
            }
        }
        
        // If there is a create meta data function, then use it to stay backwards compatible with uses before the setAutoLoadMetaData method was added.
        // Otherwise use the MetaDataReader_ as set up by setAutoLoadMetaData(...)
        if (CreateMetadataFunction_)
        {
            image->setMetadata(CreateMetadataFunction_());
        } else
            if ((MetadataReader_)&&(DefaultMetadata_))
            {
                // set default meta data which also gives access to the desired metadata class' readFromStream method.
                image->setMetadata(std::shared_ptr<ImageMetadata>(DefaultMetadata_->clone()));
                
                // update with the meta data's virtual readFromStream() method via the meta data reader.
                MetadataReader_->readFrame(*image, currentImage_);
                
                //           std::cout << "FFmpegProducer: " << image->metadata()->getString();
                //           std::cout.flush();
            }
    }
    
    releaseWriteSlot();
    
    return true;//Return true because we've reserved and released a write slot.
}
