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

#include <flitr/modules/flitr_image_processors/stabilise/fip_lk_stabilise.h>

#include <iostream>
#include <fstream>

#include <math.h>



using namespace flitr;
using std::shared_ptr;

FIPLKStabilise::FIPLKStabilise(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                               uint32_t buffer_size) :
ImageProcessor(upStreamProducer, images_per_slot, buffer_size),
frameNum_(0),
numLevels_(0), //Setup numLevels_ automatically in init().
scratchData_(0),
finalImgData_(0)
{
    //Setup image format being produced to downstream.
    for (uint32_t i=0; i<images_per_slot; ++i)
    {
        ImageFormat imgFormat=upStreamProducer.getFormat(i);
        ImageFormat_.push_back(imgFormat);
    }
}

FIPLKStabilise::~FIPLKStabilise()
{
    delete [] scratchData_;
    delete [] finalImgData_;
    
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
        
        delete [] dSqRecipVec_.back();
        dSqRecipVec_.pop_back();
    }
}

bool FIPLKStabilise::init()
{
    bool rValue=ImageProcessor::init();
    //Note: SharedImageBuffer of downstream producer is initialised with storage in ImageProcessor::init.
    
    const size_t imgNum=0;
    {
        const ImageFormat imFormat=getUpstreamFormat(imgNum);
        
        const ptrdiff_t width=imFormat.getWidth();
        const ptrdiff_t height=imFormat.getHeight();
        
        //Setup numLevels_ automatically.
        numLevels_=(width>=height) ? log2f(width) : log2f(height);
        
        //=== Image will be cropped so that at least all levels of the pyramid is divisible by 2 ===
        const ptrdiff_t croppedWidth=(width>>(numLevels_-1))<<(numLevels_-1);
        const ptrdiff_t croppedHeight=(height>>(numLevels_-1))<<(numLevels_-1);
        //const ptrdiff_t croppedWidth=1 << ((int)log2f(width));
        //const ptrdiff_t croppedHeight=1 << ((int)log2f(height));
        //=== ===
        
        scratchData_=new float[(width*height)<<1];
        memset(scratchData_, 0, ((width*height)<<1)*sizeof(float));
        
        finalImgData_=new float[croppedWidth*croppedHeight];
        memset(finalImgData_, 0, croppedWidth*croppedHeight*sizeof(float));
        
        for (size_t levelNum=0; levelNum<numLevels_; ++levelNum)
        {
            imgVec_.push_back(new float[(croppedWidth>>levelNum) * (croppedHeight>>levelNum)]);
            memset(imgVec_.back(), 0, (croppedWidth>>levelNum) * (croppedHeight>>levelNum) * sizeof(float));
            
            refImgVec_.push_back(new float[(croppedWidth>>levelNum) * (croppedHeight>>levelNum)]);
            memset(refImgVec_.back(), 0, (croppedWidth>>levelNum) * (croppedHeight>>levelNum) * sizeof(float));
            
            dxVec_.push_back(new float[(croppedWidth>>levelNum) * (croppedHeight>>levelNum)]);
            memset(dxVec_.back(), 0, (croppedWidth>>levelNum) * (croppedHeight>>levelNum) * sizeof(float));
            
            dyVec_.push_back(new float[(croppedWidth>>levelNum) * (croppedHeight>>levelNum)]);
            memset(dyVec_.back(), 0, (croppedWidth>>levelNum) * (croppedHeight>>levelNum) * sizeof(float));
            
            dSqRecipVec_.push_back(new float[(croppedWidth>>levelNum) * (croppedHeight>>levelNum)]);
            memset(dSqRecipVec_.back(), 0, (croppedWidth>>levelNum) * (croppedHeight>>levelNum) * sizeof(float));
        }
    }
    
    return rValue;
}

