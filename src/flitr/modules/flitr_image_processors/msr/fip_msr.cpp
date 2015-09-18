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

using namespace flitr;
using std::shared_ptr;

FIPMSR::FIPMSR(ImageProducer& upStreamProducer, uint32_t images_per_slot,
               uint32_t buffer_size) :
ImageProcessor(upStreamProducer, images_per_slot, buffer_size),
GF_(8),
GFScale_(15),
IntScratchData_(nullptr),
GFScratchData_(nullptr),
MSRScratchData_(nullptr),
floatScratchData_(nullptr),
doubleScratchData1_(nullptr),
doubleScratchData2_(nullptr),
histoBins_(nullptr),
triggerCount_(0)
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
    delete [] IntScratchData_;
    delete [] GFScratchData_;
    delete [] MSRScratchData_;
    delete [] floatScratchData_;
    delete [] doubleScratchData1_;
    delete [] doubleScratchData2_;
    delete [] histoBins_;
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
    
    IntScratchData_=new float[maxWidth*maxHeight];
    memset(IntScratchData_, 0, maxWidth*maxHeight*sizeof(float));
    
    GFScratchData_=new float[maxWidth*maxHeight];
    memset(GFScratchData_, 0, maxWidth*maxHeight*sizeof(float));
    
    MSRScratchData_=new float[maxWidth*maxHeight];
    memset(MSRScratchData_, 0, maxWidth*maxHeight*sizeof(float));
    
    floatScratchData_=new float[maxWidth*maxHeight];
    memset(floatScratchData_, 0, maxWidth*maxHeight*sizeof(float));
    
    doubleScratchData1_=new double[maxWidth*maxHeight];
    memset(doubleScratchData1_, 0, maxWidth*maxHeight*sizeof(double));
    
    doubleScratchData2_=new double[maxWidth*maxHeight];
    memset(doubleScratchData2_, 0, maxWidth*maxHeight*sizeof(double));
    
    histoBins_=new size_t[histoBinArrSize_];
    
    return rValue;
}

