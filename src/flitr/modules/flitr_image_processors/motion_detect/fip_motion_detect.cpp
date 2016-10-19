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

#include <flitr/modules/flitr_image_processors/motion_detect/fip_motion_detect.h>


using namespace flitr;
using std::shared_ptr;

FIPMotionDetect::FIPMotionDetect(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                                 const bool showOverlays, const bool produceOnlyMotionImages, const bool forceRGBOutput,
                                 const float motionThreshold, const int detectionThreshold,
                                 uint32_t buffer_size) :
ImageProcessor(upStreamProducer, images_per_slot, buffer_size),
_frameCounter(0),
_avrgImg(nullptr),
_varImg(nullptr),
_detectionImg(nullptr),
_detectionCountImg(nullptr),
_showOverlays(showOverlays),
_produceOnlyMotionImages(produceOnlyMotionImages),
_forceRGBOutput(forceRGBOutput),
_motionThreshold(motionThreshold),
_detectionThreshold(detectionThreshold)
{
    //!@todo can we get images_per_slot from upstream producer?
    
    //Setup image format being produced to downstream.
    for (uint32_t i=0; i<images_per_slot; i++)
    {
        ImageFormat downStreamFormat=upStreamProducer.getFormat(i);
        
        if (_forceRGBOutput)
        {
            switch (downStreamFormat.getPixelFormat()) {
                case ImageFormat::FLITR_PIX_FMT_Y_8:
                    downStreamFormat.setPixelFormat(ImageFormat::FLITR_PIX_FMT_RGB_8);
                    break;
                default:
                    logMessage(LOG_CRITICAL) << "Pixel format is already RGB or unsupported!" << __FILE__ << " " << __LINE__ << "\n";
                    break;
            }
        }
        
        ImageFormat_.push_back(downStreamFormat);
    }
}

FIPMotionDetect::~FIPMotionDetect()
{
    if (_avrgImg) delete [] _avrgImg;
    if (_varImg) delete [] _varImg;
    if (_detectionImg) delete [] _detectionImg;
    if (_detectionCountImg) delete [] _detectionCountImg;
}

bool FIPMotionDetect::init()
{
    if (!ImageProcessor::init()) return false;
    //Note: SharedImageBuffer of downstream producer is initialised with storage in ImageProcessor::init.
    
    size_t maxScratchDataSize=0;
    size_t maxScratchDataValues=0;
    
    for (uint32_t i=0; i<ImagesPerSlot_; i++)
    {
        const ImageFormat imFormat=getUpstreamFormat(i);
        
        const size_t width=imFormat.getWidth();
        const size_t height=imFormat.getHeight();
        
        const size_t bytesPerPixel=imFormat.getBytesPerPixel();
        const size_t componentsPerPixel=imFormat.getComponentsPerPixel();
        
        const size_t scratchDataSize = width * height * bytesPerPixel;
        const size_t scratchDataValues = width * height * componentsPerPixel;
        
        if (scratchDataSize>maxScratchDataSize)
        {
            maxScratchDataSize=scratchDataSize;
        }
        
        if (scratchDataValues>maxScratchDataValues)
        {
            maxScratchDataValues=scratchDataValues;
        }
    }
    
    //Allocate a buffer big enough for any of the image slots.
    _avrgImg=new float[maxScratchDataValues];
    memset(_avrgImg, 0, maxScratchDataValues * sizeof(float));
    
    _varImg=new float[maxScratchDataValues];
    memset(_varImg, 0, maxScratchDataValues * sizeof(float));
    
    _detectionImg=new uint8_t[maxScratchDataValues];
    memset(_detectionImg, 0, maxScratchDataValues * sizeof(*_detectionImg));
    
    _detectionCountImg=new int[maxScratchDataValues];
    memset(_detectionCountImg, 0, maxScratchDataValues * sizeof(int));
    
    return true;
}