bool FIPLKStabilise::trigger()
{
    if ((getNumReadSlotsAvailable())&&(getNumWriteSlotsAvailable()))
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> scopedLock(triggerMutex_);
        
        std::vector<Image**> imvRead=reserveReadSlot();
        std::vector<Image**> imvWrite=reserveWriteSlot();
        
        //Start stats measurement event.
        ProcessorStats_->tick();
        
        const size_t imgNum=0;
        {
            Image const * const imRead = *(imvRead[imgNum]);
            Image * const imWrite = *(imvWrite[imgNum]);
            float const * const dataRead=(float const * const)imRead->data();
            float * const dataWrite=(float * const)imWrite->data();
            
            const ImageFormat imFormat=getUpstreamFormat(imgNum);
            const ptrdiff_t uncroppedWidth=imFormat.getWidth();
            const ptrdiff_t uncroppedHeight=imFormat.getHeight();
            
            //=== Image will be cropped so that at least all levels of the pyramid is divisible by 2 ===
            const ptrdiff_t croppedWidth=(uncroppedWidth>>(numLevels_-1))<<(numLevels_-1);
            const ptrdiff_t croppedHeight=(uncroppedHeight>>(numLevels_-1))<<(numLevels_-1);
            //const ptrdiff_t croppedWidth=1 << ((int)log2f(uncroppedWidth));
            //const ptrdiff_t croppedHeight=1 << ((int)log2f(uncroppedHeight));
            //=== ===
            
            const ptrdiff_t startCroppedX=(uncroppedWidth - croppedWidth)>>1;
            const ptrdiff_t startCroppedY=(uncroppedHeight - croppedHeight)>>1;
            
            const ptrdiff_t endCroppedY=startCroppedY + croppedHeight - 1;
            
            
            {//=== Copy cropped input to level 0 of scale space ===
                
                float * const imgData=imgVec_[0];
                float * const refImgData=refImgVec_[0];
                
                if (frameNum_<3)
                {//=== Update ref img before new data arrives. ===//
                    memcpy(refImgData, imgData, croppedWidth*croppedHeight*sizeof(float));
                }
                
                //=== Copy input data to level 0 of scale space ===//
                for (size_t y=startCroppedY; y<=endCroppedY; ++y)
                {
                    const ptrdiff_t uncroppedLineOffset=y*uncroppedWidth + startCroppedX;
                    const ptrdiff_t croppedLineOffset=(y-startCroppedY)*croppedWidth;
                    memcpy(imgData+croppedLineOffset, dataRead+uncroppedLineOffset, croppedWidth*sizeof(float));
                }
            }//=== ===
            
            {//=== Calculate scale space pyramid. ===
                for (size_t levelNum=0; levelNum<numLevels_; ++levelNum)
                {
                    float * const imgData=imgVec_[levelNum];
                    
                    const ptrdiff_t levelHeight=croppedHeight >> levelNum;
                    const ptrdiff_t levelWidth=croppedWidth >> levelNum;
                    const ptrdiff_t levelWidthMinus1 = levelWidth - ((ptrdiff_t)1);
                    const ptrdiff_t levelWidthMinus3 = levelWidth - ((ptrdiff_t)3);
                    
                    //=== Calculate the scale space images ===
                    if (levelNum>0)//First level (incoming data) is not a downsampled image.
                    {
                        if (frameNum_<3)
                        {//Update ref img before new data arrives.
                            memcpy(refImgVec_[levelNum], imgData, levelWidth*levelHeight*sizeof(float));
                        }
                        
                        float const * const imgDataHR=imgVec_[levelNum-1];
                        const ptrdiff_t heightHR=croppedHeight >> (levelNum-1);
                        const ptrdiff_t widthHR=croppedWidth >> (levelNum-1);
                        
                        //=== Seperable Gaussian first pass - down filter x ===
                        for (size_t y=0; y<heightHR; ++y)
                        {
                            const ptrdiff_t lineOffsetScratch=y * levelWidth;
                            const ptrdiff_t lineOffsetHR=y * widthHR;
                            
                            for (ptrdiff_t x=3; x<levelWidthMinus3; ++x)
                            {
                                const ptrdiff_t xHR=(x<<1);
                                const ptrdiff_t offsetHR=lineOffsetHR + xHR;
                                
                                float filtValue=(imgDataHR[offsetHR] +
                                                 imgDataHR[offsetHR + 1] ) * (462.0f/2048.0f);//The const expr devisions will be compiled away!
                                
                                filtValue+=(imgDataHR[offsetHR - 1] +
                                            imgDataHR[offsetHR + 2] ) * (330.0f/2048.0f);
                                
                                filtValue+=(imgDataHR[offsetHR - 2] +
                                            imgDataHR[offsetHR + 3] ) * (165.0f/2048.0f);
                                
                                filtValue+=(imgDataHR[offsetHR - 3] +
                                            imgDataHR[offsetHR + 4] ) * (55.0f/2048.0f);
                                
                                filtValue+=(imgDataHR[offsetHR - 4] +
                                            imgDataHR[offsetHR + 5] ) * (11.0f/2048.0f);
                                
                                filtValue+=(imgDataHR[offsetHR - 5] +
                                            imgDataHR[offsetHR + 6] ) * (1.0f/2048.0f);
                                
                                scratchData_[lineOffsetScratch + x]=filtValue;
                            }
                        }
                        //=== ===
                        
                        //=== Seperable Gaussian second pass - down filter y===
                        for (ptrdiff_t y=((ptrdiff_t)3); y<(levelHeight-((ptrdiff_t)3)); ++y)
                        {
                            const ptrdiff_t lineOffset=y * levelWidth;
                            const ptrdiff_t lineOffsetScratch=(y<<1) * levelWidth;
                            
                            for (ptrdiff_t x=3; x<levelWidthMinus3; ++x)
                            {
                                const ptrdiff_t offsetScratch=lineOffsetScratch + x;
                                
                                float filtValue=(scratchData_[offsetScratch] +
                                                 scratchData_[offsetScratch + levelWidth] ) * (462.0f/2048.0f);//The const expr devisions will be compiled away!
                                
                                filtValue+=(scratchData_[offsetScratch - levelWidth] +
                                            scratchData_[offsetScratch + (levelWidth<<1)] ) * (330.0f/2048.0f);
                                
                                filtValue+=(scratchData_[offsetScratch - (levelWidth<<1)] +
                                            scratchData_[offsetScratch + ((levelWidth<<1) + levelWidth)] ) * (165.0f/2048.0f);
                                
                                filtValue+=(scratchData_[offsetScratch - ((levelWidth<<1) + levelWidth)] +
                                            scratchData_[offsetScratch + (levelWidth<<2)] ) * (55.0f/2048.0f);
                                
                                filtValue+=(scratchData_[offsetScratch - (levelWidth<<2)] +
                                            scratchData_[offsetScratch + ((levelWidth<<2) + levelWidth)] ) * (11.0f/2048.0f);
                                
                                filtValue+=(scratchData_[offsetScratch - ((levelWidth<<2) + levelWidth)] +
                                            scratchData_[offsetScratch + ((levelWidth<<2) + (levelWidth<<1))] ) * (1.0f/2048.0f);
                                
                                imgData[lineOffset + x]=filtValue;
                            }
                        }
                        //=== ===
                    }//=== ===
                    
                    
                    {//=== Calculate scale space gradient images ===
                        float * const dxData=dxVec_[levelNum];
                        float * const dyData=dyVec_[levelNum];
                        float * const dSqRecipData=dSqRecipVec_[levelNum];
                        
                        for (ptrdiff_t y=1; y<(levelHeight - ((ptrdiff_t)1)); ++y)
                        {
                            const ptrdiff_t lineOffset=y*levelWidth;
                            
                            for (ptrdiff_t x=((ptrdiff_t)1); x<levelWidthMinus1; ++x)
                            {
                                const ptrdiff_t offset=lineOffset + x;
                                
                                const float v1=imgData[offset-levelWidth-1];
                                const float v2=imgData[offset-levelWidth];
                                const float v3=imgData[offset-levelWidth+1];
                                const float v4=imgData[offset-1];
                                //const float v5=imgData[offset];
                                const float v6=imgData[offset+1];
                                const float v7=imgData[offset+levelWidth-1];
                                const float v8=imgData[offset+levelWidth];
                                const float v9=imgData[offset+levelWidth+1];
                                
                                //Use Scharr operator for image gradient.
                                const float dx=(v3-v1)*(3.0f/32.0f) + (v6-v4)*(10.0f/32.0f) + (v9-v7)*(3.0f/32.0f);
                                const float dy=(v7-v1)*(3.0f/32.0f) + (v8-v2)*(10.0f/32.0f) + (v9-v3)*(3.0f/32.0f);
                                
                                dxData[offset]=dx;
                                dyData[offset]=dy;
                                dSqRecipData[offset]=1.0f/(dx*dx+dy*dy+0.0000001f);
                            }
                        }
                    }//=== ===
                }
            }//=== ===
            
            
            //===========================
            //=== Calculate h vectors ===
            
            float Hx=0.0f;
            float Hy=0.0f;
            
            for (ptrdiff_t levelNum=(numLevels_-1); levelNum>=0; --levelNum)
            {
                Hx*=2.0f;
                Hy*=2.0f;
                
                float const * const imgData=imgVec_[levelNum];
                float const * const refImgData=refImgVec_[levelNum];
                
                float const * const dxData=dxVec_[levelNum];
                float const * const dyData=dyVec_[levelNum];
                float const * const dSqRecipData=dSqRecipVec_[levelNum];
                
                const ptrdiff_t levelWidth=(croppedWidth>>levelNum);
                const ptrdiff_t levelHeight=(croppedHeight>>levelNum);
                const ptrdiff_t levelWidthMinus1=levelWidth - ((ptrdiff_t)1);
                
                
                for (size_t newtonRaphsonI=0; newtonRaphsonI<7; ++newtonRaphsonI)
                {
                    float dHx=0.0f;
                    float dHy=0.0f;
                    
                    float dHxWeight=0.0f;
                    float dHyWeight=0.0f;
                    
                    for (ptrdiff_t y=((ptrdiff_t)1); y<(levelHeight - ((ptrdiff_t)1)); ++y)
                    {
                        const ptrdiff_t lineOffset=y*levelWidth;
                        
                        for (ptrdiff_t x=((ptrdiff_t)1); x<levelWidthMinus1; ++x)
                        {
                            const ptrdiff_t offset=lineOffset + x;
                            
                            const float dSqRecip=dSqRecipData[offset];
                            
                            if (dSqRecip<(1.0f/0.0001f))
                            {
                                //=== calc bilinear filter fractions ===//
                                const float floor_hx=floorf(Hx);
                                const float floor_hy=floorf(Hy);
                                const ptrdiff_t int_hx=lroundf(floor_hx);
                                const ptrdiff_t int_hy=lroundf(floor_hy);
                                const float frac_hx=Hx - floor_hx;
                                const float frac_hy=Hy - floor_hy;
                                //=== ===//
                                
                                if (((x+int_hx)>((ptrdiff_t)1))&&((y+int_hy)>((ptrdiff_t)1))&&((x+int_hx+((ptrdiff_t)2))<levelWidth)&&((y+int_hy+((ptrdiff_t)2))<levelHeight))
                                {
                                    const ptrdiff_t offsetLT=offset + int_hx + int_hy * levelWidth;
                                    const float imgRef=bilinear(refImgData, offsetLT, levelWidth, frac_hx, frac_hy);
                                    
                                    const float imgDiff=imgData[offset]-imgRef;
                                    dHx+=(imgDiff*dxData[offset])*dSqRecip;
                                    dHy+=(imgDiff*dyData[offset])*dSqRecip;
                                    
                                    
                                    //Note: Could still weight the sum with the reciprocal of the second derivative dx2,dy2 ...
                                    dHxWeight+=1.0f;
                                    dHyWeight+=1.0f;
                                }
                            }
                        }
                    }
                    
                    if (dHxWeight>0.0f)
                    {
                        dHx/=dHxWeight;
                    } else
                    {
                        dHx=0.0f;
                    }
                    
                    if (dHyWeight>0.0f)
                    {
                        dHy/=dHyWeight;
                    } else
                    {
                        dHy=0.0f;
                    }
                    
                    Hx+=dHx;
                    Hy+=dHy;
                }
            }
            //===========================
            //===========================
            
            
            
            //=== Final results ===
            {
                float * const imgDataGF=imgVec_[0];
                
                for (ptrdiff_t y=0; y<=croppedHeight; ++y)
                {
                    const ptrdiff_t lineOffset=y*croppedWidth;
                    
                    for (ptrdiff_t x=0; x<croppedWidth; ++x)
                    {
                        const ptrdiff_t offset=lineOffset + x;
                        
                        const float hx=-Hx;
                        const float hy=-Hy;
                        
                        //=== Image Dewarping ===//
                        {
                            const float blend=0.0f;//1.0f/(1.0f+dL1*10.0f);
                            const float floor_hx=floorf(hx);
                            const float floor_hy=floorf(hy);
                            const ptrdiff_t int_hx=lroundf(floor_hx);
                            const ptrdiff_t int_hy=lroundf(floor_hy);
                            
                            if ( ((x+int_hx)>((ptrdiff_t)1)) && ((y+int_hy)>((ptrdiff_t)1)) && ((x+int_hx)<(croppedWidth-1)) && ((y+int_hy)<(croppedHeight-1)) )
                            {
                                finalImgData_[offset]*=blend;
                                
                                const float frac_hx=hx - floor_hx;
                                const float frac_hy=hy - floor_hy;
                                const ptrdiff_t offsetLT=offset + int_hx + int_hy * croppedWidth;
                                
                                finalImgData_[offset]+=bilinear(imgDataGF, offsetLT, croppedWidth, frac_hx, frac_hy)*(1.0f-blend);
                            }
                        }
                        //=== ===
                    }
                }
                
                
                //Copy finalImgData_ result to uncropped output image slot.
                //  The data is copied because of the lucky region accumulation that needs to happen in the result buffer.
                for (ptrdiff_t uncroppedY=startCroppedY; uncroppedY<=endCroppedY; ++uncroppedY)
                {
                    const ptrdiff_t uncroppedLineOffset=uncroppedY*uncroppedWidth + startCroppedX;
                    const ptrdiff_t croppedY=uncroppedY-startCroppedY;
                    const ptrdiff_t croppedLineOffset=croppedY * croppedWidth;
                    
                    memcpy((dataWrite+uncroppedLineOffset), (finalImgData_+croppedLineOffset), croppedWidth*sizeof(float));
                }
            }
        }
        
        ++frameNum_;
        
        //Stop stats measurement event.
        ProcessorStats_->tock();
        
        releaseWriteSlot();
        releaseReadSlot();
        
        return true;
    }
    
    return false;
}



