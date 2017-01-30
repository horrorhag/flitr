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

#include <math.h>


using namespace flitr;
using std::shared_ptr;



FIPLKDewarp::FIPLKDewarp(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                         const float avrgImageLongevity,
                         uint32_t buffer_size) :
ImageProcessor(upStreamProducer, images_per_slot, buffer_size),
_enabled(true),
_title(std::string("LK Dewarp")),
avrgImageLongevity_(avrgImageLongevity),
recipGradientThreshold_(1.0f / 0.00025f),
numLevels_(5),//Num levels searched for scint motion.
gaussianFilter_(0.5f, 3),
gaussianDownsample_(0.5f, 2),
gaussianReguFilter_(2.5f, 11),
scratchData_(0),
inputImgDataR_(0),
inputImgDataG_(0),
inputImgDataB_(0),
finalImgDataR_(0),
finalImgDataG_(0),
finalImgDataB_(0)
{
    ProcessorStats_->setID("ImageProcessor::FIPLKDewarp");
    //Setup image format being produced to downstream.
    for (uint32_t i=0; i<images_per_slot; ++i)
    {
        ImageFormat imgFormat=upStreamProducer.getFormat(i);
        ImageFormat_.push_back(imgFormat);
    }
}

FIPLKDewarp::~FIPLKDewarp()
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
    
    delete [] finalImgDataR_;
    delete [] finalImgDataG_;
    delete [] finalImgDataB_;
    
    delete [] inputImgDataR_;
    delete [] inputImgDataG_;
    delete [] inputImgDataB_;
    
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
    
    //delete [] avrgHxData_;
    //delete [] avrgHyData_;
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
        
        scratchData_=new float[width*height];
        memset(scratchData_, 0, (width*height)*sizeof(float));
        
        inputImgDataR_=new float[croppedWidth*croppedHeight];
        memset(inputImgDataR_, 0, croppedWidth*croppedHeight*sizeof(float));
        inputImgDataG_=new float[croppedWidth*croppedHeight];
        memset(inputImgDataG_, 0, croppedWidth*croppedHeight*sizeof(float));
        inputImgDataB_=new float[croppedWidth*croppedHeight];
        memset(inputImgDataB_, 0, croppedWidth*croppedHeight*sizeof(float));
        
        finalImgDataR_=new float[croppedWidth*croppedHeight];
        memset(finalImgDataR_, 0, croppedWidth*croppedHeight*sizeof(float));
        finalImgDataG_=new float[croppedWidth*croppedHeight];
        memset(finalImgDataG_, 0, croppedWidth*croppedHeight*sizeof(float));
        finalImgDataB_=new float[croppedWidth*croppedHeight];
        memset(finalImgDataB_, 0, croppedWidth*croppedHeight*sizeof(float));
        
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
        
        //avrgHxData_=new float[croppedWidth * croppedHeight];
        //memset(avrgHxData_, 0, croppedWidth * croppedHeight * sizeof(float));
        
        //avrgHyData_=new float[croppedWidth * croppedHeight];
        //memset(avrgHyData_, 0, croppedWidth * croppedHeight * sizeof(float));
        
        //Push zero h vectors to ordinal numLevels_+1
        hxVec_.push_back(new float[(croppedWidth>>numLevels_) * ((croppedHeight>>numLevels_)+2)]);
        memset(hxVec_.back(), 0, (croppedWidth>>numLevels_) * ((croppedHeight>>numLevels_)+2) * sizeof(float));
        hyVec_.push_back(new float[(croppedWidth>>numLevels_) * ((croppedHeight>>numLevels_)+2)]);
        memset(hyVec_.back(), 0, (croppedWidth>>numLevels_) * ((croppedHeight>>numLevels_)+2) * sizeof(float));
    }
    
    return rValue;
}


