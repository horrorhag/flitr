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

#include <flitr/modules/flitr_image_processors/dewarp/fip_lk_dewarp_ex.h>

#include <iostream>
#include <fstream>

#include <math.h>


using namespace flitr;
using std::shared_ptr;



FIPLKDewarpEx::FIPLKDewarpEx(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                             const float avrgImageLongevity,
                             uint32_t buffer_size) :
ImageProcessor(upStreamProducer, images_per_slot, buffer_size),
avrgImageLongevity_(avrgImageLongevity),
gradientSqThreshold_(0.000005f),
numLevels_(7),//Num levels searched for scint motion.
gaussianFilter_(5.0f, 20),
gaussianReguFilter_(5.0f, 20),
scratchData_(0)
{
    //Setup image format being produced to downstream.
    for (uint32_t i=0; i<images_per_slot; ++i)
    {
        ImageFormat imgFormat=upStreamProducer.getFormat(i);
        ImageFormat_.push_back(imgFormat);
    }
}

FIPLKDewarpEx::~FIPLKDewarpEx()
{
    delete [] scratchData_;
    
    for (size_t levelNum=0; levelNum<numLevels_; ++levelNum)
    {
        delete [] imgVec_.back();
        imgVec_.pop_back();
        
        delete [] refImgVec_.back();
        refImgVec_.pop_back();
        
        delete [] dxVec_.back();
        dxVec_.pop_back();
        
        delete [] dyVec_.back();
        dyVec_.pop_back();
        
        delete [] dSqVec_.back();
        dSqVec_.pop_back();
        
        delete [] hxVec_.back();
        hxVec_.pop_back();
        
        delete [] hyVec_.back();
        hyVec_.pop_back();
        
        delete [] hxAVec_.back();
        hxAVec_.pop_back();
        
        delete [] hyAVec_.back();
        hyAVec_.pop_back();
    }
}

bool FIPLKDewarpEx::init()
{
    bool rValue=ImageProcessor::init();
    //Note: SharedImageBuffer of downstream producer is initialised with storage in ImageProcessor::init.
    
    const size_t imgNum=0;
    {
        const ImageFormat imFormat=getUpstreamFormat(imgNum);
        
        const ptrdiff_t width=imFormat.getWidth();
        const ptrdiff_t height=imFormat.getHeight();
        
        scratchData_=new float[width*height];
        memset(scratchData_, 0, (width*height)*sizeof(float));
        
        for (size_t levelNum=0; levelNum<numLevels_; ++levelNum)
        {
            imgVec_.push_back(new float[width*height]);
            memset(imgVec_.back(), 0, (width*height)*sizeof(float));
            
            refImgVec_.push_back(new float[width*height]);
            memset(refImgVec_.back(), 0, (width*height)*sizeof(float));
            
            dxVec_.push_back(new float[width*height]);
            memset(dxVec_.back(), 0, (width*height)*sizeof(float));
            
            dyVec_.push_back(new float[width*height]);
            memset(dyVec_.back(), 0, (width*height)*sizeof(float));
            
            dSqVec_.push_back(new float[width*height]);
            memset(dSqVec_.back(), 0, (width*height)*sizeof(float));
            
            hxVec_.push_back(new float[width*height]);
            memset(hxVec_.back(), 0, (width*height)*sizeof(float));
            
            hyVec_.push_back(new float[width*height]);
            memset(hyVec_.back(), 0, (width*height)*sizeof(float));
            
            hxAVec_.push_back(new float[width*height]);
            memset(hxAVec_.back(), 0, (width*height)*sizeof(float));
            
            hyAVec_.push_back(new float[width*height]);
            memset(hyAVec_.back(), 0, (width*height)*sizeof(float));
            
        }
        
        //Push zero h vectors to ordinal numLevels_+1
        hxVec_.push_back(new float[width*height]);
        memset(hxVec_.back(), 0, (width*height)*sizeof(float));
        hyVec_.push_back(new float[width*height]);
        memset(hyVec_.back(), 0, (width*height)*sizeof(float));
        
        //Push zero h vectors to ordinal numLevels_+1
        hxAVec_.push_back(new float[width*height]);
        memset(hxAVec_.back(), 0, (width*height)*sizeof(float));
        hyAVec_.push_back(new float[width*height]);
        memset(hyAVec_.back(), 0, (width*height)*sizeof(float));
    }
    
    return rValue;
}