bool FIPMotionDetect::trigger()
{
    if ((getNumReadSlotsAvailable())&&(getNumWriteSlotsAvailable()))
    {
        std::vector<Image**> imvRead=reserveReadSlot();
        
        //The write slot will be reserved later.
        
        //Start stats measurement event.
        ProcessorStats_->tick();
        
        for (size_t imgNum=0; imgNum<ImagesPerSlot_; ++imgNum)
        {
            Image const * const imReadUS = *(imvRead[imgNum]);
            const ImageFormat imFormat=getUpstreamFormat(imgNum);//down stream and up stream image dimensions are the same.
            const int width=imFormat.getWidth();
            const int height=imFormat.getHeight();
            const size_t componentsPerPixel=imFormat.getComponentsPerPixel();
            const size_t componentsPerImage = width * height * componentsPerPixel;
            
            
            //Mask that defines the detection bin width and height. Could be made configurable!
            const uint32_t detectImgMask = 0xFFFFFFF0;
            
            if (imFormat.getPixelFormat()==ImageFormat::FLITR_PIX_FMT_RGB_8)
            {
                logMessage(LOG_CRITICAL) << "Pixel format is not supported yet!" << __FILE__ << " " << __LINE__ << "\n";
            } else
                if (imFormat.getPixelFormat()==ImageFormat::FLITR_PIX_FMT_Y_8)
                {
                    uint8_t const * const dataReadUS=(uint8_t const * const)imReadUS->data();
                    //boxFilter_.filter(currentFrame_, scratchData_, width, height, integralImageScratchData_, true);
                    
                    
                    //Update the background variance and average.
                    if (_frameCounter>0)
                    {
                        //The frame avrg&var update filter parameter. Could be made configurable: Smaller values of a could improve sensitivity to slow moving objects.
                        const float a=1.0/100.0;
                        
                        for (size_t i=0; i<componentsPerImage; ++i)
                        {
                            _avrgImg[i]=dataReadUS[i]*a + _avrgImg[i]*(1.0f-a);
                            const float d=(int(dataReadUS[i])-int(_avrgImg[i]));//uint8_t variables are promoted to int.
                            _varImg[i]=(d*d) * a + _varImg[i]*(1.0f-a);
                        }
                    } else
                    {
                        //Initial pixel averages and variances.
                        for (size_t i=0; i<componentsPerImage; ++i)
                        {
                            _avrgImg[i]=dataReadUS[i];
                            _varImg[i]=100.0f;//Initial variance chosen quite large to suppress motion during early background learning.
                        }
                    }
                    
                    //Zero the detection image.
                    memset(_detectionImg, 0, componentsPerImage * sizeof(*_detectionImg));
                    
                    //Mark cells with detections.
                    for (int y=0; y<height; ++y)
                    {
                        const int lineOffset=y*width;
                        const int lineOffsetB=(y&detectImgMask)*width;
                        
                        for (int x=0; x<width; ++x)
                        {
                            const int offset=lineOffset + x;
                            const int offsetB=lineOffsetB + (x&detectImgMask);
                            
                            const float mvMag=fabsf(float(dataReadUS[offset]) - _avrgImg[offset]) / sqrtf(_varImg[offset]);
                            
                            if (mvMag > _motionThreshold)
                            {
                                _detectionImg[offsetB]=1;
                            }
                        }
                    }
                    
                    //Count detections over time.
                    for (size_t i=0; i<componentsPerImage; ++i)
                    {
                        if (_detectionImg[i])
                        {
                            _detectionCountImg[i]=_detectionCountImg[i] + 1;
                        } else
                        {
                            _detectionCountImg[i]=0;//_detectionCountImg[i] - 1;
                            //if (_detectionCountImg[i] < 0) _detectionCountImg[i]=0;
                        }
                    }
                    
                    //int64_t M=0;
                    //for (int i=0; i<(width*height); ++i)
                    //{
                    //    M+=std::abs(currentFrame_[i] - previousFrame_[i]);
                    //}
                    const bool frameMotion = true;//(M / (width*height*0.001f) + 0.5f) > (motionThreshold_);
                    
                    
                    if ((!_produceOnlyMotionImages) || frameMotion)
                    {
                        std::vector<Image**> imvWrite=reserveWriteSlot();
                        Image * const imWriteDS = *(imvWrite[imgNum]);
                        uint8_t * const dataWriteDS=(uint8_t * const)imWriteDS->data();

                        // Pass the metadata from the read image to the write image.
                        // By Default the base implementation will copy the pointer if no custom
                        // pass function was set.
                        if(PassMetadataFunction_ != nullptr)
                        {
                            imWriteDS->setMetadata(PassMetadataFunction_(imReadUS->metadata()));
                        }
                        
                        if ((frameMotion) && (_showOverlays))
                        {//Add motion blob overlay.
                            
                            if (_forceRGBOutput)
                            {
                                for (int y=0; y<height; ++y)
                                {
                                    const int lineOffsetDS=y*(width*3);
                                    
                                    const int lineOffset=y*width;
                                    const int lineOffsetB=(y&detectImgMask)*width;
                                    
                                    for (int x=0; x<width; ++x)
                                    {
                                        const int offsetDS=lineOffsetDS + x*3;
                                        
                                        const int offset=lineOffset + x;
                                        const int offsetB=lineOffsetB + (x & detectImgMask);
                                        
                                        const uint8_t c=(_detectionCountImg[offsetB] > _detectionThreshold) ? ((dataReadUS[offset]>>1)+128) : dataReadUS[offset];
                                        
                                        dataWriteDS[offsetDS+0]=c;
                                        dataWriteDS[offsetDS+1]=dataReadUS[offset];
                                        dataWriteDS[offsetDS+2]=c;
                                    }
                                }
                            } else
                            {
                                for (int y=0; y<height; ++y)
                                {
                                    const int lineOffset=y*width;
                                    const int lineOffsetB=(y&detectImgMask)*width;
                                    
                                    for (int x=0; x<width; ++x)
                                    {
                                        const int offset=lineOffset + x;
                                        const int offsetB=lineOffsetB + (x & detectImgMask);
                                        
                                        const uint8_t c=(_detectionCountImg[offsetB] > _detectionThreshold) ? ((dataReadUS[offset]>>1)+128) : dataReadUS[offset];
                                        
                                        dataWriteDS[offset]=c;
                                    }
                                }
                            }
                        } else
                        {//Don't add motion overlay. Just copy the data from the input.
                            memcpy(dataWriteDS, dataReadUS, width*height*3);
                        }
                        
                        releaseWriteSlot();
                    }
                }
            
            ++_frameCounter;
        }
        
        //Stop stats measurement event.
        ProcessorStats_->tock();
        
        releaseReadSlot();
        
        return true;
    }
    
    return false;
}

