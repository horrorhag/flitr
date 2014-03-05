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

#include <flitr/modules/flitr_image_processors/photometric_equalise/fip_photometric_equalise.h>

#ifdef _OPENMP
#include <omp.h>
#endif

using namespace flitr;
using std::tr1::shared_ptr;

FIPPhotometricEqualise::FIPPhotometricEqualise(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                                               float targetAverage,
                                               uint32_t buffer_size) :
ImageProcessor(upStreamProducer, images_per_slot, buffer_size),
targetAverage_(targetAverage)
{
    
    //Setup image format being produced to downstream.
    for (uint32_t i=0; i<images_per_slot; i++) {
        ImageFormat_.push_back(upStreamProducer.getFormat(i));//Output format is same as input format.
    }
    
}

FIPPhotometricEqualise::~FIPPhotometricEqualise()
{
    delete [] lineSumArray_;
}

bool FIPPhotometricEqualise::init()
{
    bool rValue=ImageProcessor::init();
    //Note: SharedImageBuffer of downstream producer is initialised with storage in ImageProcessor::init.
    
    size_t maxHeight=0;
    
    for (uint32_t i=0; i<ImagesPerSlot_; i++)
    {
        const ImageFormat imFormat=getUpstreamFormat(i);//Downstream format is same as upstream format.
        //const ImageFormat::PixelFormat pixelFormat=imFormat.getPixelFormat();
        
        //const size_t width=imFormat.getWidth();
        const size_t height=imFormat.getHeight();
        
        //const size_t componentsPerPixel=imFormat.getComponentsPerPixel();
        //const size_t bytesPerPixel=imFormat.getBytesPerPixel();
        
        if (height>maxHeight)
        {
            maxHeight=height;
        }
    }
    
    lineSumArray_ = new float[maxHeight];
    
    return rValue;
}

bool FIPPhotometricEqualise::trigger()
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
            float * const dataWrite=(float * const )imWrite->data();
            
            const ImageFormat imFormat=getUpstreamFormat(imgNum);//Downstream format is same as upstream format.
            
            const size_t width=imFormat.getWidth();
            const size_t height=imFormat.getHeight();
            const size_t componentsPerPixel=imFormat.getComponentsPerPixel();
            const size_t componentsPerLine=componentsPerPixel * width;
            const size_t componentsPerImage=componentsPerLine * height;
            
            int y=0;
            {
#ifdef _OPENMP
                //omp_set_num_threads(4);//There might be some blocking operation or mem bandwidth limited code??? because the parallel for seems to work best with much more threads than CPU cores.
#endif
                
#ifdef _OPENMP
#pragma omp parallel private(y)
#endif
                {
#ifdef _OPENMP
#pragma omp for nowait
#endif
                    for (y=0; y<(int)height; y++)
                    {
                        const size_t lineOffset=y * componentsPerLine;
                        
                        float *lineSum = lineSumArray_ + y;
                        *lineSum=0;
                        
                        for (size_t compNum=0; compNum<componentsPerLine; compNum++)
                        {
                            (*lineSum)+=dataRead[lineOffset + compNum];
                        }
                    }
                }
            }
            
            
            //Single thread!
            uint32_t imageSum=0;
            for (y=(int)height/4; y<((int)height*3)/4; y++)
                //for (y=0; y<height; y++)
            {
                imageSum+=lineSumArray_[y];
            }
            
            
            const float average = imageSum / ((float)componentsPerImage);
            const float eScale=targetAverage_ / average;
            
            
            {
#ifdef _OPENMP
                //omp_set_num_threads(4);//There might be some blocking operation or mem bandwidth limited code??? because the parallel for seems to work best with much more threads than CPU cores.
#endif
                
#ifdef _OPENMP
#pragma omp parallel private(y)
#endif
                {
#ifdef _OPENMP
#pragma omp for nowait
#endif
                    for (y=0; y<(int)height; y++)
                    {
                        const size_t lineOffset=y * componentsPerLine;
                        
                        for (size_t compNum=0; compNum<componentsPerLine; compNum++)
                        {
                            dataWrite[lineOffset + compNum]=dataRead[lineOffset + compNum] * eScale;
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

