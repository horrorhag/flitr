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

#include <flitr/modules/flitr_image_processors/average_image/fip_average_image_iir.h>

#include <iostream>

using namespace flitr;
using std::shared_ptr;

FIPAverageImageIIR::FIPAverageImageIIR(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                                       float control,
                                       uint32_t buffer_size) :
ImageProcessor(upStreamProducer, images_per_slot, buffer_size),
GF_(32),
control_(control),
doubleScratchData_(nullptr),
floatScratchData_(nullptr),
triggerCount_(0)
{
    //Setup image format being produced to downstream.
    for (uint32_t i=0; i<images_per_slot; i++) {
        ImageFormat_.push_back(upStreamProducer.getFormat(i));//Output format is same as input format.
    }
}

FIPAverageImageIIR::~FIPAverageImageIIR()
{
    for (uint32_t i=0; i<ImagesPerSlot_; ++i)
    {
        if (F32ImageVec_[i]!=nullptr) delete [] F32ImageVec_[i];
        
        delete [] aImageVec_[i];
        delete [] gfImageVec_[0][i];
        delete [] gfImageVec_[1][i];
    }
    
    F32ImageVec_.clear();
    delete [] doubleScratchData_;
}

bool FIPAverageImageIIR::init()
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
        
        const size_t componentsPerPixel=imFormat.getComponentsPerPixel();
        
        aImageVec_.push_back(new float[width*height*componentsPerPixel]);
        memset(aImageVec_[i], 0, width*height*componentsPerPixel*sizeof(float));
        
        if (imFormat.getPixelFormat()==ImageFormat::FLITR_PIX_FMT_Y_F32)
        {
            F32ImageVec_.push_back(nullptr);//The input image is already Y_F32.
        } else
        {
            F32ImageVec_.push_back(new float[width*height]);
            memset(F32ImageVec_[i], 0, width*height*sizeof(float));
        }
        
        gfImageVec_[0].push_back(new float[width*height*componentsPerPixel]);
        memset(gfImageVec_[0][i], 0, width*height*componentsPerPixel*sizeof(float));
        imgAvrg[0]=0.0f;
        
        gfImageVec_[1].push_back(new float[width*height*componentsPerPixel]);
        memset(gfImageVec_[1][i], 0, width*height*componentsPerPixel*sizeof(float));
        imgAvrg[1]=0.0f;
    }
    
    doubleScratchData_=new double[maxWidth*maxHeight];
    memset(doubleScratchData_, 0, maxWidth*maxHeight*sizeof(double));
    
    floatScratchData_=new float[maxWidth*maxHeight];
    memset(floatScratchData_, 0, maxWidth*maxHeight*sizeof(float));
    
    return rValue;
}

bool FIPAverageImageIIR::trigger()
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
            
            const ImageFormat imFormat=getUpstreamFormat(imgNum);//Downstream format is same as upstream format.
            
            const size_t width=imFormat.getWidth();
            const size_t height=imFormat.getHeight();
            
            float const * const dataRead = (float const * const)imRead->data();
            float * const dataWrite = (float * const)imWrite->data();
            float * const aImage = aImageVec_[imgNum];
            float const * F32Image=nullptr;
            
            //=== Get intensity of input ==//
            if (imFormat.getPixelFormat()==ImageFormat::FLITR_PIX_FMT_Y_F32)
            {//Input image already Y_F32. Just use its pointer.
                F32Image=dataRead;
            } else
            {//Convert input image to Y_F32 and store in pre-allocated F32Image.
                float const * const dataRead = (float const * const)imRead->data();
                float * const preAllocF32Image=F32ImageVec_[imgNum];
                
                // #parallel
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
            
            
            {
                // #parallel
                GF_.setKernelWidth(width/75);
                imgAvrg[triggerCount_%2]=GF_.filter(gfImageVec_[triggerCount_%2][imgNum], F32Image, width, height, doubleScratchData_, false);
                
                std::cout << imgAvrg[triggerCount_%2] << "\n";
                std::cout.flush();
                
                //Approximate Gaussian filt kernel...
                for (int i=0; i<1; ++i)
                {
                    // #parallel?
                    memcpy(floatScratchData_, gfImageVec_[triggerCount_%2][imgNum], width*height*sizeof(float));
                    
                    // #parallel
                    GF_.filter(gfImageVec_[triggerCount_%2][imgNum], floatScratchData_, width, height,
                               doubleScratchData_, false);
                }
            }
            
            if (imFormat.getPixelFormat()==ImageFormat::FLITR_PIX_FMT_Y_F32)
            {
                for (size_t y=0; y<height; ++y)
                {
                    const size_t lineOffset=y * width;
                    
                    for (size_t x=0; x<width; ++x)
                    {
                        const float t=(triggerCount_<2) ?  1.0f : fabsf(gfImageVec_[0][imgNum][lineOffset+x]*(imgAvrg[1]/(imgAvrg[0]+0.001f)) - gfImageVec_[1][imgNum][lineOffset+x])*control_;
                        
                        aImage[lineOffset + x]=aImage[lineOffset + x]*(1.0f-t) + dataRead[lineOffset + x]*t;
                        
                        dataWrite[lineOffset + x]=aImage[lineOffset + x];
                    }
                }
            } else
                if (imFormat.getPixelFormat()==ImageFormat::FLITR_PIX_FMT_RGB_F32)
                {
                    for (size_t y=0; y<height; ++y)
                    {
                        size_t intensOffset=y * width;
                        size_t offset=y * width*3;
                        
                        for (size_t x=0; x<width; ++x)
                        {
                            float t=(triggerCount_<2) ?  1.0f : fabsf(gfImageVec_[0][imgNum][intensOffset]*(imgAvrg[1]/(imgAvrg[0]+0.001f)) - gfImageVec_[1][imgNum][intensOffset])*control_;
                            
                            if (t>1.0f) t=1.0f; else
                                if (t<0.1f) t=0.1f;
                            
                            aImage[offset + 0]=aImage[offset + 0]*(1.0f-t) + dataRead[offset + 0]*t;
                            aImage[offset + 1]=aImage[offset + 1]*(1.0f-t) + dataRead[offset + 1]*t;
                            aImage[offset + 2]=aImage[offset + 2]*(1.0f-t) + dataRead[offset + 2]*t;
                            
                            dataWrite[offset + 0]=aImage[offset + 0];
                            dataWrite[offset + 1]=aImage[offset + 1];
                            dataWrite[offset + 2]=aImage[offset + 2];
                            //dataWrite[offset + 2]=t;
                            
                            ++intensOffset;
                            offset+=3;
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

