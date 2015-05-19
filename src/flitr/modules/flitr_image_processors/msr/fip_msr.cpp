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
scratchData_(nullptr),
scratchData2_(nullptr)
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
    delete [] scratchData_;
    delete [] scratchData2_;
    
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
    
    GFVec_.emplace_back(16);
    GFVec_.emplace_back(32);
    GFVec_.emplace_back(128);
    
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
    
    scratchData_=new float[maxWidth*maxHeight];
    memset(scratchData_, 0, maxWidth*maxHeight*sizeof(float));
    
    scratchData2_=new float[maxWidth*maxHeight];
    memset(scratchData2_, 0, maxWidth*maxHeight*sizeof(float));
    
    return rValue;
}

bool FIPMSR::trigger()
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
                    GFVec_[scaleIndex].filter(GFF32ImageVecVec_[imgNum][scaleIndex], F32Image, width, height, scratchData_);
                    
                    //Approximate Gaussian filt kernel...
                    for (int i=0; i<1; ++i)
                    {
                        memcpy(scratchData2_, GFF32ImageVecVec_[imgNum][scaleIndex], width*height*sizeof(float));
                        GFVec_[scaleIndex].filter(GFF32ImageVecVec_[imgNum][scaleIndex], scratchData2_, width, height, scratchData_);
                    }
                }
            }
            //=== ===
            
            
            //===  ===//
            memset(scratchData_, 0, width*height*sizeof(float));
            
            for (size_t scaleIndex=0; scaleIndex<GFVec_.size(); ++scaleIndex)
            {
                float const * const filteredInput=GFF32ImageVecVec_[imgNum][scaleIndex];
                
                if (imFormatUS.getPixelFormat()==ImageFormat::FLITR_PIX_FMT_Y_F32)
                {
                    for (size_t y=0; y<height; ++y)
                    {
                        size_t readOffset=y*width;
                        size_t writeOffset=y*width;
                        
                        for (size_t x=0; x<width; ++x)
                        {
                            dataWrite[writeOffset]+=filteredInput[readOffset];
                            
                            ++readOffset;
                            ++writeOffset;
                        }
                    }
                } else
                    if (imFormatUS.getPixelFormat()==ImageFormat::FLITR_PIX_FMT_RGB_F32)
                    {
                        //MSR
                        for (size_t y=0; y<height; ++y)
                        {
                            size_t offset=y*width;
                            
                            for (size_t x=0; x<width; ++x)
                            {
                                const float r=(log10f(F32Image[offset]+1.0f) - log10f(filteredInput[offset]+1.0f));//+1.0f in case the intensities are zero.
                                
                                scratchData_[offset]+=r;
                                
                                ++offset;
                            }
                        }
                        
                        
                        //Min/max
                        float min=std::numeric_limits<float>::infinity();
                        float max=-std::numeric_limits<float>::infinity();
                        
                        for (size_t y=height/3; y<(height*2)/3; ++y)
                        {
                            size_t offset=y*width + width/3;
                            
                            for (size_t x=width/3; x<(width*2/3); ++x)
                            {
                                const float r=scratchData_[offset];
                                
                                if (r<min) min=r;
                                if (r>max) max=r;
                                
                                ++offset;
                            }
                        }

                        
                        //Fit range.
                        const float recipRange=1.0f/(max-min);
                        
                        for (size_t y=0; y<height; ++y)
                        {
                            size_t intensityOffset=y*width;
                            size_t colourOffset=y*width*3;
                            
                            for (size_t x=0; x<width; ++x)
                            {
                                const float r=(scratchData_[intensityOffset]-min)*recipRange;
                                const float recipInput=1.0f/(F32Image[intensityOffset]+(5.0f/255.0f));
                                //const float r=filteredInput[readOffset];
                                
                                dataWrite[colourOffset+0]=r * (dataRead[colourOffset+0]*recipInput);
                                dataWrite[colourOffset+1]=r * (dataRead[colourOffset+1]*recipInput);
                                dataWrite[colourOffset+2]=r * (dataRead[colourOffset+2]*recipInput);
                                
                                ++intensityOffset;
                                colourOffset+=3;
                            }
                        }
                        
                    }
            }
            //=== ===
        }
        
        //Stop stats measurement event.
        ProcessorStats_->tock();
        
        releaseWriteSlot();
        releaseReadSlot();
        
        return true;
    }
    return false;
}

