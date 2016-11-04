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

#include <flitr/modules/flitr_image_processors/unsharp_mask/fip_unsharp_mask.h>


using namespace flitr;
using std::shared_ptr;

FIPUnsharpMask::FIPUnsharpMask(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                               const float gain,
                               const float filterRadius,
                               uint32_t buffer_size) :
ImageProcessor(upStreamProducer, images_per_slot, buffer_size),
gain_(gain),
gaussianFilter_(filterRadius, int(ceilf(filterRadius*2.0f+0.5)+0.5)*2 - 1)//Filter size includes 2xradius to each side.
{

    ProcessorStats_->setID("ImageProcessor::FIPUnsharpMask");
    //Setup image format being produced to downstream.
    for (uint32_t i=0; i<images_per_slot; i++) {
        ImageFormat downStreamFormat=upStreamProducer.getFormat();
        
        ImageFormat_.push_back(downStreamFormat);
    }
    
}

FIPUnsharpMask::~FIPUnsharpMask()
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
    delete [] xFiltData_;
    delete [] filtData_;
}

bool FIPUnsharpMask::init()
{
    bool rValue=ImageProcessor::init();
    //Note: SharedImageBuffer of downstream producer is initialised with storage in ImageProcessor::init.
    
    size_t maxXFiltDataSize=0;
    
    for (uint32_t i=0; i<ImagesPerSlot_; i++)
    {
        const ImageFormat imFormat=getUpstreamFormat(i);
        
        const size_t width=imFormat.getWidth();
        const size_t height=imFormat.getHeight();
        
        const size_t componentsPerPixel=imFormat.getComponentsPerPixel();
        
        const size_t xFiltDataSize = width*height*componentsPerPixel;
        
        if (xFiltDataSize>maxXFiltDataSize)
        {
            maxXFiltDataSize=xFiltDataSize;
        }
    }
    
    //Allocate a buffer big enough for any of the image slots.
    xFiltData_=new float[maxXFiltDataSize];
    filtData_=new float[maxXFiltDataSize];
    
    return rValue;
}

bool FIPUnsharpMask::trigger()
{
    if ((getNumReadSlotsAvailable())&&(getNumWriteSlotsAvailable()))
    {
        std::vector<Image**> imvRead=reserveReadSlot();
        
        std::vector<Image**> imvWrite=reserveWriteSlot();
        
        //Start stats measurement event.
        ProcessorStats_->tick();
        
        for (size_t imgNum=0; imgNum<ImagesPerSlot_; imgNum++)
        {
            Image const * const imReadUS = *(imvRead[imgNum]);
            Image * const imWriteDS = *(imvWrite[imgNum]);

            // Pass the metadata from the read image to the write image.
            // By Default the base implementation will copy the pointer if no custom
            // pass function was set.
            if(PassMetadataFunction_ != nullptr)
            {
                imWriteDS->setMetadata(PassMetadataFunction_(imReadUS->metadata()));
            }
            
            const ImageFormat imFormat=getDownstreamFormat(imgNum);//down stream and up stream formats are the same.
            
            const size_t width=imFormat.getWidth();
            const size_t height=imFormat.getHeight();
            
            if (imFormat.getPixelFormat()==ImageFormat::FLITR_PIX_FMT_Y_F32)
            {
                float const * const dataReadUS=(float const * const)imReadUS->data();
                float * const dataWriteDS=(float * const)imWriteDS->data();
                
                gaussianFilter_.filter(filtData_, dataReadUS, width, height, xFiltData_);
                
                for (size_t y=0; y<height; ++y)
                {
                    const size_t lineOffset=y*width;
                    
                    for (size_t x=0; x<width; ++x)
                    {
                        const float filtValue=filtData_[lineOffset+x];
                        const float inputValue=dataReadUS[lineOffset+x];
                        
                        dataWriteDS[lineOffset+x]=(inputValue-filtValue)*gain_ + inputValue;
                    }
                }
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

void FIPUnsharpMask::setGain(const float gain)
{
    std::lock_guard<std::mutex> scopedLock(triggerMutex_);
    
    gain_ = gain;
}

float FIPUnsharpMask::getGain() const
{
    return gain_;
}

void FIPUnsharpMask::setFilterRadius(const float filterRadius)
{
    std::lock_guard<std::mutex> scopedLock(triggerMutex_);
    
    gaussianFilter_.setFilterRadius(filterRadius);
}

float FIPUnsharpMask::getFilterRadius() const
{
    return gaussianFilter_.getFilterRadius();
}

