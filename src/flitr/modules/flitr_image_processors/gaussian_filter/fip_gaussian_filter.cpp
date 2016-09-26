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

#include <flitr/modules/flitr_image_processors/gaussian_filter/fip_gaussian_filter.h>


using namespace flitr;
using std::shared_ptr;

FIPGaussianFilter::FIPGaussianFilter(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                                     const float filterRadius,
                                     const size_t kernelWidth,
                                     const short approxIterations,
                                     uint32_t buffer_size) :
ImageProcessor(upStreamProducer, images_per_slot, buffer_size),
_approxIterations(approxIterations),
_scratchData(nullptr),
_intImageScratchData(nullptr),
_gaussianFilter(filterRadius, kernelWidth),
_boxFilter(kernelWidth),
_enabled(true),
_title(std::string("Gaussian Filter")),
_filterRadius(filterRadius),
_kernelWidth(kernelWidth)
{
    //Setup image format being produced to downstream.
    for (uint32_t i=0; i<images_per_slot; i++) {
        ImageFormat downStreamFormat=upStreamProducer.getFormat();
        
        ImageFormat_.push_back(downStreamFormat);
    }
}

FIPGaussianFilter::~FIPGaussianFilter()
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
    delete [] _scratchData;
    delete [] _intImageScratchData;
}

void FIPGaussianFilter::setFilterRadius(const float filterRadius)
{
    _gaussianFilter.setFilterRadius(filterRadius);
}

void FIPGaussianFilter::setKernelWidth(const int kernelWidth)
{
    _gaussianFilter.setKernelWidth(kernelWidth);
    _boxFilter.setKernelWidth(kernelWidth);
}

bool FIPGaussianFilter::init()
{
    bool rValue=ImageProcessor::init();
    //Note: SharedImageBuffer of downstream producer is initialised with storage in ImageProcessor::init.
    
    size_t maxScratchDataSize=0;
    size_t maxScratchDataValues=0;
    
    for (uint32_t i=0; i<ImagesPerSlot_; i++)
    {
        const ImageFormat imFormat=getUpstreamFormat(i);
        
        const size_t width=imFormat.getWidth();
        const size_t height=imFormat.getHeight();
        
        const size_t bytesPerPixel=imFormat.getBytesPerPixel();
        const size_t componentsPerPixel=imFormat.getComponentsPerPixel();
        
        const size_t scratchDataSize = width * height * bytesPerPixel;
        const size_t scratchDataValues = width * height * componentsPerPixel;
        
        if (scratchDataSize>maxScratchDataSize)
        {
            maxScratchDataSize=scratchDataSize;
        }
        
        if (scratchDataValues>maxScratchDataValues)
        {
            maxScratchDataValues=scratchDataValues;
        }
    }
    
    //Allocate a buffer big enough for any of the image slots.
    _scratchData=new uint8_t[maxScratchDataSize];
    memset(_scratchData, 0, maxScratchDataSize);
    
    _intImageScratchData=new double[maxScratchDataValues];
    memset(_intImageScratchData, 0, maxScratchDataValues*sizeof(double));
    
    return rValue;
}