bool FIPLKDewarpEx::trigger()
{
    if ((getNumReadSlotsAvailable())&&(getNumWriteSlotsAvailable()))
    {
        std::vector<Image**> imvRead=reserveReadSlot();
        std::vector<Image**> imvWrite=reserveWriteSlot();
        
        //Start stats measurement event.
        ProcessorStats_->tick();
        
        const size_t imgNum=0;
        {
            Image const * const imRead = *(imvRead[imgNum]);
            Image * const imWrite = *(imvWrite[imgNum]);
            
            const ImageFormat imFormat=getUpstreamFormat(imgNum);
            const ptrdiff_t width=imFormat.getWidth();
            const ptrdiff_t widthMinus1 = width - ((ptrdiff_t)1);
            const ptrdiff_t height=imFormat.getHeight();
            
            if (imFormat.getPixelFormat()==ImageFormat::FLITR_PIX_FMT_Y_F32)
            {
                float const * const dataRead=(float const * const)imRead->data();
                float * const dataWrite=(float * const)imWrite->data();
                
                //****************
                //Replace the reference image during the first couple of frames.
                const float avrgImageLongevityConst = (frameNumber_ < 3) ? 0.0f : avrgImageLongevity_;
                //****************
                
                {//=== Calculate scale space images. ===
                    for (size_t levelNum=0; levelNum<numLevels_; ++levelNum)
                    {
                        float * const imgData=imgVec_[levelNum];
                        //float * const refImgData=refImgVec_[levelNum];
                        
                        {//Update ref/avrg img before new data arrives.
                            float * const refImgData=refImgVec_[levelNum];
                            
                            for (ptrdiff_t y=0; y<height; ++y)
                            {
                                const ptrdiff_t lineOffset=y*width;
                                
                                for (ptrdiff_t x=0; x<width; ++x)
                                {
                                    const ptrdiff_t offset=lineOffset + x;
                                    
                                    refImgData[offset]*=avrgImageLongevityConst;
                                    refImgData[offset]+=imgData[offset] * (1.0f - avrgImageLongevityConst);
                                }
                            }
                        }
                        
                        if (levelNum==0)
                        {//Do Gaussian filter of initial input
                            gaussianFilter_.filter(imgData, dataRead, width, height, scratchData_);
                        } else
                        {//Downsample higher resolution level.
                            gaussianFilter_.filter(imgData, imgVec_[levelNum-1], width, height, scratchData_);
                        }
                        
                        
                        {//=== Calculate scale space gradient images ===
                            float * const dxData=dxVec_[levelNum];
                            float * const dyData=dyVec_[levelNum];
                            float * const dSqData=dSqVec_[levelNum];
                            
                            for (ptrdiff_t y=1; y<(height - ((ptrdiff_t)1)); ++y)
                            {
                                const ptrdiff_t lineOffset=y*width;
                                
                                for (ptrdiff_t x=((ptrdiff_t)1); x<widthMinus1; ++x)
                                {
                                    const ptrdiff_t offset=lineOffset + x;
                                    
                                    const float v1=imgData[offset-width-1];
                                    const float v2=imgData[offset-width];
                                    const float v3=imgData[offset-width+1];
                                    const float v4=imgData[offset-1];
                                    //const float v5=imgData[offset];
                                    const float v6=imgData[offset+1];
                                    const float v7=imgData[offset+width-1];
                                    const float v8=imgData[offset+width];
                                    const float v9=imgData[offset+width+1];
                                    
                                    //Use Scharr operator for image gradient. It has good rotation independence!
                                    const float dx=(v3-v1)*(3.0f/32.0f) + (v6-v4)*(10.0f/32.0f) + (v9-v7)*(3.0f/32.0f);
                                    const float dy=(v7-v1)*(3.0f/32.0f) + (v8-v2)*(10.0f/32.0f) + (v9-v3)*(3.0f/32.0f);
                                    
                                    dxData[offset]=dx;
                                    dyData[offset]=dy;
                                    dSqData[offset]=dx*dx+dy*dy;
                                }
                            }
                        }//=== ===
                    }
                }//=== ===
                
                
                //===========================
                //=== Calculate h vectors ===
                memset(hxVec_.back(), 0, width * height * sizeof(float));
                memset(hyVec_.back(), 0, width * height * sizeof(float));
                
                for (ptrdiff_t levelNum=(numLevels_-1); levelNum>=0; --levelNum)
                {
                    float const * const imgData=imgVec_[levelNum];
                    float const * const refImgData=refImgVec_[levelNum];
                    
                    float const * const dxData=dxVec_[levelNum];
                    float const * const dyData=dyVec_[levelNum];
                    float const * const dSqData=dSqVec_[levelNum];
                    
                    float * const hxData=hxVec_[levelNum];
                    float * const hyData=hyVec_[levelNum];
                    float const * const hxDataLR=hxVec_[levelNum+1];
                    float const * const hyDataLR=hyVec_[levelNum+1];
                    
                    //=== Start with h-vectors from lower resolution level ===//
                    for (ptrdiff_t y=1; y<(height - ((ptrdiff_t)1)); ++y)
                    {
                        const ptrdiff_t lineOffset=y*width;
                        
                        for (ptrdiff_t x=((ptrdiff_t)1); x<widthMinus1; ++x)
                        {
                            const ptrdiff_t offset=lineOffset + x;
                            
                            hxData[offset]=hxDataLR[offset];
                            hyData[offset]=hyDataLR[offset];
                        }
                    }
                    //=== ===//
                    
                    //if (levelNum>=4)
                    {
                        for (size_t newtonRaphsonI=0; newtonRaphsonI<6; ++newtonRaphsonI)//5 or more iterations seem to work well.
                        {
                            for (ptrdiff_t y=((ptrdiff_t)1); y<(height - ((ptrdiff_t)1)); ++y)
                            {
                                const ptrdiff_t lineOffset=y*width;
                                
                                for (ptrdiff_t x=((ptrdiff_t)1); x<widthMinus1; ++x)
                                {
                                    const ptrdiff_t offset=lineOffset + x;
                                    
                                    const float dSq=dSqData[offset];
                                    
                                    if (dSq > gradientSqThreshold_)
                                    {
                                        float hx=hxData[offset];
                                        float hy=hyData[offset];
                                        const ptrdiff_t int_hx=lroundf(hx);
                                        const ptrdiff_t int_hy=lroundf(hy);
                                        
                                        if (((x+int_hx)>((ptrdiff_t)1)) &&
                                            ((y+int_hy)>((ptrdiff_t)1)) &&
                                            ((x+int_hx+((ptrdiff_t)2))<width) &&
                                            ((y+int_hy+((ptrdiff_t)2))<height))
                                        {
                                            const ptrdiff_t offsetH=offset + int_hx + int_hy * width;
                                            
                                            const float dx=dxData[offsetH];
                                            const float dy=dyData[offsetH];
                                            
                                            const float imgDiff=imgData[offsetH]-refImgData[offset];
                                            
                                            //Unsimplified code:
                                            //h=imgDiff/d;
                                            //hx=h * dx/d;
                                            //hy=h * dx/d;
                                            
                                            //Simplified code.
                                            hx-=0.5f*imgDiff*(dx/dSq);
                                            hy-=0.5f*imgDiff*(dy/dSq);
                                        }
                                        
                                        hxData[offset]=hx;
                                        hyData[offset]=hy;
                                    }
                                }
                            }
                            
                            
                            if (levelNum<=1)
                            {//=== Smooth/regularise the vector field of this iteration using Gaussian filters in x and y ===//
                            }//=== ===
                            
                        }
                    }
                }
                //===========================
                //===========================
                
                
                
                //=== Final results ===
                {
                    float * const hxDataGF=hxVec_[0];
                    float * const hyDataGF=hyVec_[0];
                    
                    for (ptrdiff_t y=0; y<height; ++y)
                    {
                        const ptrdiff_t lineOffset=y*width;
                        
                        for (ptrdiff_t x=0; x<width; ++x)
                        {
                            const ptrdiff_t offset=lineOffset + x;
                            
                            if (false)
                            {
                                //=== Image Dewarping ===//
                                {
                                    const float hx=hxDataGF[offset];
                                    const float hy=hyDataGF[offset];
                                    
                                    const float floor_hx=floorf(hx);
                                    const float floor_hy=floorf(hy);
                                    const ptrdiff_t int_hx=lroundf(floor_hx);
                                    const ptrdiff_t int_hy=lroundf(floor_hy);
                                    
                                    if ( ((x+int_hx)>((ptrdiff_t)1)) && ((y+int_hy)>((ptrdiff_t)1)) && ((x+int_hx)<(width-1)) && ((y+int_hy)<(height-1)) )
                                    {
                                        const float frac_hx=hx - floor_hx;
                                        const float frac_hy=hy - floor_hy;
                                        const ptrdiff_t offsetLT=offset + int_hx + int_hy * width;
                                        
                                        const float img=bilinearRead(dataRead, offsetLT, width, frac_hx, frac_hy);
                                        dataWrite[offset]=img;
                                    }
                                }
                                //=== ===
                            }
                            else //OR
                            {
                                const size_t levelIndex=1;//(numLevels_-1) - 5;
                                
                                //const float i=imgVec_[levelIndex][offset];
                                //const float r=refImgVec_[levelIndex][offset];
                                const float hx=hxVec_[levelIndex][offset];
                                const float hy=hyVec_[levelIndex][offset];
                                
                                const float fc=0.99f;
                                
                                hxAVec_[levelIndex][offset]*=fc;
                                hxAVec_[levelIndex][offset]+=hx*(1.0f-fc);
                                
                                hyAVec_[levelIndex][offset]*=fc;
                                hyAVec_[levelIndex][offset]+=hy*(1.0f-fc);
                                
                                //const float hxA=hxAVec_[levelIndex][offset];
                                //const float hyA=hyAVec_[levelIndex][offset];
                                
                                const float dhx=(hx);//-hxA);
                                const float dhy=(hy);//-hyA);
                                
                                //dataWrite[offset]=r;
                                dataWrite[offset]=sqrtf(dhx*dhx + dhy*dhy)*0.075f;
                            }
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



