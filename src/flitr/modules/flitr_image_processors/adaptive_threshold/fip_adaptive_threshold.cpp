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

#include <flitr/modules/flitr_image_processors/adaptive_threshold/fip_adaptive_threshold.h>


using namespace flitr;
using std::shared_ptr;

FIPAdaptiveThreshold::FIPAdaptiveThreshold(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                                           const int kernelWidth,
                                           const short numIntegralImageLevels,
                                           const float thresholdOffset,
                                           uint32_t buffer_size) :
ImageProcessor(upStreamProducer, images_per_slot, buffer_size),
_numIntegralImageLevels(numIntegralImageLevels),
_noiseFilter(7),
_noiseFilteredInputData(nullptr),
_scratchData(nullptr),
_integralImageScratchData(nullptr),
_boxFilter(kernelWidth),
_enabled(true),
_title(std::string("Adaptive Threshold Filter")),
_kernelWidth(kernelWidth),
_thresholdOffset(thresholdOffset),
_thresholdAvrg(0.0)
{
    //Setup image format being produced to downstream.
    for (uint32_t i=0; i<images_per_slot; i++) {
        ImageFormat downStreamFormat=upStreamProducer.getFormat();
        
        ImageFormat_.push_back(downStreamFormat);
    }
}

FIPAdaptiveThreshold::~FIPAdaptiveThreshold()
{
    delete [] _noiseFilteredInputData;
    delete [] _scratchData;
    delete [] _integralImageScratchData;
}


void FIPAdaptiveThreshold::setKernelWidth(const int kernelWidth)
{
    _boxFilter.setKernelWidth(kernelWidth);
    _kernelWidth=kernelWidth;
}

bool FIPAdaptiveThreshold::init()
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
    _noiseFilteredInputData=new uint8_t[maxScratchDataSize];
    _scratchData=new uint8_t[maxScratchDataSize];
    memset(_noiseFilteredInputData, 0, maxScratchDataSize);
    memset(_scratchData, 0, maxScratchDataSize);
    
    _integralImageScratchData=new double[maxScratchDataValues];
    memset(_integralImageScratchData, 0, maxScratchDataValues*sizeof(double));
    
    return rValue;
}

bool FIPAdaptiveThreshold::trigger()
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
                const size_t numElements=width * height * imFormat.getComponentsPerPixel();
                
                if (imFormat.getPixelFormat()==ImageFormat::FLITR_PIX_FMT_Y_F32)
                {
                    float const * const dataReadUS=(float const * const)imReadUS->data();
                    float * const dataWriteDS=(float * const)imWriteDS->data();
                    
                    //Small kernel noise filter.
                    _noiseFilter.filter((float *)_noiseFilteredInputData, dataReadUS, width, height,
                                        _integralImageScratchData, true);
                    
                    for (short i=1; i<_numIntegralImageLevels; ++i)
                    {
                        memcpy(_scratchData, _noiseFilteredInputData, width*height*sizeof(uint8_t));
                        
                        _noiseFilter.filter((float *)_noiseFilteredInputData, (float *)_scratchData, width, height,
                                            _integralImageScratchData, true);
                    }
                    
                    
                    for (size_t i=0; i<numElements; ++i)
                    {
                        dataWriteDS[i]=1.0;
                    }
                    
                    
                    //Large kernel adaptive reference.
                    _boxFilter.filter(dataWriteDS, dataReadUS, width, height,
                                      _integralImageScratchData, true);
                    
                    for (short i=1; i<_numIntegralImageLevels; ++i)
                    {
                        memcpy(_scratchData, dataWriteDS, width*height*sizeof(float));
                        
                        _boxFilter.filter(dataWriteDS, (float *)_scratchData, width, height,
                                          _integralImageScratchData, true);
                    }
                    
                    
                    const float tovF32=_thresholdOffset * 1.0f;
                    size_t tpc=0;
                    
                    for (size_t i=0; i<numElements; ++i)
                    {
                        if (( ((float *)_noiseFilteredInputData)[i] - tovF32) > dataWriteDS[i])
                        {
                            dataWriteDS[i]=1.0;
                            ++tpc;
                        } else
                        {
                            dataWriteDS[i]=0.0;
                        }
                    }
                    
                    _thresholdAvrg=tpc;// / double(width*height);
                } else
                    if (imFormat.getPixelFormat()==ImageFormat::FLITR_PIX_FMT_Y_8)
                    {
                        uint8_t const * const dataReadUS=(uint8_t const * const)imReadUS->data();
                        uint8_t * const dataWriteDS=(uint8_t * const)imWriteDS->data();
                        
                        //Small kernel noise filter.
                        _noiseFilter.filter((uint8_t *)_noiseFilteredInputData, dataReadUS, width, height,
                                            _integralImageScratchData, true);
                        
                        for (short i=1; i<_numIntegralImageLevels; ++i)
                        {
                            memcpy(_scratchData, _noiseFilteredInputData, width*height*sizeof(uint8_t));
                            
                            _noiseFilter.filter((uint8_t *)_noiseFilteredInputData, (uint8_t *)_scratchData, width, height,
                                                _integralImageScratchData, true);
                        }
                        
                        
                        for (size_t i=0; i<numElements; ++i)
                        {
                            dataWriteDS[i]=255;
                        }
                        
                        
                        //Large kernel adaptive reference.
                        _boxFilter.filter(dataWriteDS, dataReadUS, width, height,
                                          _integralImageScratchData, true);
                        
                        for (short i=1; i<_numIntegralImageLevels; ++i)
                        {
                            memcpy(_scratchData, dataWriteDS, width*height*sizeof(uint8_t));
                            
                            _boxFilter.filter(dataWriteDS, (uint8_t *)_scratchData, width, height,
                                              _integralImageScratchData, true);
                        }
                        
                        
                        const uint8_t tovUInt8=_thresholdOffset * 255.5;
                        size_t tpc=0;
                        
                        for (size_t i=0; i<numElements; ++i)
                        {
                            if (( ((uint8_t *)_noiseFilteredInputData)[i] - tovUInt8) > dataWriteDS[i])
                            {
                                dataWriteDS[i]=255;
                                ++tpc;
                            } else
                            {
                                dataWriteDS[i]=0;
                            }
                        }
                        
                        _thresholdAvrg=tpc / double(width*height);
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

