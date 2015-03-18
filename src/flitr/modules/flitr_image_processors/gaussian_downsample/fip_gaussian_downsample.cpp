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

#include <flitr/modules/flitr_image_processors/gaussian_downsample/fip_gaussian_downsample.h>

using namespace flitr;
using std::shared_ptr;

FIPGaussianDownsample::FIPGaussianDownsample(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                                             const float filterRadius,
                                             const size_t kernelWidth,
                                             uint32_t buffer_size) :
ImageProcessor(upStreamProducer, images_per_slot, buffer_size),
xFiltData_(nullptr),
gaussianDownsample_(filterRadius, kernelWidth)
{
    
    //Setup image format being produced to downstream.
    for (uint32_t i=0; i<images_per_slot; i++) {
        ImageFormat downStreamFormat=upStreamProducer.getFormat();
        
        downStreamFormat.downSampleByTwo();
        
        ImageFormat_.push_back(downStreamFormat);
    }
    
}

FIPGaussianDownsample::~FIPGaussianDownsample()
{
    delete [] xFiltData_;
}


void FIPGaussianDownsample::setFilterRadius(const float filterRadius)
{
    gaussianDownsample_.setFilterRadius(filterRadius);
}

void FIPGaussianDownsample::setKernelWidth(const int kernelWidth)
{
    gaussianDownsample_.setKernelWidth(kernelWidth);
}

bool FIPGaussianDownsample::init()
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
        
        const size_t xFiltDataSize = (width/2+1) * height * componentsPerPixel;
        
        if (xFiltDataSize>maxXFiltDataSize)
        {
            maxXFiltDataSize=xFiltDataSize;
        }
    }
    
    //Allocate a buffer big enough for the biggest of the image slots.
    xFiltData_=new float[maxXFiltDataSize];

    return rValue;
}

bool FIPGaussianDownsample::trigger()
{
    if ((getNumReadSlotsAvailable())&&(getNumWriteSlotsAvailable()))
    {//There are images to consume and the downstream producer has space to produce.
        std::vector<Image**> imvRead=reserveReadSlot();
        std::vector<Image**> imvWrite=reserveWriteSlot();
        
        //Start stats measurement event.
        ProcessorStats_->tick();
        
        for (size_t imgNum=0; imgNum<ImagesPerSlot_; imgNum++)
        {
            Image const * const imReadUS = *(imvRead[imgNum]);
            Image * const imWriteDS = *(imvWrite[imgNum]);
            
            //US and DS pixel formats are the same, but the image sizes are not.
            const ImageFormat imFormatDS=getDownstreamFormat(imgNum);
            const ImageFormat imFormatUS=getUpstreamFormat(imgNum);
            
            const size_t widthUS=imFormatUS.getWidth();
            const size_t heightUS=imFormatUS.getHeight();
            

            if (imFormatDS.getPixelFormat()==ImageFormat::FLITR_PIX_FMT_Y_F32)
            {
                float const * const dataReadUS=(float const * const)imReadUS->data();
                float * const dataWriteDS=(float * const)imWriteDS->data();

                gaussianDownsample_.downsample(dataWriteDS, dataReadUS, widthUS, heightUS, xFiltData_);
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

