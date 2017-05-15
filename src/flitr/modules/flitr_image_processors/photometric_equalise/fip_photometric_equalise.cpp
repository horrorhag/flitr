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

#include <flitr/modules/flitr_image_processors/photometric_equalise/fip_photometric_equalise.h>
//#include <iostream>

using namespace flitr;
using std::shared_ptr;

FIPPhotometricEqualise::FIPPhotometricEqualise(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                                               float targetAverage,
                                               uint32_t buffer_size) :
ImageProcessor(upStreamProducer, images_per_slot, buffer_size),
targetAverage_(targetAverage),
Title_(std::string("Photometric Equalise"))
{

    ProcessorStats_->setID("ImageProcessor::FIPPhotometricEqualise");
    //Setup image format being produced to downstream.
    for (uint32_t i=0; i<images_per_slot; i++) {
        ImageFormat_.push_back(upStreamProducer.getFormat(i));//Output format is same as input format.
    }
    
}

FIPPhotometricEqualise::~FIPPhotometricEqualise()
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
    delete [] lineSumArray_;
}

bool FIPPhotometricEqualise::init()
{
    bool rValue=ImageProcessor::init();
    //Note: SharedImageBuffer of downstream producer is initialised with storage in ImageProcessor::init.
    
    size_t maxHeight=0;
    
    for (uint32_t i=0; i<ImagesPerSlot_; i++)
    {
        const ImageFormat imFormat=getUpstreamFormat(i);//Downstream format is same as upstream format.
        //const ImageFormat::PixelFormat pixelFormat=imFormat.getPixelFormat();
        
        //const size_t width=imFormat.getWidth();
        const size_t height=imFormat.getHeight();
        
        //const size_t componentsPerPixel=imFormat.getComponentsPerPixel();
        //const size_t bytesPerPixel=imFormat.getBytesPerPixel();
        
        if (height>maxHeight)
        {
            maxHeight=height;
        }
    }
    
    lineSumArray_ = new double[maxHeight];
    
    return rValue;
}

bool FIPPhotometricEqualise::trigger()
{
    if ((getNumReadSlotsAvailable())&&(getNumWriteSlotsAvailable()))
    {//There are images to consume and the downstream producer has space to produce.
        std::vector<Image**> imvRead=reserveReadSlot();
        std::vector<Image**> imvWrite=reserveWriteSlot();
        
        ProcessorStats_->tick();
        
        for (size_t imgNum=0; imgNum<ImagesPerSlot_; imgNum++)
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
            //const ImageFormat::PixelFormat pixelFormat=imFormat.getPixelFormat();
            const ImageFormat::DataType pixelDataType=imFormat.getDataType();
            
            const size_t width=imFormat.getWidth();
            const size_t height=imFormat.getHeight();
            const size_t componentsPerLine=imFormat.getComponentsPerPixel() * width;
            
            if (pixelDataType==ImageFormat::DataType::FLITR_PIX_DT_UINT8)
            {
                process((uint8_t * const )imWrite->data(), (uint8_t const * const)imRead->data(),
                        componentsPerLine, height);
            } else
                if (pixelDataType==ImageFormat::DataType::FLITR_PIX_DT_UINT16)
                {
                    process((uint16_t * const )imWrite->data(), (uint16_t const * const)imRead->data(),
                            componentsPerLine, height);
                } else
                    if (pixelDataType==ImageFormat::DataType::FLITR_PIX_DT_FLOAT32)
                    {
                        process((float * const )imWrite->data(), (float const * const)imRead->data(),
                                componentsPerLine, height);
                    }
        }
        
        
        ++frameNumber_;
        
        //Stop stats measurement event.
        ProcessorStats_->tock();
        
        releaseWriteSlot();
        releaseReadSlot();
        
        return true;
    }
    
    return false;
}




FIPLocalPhotometricEqualise::FIPLocalPhotometricEqualise(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                                                         const float targetAverage, const size_t windowSize,
                                                         uint32_t buffer_size) :
ImageProcessor(upStreamProducer, images_per_slot, buffer_size),
targetAverage_(targetAverage),
windowSize_(windowSize|1), //Make sure that the window size is odd.
Title_(std::string("Local Photometric Equalise"))

