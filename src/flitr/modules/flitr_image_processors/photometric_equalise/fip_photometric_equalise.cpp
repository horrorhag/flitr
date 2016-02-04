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
targetAverage_(targetAverage)
{
    
    //Setup image format being produced to downstream.
    for (uint32_t i=0; i<images_per_slot; i++) {
        ImageFormat_.push_back(upStreamProducer.getFormat(i));//Output format is same as input format.
    }
    
}

FIPPhotometricEqualise::~FIPPhotometricEqualise()
{
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
            
            const ImageFormat imFormat=getUpstreamFormat(imgNum);//Downstream format is same as upstream format.
            const ImageFormat::PixelFormat pixelFormat=imFormat.getPixelFormat();
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
windowSize_(windowSize|1)//Make sure that the window size is odd.
{
    
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
    
    size_t maxWidth=0;
    size_t maxHeight=0;
    
    for (uint32_t i=0; i<ImagesPerSlot_; ++i)
    {
        const ImageFormat imFormat=getUpstreamFormat(i);//Downstream format is same as upstream format.
        
        const size_t width=imFormat.getWidth();
        const size_t height=imFormat.getHeight();
        
        if (width>maxWidth) maxWidth=width;
        if (height>maxHeight) maxHeight=height;
    }
    
    integralImageData_=new double[maxWidth*maxHeight];
    memset(integralImageData_, 0, maxWidth*maxHeight*sizeof(double));
    
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
            Image const * const imRead = *(imvRead[imgNum]);
            Image * const imWrite = *(imvWrite[imgNum]);
            
            const ImageFormat imFormat=getUpstreamFormat(imgNum);//Downstream format is same as upstream format.
            const ImageFormat::PixelFormat pixelFormat=imFormat.getPixelFormat();
            const ImageFormat::DataType pixelDataType=imFormat.getDataType();
            
            const size_t width=imFormat.getWidth();
            const size_t height=imFormat.getHeight();
            const size_t halfWindowSize=windowSize_>>1;
            const size_t widthMinusWindowSize=width - windowSize_;
            const size_t heightMinusWindowSize=height - windowSize_;
            const double recipWindowSizeSquared=1.0f / float(windowSize_*windowSize_);
            
            
            uint8_t * const dataWrite=(uint8_t * const )imWrite->data();
            uint8_t const * const dataRead=(uint8_t const * const)imRead->data();

            //Calculate the integral image.
            integralImage_.process(integralImageData_, dataRead, width, height);
            
            for (size_t y=1; y<heightMinusWindowSize; ++y)
            {
                const size_t lineOffsetUS=y * width;
                const size_t lineOffsetDS=(y+halfWindowSize) * width + halfWindowSize;
                
                for (size_t x=1; x<widthMinusWindowSize; ++x)
                {
                    const double windowSum=integralImageData_[(lineOffsetUS+x) + (windowSize_-1) + (windowSize_-1)*width]
                                        - integralImageData_[(lineOffsetUS+x) + (windowSize_-1) - width]
                                        - integralImageData_[(lineOffsetUS+x) - 1 + (windowSize_-1)*width]
                                        + integralImageData_[(lineOffsetUS+x) - 1 - width];
                    
                    const float windowAvrg=float(windowSum * recipWindowSizeSquared);
                    
                    const float eScale=targetAverage_ / windowAvrg;
                    
                    dataWrite[lineOffsetDS + x]=uint8_t(dataRead[lineOffsetDS + x] * eScale + 0.5f);
                }
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

