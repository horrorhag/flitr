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

#include <flitr/modules/flitr_image_processors/average_image/fip_average_image.h>

using namespace flitr;
using std::tr1::shared_ptr;

FIPAverageImage::FIPAverageImage(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                                 uint8_t base2WindowLength,
                                 uint32_t buffer_size) :
ImageProcessor(upStreamProducer, images_per_slot, buffer_size),
base2WindowLength_(base2WindowLength),
windowLength_((uint32_t)(powf(2.0f, base2WindowLength_)+0.5f)),
recipWindowLength_(1.0f/((float)windowLength_)),
oldestHistorySlot_(0)
{
    
    //Setup image format being produced to downstream.
    for (uint32_t i=0; i<images_per_slot; i++) {
        ImageFormat_.push_back(upStreamProducer.getFormat(i));//Output format is same as input format.
    }
    
}

FIPAverageImage::~FIPAverageImage()
{
    for (uint32_t i=0; i<ImagesPerSlot_; i++)
    {
        delete [] sumImageVec_[i];
        
        for (size_t historyIndex=0; historyIndex<windowLength_; historyIndex++)
        {
            delete [] historyImageVecVec_[i][historyIndex];
        }
        historyImageVecVec_[i].clear();
    }
    
    sumImageVec_.clear();
}

bool FIPAverageImage::init()
{
    bool rValue=ImageProcessor::init();
    //Note: SharedImageBuffer of downstream producer is initialised with storage in ImageProcessor::init.
    
    for (uint32_t i=0; i<ImagesPerSlot_; i++)
    {
        const ImageFormat imFormat=getUpstreamFormat(i);//Downstream format is same as upstream format.
        
        const size_t width=imFormat.getWidth();
        const size_t height=imFormat.getHeight();
        
        const size_t componentsPerPixel=imFormat.getComponentsPerPixel();
        
        sumImageVec_.push_back(new float[width*height*componentsPerPixel]);
        memset(sumImageVec_[i], 0, width*height*componentsPerPixel*sizeof(float));
        
        historyImageVecVec_.push_back(std::vector<float *>());
        for (size_t historyIndex=0; historyIndex<windowLength_; historyIndex++)
        {
            historyImageVecVec_[i].push_back(new float[width*height*componentsPerPixel]);
            memset(historyImageVecVec_[i][historyIndex], 0, width*height*componentsPerPixel*sizeof(float));
        }
    }
    
    return rValue;
}

bool FIPAverageImage::trigger()
{
    if ((getNumReadSlotsAvailable())&&(getNumWriteSlotsAvailable()))
    {//There are images to consume and the downstream producer has space to produce.
        std::vector<Image**> imvRead=reserveReadSlot();
        std::vector<Image**> imvWrite=reserveWriteSlot();
        
        //Start stats measurement event.
        ProcessorStats_->tick();
        
        for (size_t imgNum=0; imgNum<ImagesPerSlot_; ++imgNum)
        {
            Image const * const imRead = *(imvRead[imgNum]);
            Image * const imWrite = *(imvWrite[imgNum]);
            
            float const * const dataRead = (float const * const)imRead->data();
            float * const dataWrite = (float * const)imWrite->data();
            float * const sumImage = sumImageVec_[imgNum];
            float * const oldestHistoryImage = historyImageVecVec_[imgNum][oldestHistorySlot_];
            
            const ImageFormat imFormat=getUpstreamFormat(imgNum);//Downstream format is same as upstream format.
            
            const size_t width=imFormat.getWidth();
            const size_t height=imFormat.getHeight();
            const size_t componentsPerPixel=imFormat.getComponentsPerPixel();
            const size_t componentsPerLine=componentsPerPixel * width;
            
            //Update this slot's average image here...
            int y=0;
            {
                {
                    for (y=0; y<(int)height; ++y)
                    {
                        const size_t lineOffset=y * componentsPerLine;
                        
                        for (size_t compNum=0; compNum<componentsPerLine; ++compNum)
                        {
                            sumImage[lineOffset + compNum]+=dataRead[lineOffset + compNum];
                            sumImage[lineOffset + compNum]-=oldestHistoryImage[lineOffset + compNum];
                            oldestHistoryImage[lineOffset + compNum]=dataRead[lineOffset + compNum];
                            
                            dataWrite[lineOffset + compNum]=sumImage[lineOffset + compNum] * recipWindowLength_;
                        }
                    }
                }
            }
        }
        
        oldestHistorySlot_=(oldestHistorySlot_+1) % windowLength_;
        
        //Stop stats measurement event.
        ProcessorStats_->tock();
        
        releaseWriteSlot();
        releaseReadSlot();
        
        return true;
    }
    
    return false;
}