bool FIPLKDewarp::trigger()
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
            
            // Pass the metadata from the read image to the write image.
            // By Default the base implementation will copy the pointer if no custom
            // pass function was set.
            if(PassMetadataFunction_ != nullptr)
            {
                imWrite->setMetadata(PassMetadataFunction_(imRead->metadata()));
            }
            
            const ImageFormat imFormat=getUpstreamFormat(imgNum);
            
            if (!_enabled)
            {//Pass not enabled! Just copy the data from input to output.
                uint8_t const * const dataReadUS=(uint8_t const * const)imRead->data();
                uint8_t * const dataWriteDS=(uint8_t * const)imWrite->data();
                const size_t bytesPerImage=imFormat.getBytesPerImage();
                memcpy(dataWriteDS, dataReadUS, bytesPerImage);
            } else
            {//Pass enabled...
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
                
                if (imFormat.getDataType()==ImageFormat::FLITR_PIX_DT_FLOAT32)
                {
                    float const * const dataRead=(float const * const)imRead->data();
                    float * const dataWrite=(float * const)imWrite->data();
                    
                    //****************
                    //Prime the reference image during the first couple of frames.
                    const float avrgImageLongevityConst = (frameNumber_ < 3) ? 0.0f : avrgImageLongevity_;
                    //****************
                    
                    {//=== ===
                        
                        {//=== Update ref/avrg img before new data arrives. ===//
                            float * const imgData=imgVec_[0];
                            float * const refImgData=refImgVec_[0];
                            
                            for (ptrdiff_t y=0; y<croppedHeight; ++y)
                            {
                                const ptrdiff_t lineOffset=y*croppedWidth;
                                
                                for (ptrdiff_t x=0; x<croppedWidth; ++x)
                                {
                                    const ptrdiff_t offset=lineOffset + x;
                                    
                                    refImgData[offset]*=avrgImageLongevityConst;
                                    refImgData[offset]+=imgData[offset] * (1.0f - avrgImageLongevityConst);
                                }
                            }
                        }
                        
                        //=== Crop input data ===//
                        if (imFormat.getPixelFormat()==flitr::ImageFormat::FLITR_PIX_FMT_Y_F32)
                        {
                            for (ptrdiff_t y=startCroppedY; y<=endCroppedY; ++y)
                            {
                                const ptrdiff_t uncroppedLineOffset=y*uncroppedWidth + startCroppedX;
                                const ptrdiff_t croppedLineOffset=(y-startCroppedY)*croppedWidth;
                                
                                //Green channel is used if mono image.
                                memcpy((inputImgDataG_+croppedLineOffset), (dataRead+uncroppedLineOffset), croppedWidth*sizeof(float));
                            }
                        } else
                            if (imFormat.getPixelFormat()==flitr::ImageFormat::FLITR_PIX_FMT_RGB_F32)
                            {
                                for (size_t y=startCroppedY; y<=endCroppedY; ++y)
                                {
                                    const ptrdiff_t uncroppedLineOffset=(y*uncroppedWidth + startCroppedX)*3; //x*3 is optimised by compiler to (x<<1)+x.
                                    const ptrdiff_t croppedLineOffset=(y-startCroppedY)*croppedWidth;
                                    
                                    //RGB image is split into R, G and B images.
                                    for (int x=0; x<croppedWidth; ++x)
                                    {
                                        inputImgDataR_[croppedLineOffset+x]=dataRead[uncroppedLineOffset + x*3 + 0];//x*3 is optimised by compiler to (x<<1)+x.
                                        inputImgDataG_[croppedLineOffset+x]=dataRead[uncroppedLineOffset + x*3 + 1];
                                        inputImgDataB_[croppedLineOffset+x]=dataRead[uncroppedLineOffset + x*3 + 2];
                                    }
                                }
                            }
                        
                        //=== Do Gaussian filter of initial input and store in first level of pyramid - Green channel is used for optical flow! ===//
                        gaussianFilter_.filter(imgVec_[0], inputImgDataG_, croppedWidth, croppedHeight, scratchData_);
                        
                    }//=== ===
                    
                    {//=== Calculate scale space pyramid. ===
                        for (size_t levelNum=0; levelNum<numLevels_; ++levelNum)
                        {
                            float * const imgData=imgVec_[levelNum];
                            //float * const refImgData=refImgVec_[levelNum];
                            
                            const ptrdiff_t levelHeight=croppedHeight >> levelNum;
                            const ptrdiff_t levelWidth=croppedWidth >> levelNum;
                            const ptrdiff_t levelWidthMinus1 = levelWidth - ((ptrdiff_t)1);
                            
                            //=== Calculate the scale space images ===
                            if (levelNum>0)//First level (incoming data) is not a downsampled image.
                            {
                                {//Update ref/avrg img before new data arrives.
                                    float * const refImgData=refImgVec_[levelNum];
                                    
                                    for (ptrdiff_t y=0; y<levelHeight; ++y)
                                    {
                                        const ptrdiff_t lineOffset=y*levelWidth;
                                        
                                        for (ptrdiff_t x=0; x<levelWidth; ++x)
                                        {
                                            const ptrdiff_t offset=lineOffset + x;
                                            
                                            refImgData[offset]*=avrgImageLongevityConst;
                                            refImgData[offset]+=imgData[offset] * (1.0f - avrgImageLongevityConst);
                                        }
                                    }
                                }
                                
                                {//Downsample higher resolution level.
                                    const ptrdiff_t heightUS=croppedHeight >> (levelNum-1);
                                    const ptrdiff_t widthUS=croppedWidth >> (levelNum-1);
                                    
                                    gaussianDownsample_.downsample(imgData, imgVec_[levelNum-1],
                                                                   widthUS, heightUS,
                                                                   scratchData_);
                                }//=== ===
                                
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
                                        
                                        //#define DESCINT_SCATTER
                                        
#ifdef DESCINT_SCATTER
                                        DESCINT_SCATTER CODE NOT REALLY USED
                                        
                                        const float v1=refImgData[offset-levelWidth-1];
                                        const float v2=refImgData[offset-levelWidth];
                                        const float v3=refImgData[offset-levelWidth+1];
                                        const float v4=refImgData[offset-1];
                                        //const float v5=refImgData[offset];
                                        const float v6=refImgData[offset+1];
                                        const float v7=refImgData[offset+levelWidth-1];
                                        const float v8=refImgData[offset+levelWidth];
                                        const float v9=refImgData[offset+levelWidth+1];
#else//DESCINT_GATHER
                                        const float v1=imgData[offset-levelWidth-1];
                                        const float v2=imgData[offset-levelWidth];
                                        const float v3=imgData[offset-levelWidth+1];
                                        const float v4=imgData[offset-1];
                                        //const float v5=imgData[offset];
                                        const float v6=imgData[offset+1];
                                        const float v7=imgData[offset+levelWidth-1];
                                        const float v8=imgData[offset+levelWidth];
                                        const float v9=imgData[offset+levelWidth+1];
#endif
                                        
                                        //Use Scharr operator for image gradient. It has good rotation independence!
                                        const float dx=(v3-v1)*(3.0f/32.0f) + (v6-v4)*(10.0f/32.0f) + (v9-v7)*(3.0f/32.0f); //32 = (3+10+3)*2
                                        const float dy=(v7-v1)*(3.0f/32.0f) + (v8-v2)*(10.0f/32.0f) + (v9-v3)*(3.0f/32.0f); //32 = (3+10+3)*2
                                        
                                        dxData[offset]=dx;
                                        dyData[offset]=dy;
                                        dSqRecipData[offset]=1.0f/(dx*dx+dy*dy);
                                    }
                                }
                            }//=== ===
                        }
                    }//=== ===
                    
                    
                    //===========================
                    //=== Calculate h vectors ===
                    memset(hxVec_.back(), 0, (croppedWidth>>numLevels_) * (croppedHeight>>numLevels_) * sizeof(float));
                    memset(hyVec_.back(), 0, (croppedWidth>>numLevels_) * (croppedHeight>>numLevels_) * sizeof(float));
                    
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
                        
                        //=== Start with h-vectors from lower resolution scale space ===//
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
                                
                                const ptrdiff_t offset=lineOffset + x;
                                hxData[offset]=/*hxDataLR[offsetLR]*2.0f;*/bilinearRead(hxDataLR, offsetLT, levelWidthLR, fx, fy) * 2.0f;
                                hyData[offset]=/*hyDataLR[offsetLR]*2.0f;*/bilinearRead(hyDataLR, offsetLT, levelWidthLR, fx, fy) * 2.0f;
                            }
                        }
                        //=== ===//
                        
                        {
                            for (size_t newtonRaphsonI=0; newtonRaphsonI<7; ++newtonRaphsonI)
                                //5 or more iterations seem to work well.
                            {
                                for (ptrdiff_t y=((ptrdiff_t)1); y<(levelHeight - ((ptrdiff_t)1)); ++y)
                                {
                                    const ptrdiff_t lineOffset=y*levelWidth;
                                    
                                    for (ptrdiff_t x=((ptrdiff_t)1); x<levelWidthMinus1; ++x)
                                    {
                                        const ptrdiff_t offset=lineOffset + x;
                                        
                                        const float dSqRecip=dSqRecipData[offset];
                                        
                                        if (dSqRecip<(recipGradientThreshold_))
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
                                            
                                            
#ifdef DESCINT_SCATTER
                                            DESCINT_SCATTER CODE NOT REALLY USED
                                            
                                            if (((x+int_hx)>((ptrdiff_t)1))&&
                                                ((y+int_hy)>((ptrdiff_t)1))&&
                                                ((x+int_hx+((ptrdiff_t)2))<levelWidth)&&
                                                ((y+int_hy+((ptrdiff_t)2))<levelHeight))
                                            {
                                                const ptrdiff_t offsetLT=offset + int_hx + int_hy * levelWidth;
                                                
                                                const float refImg=bilinearRead(refImgData, offsetLT, levelWidth, frac_hx, frac_hy);
                                                
                                                const float dx=bilinearRead(dxData, offsetLT, levelWidth, frac_hx, frac_hy);
                                                const float dy=bilinearRead(dyData, offsetLT, levelWidth, frac_hx, frac_hy);
                                                
                                                const float imgDiff=imgData[offset]-refImg;
                                                hx+=(imgDiff*dx)*dSqRecip;
                                                hy+=(imgDiff*dy)*dSqRecip;
                                            }
#else //DESCINT_GATHER
                                            if (((x+int_hx)>((ptrdiff_t)1))&&
                                                ((y+int_hy)>((ptrdiff_t)1))&&
                                                ((x+int_hx+((ptrdiff_t)2))<levelWidth)&&
                                                ((y+int_hy+((ptrdiff_t)2))<levelHeight))
                                            {
                                                const ptrdiff_t offsetLT=offset + int_hx + int_hy * levelWidth;
                                                
                                                const float img=bilinearRead(imgData, offsetLT, levelWidth, frac_hx, frac_hy);
                                                
                                                const float dx=bilinearRead(dxData, offsetLT, levelWidth, frac_hx, frac_hy);
                                                const float dy=bilinearRead(dyData, offsetLT, levelWidth, frac_hx, frac_hy);
                                                
                                                const float imgDiff=img-refImgData[offset];
                                                
                                                
                                                {//Calc h vector update, but clamp to certain deltaMax length.
                                                    float delta_hx = (imgDiff*dx)*dSqRecip; //Adapted h displacement using Scharr gradient.
                                                    float delta_hy = (imgDiff*dy)*dSqRecip; //Adapted h displacement using Scharr gradient.
                                                    
                                                    const float delta=sqrtf(delta_hx*delta_hx+delta_hy*delta_hy);
                                                    const float deltaMax=0.75f;
                                                    
                                                    if (delta>deltaMax)
                                                    {//If an error occurs then clamp the resulting h vector.
                                                        const float recipH=deltaMax/delta;
                                                        
                                                        delta_hx*=recipH;
                                                        delta_hy*=recipH;
                                                    }
                                                    
                                                    hx-=delta_hx;
                                                    hy-=delta_hy;
                                                }
                                            }
#endif
                                            
                                            hxData[offset]=hx;
                                            hyData[offset]=hy;
                                        }
                                    }
                                }
                                
                                //if (levelNum>=1)
                                {//=== Smooth/regularise the vector field of this iteration using Gaussian filters in x and y ===//
                                    gaussianReguFilter_.filter(hxData, hxData, levelWidth, levelHeight, scratchData_);
                                    gaussianReguFilter_.filter(hyData, hyData, levelWidth, levelHeight, scratchData_);
                                }//=== ===
                            }
                        }
                    }
                    //===========================
                    //===========================
                    
                    
                    
                    //=== Final results ===
                    {
                        //memset(finalImgDataR_, 0, croppedWidth*croppedHeight*sizeof(float));
                        //memset(finalImgDataG_, 0, croppedWidth*croppedHeight*sizeof(float));
                        //memset(finalImgDataB_, 0, croppedWidth*croppedHeight*sizeof(float));
                        
                        float * const hxDataGF=hxVec_[0];
                        float * const hyDataGF=hyVec_[0];
                        
                        for (ptrdiff_t y=0; y<croppedHeight; ++y)
                        {
                            const ptrdiff_t lineOffset=y*croppedWidth;
                            
                            for (ptrdiff_t x=0; x<croppedWidth; ++x)
                            {
                                const ptrdiff_t offset=lineOffset + x;
                                
                                if (1)
                                {//Dewarp the cropped input image data
                                    //=== Image Dewarping ===//
                                    {
                                        //Note: Use a local contrast measure to control blending...
                                        //      It is assumed the the best lucky frame/region has the best local contrast.
                                        //      Could look at RMS contrast (the standard deviation) over an image patch centred at the desired location!
#ifdef DESCINT_SCATTER
                                        DESCINT_SCATTER CODE NOT REALLY USED
                                        
                                        const float hx=-hxDataGF[offset];
                                        const float hy=-hyDataGF[offset];
                                        
                                        //const float f=0.1;
                                        //avrgHxData_[offset]=avrgHxData_[offset]*(1.0f-f) + hx*f;
                                        //avrgHyData_[offset]=avrgHyData_[offset]*(1.0f-f) + hy*f;
                                        //const float avrgHSq = avrgHxData_[offset]*avrgHxData_[offset] + avrgHyData_[offset]*avrgHyData_[offset];
                                        
                                        const float floor_hx=floorf(hx);
                                        const float floor_hy=floorf(hy);
                                        const ptrdiff_t int_hx=lroundf(floor_hx);
                                        const ptrdiff_t int_hy=lroundf(floor_hy);
                                        
                                        if ( ((x+int_hx)>((ptrdiff_t)1)) && ((y+int_hy)>((ptrdiff_t)1)) && ((x+int_hx)<(croppedWidth-1)) && ((y+int_hy)<(croppedHeight-1)) )
                                        {
                                            const float frac_hx=hx - floor_hx;
                                            const float frac_hy=hy - floor_hy;
                                            const ptrdiff_t offsetLT=offset + int_hx + int_hy * croppedWidth;
                                            
                                            finalImgDataG_[offset]=bilinearRead(inputImgDataG_, offsetLT, croppedWidth, frac_hx, frac_hy);
                                            
                                            if (imFormat.getPixelFormat() == flitr::ImageFormat::FLITR_PIX_FMT_RGB_F32)
                                            {
                                                finalImgDataR_[offset]=bilinearRead(inputImgDataR_, offsetLT, croppedWidth, frac_hx, frac_hy);
                                                finalImgDataB_[offset]=bilinearRead(inputImgDataB_, offsetLT, croppedWidth, frac_hx, frac_hy);
                                            }
                                            //bilinearAdd(imgDataGF[offset], finalImgData_, offsetLT, croppedWidth, frac_hx, frac_hy);
                                            //finalImgData_[offsetLT]=imgDataGF[offset];
                                        }
#else//DESCINT_GATHER
                                        const float hx=hxDataGF[offset];
                                        const float hy=hyDataGF[offset];
                                        
                                        const float floor_hx=floorf(hx);
                                        const float floor_hy=floorf(hy);
                                        const ptrdiff_t int_hx=lroundf(floor_hx);
                                        const ptrdiff_t int_hy=lroundf(floor_hy);
                                        
                                        if (((x+int_hx)>((ptrdiff_t)1)) &&
                                            ((y+int_hy)>((ptrdiff_t)1)) &&
                                            ((x+int_hx)<(croppedWidth-1)) &&
                                            ((y+int_hy)<(croppedHeight-1)) )
                                        {
                                            const float frac_hx=hx - floor_hx;
                                            const float frac_hy=hy - floor_hy;
                                            const ptrdiff_t offsetLT=offset + int_hx + int_hy * croppedWidth;
                                            
                                            finalImgDataG_[offset]=bilinearRead(inputImgDataG_, offsetLT, croppedWidth, frac_hx, frac_hy);
                                            
                                            if (imFormat.getPixelFormat() == flitr::ImageFormat::FLITR_PIX_FMT_RGB_F32)
                                            {
                                                finalImgDataR_[offset]=bilinearRead(inputImgDataR_, offsetLT, croppedWidth, frac_hx, frac_hy);
                                                finalImgDataB_[offset]=bilinearRead(inputImgDataB_, offsetLT, croppedWidth, frac_hx, frac_hy);
                                            }
                                        }
#endif
                                    }
                                    //=== ===
                                }
                                else //OR
                                {//Debug mode...
                                    const size_t levelIndex=0;//(numLevels_-1) - 5;
                                    const size_t offsetB=(x>>levelIndex) + (y>>levelIndex)*(croppedWidth>>levelIndex);
                                    
                                    const float hxB=hxVec_[levelIndex][offsetB];
                                    const float hyB=hyVec_[levelIndex][offsetB];
                                    
                                    //finalImgDataR_[offset]=sqrtf(hxB*hxB+hyB*hyB)*0.1f;
                                    //finalImgDataG_[offset]=sqrtf(hxB*hxB+hyB*hyB)*0.1f;
                                    //finalImgDataB_[offset]=sqrtf(hxB*hxB+hyB*hyB)*0.1f;
                                    
                                    //finalImgDataR_[offset]=imgVec_[levelIndex][offsetB];
                                    //finalImgDataR_[offset]=refImgVec_[levelIndex][offsetB];
                                    
                                    finalImgDataR_[offset]=(imgVec_[levelIndex][offsetB] - refImgVec_[levelIndex][offsetB])*100.0f+0.5f;
                                }
                            }
                        }
                        
                        //=== Copy finalImgData results to uncropped output image slot ===//
                        //  The data is copied because of the lucky region accumulation that happens in the result buffer.
                        if (imFormat.getPixelFormat()==flitr::ImageFormat::FLITR_PIX_FMT_Y_F32)
                        {
                            for (ptrdiff_t uncroppedY=startCroppedY; uncroppedY<=endCroppedY; ++uncroppedY)
                            {
                                const ptrdiff_t uncroppedLineOffset=uncroppedY*uncroppedWidth + startCroppedX;
                                const ptrdiff_t croppedY=uncroppedY-startCroppedY;
                                const ptrdiff_t croppedLineOffset=croppedY * croppedWidth;
                                
                                memcpy((dataWrite+uncroppedLineOffset), (finalImgDataG_+croppedLineOffset), croppedWidth*sizeof(float));
                            }
                        } else
                        {//flitr::ImageFormat::FLITR_PIX_FMT_RGB_F32
                            for (ptrdiff_t uncroppedY=startCroppedY; uncroppedY<=endCroppedY; ++uncroppedY)
                            {
                                const ptrdiff_t uncroppedLineOffset=(uncroppedY*uncroppedWidth + startCroppedX) * 3;
                                const ptrdiff_t croppedY=uncroppedY-startCroppedY;
                                const ptrdiff_t croppedLineOffset=croppedY * croppedWidth;
                                
                                for (int x=0; x<croppedWidth; ++x)
                                {
                                    dataWrite[uncroppedLineOffset + x*3 + 0] = finalImgDataR_[croppedLineOffset + x]; //x*3 is optimised by compiler to (x<<1)+x.
                                    dataWrite[uncroppedLineOffset + x*3 + 1] = finalImgDataG_[croppedLineOffset + x];
                                    dataWrite[uncroppedLineOffset + x*3 + 2] = finalImgDataB_[croppedLineOffset + x];
                                }
                            }
                        }
                        //=== ===//
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



