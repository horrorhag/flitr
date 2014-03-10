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

#include <flitr/modules/flitr_image_processors/dewarp/fip_lk_dewarp.h>

#include <iostream>
#include <fstream>



using namespace flitr;
using std::tr1::shared_ptr;

FIPLKDewarp::FIPLKDewarp(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                     uint32_t buffer_size) :
ImageProcessor(upStreamProducer, images_per_slot, buffer_size),
frameNum_(0),
numLevels_(4),
scratchData_(0),
//finalHxData_(0),
//finalHyData_(0),
finalImgData_(0)
//m2Data_(0),
//avrgData_(0),
//n_(0.0)
{
    //Setup image format being produced to downstream.
    for (uint32_t i=0; i<images_per_slot; ++i) {
        ImageFormat imgFormat=upStreamProducer.getFormat(i);
        ImageFormat_.push_back(imgFormat);
    }
}

FIPLKDewarp::~FIPLKDewarp()
{
    delete [] scratchData_;
    //delete [] finalHxData_;
    //delete [] finalHyData_;
    delete [] finalImgData_;
    
    //delete [] m2Data_;
    //delete [] avrgData_;
    
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
        
        delete [] hxVec_.back();
        hxVec_.pop_back();
        
        delete [] hyVec_.back();
        hyVec_.pop_back();
    }
}

bool FIPLKDewarp::init()
{
    bool rValue=ImageProcessor::init();
    //Note: SharedImageBuffer of downstream producer is initialised with storage in ImageProcessor::init.
    
    const size_t imgNum=0;
    {
        const ImageFormat imFormat=getUpstreamFormat(imgNum);
        
        const ptrdiff_t width=imFormat.getWidth();
        const ptrdiff_t height=imFormat.getHeight();
        
        //=== Image will be cropped so that at least all levels of the pyramid is divisible by 2 ===
        const ptrdiff_t croppedWidth=(width>>(numLevels_-1))<<(numLevels_-1);
        const ptrdiff_t croppedHeight=(height>>(numLevels_-1))<<(numLevels_-1);
        //const ptrdiff_t croppedWidth=1 << ((int)log2f(width));
        //const ptrdiff_t croppedHeight=1 << ((int)log2f(height));
        //=== ===
        
        scratchData_=new float[(croppedWidth*croppedHeight)<<1];
        memset(scratchData_, 0, ((croppedWidth*croppedHeight)<<1)*sizeof(float));
        
        finalImgData_=new float[croppedWidth*croppedHeight];
        memset(finalImgData_, 0, croppedWidth*croppedHeight*sizeof(float));
        
        //finalHxData_=new float[croppedWidth*croppedHeight];
        //memset(finalHxData_, 0, croppedWidth*croppedHeight*sizeof(float));
        
        //finalHyData_=new float[croppedWidth*croppedHeight];
        //memset(finalHyData_, 0, croppedWidth*croppedHeight*sizeof(float));
        
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
            
            hxVec_.push_back(new float[(croppedWidth>>levelNum) * (croppedHeight>>levelNum)]);
            memset(hxVec_.back(), 0, (croppedWidth>>levelNum) * (croppedHeight>>levelNum) * sizeof(float));
            
            hyVec_.push_back(new float[(croppedWidth>>levelNum) * (croppedHeight>>levelNum)]);
            memset(hyVec_.back(), 0, (croppedWidth>>levelNum) * (croppedHeight>>levelNum) * sizeof(float));
        }
        
        //Push zero h vectors to ordinal numLevels_+1
        hxVec_.push_back(new float[(croppedWidth>>numLevels_) * (croppedHeight>>numLevels_)]);
        memset(hxVec_.back(), 0, (croppedWidth>>numLevels_) * (croppedHeight>>numLevels_) * sizeof(float));
        hyVec_.push_back(new float[(croppedWidth>>numLevels_) * (croppedHeight>>numLevels_)]);
        memset(hyVec_.back(), 0, (croppedWidth>>numLevels_) * (croppedHeight>>numLevels_) * sizeof(float));
    }
    
    return rValue;
}


