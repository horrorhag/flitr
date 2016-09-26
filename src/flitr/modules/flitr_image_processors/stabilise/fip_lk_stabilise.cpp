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
                               Mode outputMode,
                               uint32_t buffer_size) :
ImageProcessor(upStreamProducer, images_per_slot, buffer_size),
Title_(std::string("LK Stabilise")),
numLevels_(0), //Setup numLevels_ automatically in init().
scratchData_(0),
outputMode_(outputMode),
latestHx_(0.0),
latestHy_(0.0),
latestHFrameNumber_(0),
sumHx_(0.0f),
sumHy_(0.0f),
burnFx_(1.0f),
burnFy_(1.0f)
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

            // Pass the metadata from the read image to the write image.
            // By Default the base implementation will copy the pointer if no custom
            // pass function was set.
            if(PassMetadataFunction_ != nullptr)
            {
                imWrite->setMetadata(PassMetadataFunction_(imRead->metadata()));
            }
            
            const ImageFormat imFormat=getUpstreamFormat(imgNum);
            const ptrdiff_t uncroppedWidth=imFormat.getWidth();
            const ptrdiff_t uncroppedHeight=imFormat.getHeight();
            const size_t bytesPerPixel=imFormat.getBytesPerPixel();
            
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
                
                {//=== Update ref img before new data arrives. ===//
                    memcpy(refImgData, imgData, croppedWidth*croppedHeight*sizeof(float));
                }
                
                //=== Crop copy input data to level 0 of scale space ===//
                if (imFormat.getPixelFormat()==flitr::ImageFormat::FLITR_PIX_FMT_Y_F32)
                {
                    for (size_t y=startCroppedY; y<=endCroppedY; ++y)
                    {
                        const ptrdiff_t uncroppedLineOffset=y*uncroppedWidth + startCroppedX;
                        const ptrdiff_t croppedLineOffset=(y-startCroppedY)*croppedWidth;
                        memcpy(imgData+croppedLineOffset, dataRead+uncroppedLineOffset, croppedWidth*sizeof(float));
                    }
                } else
                    if (imFormat.getPixelFormat()==flitr::ImageFormat::FLITR_PIX_FMT_RGB_F32)
                    {
                        for (size_t y=startCroppedY; y<=endCroppedY; ++y)
                        {
                            const ptrdiff_t uncroppedLineOffset=(y*uncroppedWidth + startCroppedX)*3;
                            const ptrdiff_t croppedLineOffset=(y-startCroppedY)*croppedWidth;
                            
                            for (int x=0; x<croppedWidth; ++x)
                            {
                                imgData[croppedLineOffset+x]=dataRead[uncroppedLineOffset + x*3 + 1];//Use the green channel to stabilise!
                            }
                        }
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
                                                 imgDataHR[offsetHR + 1] ) * (462.0f/2048.0f);//The const expr devisions will be compiled/folded away!
                                
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
                                                 scratchData_[offsetScratch + levelWidth] ) * (462.0f/2048.0f);//The const expr devisions will be compiled/folded away!
                                
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
                                dSqRecipData[offset]=1.0f/(dx*dx+dy*dy+0.000000001f);
                            }
                        }
                    }//=== ===
                }
            }//=== ===
            
            
            //===========================
            //=== Calculate h vectors ===
            
            float Hx=0.0f;
            float Hy=0.0f;
            
            const ptrdiff_t levelsToSkip=1;
            
            for (ptrdiff_t levelNum=(numLevels_-1); levelNum>=levelsToSkip; --levelNum)
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
                    size_t hCount=0;
                    
                    for (ptrdiff_t y=((ptrdiff_t)1); y<(levelHeight - ((ptrdiff_t)1)); ++y)
                    {
                        const ptrdiff_t lineOffset=y*levelWidth;
                        
                        for (ptrdiff_t x=((ptrdiff_t)1); x<levelWidthMinus1; ++x)
                        {
                            const ptrdiff_t offset=lineOffset + x;
                            
                            const float dSqRecip=dSqRecipData[offset];
                            
                            if (dSqRecip<(1.0f/0.0001f)) //Only do processing when the image gradient is above a certain limit. The calculation seems inaccurate anyway for small gradients...
                            {
                                //=== calc bilinear filter fractions ===//
                                const float floor_hx=floorf(Hx);
                                const float floor_hy=floorf(Hy);
                                const ptrdiff_t int_hx=lroundf(floor_hx);
                                const ptrdiff_t int_hy=lroundf(floor_hy);
                                const float frac_hx=Hx - floor_hx;
                                const float frac_hy=Hy - floor_hy;
                                //=== ===//
                                
                                if (((x+int_hx)>((ptrdiff_t)1))&&((y+int_hy)>((ptrdiff_t)1))&&
                                    ((x+int_hx+((ptrdiff_t)2))<levelWidth)&&((y+int_hy+((ptrdiff_t)2))<levelHeight))
                                {
                                    const ptrdiff_t offsetLT=offset + int_hx + int_hy * levelWidth;
                                    
                                    //Moving&interpolating the reference image allows one to avoid having to bilinear filter the img gradients dx and dy!!!
                                    const float imgRef=bilinear(refImgData, offsetLT, levelWidth, frac_hx, frac_hy);
                                    
                                    const float imgDiff=imgData[offset]-imgRef;
                                    
                                    dHx+=(imgDiff*dxData[offset])*dSqRecip;
                                    dHy+=(imgDiff*dyData[offset])*dSqRecip;
                                    ++hCount;
                                }
                            }
                        }
                    }
                    
                    if (hCount>0)
                    {
                        const float recipHCount=1.0f/hCount;
                        Hx+=dHx*recipHCount;
                        Hy+=dHy*recipHCount;
                    }
                }
            }
            //===========================
            //===========================
            
            
            Hx*=powf(2.0f, float(levelsToSkip));
            Hy*=powf(2.0f, float(levelsToSkip));
            
            
            {
                std::lock_guard<std::mutex> scopedLock(latestHMutex_);
                
                //Save latest H vector.
                latestHx_=Hx;
                latestHy_=Hy;
                latestHFrameNumber_=frameNumber_;
            }
            
            
            if (frameNumber_>2)
            {
                sumHx_+=latestHx_;
                sumHy_+=latestHy_;
                
                sumHx_*=burnFx_;
                sumHy_*=burnFy_;
            }
            
            
            //=== Results ===
            memset(dataWrite, 0, uncroppedWidth*uncroppedHeight*bytesPerPixel);//Clear the output buffer.
            
            if (outputMode_==Mode::CROP_FILTER_SUBPIXELSTAB)
            {
                float const * const imgDataGF=imgVec_[0];
                
                const float hx=-sumHx_;
                const float hy=-sumHy_;
                const float floor_hx=floorf(hx);
                const float floor_hy=floorf(hy);
                const ptrdiff_t int_hx=lroundf(floor_hx);
                const ptrdiff_t int_hy=lroundf(floor_hy);
                const float frac_hx=hx - floor_hx;
                const float frac_hy=hy - floor_hy;
                
                if (imFormat.getPixelFormat()==flitr::ImageFormat::FLITR_PIX_FMT_Y_F32)
                {
                    for (ptrdiff_t uncroppedY=startCroppedY; uncroppedY<=endCroppedY; ++uncroppedY)
                    {
                        const ptrdiff_t uncroppedLineOffset=uncroppedY*uncroppedWidth + startCroppedX;
                        const ptrdiff_t croppedY=uncroppedY-startCroppedY;
                        const ptrdiff_t croppedLineOffset=croppedY * croppedWidth;
                        
                        for (ptrdiff_t croppedX=0; croppedX<croppedWidth; ++croppedX)
                        {
                            const ptrdiff_t croppedOffset=croppedLineOffset + croppedX;
                            const ptrdiff_t unCroppedOffset=uncroppedLineOffset + croppedX;
                            
                            if ( ((croppedX+int_hx)>((ptrdiff_t)1)) && ((croppedY+int_hy)>((ptrdiff_t)1)) && ((croppedX+int_hx)<(croppedWidth-1)) && ((croppedY+int_hy)<(croppedHeight-1)) )
                            {
                                const ptrdiff_t offsetLT=croppedOffset + int_hx + int_hy * croppedWidth;
                                
                                dataWrite[unCroppedOffset]=bilinear(imgDataGF, offsetLT, croppedWidth, frac_hx, frac_hy);
                            }
                        }
                    }
                }
            } else
                if (outputMode_==Mode::SUBPIXELSTAB)
                {
                    const float hx=-sumHx_;
                    const float hy=-sumHy_;
                    const float floor_hx=floorf(hx);
                    const float floor_hy=floorf(hy);
                    const ptrdiff_t int_hx=lroundf(floor_hx);
                    const ptrdiff_t int_hy=lroundf(floor_hy);
                    const float frac_hx=hx - floor_hx;
                    const float frac_hy=hy - floor_hy;
                    
                    if (imFormat.getPixelFormat()==flitr::ImageFormat::FLITR_PIX_FMT_Y_F32)
                    {
                        for (ptrdiff_t uncroppedY=0; uncroppedY<uncroppedHeight; ++uncroppedY)
                        {
                            const ptrdiff_t uncroppedLineOffset=uncroppedY*uncroppedWidth;
                            
                            for (ptrdiff_t uncroppedX=0; uncroppedX<uncroppedWidth; ++uncroppedX)
                            {
                                const ptrdiff_t unCroppedOffset=uncroppedLineOffset + uncroppedX;
                                
                                if (((uncroppedX+int_hx)>((ptrdiff_t)1)) && ((uncroppedY+int_hy)>((ptrdiff_t)1)) &&
                                    ((uncroppedX+int_hx)<(uncroppedWidth-1)) && ((uncroppedY+int_hy)<(uncroppedHeight-1)) )
                                {
                                    const ptrdiff_t offsetLT=unCroppedOffset + int_hx + int_hy * uncroppedWidth;
                                    
                                    dataWrite[unCroppedOffset]=bilinear(dataRead, offsetLT, uncroppedWidth, frac_hx, frac_hy);
                                }
                            }
                        }
                    } else
                        if (imFormat.getPixelFormat()==flitr::ImageFormat::FLITR_PIX_FMT_RGB_F32)
                        {
                            //Not implemented yet.
                        }
                } else
                    if (outputMode_==Mode::INTSTAB)
                    {
                        const ptrdiff_t int_hx=lroundf(-sumHx_);
                        const ptrdiff_t int_hy=lroundf(-sumHy_);
                        
                        if (imFormat.getPixelFormat()==flitr::ImageFormat::FLITR_PIX_FMT_Y_F32)
                        {
                            for (ptrdiff_t uncroppedY=0; uncroppedY<uncroppedHeight; ++uncroppedY)
                            {
                                const ptrdiff_t uncroppedLineOffset=uncroppedY*uncroppedWidth;
                                
                                for (ptrdiff_t uncroppedX=0; uncroppedX<uncroppedWidth; ++uncroppedX)
                                {
                                    const ptrdiff_t unCroppedOffset=uncroppedLineOffset + uncroppedX;
                                    
                                    if (((uncroppedX+int_hx)>((ptrdiff_t)1)) && ((uncroppedY+int_hy)>((ptrdiff_t)1)) &&
                                        ((uncroppedX+int_hx)<(uncroppedWidth-1)) && ((uncroppedY+int_hy)<(uncroppedHeight-1)) )
                                    {
                                        const ptrdiff_t offsetLT=unCroppedOffset + int_hx + int_hy * uncroppedWidth;
                                        dataWrite[unCroppedOffset]=dataRead[offsetLT];
                                    }
                                }
                            }
                        } else
                        if (imFormat.getPixelFormat()==flitr::ImageFormat::FLITR_PIX_FMT_RGB_F32)
                        {
                            for (ptrdiff_t uncroppedY=0; uncroppedY<uncroppedHeight; ++uncroppedY)
                            {
                                const ptrdiff_t uncroppedLineOffset=uncroppedY*uncroppedWidth;
                                
                                for (ptrdiff_t uncroppedX=0; uncroppedX<uncroppedWidth; ++uncroppedX)
                                {
                                    const ptrdiff_t unCroppedOffset=uncroppedLineOffset + uncroppedX;
                                    
                                    if (((uncroppedX+int_hx)>((ptrdiff_t)1)) && ((uncroppedY+int_hy)>((ptrdiff_t)1)) &&
                                        ((uncroppedX+int_hx)<(uncroppedWidth-1)) && ((uncroppedY+int_hy)<(uncroppedHeight-1)) )
                                    {
                                        const ptrdiff_t offsetLT=unCroppedOffset + int_hx + int_hy * uncroppedWidth;
                                        dataWrite[unCroppedOffset*3+0]=dataRead[offsetLT*3+0];
                                        dataWrite[unCroppedOffset*3+1]=dataRead[offsetLT*3+1];
                                        dataWrite[unCroppedOffset*3+2]=dataRead[offsetLT*3+2];
                                    }
                                }
                            }
                        }
                    } else
                        if (outputMode_==Mode::NOTRANSFORM)
                        {
                            //Copy input to output image slot.
                            memcpy(dataWrite, dataRead, uncroppedWidth*uncroppedHeight*bytesPerPixel);
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