bool FIPMSR::trigger()
{
    if ((getNumReadSlotsAvailable())&&(getNumWriteSlotsAvailable()))
    {//There are images to consume and the downstream producer has space to produce.
        std::vector<Image**> imvRead=reserveReadSlot();
        std::vector<Image**> imvWrite=reserveWriteSlot();
        
        ++triggerCount_;
        
        //Start stats measurement event.
        ProcessorStats_->tick();
        
        for (size_t imgNum=0; imgNum<ImagesPerSlot_; ++imgNum)
        {
            Image const * const imRead = *(imvRead[imgNum]);
            Image * const imWrite = *(imvWrite[imgNum]);
            
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
                        IntScratchData_[writeOffset]=(dataRead[readOffset+0] + dataRead[readOffset+1] + dataRead[readOffset+2])*(1.0f/3.0f);
                        readOffset+=3;
                        ++writeOffset;
                    }
                }
                
                F32Image=IntScratchData_;
            }
            //=== ===//
            
            
            
            memset(MSRScratchData_, 0, width*height*sizeof(float));
            
            const size_t numScales=3;
            const float recipNumScales=1.0f/numScales;
            
            for (size_t scaleIndex=0; scaleIndex<numScales; ++scaleIndex)
            {
                const size_t kernelWidth=(imFormatUS.getWidth() / (GFScale_*(1<<(scaleIndex*1)))) | 1;
                const size_t halfKernelWidth=kernelWidth >> 1;
                
                GF_.setKernelWidth(kernelWidth);
                
                // #parallel
                GF_.filter(GFScratchData_, F32Image, width, height,
                                          doubleScratchData1_,
                           scaleIndex==0 ? true : false);
                
                //Approximate Gaussian filt kernel...
                for (int i=0; i<1; ++i)
                {
                    // #parallel?
                    memcpy(floatScratchData_, GFScratchData_, width*height*sizeof(float));
                    
                    // #parallel
                    GF_.filter(GFScratchData_, floatScratchData_, width, height,
                                              doubleScratchData2_, true);
                }
                
                //Calc SSR image...
                for (size_t y=0; y<height; ++y)
                {
                    size_t offset=y*width;
                    
                    for (size_t x=0; x<width; ++x)
                    {
                        //const float r=(log10f(F32Image[offset]+1.0f) - log10f(GFScratchData_[offset]+1.0f));//+1.0f in case the intensities are zero.
                        const float r=(F32Image[offset] - GFScratchData_[offset]);
                        //Note: The log version (true retinex) seems to not have a big impact AND it is slower.
                        //      The gain and chromatGain below may be used to tweak the image.
                        
                        floatScratchData_[offset]=r * 1.0f;
                        
                        ++offset;
                    }
                }
                
                /*
                //=== Update MSR with SSR scaled using local min/max : Local DC level has very little impact; SLOW!===//
                II_.process(doubleScratchData2_, floatScratchData_, width, height);
                
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
                                const float r=floatScratchData_[offset+lx];
                                
                                if (r<rmin) rmin=r;
                                if (r>rmax) rmax=r;
                            }
                        }
                        
                        const float recipRange=1.0f/(rmax - rmin);

                        const float r=(floatScratchData_[offset+x]-rmin) * (recipRange * recipNumScales);
                        MSRScratchData_[offset+x]+=r;
                    }
                }
                //=====================================//
                 */
                //=== OR ===//
                //=== Update MSR with global min/max minus outliers : MUCH faster than local min/max window; Global min/max means filter is not strictly local, but results still very good ===//
                float rmin=-1.0f;
                float rmax=1.0f;
                
                size_t numHistoSamples=0;
                memset(histoBins_, 0, histoBinArrSize_*sizeof(size_t));
                
                for (size_t y=height/4; y<(height*3)/4; ++y)
                {
                    const size_t offset=y*width;
                    
                    for (size_t x=width/4; x<(width*3/4); ++x)
                    {
                        const float r=floatScratchData_[offset+x];
                        
                        const int histoBinNum=int(((r + 1.0f)*0.5f) * (histoBinArrSize_-1) + 0.5f);
                        
                        histoBins_[histoBinNum]=histoBins_[histoBinNum]+1;
                        
                        ++numHistoSamples;
                    }
                }
                 
                //Remove outliers.
                size_t lowerToRemove=numHistoSamples*0.005f;
                size_t lowerRemoved=0;
                
                for (int binNum=0; binNum<histoBinArrSize_; ++binNum)
                {
                    size_t removedFromThisBin=std::min(histoBins_[binNum], lowerToRemove - lowerRemoved);
                    lowerRemoved+=removedFromThisBin;
                    histoBins_[binNum]=0;
                    
                    if (lowerRemoved>=lowerToRemove) break;
                }

                size_t upperToRemove=numHistoSamples*0.001f;
                size_t upperRemoved=0;
                
                for (int binNum=histoBinArrSize_-1; binNum>=0; --binNum)
                {
                    size_t removedFromThisBin=std::min(histoBins_[binNum], upperToRemove - upperRemoved);
                    upperRemoved+=removedFromThisBin;
                    histoBins_[binNum]=0;
                    
                    if (upperRemoved>=upperToRemove) break;
                }
                
                
                //Find min/max from histoBins_.
                for (int binNum=0; binNum<histoBinArrSize_; ++binNum)
                {
                    if (histoBins_[binNum])
                    {
                        rmin=(binNum/float(histoBinArrSize_))*2.0f-1.0f;
                        break;
                    }
                }
                for (int binNum=histoBinArrSize_-1; binNum>=0; --binNum)
                {
                    if (histoBins_[binNum]) 
                    {
                        rmax=((binNum+1)/float(histoBinArrSize_))*2.0f-1.0f;
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
                        const float r=(floatScratchData_[offset+x]-rmin) * (recipRange * recipNumScales);
                        MSRScratchData_[offset+x]+=r;
                    }
                }
                 //=====================================================//
                
            }
            
            //MSR cont.
            const float gain=1.0f;//Boosts image intensity.
            const float chromatGain=1.0f;//Boosts colour.
            const float blacknessFloor=0.05f/255.0f;//Limits the enhancement of low signal (black) areas.
            
            if (imFormatUS.getPixelFormat()==ImageFormat::FLITR_PIX_FMT_RGB_F32)
            {
                for (size_t y=0; y<height; ++y)
                {
                    size_t intensityOffset=y*width;
                    size_t colourOffset=y*width*3;
                    
                    for (size_t x=0; x<width; ++x)
                    {
                        const float r=MSRScratchData_[intensityOffset] * gain;
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
                        const float r=MSRScratchData_[offset+x] * gain;
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

