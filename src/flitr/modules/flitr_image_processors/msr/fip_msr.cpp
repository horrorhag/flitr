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

#include <flitr/modules/flitr_image_processors/msr/fip_msr.h>
#include <iostream>
#include <algorithm>

using namespace flitr;
using std::shared_ptr;

FIPMSR::FIPMSR(ImageProducer& upStreamProducer, uint32_t images_per_slot,
               uint32_t buffer_size) :
ImageProcessor(upStreamProducer, images_per_slot, buffer_size),
#ifdef MSR_USE_GFXY
_GFXY(1.0, 4),
#endif
#ifdef MSR_USE_BFII
_GFII(1),
#endif
#ifdef MSR_USE_BFRS
_GFRS(1),
#endif
_GFScale(20),
_numScales(1),
_intensityScratchData(nullptr),
_GFScratchData(nullptr),
_MSRScratchData(nullptr),
_floatScratchData(nullptr),
_doubleScratchData1(nullptr),
_doubleScratchData2(nullptr),
_histoBins(nullptr),
_triggerCount(0)
{
    //Setup image format being produced to downstream.
    for (uint32_t i=0; i<images_per_slot; i++) {
        //ImageFormat(uint32_t w=0, uint32_t h=0, PixelFormat pix_fmt=FLITR_PIX_FMT_Y_8, bool flipV = false, bool flipH = false):
        ImageFormat downStreamFormat(upStreamProducer.getFormat().getWidth(), upStreamProducer.getFormat().getHeight(),
                                     upStreamProducer.getFormat().getPixelFormat());
        
        ImageFormat_.push_back(downStreamFormat);
    }
    
}

FIPMSR::~FIPMSR()
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
    delete [] _intensityScratchData;
    delete [] _GFScratchData;
    delete [] _MSRScratchData;
    delete [] _floatScratchData;
    delete [] _doubleScratchData1;
    delete [] _doubleScratchData2;
    delete [] _histoBins;
}

bool FIPMSR::init()
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
    
    _intensityScratchData=new float[maxWidth*maxHeight];
    memset(_intensityScratchData, 0, maxWidth*maxHeight*sizeof(float));
    
    _GFScratchData=new float[maxWidth*maxHeight];
    memset(_GFScratchData, 0, maxWidth*maxHeight*sizeof(float));
    
    _MSRScratchData=new float[maxWidth*maxHeight];
    memset(_MSRScratchData, 0, maxWidth*maxHeight*sizeof(float));
    
    _floatScratchData=new float[maxWidth*maxHeight];
    memset(_floatScratchData, 0, maxWidth*maxHeight*sizeof(float));
    
    _doubleScratchData1=new double[maxWidth*maxHeight];
    memset(_doubleScratchData1, 0, maxWidth*maxHeight*sizeof(double));
    
    _doubleScratchData2=new double[maxWidth*maxHeight];
    memset(_doubleScratchData2, 0, maxWidth*maxHeight*sizeof(double));
    
    _histoBins=new size_t[_histoBinArrSize];
    
    return rValue;
}