bool FIPGaussianFilter::trigger()
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

            // Pass the metadata from the read image to the write image.
            // By Default the base implementation will copy the pointer if no custom
            // pass function was set.
            if(PassMetadataFunction_ != nullptr)
            {
                imWriteDS->setMetadata(PassMetadataFunction_(imReadUS->metadata()));
            }
            const ImageFormat imFormat=getDownstreamFormat(imgNum);//down stream and up stream formats are the same.
            
            if (!_enabled)
            {
                uint8_t const * const dataReadUS=(uint8_t const * const)imReadUS->data();
                uint8_t * const dataWriteDS=(uint8_t * const)imWriteDS->data();
                const size_t bytesPerImage=imFormat.getBytesPerImage();
                memcpy(dataWriteDS, dataReadUS, bytesPerImage);
            } else
            {
                const size_t width=imFormat.getWidth();
                const size_t height=imFormat.getHeight();
                
                
                if (imFormat.getPixelFormat()==ImageFormat::FLITR_PIX_FMT_Y_F32)
                {
                    float const * const dataReadUS=(float const * const)imReadUS->data();
                    float * const dataWriteDS=(float * const)imWriteDS->data();
                    
                    if (_approxIterations==0)
                    {
                        _gaussianFilter.filter(dataWriteDS, dataReadUS, width, height, (float *)_scratchData);
                    } else
                    {
#ifdef APPROX_GAUSS_FILT_USE_INTEGRAL_IMAGES
                        _boxFilter.filter(dataWriteDS, dataReadUS, width, height, _intImageScratchData, true);
#else
                        _boxFilter.filter(dataWriteDS, dataReadUS, width, height, (float *)_scratchData);
#endif
                        
                        for (short i=1; i<_approxIterations; ++i)
                        {
                            memcpy(_scratchData, dataWriteDS, width*height*sizeof(float));
                            
#ifdef APPROX_GAUSS_FILT_USE_INTEGRAL_IMAGES
                            _boxFilter.filter(dataWriteDS, (float *)_scratchData, width, height, _intImageScratchData, true);
#else
                            _boxFilter.filter(dataWriteDS, (float *)_scratchData, width, height, (float *)_scratchData);
#endif
                        }
                    }
                } else
                    if (imFormat.getPixelFormat()==ImageFormat::FLITR_PIX_FMT_Y_8)
                    {
                        uint8_t const * const dataReadUS=(uint8_t const * const)imReadUS->data();
                        uint8_t * const dataWriteDS=(uint8_t * const)imWriteDS->data();
                        
                        if (_approxIterations==0)
                        {
                            _gaussianFilter.filter(dataWriteDS, dataReadUS, width, height, (uint8_t *)_scratchData);
                        } else
                        {
#ifdef APPROX_GAUSS_FILT_USE_INTEGRAL_IMAGES
                            _boxFilter.filter(dataWriteDS, dataReadUS, width, height, _intImageScratchData, true);
#else
                            _boxFilter.filter(dataWriteDS, dataReadUS, width, height, (uint8_t *)_scratchData);
#endif
                            
                            for (short i=1; i<_approxIterations; ++i)
                            {
                                memcpy(_scratchData, dataWriteDS, width*height*sizeof(uint8_t));
                                
#ifdef APPROX_GAUSS_FILT_USE_INTEGRAL_IMAGES
                                _boxFilter.filter(dataWriteDS, dataReadUS, width, height, _intImageScratchData, true);
#else
                                _boxFilter.filter(dataWriteDS, (uint8_t *)_scratchData, width, height, (uint8_t *)_scratchData);
#endif
                            }
                        }
                    } else
                        if (imFormat.getPixelFormat()==ImageFormat::FLITR_PIX_FMT_RGB_F32)
                        {
                            float const * const dataReadUS=(float const * const)imReadUS->data();
                            float * const dataWriteDS=(float * const)imWriteDS->data();
                            
                            if (_approxIterations==0)
                            {
                                _gaussianFilter.filterRGB(dataWriteDS, dataReadUS, width, height, (float *)_scratchData);
                            } else
                            {
#ifdef APPROX_GAUSS_FILT_USE_INTEGRAL_IMAGES
                                _boxFilter.filterRGB(dataWriteDS, dataReadUS, width, height, _intImageScratchData, true);
#else
                                _boxFilter.filterRGB(dataWriteDS, dataReadUS, width, height, (float *)_scratchData);
#endif
                                
                                for (short i=1; i<_approxIterations; ++i)
                                {
                                    memcpy(_scratchData, dataWriteDS, width*height*sizeof(float)*3);
                                    
#ifdef APPROX_GAUSS_FILT_USE_INTEGRAL_IMAGES
                                    _boxFilter.filterRGB(dataWriteDS, (float *)_scratchData, width, height, _intImageScratchData, true);
#else
                                    _boxFilter.filterRGB(dataWriteDS, (float *)_scratchData, width, height, (float *)_scratchData);
#endif
                                }
                            }
                        } else
                            if (imFormat.getPixelFormat()==ImageFormat::FLITR_PIX_FMT_RGB_8)
                            {
                                uint8_t const * const dataReadUS=(uint8_t const * const)imReadUS->data();
                                uint8_t * const dataWriteDS=(uint8_t * const)imWriteDS->data();
                                
                                if (_approxIterations==0)
                                {
                                    _gaussianFilter.filterRGB(dataWriteDS, dataReadUS, width, height, (uint8_t *)_scratchData);
                                } else
                                {
#ifdef APPROX_GAUSS_FILT_USE_INTEGRAL_IMAGES
                                    _boxFilter.filterRGB(dataWriteDS, dataReadUS, width, height, _intImageScratchData, true);
#else
                                    _boxFilter.filterRGB(dataWriteDS, dataReadUS, width, height, (uint8_t *)_scratchData);
#endif
                                    
                                    for (short i=1; i<_approxIterations; ++i)
                                    {
                                        memcpy(_scratchData, dataWriteDS, width*height*sizeof(uint8_t)*3);
                                        
#ifdef APPROX_GAUSS_FILT_USE_INTEGRAL_IMAGES
                                        _boxFilter.filterRGB(dataWriteDS, _scratchData, width, height, _intImageScratchData, true);
#else
                                        _boxFilter.filterRGB(dataWriteDS, _scratchData, width, height, (uint8_t *)_scratchData);
#endif
                                    }
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