inline float bilinear(float const * const data, const ptrdiff_t offsetLT, const ptrdiff_t width, const float fx, const float fy)
{
    return
    (data[offsetLT] * (1.0f-fx) + data[offsetLT+((ptrdiff_t)1)] * fx) * (1.0f-fy) +
    (data[offsetLT+width] * (1.0f-fx) + data[offsetLT+((ptrdiff_t)1)+width] * fx) * fy;
}


bool FIPLKDewarp::trigger()
{
    if ((getNumReadSlotsAvailable())&&(getNumWriteSlotsAvailable()))
    {//There are images to consume and the downstream producer has space to produce. BUG: Sometime we get in here and then the reserve write failes later on!!!
        
        std::vector<Image**> imvRead=reserveReadSlot();
        
        if (imvRead.size()>0)
        {//Check to ensure that we can read to work round bug mentioned above!
            std::vector<Image**> imvWrite=reserveWriteSlot();
            
            if (imvWrite.size()>0)
            {//Check to ensure that we can write to work round bug mentioned above! The work around might drop frames!!!
                
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
                    
                    //****************
                    const float refImgFiltConst=(frameNum_<3) ? 1.0f : 0.05f;
                    //****************
                    
                    {//=== Copy cropped input to level 0 of scale space ===
                        {//Update ref/avrg img before new data arrives.
                            float * const imgData=imgVec_[0];
                            float * const refImgData=refImgVec_[0];
                            
                            for (ptrdiff_t y=0; y<croppedHeight; ++y)
                            {
                                const ptrdiff_t lineOffset=y*croppedWidth;
                                
                                for (ptrdiff_t x=0; x<croppedWidth; ++x)
                                {
                                    const ptrdiff_t offset=lineOffset + x;
                                    refImgData[offset]*=1.0f-refImgFiltConst;
                                    refImgData[offset]+=imgData[offset] * refImgFiltConst;
                                }
                            }
                        }
                        
                        for (size_t y=startCroppedY; y<=endCroppedY; ++y)
                        {
                            const ptrdiff_t uncroppedLineOffset=y*uncroppedWidth + startCroppedX;
                            const ptrdiff_t croppedLineOffset=(y-startCroppedY)*croppedWidth;
                            memcpy((imgVec_[0]+croppedLineOffset), (dataRead+uncroppedLineOffset), croppedWidth*sizeof(float));
                        }
                    }//=== ===
                    
                    {//=== Calculate scale space pyramid. ===
                        for (size_t levelNum=0; levelNum<numLevels_; ++levelNum)
                        {
                            float * const imgData=imgVec_[levelNum];
                            
                            const ptrdiff_t levelHeight=croppedHeight >> levelNum;
                            const ptrdiff_t levelWidth=croppedWidth >> levelNum;
                            const ptrdiff_t levelWidthMinus1 = levelWidth - ((ptrdiff_t)1);
                            const ptrdiff_t levelWidthMinus5 = levelWidth - ((ptrdiff_t)5);
                            
                            //=== Calculate the scale space images ===
                            if (levelNum>0)//First level (incoming data) is not downsampled.
                            {
                                {//Update ref/avrg img before new data arrives.
                                    float * const refImgData=refImgVec_[levelNum];
                                    
                                    for (ptrdiff_t y=0; y<levelHeight; ++y)
                                    {
                                        const ptrdiff_t lineOffset=y*levelWidth;
                                        
                                        for (ptrdiff_t x=0; x<levelWidth; ++x)
                                        {
                                            const ptrdiff_t offset=lineOffset + x;
                                            
                                            refImgData[offset]*=1.0f-refImgFiltConst;
                                            refImgData[offset]+=imgData[offset] * refImgFiltConst;
                                        }
                                    }
                                }
                                
                                float const * const imgDataHR=imgVec_[levelNum-1];
                                const ptrdiff_t heightHR=croppedHeight >> (levelNum-1);
                                const ptrdiff_t widthHR=croppedWidth >> (levelNum-1);
                                
                                //=== Seperable Gaussian first pass - down filter x ===
                                for (size_t y=0; y<heightHR; ++y)
                                {
                                    const ptrdiff_t lineOffsetScratch=y * levelWidth;
                                    const ptrdiff_t lineOffsetHR=y * widthHR;
                                    
                                    for (ptrdiff_t x=5; x<levelWidthMinus5; ++x)
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
                                for (ptrdiff_t y=((ptrdiff_t)5); y<(levelHeight-((ptrdiff_t)5)); ++y)
                                {
                                    const ptrdiff_t lineOffset=y * levelWidth;
                                    const ptrdiff_t lineOffsetScratch=(y<<1) * levelWidth;
                                    
                                    for (ptrdiff_t x=5; x<levelWidthMinus5; ++x)
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
                                        const float dSq=dx*dx+dy*dy;
                                        
                                        dxData[offset]=dx;
                                        dyData[offset]=dy;
                                        dSqRecipData[offset]=1.0f/dSq;
                                    }
                                }
                            }//=== ===
                        }
                    }//=== ===
                    
                    
                    //===========================
                    //=== Calculate h vectors ===
                    for (ptrdiff_t levelNum=(numLevels_-1); levelNum>=0; --levelNum)
                    {
                        float const * const imgData=imgVec_[levelNum];
                        float const * const refImgData=refImgVec_[levelNum];
                        
                        float const * const dxData=dxVec_[levelNum];
                        float const * const dyData=dyVec_[levelNum];
                        float const * const dSqRecipData=dSqRecipVec_[levelNum];
                        
                        float * const hxData=hxVec_[levelNum];
                        float * const hyData=hyVec_[levelNum];
                        float const * const hxDataLR=hxVec_[levelNum+1];
                        float const * const hyDataLR=hyVec_[levelNum+1];
                        
                        const ptrdiff_t levelWidth=(croppedWidth>>levelNum);
                        const ptrdiff_t levelHeight=(croppedHeight>>levelNum);
                        const ptrdiff_t levelWidthLR=(levelWidth>>1);
                        const ptrdiff_t levelWidthMinus1=levelWidth - ((ptrdiff_t)1);
                        
                        //=== Start with h-vectors from lower resolution level ===//
                        for (ptrdiff_t y=1; y<(levelHeight - ((ptrdiff_t)1)); ++y)
                        {
                            const ptrdiff_t lineOffset=y*levelWidth;
                            const ptrdiff_t lineOffsetLR=(y>>1)*levelWidthLR;
                            
                            for (ptrdiff_t x=((ptrdiff_t)1); x<levelWidthMinus1; ++x)
                            {
                                const ptrdiff_t offsetLR=lineOffsetLR + (x>>((ptrdiff_t)1));
                                
                                const ptrdiff_t offsetLT=offsetLR-levelWidthLR-((ptrdiff_t)1)+(y&((ptrdiff_t)1))*levelWidthLR+(x&((ptrdiff_t)1));
                                const float fx=(x&((ptrdiff_t)1))*(-0.5f)+0.75f;
                                const float fy=(y&((ptrdiff_t)1))*(-0.5f)+0.75f;
                                
                                const float hx=/*hxDataLR[offsetLR]*2.0f;*/bilinear(hxDataLR, offsetLT, levelWidthLR, fx, fy) * 2.0f;
                                const float hy=/*hyDataLR[offsetLR]*2.0f;*/bilinear(hyDataLR, offsetLT, levelWidthLR, fx, fy) * 2.0f;
                                
                                const ptrdiff_t offset=lineOffset + x;
                                hxData[offset]=hx;
                                hyData[offset]=hy;
                            }
                        }
                        //=== ===//
                        
                        if (levelNum>0)//No need to refine h-vectors at level 0 if super res is not attempted!
                        {
                            for (size_t newtonRaphsonI=0; newtonRaphsonI<4; ++newtonRaphsonI)
                            {
                                for (ptrdiff_t y=((ptrdiff_t)1); y<(levelHeight - ((ptrdiff_t)1)); ++y)
                                {
                                    const ptrdiff_t lineOffset=y*levelWidth;
                                    
                                    for (ptrdiff_t x=((ptrdiff_t)1); x<levelWidthMinus1; ++x)
                                    {
                                        const ptrdiff_t offset=lineOffset + x;
                                        
                                        const float dSqRecip=dSqRecipData[offset];
                                        
                                        if (dSqRecip<(1.0f/0.0005f))
                                        {
                                            float hx=hxData[offset];
                                            float hy=hyData[offset];
                                            
                                            //=== calc bilinear filter fractions ===//
                                            const float floor_hx=floorf(hx);
                                            const float floor_hy=floorf(hy);
                                            const ptrdiff_t int_hx=lroundf(floor_hx);
                                            const ptrdiff_t int_hy=lroundf(floor_hy);
                                            const float frac_hx=hx - floor_hx;
                                            const float frac_hy=hy - floor_hy;
                                            //=== ===//
                                            
                                            if (((x+int_hx)>((ptrdiff_t)1))&&((y+int_hy)>((ptrdiff_t)1))&&((x+int_hx+((ptrdiff_t)2))<levelWidth)&&((y+int_hy+((ptrdiff_t)2))<levelHeight))
                                            {
                                                const ptrdiff_t offsetLT=offset + int_hx + int_hy * levelWidth;
                                                const float imgRef=bilinear(refImgData, offsetLT, levelWidth, frac_hx, frac_hy);
                                                
                                                const float imgDiff=imgData[offset]-imgRef;
                                                hx+=(imgDiff*dxData[offset])*dSqRecip;
                                                hy+=(imgDiff*dyData[offset])*dSqRecip;
                                            }
                                            
                                            hxData[offset]=hx;
                                            hyData[offset]=hy;
                                        }
                                    }
                                }
                                
                                
                                {//=== Smooth/regularise the vector field of this iteration using Gaussian filters in x and y ===//
                                    const ptrdiff_t levelWidthMinus5=levelWidth - ((ptrdiff_t)5);
                                    const ptrdiff_t levelHeightMinus5=levelHeight - ((ptrdiff_t)5);
                                    
                                    for (ptrdiff_t y=0; y<levelHeight; ++y)
                                    {
                                        const ptrdiff_t lineOffset=y*levelWidth;
                                        const ptrdiff_t lineOffsetHy=lineOffset+levelWidth*levelHeight;
                                        
                                        for (ptrdiff_t x=5; x<levelWidthMinus5; ++x)
                                        {
                                            const ptrdiff_t offset=lineOffset + x;
                                            
                                            float filtValue=(hxData[offset]) * (252.0f/1002.0f);
                                            
                                            filtValue+=(hxData[offset - 1] +
                                                        hxData[offset + 1]) * (210.0f/1002.0f);
                                            
                                            filtValue+=(hxData[offset - 2] +
                                                        hxData[offset + 2]) * (120.0f/1002.0f);
                                            
                                            filtValue+=(hxData[offset - 3] +
                                                        hxData[offset + 3] ) * (45.0f/1002.0f);
                                            
                                            /*
                                             filtValue+=( hxData[offset - 4] +
                                             hxData[offset + 4] ) * (10.0f/1002.0f);
                                             
                                             filtValue+=( hxData[offset - 5] +
                                             hxData[offset + 5] ) * (1.0f/1002.0f);
                                             */
                                            scratchData_[offset]=filtValue;
                                            
                                            
                                            filtValue=( hyData[offset] ) * (252.0f/1002.0f);
                                            
                                            filtValue+=( hyData[offset - ((ptrdiff_t)1)] +
                                                        hyData[offset + ((ptrdiff_t)1)] ) * (210.0f/1002.0f);
                                            
                                            filtValue+=( hyData[offset - ((ptrdiff_t)2)] +
                                                        hyData[offset + ((ptrdiff_t)2)] ) * (120.0f/1002.0f);
                                            
                                            filtValue+=( hyData[offset - ((ptrdiff_t)3)] +
                                                        hyData[offset + ((ptrdiff_t)3)] ) * (45.0f/1002.0f);
                                            
                                            /*
                                             filtValue+=( hyData[offset - ((ptrdiff_t)4)] +
                                             hyData[offset + ((ptrdiff_t)4)] ) * (10.0f/1002.0f);
                                             
                                             filtValue+=( hyData[offset - ((ptrdiff_t)5)] +
                                             hyData[offset + ((ptrdiff_t)5)] ) * (1.0f/1002.0f);
                                             */
                                            scratchData_[lineOffsetHy + x]=filtValue;
                                        }
                                    }
                                    
                                    for (ptrdiff_t y=5; y<levelHeightMinus5; ++y)
                                    {
                                        const ptrdiff_t lineOffset=y*levelWidth;
                                        const ptrdiff_t lineOffsetHy=lineOffset+levelWidth*levelHeight;
                                        
                                        for (ptrdiff_t x=0; x<levelWidth; ++x)
                                        {
                                            const ptrdiff_t offset=lineOffset + x;
                                            
                                            float filtValue=( scratchData_[offset] ) * (252.0f/1002.0f);
                                            
                                            filtValue+=( scratchData_[offset - levelWidth] +
                                                        scratchData_[offset + levelWidth] ) * (210.0f/1002.0f);
                                            
                                            filtValue+=( scratchData_[offset - (levelWidth<<1)] +
                                                        scratchData_[offset + (levelWidth<<1)] ) * (120.0f/1002.0f);
                                            
                                            filtValue+=( scratchData_[offset - ((levelWidth<<1)+levelWidth)] +
                                                        scratchData_[offset + ((levelWidth<<1)+levelWidth)] ) * (45.0f/1002.0f);
                                            /*
                                             filtValue+=( scratchData_[offset - (levelWidth<<2)] +
                                             scratchData_[offset + (levelWidth<<2)] ) * (10.0f/1002.0f);
                                             
                                             filtValue+=( scratchData_[offset - ((levelWidth<<2)+levelWidth)] +
                                             scratchData_[offset + ((levelWidth<<2)+levelWidth)] ) * (1.0f/1002.0f);
                                             */
                                            hxData[offset]=filtValue;
                                            
                                            
                                            const ptrdiff_t offsetHy=lineOffsetHy + x;
                                            
                                            filtValue=( scratchData_[offsetHy] ) * (252.0f/1002.0f);
                                            
                                            filtValue+=( scratchData_[offsetHy - levelWidth] +
                                                        scratchData_[offsetHy + levelWidth] ) * (210.0f/1002.0f);
                                            
                                            filtValue+=( scratchData_[offsetHy - (levelWidth<<1)] +
                                                        scratchData_[offsetHy + (levelWidth<<1)] ) * (120.0f/1002.0f);
                                            
                                            filtValue+=( scratchData_[offsetHy - ((levelWidth<<1)+levelWidth)] +
                                                        scratchData_[offsetHy + ((levelWidth<<1)+levelWidth)] ) * (45.0f/1002.0f);
                                            /*
                                             filtValue+=( scratchData_[offsetHy - (levelWidth<<2)] +
                                             scratchData_[offsetHy + (levelWidth<<2)] ) * (10.0f/1002.0f);
                                             
                                             filtValue+=( scratchData_[offsetHy - ((levelWidth<<2)+levelWidth)] +
                                             scratchData_[offsetHy + ((levelWidth<<2)+levelWidth)] ) * (1.0f/1002.0f);
                                             */
                                            hyData[offset]=filtValue;
                                        }
                                    }
                                }//=== ===
                            }
                        }
                    }
                    //===========================
                    //===========================
                    
                    //Update the standard deviation's M2.
                    /*
                     {
                     size_t levelNum=1;
                     
                     float * const hxDataGF=hxDataVec_[levelNum];
                     float * const hyDataGF=hyDataVec_[levelNum];
                     
                     for (ptrdiff_t y=startCroppedY; y<=endCroppedY; ++y)
                     {
                     const ptrdiff_t lineOffset=y*uncroppedWidth + startCroppedX;
                     const ptrdiff_t levelLineOffset=((y-startCroppedY)>>levelNum) * (croppedWidth>>levelNum);
                     
                     for (ptrdiff_t x=0; x<croppedWidth; ++x)
                     {
                     const ptrdiff_t offset=lineOffset + x;
                     const ptrdiff_t levelOffset=levelLineOffset + (x>>levelNum);
                     
                     double h=hxDataGF[levelOffset]*hxDataGF[levelOffset];
                     
                     n_+=1.0;
                     const double delta=h - avrgData_[offset];
                     avrgData_[offset]+=delta/n_;
                     m2Data_[offset]+=delta*(h)-avrgData_[offset];
                     
                     double variance=m2Data_[offset]/(n_ - 1.0);
                     dataWrite[offset]=sqrt(variance)*100.0;
                     }
                     }
                     
                     FILE *fp=fopen("dewarp.ieee", "wb");
                     
                     uint32_t ui32Width=uncroppedWidth;
                     uint32_t ui32Height=uncroppedHeight;
                     fwrite((unsigned char *)(&ui32Width), sizeof(uint32_t), 1, fp);
                     fwrite((unsigned char *)(&ui32Height), sizeof(uint32_t), 1, fp);
                     fwrite((unsigned char *)dataWrite, sizeof(float), uncroppedWidth*uncroppedHeight, fp);
                     
                     fclose(fp);
                     
                     }
                     */
                    
                    //=== Final results ===
                    //if (frameNum_>=2)
                    {
                        float * const imgDataGF=imgVec_[0];
                        float * const refImgDataGF=refImgVec_[0];
                        float * const dxDataGF=dxVec_[0];
                        float * const dyDataGF=dyVec_[0];
                        float * const hxDataGF=hxVec_[0];
                        float * const hyDataGF=hyVec_[0];
                        
                        /*
                         for (ptrdiff_t y=0; y<=croppedHeight; ++y)
                         {
                         const ptrdiff_t lineOffset=y*croppedWidth;
                         
                         for (ptrdiff_t x=0; x<croppedWidth; ++x)
                         {
                         const ptrdiff_t offset=lineOffset + x;
                         
                         finalHxData_[offset]=hxDataGF[offset];
                         finalHyData_[offset]=hyDataGF[offset];
                         }
                         }
                         */
                        
                        for (ptrdiff_t y=0; y<=croppedHeight; ++y)
                        {
                            const ptrdiff_t lineOffset=y*croppedWidth;
                            
                            for (ptrdiff_t x=0; x<croppedWidth; ++x)
                            {
                                const ptrdiff_t offset=lineOffset + x;
                                
                                //=== calc bilinear filter fractions ===//
                                const float hx=-hxDataGF[offset];
                                const float hy=-hyDataGF[offset];
                                const float dx=dxDataGF[offset];
                                const float dy=dyDataGF[offset];
                                
                                const float floor_hx=floorf(hx);
                                const float floor_hy=floorf(hy);
                                const ptrdiff_t int_hx=lroundf(floor_hx);
                                const ptrdiff_t int_hy=lroundf(floor_hy);
                                
                                if ( ((x+int_hx)>((ptrdiff_t)1)) && ((y+int_hy)>((ptrdiff_t)1)) && ((x+int_hx)<(croppedWidth-1)) && ((y+int_hy)<(croppedHeight-1)) )
                                {
                                    float blend=1.0f/(1.0f+(fabsf(dx)+fabsf(dy))*10.0f);
                                    finalImgData_[offset]*=blend;
                                    
                                    const float frac_hx=hx - floor_hx;
                                    const float frac_hy=hy - floor_hy;
                                    const ptrdiff_t offsetLT=offset + int_hx + int_hy * croppedWidth;
                                    
                                    finalImgData_[offset]+=bilinear(imgDataGF, offsetLT, croppedWidth, frac_hx, frac_hy)*(1.0f-blend);
                                }
                                //=== ===//
                                
                                //Some testing code goes here:
                                //finalImgData_[offset]=fabsf(refImgDataGF[offset]);
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
                
                releaseWriteSlot();
            }
            releaseReadSlot();
        }
    }
    
    //Stop stats measurement event.
    ProcessorStats_->tock();
    
    return true;
}