bool FIPMSR::trigger()
{
    if ((getNumReadSlotsAvailable())&&(getNumWriteSlotsAvailable()))
    {//There are images to consume and the downstream producer has space to produce.
        std::vector<Image**> imvRead=reserveReadSlot();
        std::vector<Image**> imvWrite=reserveWriteSlot();
        
        ++_triggerCount;
        
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
            
            float const * const dataRead=(float *)imRead->data();
            
            float * const dataWrite=(float *)imWrite->data();
            
            const ImageFormat imFormatUS=getUpstreamFormat(imgNum);
            
            const size_t width=imFormatUS.getWidth();
            const size_t height=imFormatUS.getHeight();
            
            float const * F32Image=nullptr;
            
            //=== Get intensity of input ==//
            if (imFormatUS.getPixelFormat()==ImageFormat::FLITR_PIX_FMT_Y_F32)
            {//Input image already Y_F32. Just use its pointer.
                F32Image=dataRead;
            } else
            {//Convert input image to Y_F32 and store in pre-allocated F32Image.
                // #parallel
                for (size_t y=0; y<height; ++y)
                {
                    size_t readOffset=y*width*3;
                    size_t writeOffset=y*width;
                    
                    for (size_t x=0; x<width; ++x)
                    {
                        _intensityScratchData[writeOffset]=(dataRead[readOffset+0] + dataRead[readOffset+1] + dataRead[readOffset+2])*(1.0f/3.0f);
                        readOffset+=3;
                        ++writeOffset;
                    }
                }
                
                F32Image=_intensityScratchData;
            }
            //=== ===//
            
            
            
            memset(_MSRScratchData, 0, width*height*sizeof(float));
            
            const float recipNumScales=1.0f/_numScales;
            
            for (size_t scaleIndex=0; scaleIndex<_numScales; ++scaleIndex)
            {
                const size_t kernelWidth=(imFormatUS.getWidth() / (_GFScale*(1 << scaleIndex))) | 1; // | 1 to make sure kernelWidth is odd.
                
#ifdef MSR_USE_GFXY
                _GFXY.setKernelWidth(kernelWidth);
                _GFXY.setFilterRadius(kernelWidth*0.25f);
                
                // #parallel
                _GFXY.filter(_GFScratchData, F32Image, width, height, _floatScratchData);
#endif
#ifdef MSR_USE_BFII
                _GFII.setKernelWidth(kernelWidth);
                
                // #parallel
                _GFII.filter(_GFScratchData, F32Image, width, height,
                                          _doubleScratchData1,
                           scaleIndex==0 ? true : false);
                
                //Approximate Gaussian filt kernel...
                for (int i=0; i<2; ++i)
                {
                    // #parallel?
                    memcpy(_floatScratchData, _GFScratchData, width*height*sizeof(float));
                    
                    // #parallel
                    _GFII.filter(_GFScratchData, _floatScratchData, width, height,
                                              _doubleScratchData2, true);
                }
#endif
#ifdef MSR_USE_BFRS
                _GFRS.setKernelWidth(kernelWidth);
                
                // #parallel
                _GFRS.filter(_GFScratchData, F32Image, width, height, _floatScratchData);
                
                //Approximate Gaussian filt kernel...
                for (int i=0; i<2; ++i)
                {
                    // #parallel?
                    memcpy(_floatScratchData, _GFScratchData, width*height*sizeof(float));
                    
                    // #parallel
                    _GFRS.filter(_GFScratchData, _floatScratchData, width, height, _floatScratchData);
                }
#endif
                
                //Calc SSR image...
                for (size_t y=0; y<height; ++y)
                {
                    size_t offset=y*width;
                    
                    for (size_t x=0; x<width; ++x)
                    {
                        //const float r=(log10f(F32Image[offset]+1.0f) - log10f(GFScratchData_[offset]+1.0f));//+1.0f in case the intensities are zero.
                        const float r=(F32Image[offset] - _GFScratchData[offset]);
                        //Note: The log version (true retinex) seems to not have a big impact AND it is slower.
                        //      The gain and chromatGain below may be used to tweak the image.
                        
                        _floatScratchData[offset]=r * 1.0f;
                        
                        ++offset;
                    }
                }

#ifdef SR_LOCAL_CONTRAST
                //=== Update MSR with SSR scaled using local min/max : Local DC level has very little impact; SLOW!===//
                _II.process(_doubleScratchData2, _floatScratchData, width, height);
                
                const size_t halfKernelWidth=kernelWidth >> 1;

                for (size_t y=halfKernelWidth; y<(height-halfKernelWidth); ++y)
                {
                    const size_t offset=y*width;
                    
                    for (size_t x=halfKernelWidth; x<(width-halfKernelWidth); ++x)
                    {
                        float rmin=std::numeric_limits<float>::infinity();
                        float rmax=-std::numeric_limits<float>::infinity();
                        
                        for (int ly=-int(halfKernelWidth); ly<=int(halfKernelWidth); ++ly)
                        {
                            size_t offset=(y+ly)*width + x;
                            
                            for (int lx=-int(halfKernelWidth); lx<=int(halfKernelWidth); ++lx)
                            {
                                const float r=_floatScratchData[offset+lx];
                                
                                if (r<rmin) rmin=r;
                                if (r>rmax) rmax=r;
                            }
                        }
                        
                        const float recipRange=1.0f/(rmax - rmin);

                        const float r=(_floatScratchData[offset+x]-rmin) * (recipRange * recipNumScales);
                        _MSRScratchData[offset+x]+=r;
                    }
                }
                //=====================================//
#else                
                //=== Update MSR with global min/max minus outliers : MUCH faster than local min/max window; Global min/max means filter is not strictly local, but results still very good ===//
                float rmin=-1.0f;
                float rmax=1.0f;
                
                size_t numHistoSamples=0;
                memset(_histoBins, 0, _histoBinArrSize*sizeof(size_t));
                
                for (size_t y=height/4; y<(height*3)/4; ++y)
                {
                    const size_t offset=y*width;
                    
                    for (size_t x=width/4; x<(width*3/4); ++x)
                    {
                        const float r=_floatScratchData[offset+x];
                        
                        const int histoBinNum=int(((r + 1.0f)*0.5f) * (_histoBinArrSize-1) + 0.5f);
                        
                        if ((histoBinNum>=0) && (histoBinNum<_histoBinArrSize))
                        {
                            _histoBins[histoBinNum]=_histoBins[histoBinNum]+1;
                        }
                        
                        ++numHistoSamples;
                    }
                }
                 
                //Remove outliers.
                size_t lowerToRemove=numHistoSamples*0.0001f;
                size_t lowerRemoved=0;
                
                for (int binNum=0; binNum<_histoBinArrSize; ++binNum)
                {
                    const size_t removedFromThisBin=std::min(_histoBins[binNum], lowerToRemove - lowerRemoved);
                    lowerRemoved+=removedFromThisBin;
                    _histoBins[binNum]=0;
                    
                    if (lowerRemoved>=lowerToRemove) break;
                }

                size_t upperToRemove=numHistoSamples*0.0005f;
                size_t upperRemoved=0;
                
                for (int binNum=_histoBinArrSize-1; binNum>=0; --binNum)
                {
                    const size_t removedFromThisBin=std::min(_histoBins[binNum], upperToRemove - upperRemoved);
                    upperRemoved+=removedFromThisBin;
                    _histoBins[binNum]=0;
                    
                    if (upperRemoved>=upperToRemove) break;
                }
                
                
                //Find min/max from histoBins_.
                for (int binNum=0; binNum<_histoBinArrSize; ++binNum)
                {
                    if (_histoBins[binNum])
                    {
                        rmin=(binNum/float(_histoBinArrSize))*2.0f-1.0f;
                        break;
                    }
                }
                for (int binNum=_histoBinArrSize-1; binNum>=0; --binNum)
                {
                    if (_histoBins[binNum])
                    {
                        rmax=((binNum+1)/float(_histoBinArrSize))*2.0f-1.0f;
                        break;
                    }
                }
                
                
                const float recipRange=1.0f/(rmax - rmin);
                
                //Update MSR image...
                for (size_t y=0; y<height; ++y)
                {
                    const size_t offset=y*width;
                    
                    for (size_t x=0; x<width; ++x)
                    {
                        const float r=(_floatScratchData[offset+x]-rmin) * (recipRange * recipNumScales);
                        _MSRScratchData[offset+x]+=r;
                    }
                }
                 //=====================================================//
#endif
            }
            
            //MSR cont.
            const float gain=0.9f;//Boosts image intensity.
            const float chromatGain=1.0f;//Boosts colour.
            const float blacknessFloor=1.0f/255.0f;//Limits the enhancement of low signal (black) areas.
            
            if (imFormatUS.getPixelFormat()==ImageFormat::FLITR_PIX_FMT_RGB_F32)
            {
                for (size_t y=0; y<height; ++y)
                {
                    size_t intensityOffset=y*width;
                    size_t colourOffset=y*width*3;
                    
                    for (size_t x=0; x<width; ++x)
                    {
                        const float r=_MSRScratchData[intensityOffset] * gain;
                        const float intInput=F32Image[intensityOffset];
                        
                        const float recipIntInput=1.0f/(intInput+blacknessFloor);//Bias very dark colours more towards black...
                        
                        dataWrite[colourOffset+0]=r * (dataRead[colourOffset+0]*recipIntInput) + (chromatGain) * (dataRead[colourOffset+0]-intInput);
                        dataWrite[colourOffset+1]=r * (dataRead[colourOffset+1]*recipIntInput) + (chromatGain) * (dataRead[colourOffset+1]-intInput);
                        dataWrite[colourOffset+2]=r * (dataRead[colourOffset+2]*recipIntInput) + (chromatGain) * (dataRead[colourOffset+2]-intInput);
                        
                        ++intensityOffset;
                        colourOffset+=3;
                    }
                }
            } else
            {
                for (size_t y=0; y<height; ++y)
                {
                    const size_t offset=y*width;
                    
                    for (size_t x=0; x<width; ++x)
                    {
                        const float r=_MSRScratchData[offset+x] * gain;
                        const float intInput=F32Image[offset+x];
                        
                        dataWrite[offset+x]=r * (intInput/(intInput+blacknessFloor));//Bias very dark colours more towards black...
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