{
    ProcessorStats_->setID("ImageProcessor::FIPLocalPhotometricEqualise");
    //Setup image format being produced to downstream.
    for (uint32_t i=0; i<images_per_slot; i++) {
        ImageFormat_.push_back(upStreamProducer.getFormat(i));//Output format is same as input format.
    }
    
}

FIPLocalPhotometricEqualise::~FIPLocalPhotometricEqualise()
{}

bool FIPLocalPhotometricEqualise::init()
{
    bool rValue=ImageProcessor::init();
    //Note: SharedImageBuffer of downstream producer is initialised with storage in ImageProcessor::init.
    
    size_t maxDataValues=0;
    
    for (uint32_t i=0; i<ImagesPerSlot_; ++i)
    {
        const ImageFormat imFormat=getUpstreamFormat(i);//Downstream format is same as upstream format.
        
        const size_t width=imFormat.getWidth();
        const size_t height=imFormat.getHeight();
        const size_t dataValues=imFormat.getComponentsPerPixel() * width * height;
        
        if (dataValues > maxDataValues) maxDataValues=dataValues;
    }
    
    integralImageData_=new double[maxDataValues];
    memset(integralImageData_, 0, maxDataValues*sizeof(double));
    
    return rValue;
}

bool FIPLocalPhotometricEqualise::trigger()
{
    if ((getNumReadSlotsAvailable())&&(getNumWriteSlotsAvailable()))
    {//There are images to consume and the downstream producer has space to produce.
        std::vector<Image**> imvRead=reserveReadSlot();
        std::vector<Image**> imvWrite=reserveWriteSlot();
        
        ProcessorStats_->tick();
        
        for (size_t imgNum=0; imgNum<ImagesPerSlot_; imgNum++)
        {
            Image const * const imReadUS = *(imvRead[imgNum]);
            Image * const imWriteDS = *(imvWrite[imgNum]);
            
            const ImageFormat imFormat=getUpstreamFormat(imgNum);//Downstream format is same as upstream format.
            //const ImageFormat::PixelFormat pixelFormat=imFormat.getPixelFormat();
            //const ImageFormat::DataType pixelDataType=imFormat.getDataType();
            
            const size_t width=imFormat.getWidth();
            const size_t height=imFormat.getHeight();

            
            if (imFormat.getPixelFormat()==ImageFormat::FLITR_PIX_FMT_Y_F32)
            {
                float const * const dataReadUS=(float const * const)imReadUS->data();
                float * const dataWriteDS=(float * const)imWriteDS->data();
                
                integralImage_.process(integralImageData_, dataReadUS, width, height);
                this->process(dataWriteDS, dataReadUS, width, height);//Also accesses integralImageData_ member.
            } else
                if (imFormat.getPixelFormat()==ImageFormat::FLITR_PIX_FMT_Y_8)
                {
                    uint8_t const * const dataReadUS=(uint8_t const * const)imReadUS->data();
                    uint8_t * const dataWriteDS=(uint8_t * const)imWriteDS->data();
                    
                    integralImage_.process(integralImageData_, dataReadUS, width, height);
                    this->process(dataWriteDS, dataReadUS, width, height);//Also accesses integralImageData_ member.
                } else
                    if (imFormat.getPixelFormat()==ImageFormat::FLITR_PIX_FMT_RGB_F32)
                    {
                        float const * const dataReadUS=(float const * const)imReadUS->data();
                        float * const dataWriteDS=(float * const)imWriteDS->data();
                        
                        integralImage_.processRGB(integralImageData_, dataReadUS, width, height);
                        this->processRGB(dataWriteDS, dataReadUS, width, height);//Also accesses integralImageData_ member.
                    } else
                        if (imFormat.getPixelFormat()==ImageFormat::FLITR_PIX_FMT_RGB_8)
                        {
                            uint8_t const * const dataReadUS=(uint8_t const * const)imReadUS->data();
                            uint8_t * const dataWriteDS=(uint8_t * const)imWriteDS->data();
                            
                            integralImage_.processRGB(integralImageData_, dataReadUS, width, height);
                            this->processRGB(dataWriteDS, dataReadUS, width, height);//Also accesses integralImageData_ member.
                        }
        }
        
        ++frameNumber_;
        
        //Stop stats measurement event.
        ProcessorStats_->tock();
        
        releaseWriteSlot();
        releaseReadSlot();
        
        return true;
    }
    
    return false;
}

