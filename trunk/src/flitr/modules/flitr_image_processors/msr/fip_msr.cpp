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

using namespace flitr;
using std::shared_ptr;

FIPMSR::FIPMSR(ImageProducer& upStreamProducer, uint32_t images_per_slot,
               uint32_t buffer_size) :
ImageProcessor(upStreamProducer, images_per_slot, buffer_size),
floatScratchData_(nullptr),
doubleScratchData_(nullptr),
fmin_(std::numeric_limits<float>::infinity()),
fmax_(-std::numeric_limits<float>::infinity()),
triggerCount_(0),
GFScale_(0)
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
    delete [] floatScratchData_;
    delete [] doubleScratchData_;
    
    for (uint32_t i=0; i<ImagesPerSlot_; ++i)
    {
        if (F32ImageVec_[i]!=nullptr) delete [] F32ImageVec_[i];
        
        for (size_t scaleIndex=0; scaleIndex<GFVec_.size(); ++scaleIndex)
        {
            delete [] GFF32ImageVecVec_[i][scaleIndex];
        }
        GFF32ImageVecVec_[i].clear();
    }
    
    F32ImageVec_.clear();
}

bool FIPMSR::init()
{
    bool rValue=ImageProcessor::init();
    //Note: SharedImageBuffer of downstream producer is initialised with storage in ImageProcessor::init.
    
    //Initial GF width, but this is updated in trigger() for different image slot sizes.
    GFVec_.emplace_back(8);
    GFVec_.emplace_back(32);
    GFVec_.emplace_back(64);
    GFScale_=12;
    
    size_t maxWidth=0;
    size_t maxHeight=0;
    
    for (uint32_t i=0; i<ImagesPerSlot_; ++i)
    {
        const ImageFormat imFormat=getUpstreamFormat(i);//Downstream format is same as upstream format.
        
        const size_t width=imFormat.getWidth();
        const size_t height=imFormat.getHeight();
        
        if (width>maxWidth) maxWidth=width;
        if (height>maxHeight) maxHeight=height;
        
        if (imFormat.getPixelFormat()==ImageFormat::FLITR_PIX_FMT_Y_F32)
        {
            F32ImageVec_.push_back(nullptr);//The input image is already Y_F32.
        } else
        {
            F32ImageVec_.push_back(new float[width*height]);
            memset(F32ImageVec_[i], 0, width*height*sizeof(float));
        }
        
        GFF32ImageVecVec_.push_back(std::vector<float *>());
        for (size_t scaleIndex=0; scaleIndex<GFVec_.size(); ++scaleIndex)
        {
            GFF32ImageVecVec_[i].push_back(new float[width*height]);
            memset(GFF32ImageVecVec_[i][scaleIndex], 0, width*height*sizeof(float));
        }
    }
    
    floatScratchData_=new float[maxWidth*maxHeight];
    memset(floatScratchData_, 0, maxWidth*maxHeight*sizeof(float));
    
    doubleScratchData_=new double[maxWidth*maxHeight];
    memset(doubleScratchData_, 0, maxWidth*maxHeight*sizeof(double));
    
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
                float * const preAllocF32Image=F32ImageVec_[imgNum];
                
                for (size_t y=0; y<height; ++y)
                {
                    size_t readOffset=y*width*3;
                    size_t writeOffset=y*width;
                    
                    for (size_t x=0; x<width; ++x)
                    {
                        preAllocF32Image[writeOffset]=(dataRead[readOffset+0] + dataRead[readOffset+1] + dataRead[readOffset+2])*(1.0f/3.0f);
                        readOffset+=3;
                        ++writeOffset;
                    }
                }
                
                F32Image=preAllocF32Image;
            }
            //=== ===//
            
            
            //=== Calculate Gaussian scale space ===//
            {
                const size_t numScales=GFVec_.size();
                
                for (size_t scaleIndex=0; scaleIndex<numScales; ++scaleIndex)
                {
                    GFVec_[scaleIndex].setKernelWidth(imFormatUS.getWidth() / (GFScale_*(1<<scaleIndex)) );
                    
                    GFVec_[scaleIndex].filter(GFF32ImageVecVec_[imgNum][scaleIndex], F32Image, width, height,
                                              doubleScratchData_);
                    
                    //Approximate Gaussian filt kernel...
                    for (int i=0; i<1; ++i)
                    {
                        memcpy(floatScratchData_, GFF32ImageVecVec_[imgNum][scaleIndex], width*height*sizeof(float));
                        
                        GFVec_[scaleIndex].filter(GFF32ImageVecVec_[imgNum][scaleIndex], floatScratchData_, width, height,
                                                  doubleScratchData_);
                    }
                }
            }
            //=== ===
            
            
            //MSR
            memset(floatScratchData_, 0, width*height*sizeof(float));
            
            for (size_t scaleIndex=0; scaleIndex<GFVec_.size(); ++scaleIndex)
            {
                float const * const filteredInput=GFF32ImageVecVec_[imgNum][scaleIndex];
                
                    for (size_t y=0; y<height; ++y)
                    {
                        size_t offset=y*width;
                        
                        for (size_t x=0; x<width; ++x)
                        {
                            //const float r=(log10f(F32Image[offset]+1.0f) - log10f(filteredInput[offset]+1.0f));//+1.0f in case the intensities are zero.
                            const float r=(F32Image[offset] - filteredInput[offset]);
                            //Note: The log version seems to not have a big impact AND it is slower.
                            //      The gain and chromatGain below may be used to tweak the image.
                            
                            floatScratchData_[offset]+=r;
                            
                            ++offset;
                        }
                    }
            }
            
            
            //Min/max
            float rmin=std::numeric_limits<float>::infinity();
            float rmax=-std::numeric_limits<float>::infinity();
            
            for (size_t y=height/3; y<(height*2)/3; ++y)
            {
                const size_t offset=y*width;
                
                for (size_t x=width/3; x<(width*2/3); ++x)
                {
                    const float r=floatScratchData_[offset+x];
                    
                    if (r<rmin) rmin=r;
                    if (r>rmax) rmax=r;
                }
            }
            
            
            //Filter the min/max limits to prevent flickering.
            //  Note: Filtering then min/max values is somewhat faster than removing outliers.
            if (triggerCount_<3)
            {
                fmin_=rmin;
                fmax_=rmax;
            } else
            {
                const float a=0.2f;
                
                fmin_=fmin_*(1.0f-a) + rmin*a;
                fmax_=fmax_*(1.0f-a) + rmax*a;
            }
            
            
            //Fit range.
            const float gain=1.0f;//Boosts image intensity.
            const float chromatGain=1.0f;//Boosts colour.
            const float blacknessFloor=2.5f/255.0f;//Limits the enhancement of low signal (black) areas.
            
            const float recipRange=1.0f/(fmax_-fmin_);
            
            if (imFormatUS.getPixelFormat()==ImageFormat::FLITR_PIX_FMT_RGB_F32)
            {
                for (size_t y=0; y<height; ++y)
                {
                    size_t intensityOffset=y*width;
                    size_t colourOffset=y*width*3;
                    
                    for (size_t x=0; x<width; ++x)
                    {
                        const float r=(floatScratchData_[intensityOffset]-fmin_) * recipRange * gain;
                        const float intInput=F32Image[intensityOffset];
                        const float recipIntInput=1.0f/(intInput+blacknessFloor);

                        dataWrite[colourOffset+0]=r * (dataRead[colourOffset+0]*recipIntInput) + (recipRange * chromatGain) * (dataRead[colourOffset+0]-intInput);
                        dataWrite[colourOffset+1]=r * (dataRead[colourOffset+1]*recipIntInput) + (recipRange * chromatGain) * (dataRead[colourOffset+1]-intInput);
                        dataWrite[colourOffset+2]=r * (dataRead[colourOffset+2]*recipIntInput) + (recipRange * chromatGain) * (dataRead[colourOffset+2]-intInput);
                        
                        /*
                        const float r=(floatScratchData_[intensityOffset]-fmin_)*recipRange * gain;
                        const float intInput=F32Image[intensityOffset];
                        const float recipIntInput=1.0f/(intInput+chromatIntensFloor);
                        
                        dataWrite[colourOffset+0]=r ;//+ recipRange * (dataRead[colourOffset+0]-intInput);
                        dataWrite[colourOffset+1]=r ;//+ recipRange * (dataRead[colourOffset+1]-intInput);
                        dataWrite[colourOffset+2]=r ;//+ recipRange * (dataRead[colourOffset+2]-intInput);
                         */
                        
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
                        const float r=(floatScratchData_[offset+x]-fmin_) * recipRange * gain;
                        const float intInput=F32Image[offset+x];

                        dataWrite[offset+x]=r * (intInput/(intInput+blacknessFloor));
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

