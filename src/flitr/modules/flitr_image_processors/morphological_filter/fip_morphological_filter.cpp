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

#include <flitr/modules/flitr_image_processors/morphological_filter/fip_morphological_filter.h>


using namespace flitr;
using std::shared_ptr;

FIPMorphologicalFilter::FIPMorphologicalFilter(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                                               const size_t structuringElementSize,
                                               const float threshold,
                                               const float binaryMax,
                                               uint32_t buffer_size) :
ImageProcessor(upStreamProducer, images_per_slot, buffer_size),
structuringElementSize_(structuringElementSize),
threshold_(threshold),
binaryMax_(binaryMax),
morphologicalFilter_()
{
    //Setup image format being produced to downstream.
    for (uint32_t i=0; i<images_per_slot; i++) {
        ImageFormat downStreamFormat=upStreamProducer.getFormat();
        
        ImageFormat_.push_back(downStreamFormat);
    }
}

FIPMorphologicalFilter::~FIPMorphologicalFilter()
{
    delete [] passScratchData_[0];
    delete [] passScratchData_[1];
    delete tmpScratchData_;
}

bool FIPMorphologicalFilter::init()
{
    bool rValue=ImageProcessor::init();
    //Note: SharedImageBuffer of downstream producer is initialised with storage in ImageProcessor::init.
    
    size_t maxScratchDataSize=0;
    
    for (uint32_t i=0; i<ImagesPerSlot_; i++)
    {
        const ImageFormat imFormat=getUpstreamFormat(i);
        
        const size_t width=imFormat.getWidth();
        const size_t height=imFormat.getHeight();
        
        const size_t bytesPerPixel=imFormat.getBytesPerPixel();
        
        const size_t scratchDataSize = width * height * bytesPerPixel;
        
        if (scratchDataSize>maxScratchDataSize)
        {
            maxScratchDataSize=scratchDataSize;
        }
    }
    
    //Allocate a buffer big enough for any of the image slots.
    passScratchData_[0]=new uint8_t[maxScratchDataSize];
    passScratchData_[1]=new uint8_t[maxScratchDataSize];
    tmpScratchData_=new uint8_t[maxScratchDataSize];
    
    return rValue;
}

bool FIPMorphologicalFilter::trigger()
{
    if ((getNumReadSlotsAvailable())&&(getNumWriteSlotsAvailable()))
    {
        std::vector<Image**> imvRead=reserveReadSlot();
        
        std::vector<Image**> imvWrite=reserveWriteSlot();
        
        //Start stats measurement event.
        ProcessorStats_->tick();
        
        for (size_t imgNum=0; imgNum<ImagesPerSlot_; ++imgNum)
        {
            Image const * const imReadUS = *(imvRead[imgNum]);
            Image * const imWriteDS = *(imvWrite[imgNum]);
            const ImageFormat imFormat=getDownstreamFormat(imgNum);//down stream and up stream formats are the same.
            const size_t width=imFormat.getWidth();
            const size_t height=imFormat.getHeight();
            
            
            if (imFormat.getPixelFormat()==ImageFormat::FLITR_PIX_FMT_Y_F32)
            {
                float const * const dataReadUS=(float const * const)imReadUS->data();
                float * const dataWriteDS=(float * const)imWriteDS->data();
                
                doMorphoPasses(dataReadUS, dataWriteDS, width, height);
            } else
                if (imFormat.getPixelFormat()==ImageFormat::FLITR_PIX_FMT_Y_8)
                {
                    uint8_t const * const dataReadUS=(uint8_t const * const)imReadUS->data();
                    uint8_t * const dataWriteDS=(uint8_t * const)imWriteDS->data();
                    
                    doMorphoPasses(dataReadUS, dataWriteDS, width, height);
                } else
                    if (imFormat.getPixelFormat()==ImageFormat::FLITR_PIX_FMT_RGB_F32)
                    {
                        float const * const dataReadUS=(float const * const)imReadUS->data();
                        float * const dataWriteDS=(float * const)imWriteDS->data();
                        
                        doMorphoPassesRGB(dataReadUS, dataWriteDS, width, height);
                    } else
                        if (imFormat.getPixelFormat()==ImageFormat::FLITR_PIX_FMT_RGB_8)
                        {
                            uint8_t const * const dataReadUS=(uint8_t const * const)imReadUS->data();
                            uint8_t * const dataWriteDS=(uint8_t * const)imWriteDS->data();
                            
                            doMorphoPassesRGB(dataReadUS, dataWriteDS, width, height);
                        }
        }
        
        //Stop stats measurement event.
        ProcessorStats_->tock();
        
        releaseWriteSlot();
        releaseReadSlot();
        
        return true;
    }
    
    return false;
}

