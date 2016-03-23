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


MultiLibTiffProducer::MultiLibTiffProducer(const std::vector<std::string> &fileVec, const uint32_t buffer_size) :
fileVec_(fileVec),
buffer_size_(buffer_size)
{
    for (auto & filename : fileVec_)
    {
        //Get image dimensions and scan line size (in bytes)...
        tifVec_.push_back(TIFFOpen(filename.c_str(), "r"));
        
        currentPage_=0;
        currentImage_=-1;
        
        if (tifVec_.back())
        {
            uint32 width, height;
            
            TIFFGetField(tifVec_.back(), TIFFTAG_IMAGEWIDTH, &width);
            TIFFGetField(tifVec_.back(), TIFFTAG_IMAGELENGTH, &height);
            
            tifScanLineVec_.push_back((uint8_t*) _TIFFmalloc(TIFFScanlineSize(tifVec_.back())));
            
            ImageFormat imf(width, height, flitr::ImageFormat::PixelFormat::FLITR_PIX_FMT_Y_8);
            ImageFormat_.push_back(imf);
            
            TIFFClose(tifVec_.back());
            tifVec_.back()=nullptr;
        } else
        {
            tifScanLineVec_.push_back(nullptr);
            
            ImageFormat imf(0, 0, flitr::ImageFormat::PixelFormat::FLITR_PIX_FMT_Y_8);
            ImageFormat_.push_back(imf);
        }
    }
}

MultiLibTiffProducer::~MultiLibTiffProducer()
{
    for (auto scanLine : tifScanLineVec_)
    {
        _TIFFfree(scanLine);
    }
}

bool MultiLibTiffProducer::setAutoLoadMetaData(std::shared_ptr<ImageMetadata> defaultMetadata)
{
    DefaultMetadata_=std::shared_ptr<ImageMetadata>(defaultMetadata->clone());
    
    const size_t posOfDot=fileVec_[0].find_last_of('.');
    if (posOfDot!=std::string::npos)
    {
        std::string metadataFilename=".meta";
        metadataFilename.insert(0,fileVec_[0],0,posOfDot);
        
        MetadataReader_=std::shared_ptr<MetadataReader>(new MetadataReader(metadataFilename));
        
        return true;
    }
    else
    {
        return false;
    }
}

bool MultiLibTiffProducer::init()
{
    const size_t numFiles=fileVec_.size();
    
    for (size_t fileNum=0; fileNum<numFiles; ++fileNum)
    {
        //Check again if file can be opened so that false can be returned if not.
        tifVec_[fileNum] = TIFFOpen(fileVec_[fileNum].c_str(), "r");
        
        if (!tifVec_[fileNum]) return false;
        
        TIFFClose(tifVec_[fileNum]);
        tifVec_[fileNum]=nullptr;
    }
    
    // Allocate storage
    SharedImageBuffer_ = shared_ptr<SharedImageBuffer>(new SharedImageBuffer(*this, buffer_size_, numFiles));
    SharedImageBuffer_->initWithStorage();
    
    return true;
}

bool MultiLibTiffProducer::trigger()
{
    return seek((currentPage_<0xFFFFFFFF) ? (currentPage_ + 1) : 0xFFFFFFFF);
}

