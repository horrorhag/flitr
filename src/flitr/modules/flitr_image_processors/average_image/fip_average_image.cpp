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
using std::shared_ptr;

FIPAverageImage::FIPAverageImage(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                                 uint8_t base2WindowLength,
                                 uint32_t buffer_size) :
ImageProcessor(upStreamProducer, images_per_slot, buffer_size),
base2WindowLength_(base2WindowLength),
windowLength_((uint32_t)(powf(2.0f, base2WindowLength_)+0.5f)),
recipWindowLength_(1.0f/((float)windowLength_)),
Title_(std::string("Average Image")),
oldestHistorySlot_(0)
{
    ProcessorStats_->setID("ImageProcessor::FIPAverageImage");
    //Setup image format being produced to downstream.
    for (uint32_t i=0; i<images_per_slot; i++) {
        ImageFormat_.push_back(upStreamProducer.getFormat(i));//Output format is same as input format.
    }
    
}

FIPAverageImage::~FIPAverageImage()
{
    // First stop the trigger thread. The stopTriggerThread() function will
    // also wait for the thread to stop using the join() function.
    // It is essential to wait for the thread to exit before starting
    // to clean up otherwise if the thread is still in the trigger() function
    // and cleaning up starts, the application will crash.
    // If the user called stopTriggerThread() manually, this call will do
    // nothing. stopTriggerThread() will get called in the base destructor, but
    // at that time it might be too late.
    stopTriggerThread();
    // Thread should be done, cleaning up can start. This might still be a problem
    // if the application calls trigger() and not the triggerThread.
    for (uint32_t i=0; i<ImagesPerSlot_; ++i)
    {
        delete [] sumImageVec_[i];
        
        for (size_t historyIndex=0; historyIndex<windowLength_; ++historyIndex)
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
    
    for (uint32_t i=0; i<ImagesPerSlot_; ++i)
    {
        const ImageFormat imFormat=getUpstreamFormat(i);//Downstream format is same as upstream format.
        
        const size_t width=imFormat.getWidth();
        const size_t height=imFormat.getHeight();
        
        const size_t componentsPerPixel=imFormat.getComponentsPerPixel();
        
        sumImageVec_.push_back(new float[width*height*componentsPerPixel]);
        memset(sumImageVec_[i], 0, width*height*componentsPerPixel*sizeof(float));
        
        historyImageVecVec_.push_back(std::vector<float *>());
        for (size_t historyIndex=0; historyIndex<windowLength_; ++historyIndex)
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

            // Pass the metadata from the read image to the write image.
            // By Default the base implementation will copy the pointer if no custom
            // pass function was set.
            if(PassMetadataFunction_ != nullptr)
            {
                imWrite->setMetadata(PassMetadataFunction_(imRead->metadata()));
            }
            
            const ImageFormat imFormat=getUpstreamFormat(imgNum);//Downstream format is same as upstream format.
            
            const size_t width=imFormat.getWidth();
            const size_t height=imFormat.getHeight();
            
            //Update this slot's average image here...
            if (imFormat.getPixelFormat()==ImageFormat::FLITR_PIX_FMT_Y_F32)
            {
                float const * const dataRead = (float const * const)imRead->data();
                float * const dataWrite = (float * const)imWrite->data();
                float * const sumImage = sumImageVec_[imgNum];
                float * const oldestHistoryImage = historyImageVecVec_[imgNum][oldestHistorySlot_];
                
                for (size_t y=0; y<height; ++y)
                {
                    const size_t lineOffset=y * width;
                    
                    for (size_t x=0; x<width; ++x)
                    {
                        sumImage[lineOffset + x]+=dataRead[lineOffset + x];
                        sumImage[lineOffset + x]-=oldestHistoryImage[lineOffset + x];
                        oldestHistoryImage[lineOffset + x]=dataRead[lineOffset + x];
                        dataWrite[lineOffset + x]=sumImage[lineOffset + x] * recipWindowLength_;
                    }
                }
            } else
                if (imFormat.getPixelFormat()==ImageFormat::FLITR_PIX_FMT_RGB_F32)
                {
                    float const * const dataRead = (float const * const)imRead->data();
                    float * const dataWrite = (float * const)imWrite->data();
                    float * const sumImage = sumImageVec_[imgNum];
                    float * const oldestHistoryImage = historyImageVecVec_[imgNum][oldestHistorySlot_];
                    
                    for (size_t y=0; y<height; ++y)
                    {
                        size_t offset=y * width*3;
                        
                        for (size_t x=0; x<width; ++x)
                        {
                            sumImage[offset + 0]+=dataRead[offset + 0];
                            sumImage[offset + 0]-=oldestHistoryImage[offset + 0];
                            oldestHistoryImage[offset + 0]=dataRead[offset + 0];
                            dataWrite[offset + 0]=sumImage[offset + 0] * recipWindowLength_;
                            
                            sumImage[offset + 1]+=dataRead[offset + 1];
                            sumImage[offset + 1]-=oldestHistoryImage[offset + 1];
                            oldestHistoryImage[offset + 1]=dataRead[offset + 1];
                            dataWrite[offset + 1]=sumImage[offset + 1] * recipWindowLength_;
                            
                            sumImage[offset + 2]+=dataRead[offset + 2];
                            sumImage[offset + 2]-=oldestHistoryImage[offset + 2];
                            oldestHistoryImage[offset + 2]=dataRead[offset + 2];
                            dataWrite[offset + 2]=sumImage[offset + 2] * recipWindowLength_;
                            
                            offset+=3;
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

