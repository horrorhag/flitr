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

#include <flitr/modules/flitr_image_processors/dewarp/fip_dewarp.h>

#ifdef _OPENMP
#include <omp.h>
#endif

using namespace flitr;
using std::tr1::shared_ptr;

FIPDewarp::FIPDewarp(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                     uint32_t buffer_size) :
ImageProcessor(upStreamProducer, images_per_slot, buffer_size)
{
    //Setup image format being produced to downstream.
    for (uint32_t i=0; i<images_per_slot; i++) {
        ImageFormat_.push_back(upStreamProducer.getFormat(i));
    }
    
}

FIPDewarp::~FIPDewarp()
{
    for (uint32_t i=0; i<ImagesPerSlot_; i++)
    {
        delete [] dxDataVec_[i];
        delete [] dyDataVec_[i];
        delete [] d2xDataVec_[i];
        delete [] d2yDataVec_[i];
        
        delete [] refImgDataVec_[i];
    }
}

bool FIPDewarp::init()
{
    bool rValue=ImageProcessor::init();
    //Note: SharedImageBuffer of downstream producer is initialised with storage in ImageProcessor::init.
        
    for (uint32_t i=0; i<ImagesPerSlot_; i++)
    {
        const ImageFormat imFormat=getUpstreamFormat(i);
        
        const size_t width=imFormat.getWidth();
        const size_t height=imFormat.getHeight();
        const size_t componentsPerPixel=imFormat.getComponentsPerPixel();
        
        dxDataVec_.push_back(new float[width*height*componentsPerPixel]);
        dyDataVec_.push_back(new float[width*height*componentsPerPixel]);
        d2xDataVec_.push_back(new float[width*height*componentsPerPixel]);
        d2yDataVec_.push_back(new float[width*height*componentsPerPixel]);
        refImgDataVec_.push_back(new float[width*height*componentsPerPixel]);
    }
    
    return rValue;
}

bool FIPDewarp::trigger()
{
    if ((getNumReadSlotsAvailable())&&(getNumWriteSlotsAvailable()))
    {//There are images to consume and the downstream producer has space to produce.
        std::vector<Image**> imvRead=reserveReadSlot();
        std::vector<Image**> imvWrite=reserveWriteSlot();
        
        //Start stats measurement event.
        ProcessorStats_->tick();
        
        for (size_t imgNum=0; imgNum<ImagesPerSlot_; imgNum++)
        {
            Image const * const imRead = *(imvRead[imgNum]);
            Image * const imWrite = *(imvWrite[imgNum]);
            
            float const * const dataRead=(float const * const)imRead->data();
            float * const dataWrite=(float * const)imWrite->data();
            
            const ImageFormat imFormat=getUpstreamFormat(imgNum);
            
            const size_t width=imFormat.getWidth();
            const size_t height=imFormat.getHeight();
            
            const size_t componentsPerPixel=imFormat.getComponentsPerPixel();
            const size_t componentsPerLine=componentsPerPixel * width;
            
            float *dxData = dxDataVec_[imgNum];
            float *dyData = dyDataVec_[imgNum];
            float *d2xData = d2xDataVec_[imgNum];
            float *d2yData = d2yDataVec_[imgNum];
            
            float *refImgData = refImgDataVec_[imgNum];
            
            size_t y=0;
            {
#ifdef _OPENMP
                //omp_set_num_threads(4);//There might be some blocking operation or mem bandwidth limited code??? because the parallel for seems to work best with much more threads than CPU cores.
#endif
                
#ifdef _OPENMP
#pragma omp parallel default(none) private(y) shared(none)
#endif
                {
#ifdef _OPENMP
#pragma omp for nowait
#endif
                    for (y=2; y<height; y++)
                    {
                        const size_t lineOffset=y * componentsPerLine;
                        
                        for (size_t compNum=2; compNum<componentsPerLine; compNum++)
                        {
                            const float pixelValue=dataRead[lineOffset + compNum];
                            const float pixelValueL=dataRead[lineOffset - 1 + compNum];
                            const float pixelValueU=dataRead[lineOffset - componentsPerLine + compNum];
                            
                            dxData[lineOffset + compNum] = pixelValue - pixelValueL;
                            dyData[lineOffset + compNum] = pixelValue - pixelValueU;
                            
                            d2xData[lineOffset + compNum] = (pixelValue - pixelValueL) - (pixelValueL - dataRead[lineOffset - 2 + compNum]);
                            d2yData[lineOffset + compNum] = (pixelValue - pixelValueU) - (pixelValueU - dataRead[lineOffset - (componentsPerLine<<1) + compNum]);
                            
                            const float refPixelValue=refImgData[lineOffset + compNum];
                            
                            const float weightX=1.0f/(fabsf(d2xData[lineOffset + compNum])*100.0f+1.0f);
                            const float weightY=1.0f/(fabsf(d2yData[lineOffset + compNum])*100.0f+1.0f);
                            
                            dataWrite[lineOffset + compNum] = weightX*1.0f;
                            //dataWrite[lineOffset + compNum] = dxData[lineOffset + compNum]*weightX+0.5f;
                            //dataWrite[lineOffset + compNum] = (dxData[lineOffset + compNum] * (((float)pixelValue) - ((float)refPixelValue)) )*100.0f+128.5f;
                            //dataWrite[lineOffset + compNum] = weightX * (dxData[lineOffset + compNum] * (pixelValue - refPixelValue)) * 1000.0f + 0.5f;
                            
                            refImgData[lineOffset + compNum]=pixelValue;
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