bool MultiLibTiffProducer::seek(uint32_t position)
{
    const size_t numFiles=fileVec_.size();
    
    //Open files and go to appropriate tiff page...
    for (size_t fileNum=0; fileNum<numFiles; ++fileNum)
    {
        //!ToDo scan through base, X2, X3, etc. until position's chunk or last chunk reached...
        
        uint32_t numPages=0;

        tifVec_[fileNum] = TIFFOpen(fileVec_[fileNum].c_str(), "r");
        if (position!=0xFFFFFFFF)
        {//Short circuit the tif page count if seeking to end of last chunk.
            numPages=TIFFNumberOfDirectories(tifVec_[fileNum]);
        }
        uint32_t chunkStartPage=0;
        
        const size_t dotPos=fileVec_[fileNum].find_last_of('.');
        const std::string baseFileName=fileVec_[fileNum].substr(0, dotPos);
        const std::string extension=fileVec_[fileNum].substr(dotPos);
        
        TIFF* tifNextChunk=nullptr;
        int chunkID=2;
        
        while ((position>=numPages) &&
               (tifNextChunk = TIFFOpen((baseFileName+std::string("_X")+std::to_string(chunkID)+extension).c_str(), "r")) )
        {
            TIFFClose(tifVec_[fileNum]);//Close the previous chunk.
            
            tifVec_[fileNum]=tifNextChunk;
            
            if (position!=0xFFFFFFFF)
            {//Short circuit the tif page count if seeking to end of last chunk.
                chunkStartPage=numPages;
                numPages+=TIFFNumberOfDirectories(tifVec_[fileNum]);
            }
            
            ++chunkID;
        }
        
        if (position==0xFFFFFFFF)
        {//If we're seeking to end of file then get the numPages in current open file. chunkStartPage is still zero.
            numPages=TIFFNumberOfDirectories(tifVec_[fileNum]) - 1;
        }
        
        int rvalue=TIFFSetDirectory(tifVec_[fileNum], (position>=numPages) ? ((numPages-1)-chunkStartPage) : (position-chunkStartPage));
        
        if ((rvalue!=0) && (fileNum==0))
        {
            currentPage_=position;
        }
    }
    
    
    //Read image slot...
    const bool rvalue=readSlot();
    
    
    //Close files.
    for (size_t fileNum=0; fileNum<numFiles; ++fileNum)
    {
        TIFFClose(tifVec_[fileNum]);
        tifVec_[fileNum]=nullptr;
    }
    
    return rvalue;
}


uint32_t MultiLibTiffProducer::getNumImages(const size_t fileNum)
{
    //Open first tiff chunk.
    tifVec_[fileNum] = TIFFOpen(fileVec_[fileNum].c_str(), "r");
    uint32_t numPages=TIFFNumberOfDirectories(tifVec_[fileNum]);
    TIFFClose(tifVec_[fileNum]);

    const size_t dotPos=fileVec_[fileNum].find_last_of('.');
    const std::string baseFileName=fileVec_[fileNum].substr(0, dotPos);
    const std::string extension=fileVec_[fileNum].substr(dotPos);
    
    //!Add num pages from _X2, _X3, etc. if they exist.
    int chunkID=2;
    while ( (tifVec_[fileNum] = TIFFOpen((baseFileName+std::string("_X")+std::to_string(chunkID)+extension).c_str(), "r")) )
    {
        numPages+=TIFFNumberOfDirectories(tifVec_[fileNum]);
        TIFFClose(tifVec_[fileNum]);
        
        ++chunkID;
    }
    
    tifVec_[fileNum]=nullptr;
    
    return numPages;
}


bool MultiLibTiffProducer::readSlot()
{
    std::vector<Image**> imv = reserveWriteSlot();
    
    if (imv.size() == 0)
    {
        // no storage available
        return false;
    }
    
    const size_t numImages=imv.size();
    
    currentImage_=currentPage_;
    
    for (size_t imageNum=0; imageNum<numImages; ++imageNum)
    {
        Image * const image=*(imv[imageNum]);
        const uint32_t width=ImageFormat_[imageNum].getWidth();
        const uint32_t height=ImageFormat_[imageNum].getHeight();
        
        uint8_t * const downStreamData=image->data();
        
        for (size_t y=0; y<height; ++y)
        {
            TIFFReadScanline(tifVec_[imageNum], tifScanLineVec_[imageNum], y);
            
            uint16_t * const scanLineData=(uint16_t *)tifScanLineVec_[imageNum];
            const size_t dsLineOffset=y*width;
            
            for (size_t x=0; x<width; ++x)
            {
                downStreamData[dsLineOffset + x] = (scanLineData[x]) >> 0;
            }
        }
        
        
        if (imageNum==0)
        {
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
                }
        }
    }
    
    releaseWriteSlot();
    
    return true;//Return true because we've reserved and released a write slot.
}
