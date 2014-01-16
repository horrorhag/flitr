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

#include <flitr/modules/flitr_image_processors/gaussian_downsample/fip_gaussian_downsample.h>

#ifdef _OPENMP
#include <omp.h>
#endif

using namespace flitr;
using std::tr1::shared_ptr;

FIPGaussianDownsample::FIPGaussianDownsample(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                                             uint32_t buffer_size) :
ImageProcessor(upStreamProducer, images_per_slot, buffer_size)
{
    
    //Setup image format being produced to downstream.
    for (uint32_t i=0; i<images_per_slot; i++) {
        ImageFormat downStreamFormat=upStreamProducer.getFormat();
        
        downStreamFormat.downSampleByTwo();
        
        ImageFormat_.push_back(downStreamFormat);
    }
    
}

FIPGaussianDownsample::~FIPGaussianDownsample()
{
    delete [] xFiltData_;
}

bool FIPGaussianDownsample::init()
{
    bool rValue=ImageProcessor::init();
    //Note: SharedImageBuffer of downstream producer is initialised with storage in ImageProcessor::init.
    
    size_t maxXFiltDataSize=0;
    
    for (uint32_t i=0; i<ImagesPerSlot_; i++)
    {
        const ImageFormat imFormat=getUpstreamFormat(i);
        
        const size_t width=imFormat.getWidth();
        const size_t height=imFormat.getHeight();
        
        const size_t componentsPerPixel=imFormat.getComponentsPerPixel();
        
        const size_t xFiltDataSize = (width/2+1)*height*componentsPerPixel;
        
        if (xFiltDataSize>maxXFiltDataSize)
        {
            maxXFiltDataSize=xFiltDataSize;
        }
    }
    
    //Allocate a buffer big enough for any of the image slots.
    xFiltData_=new float[maxXFiltDataSize];
    
    return rValue;
}

bool FIPGaussianDownsample::trigger()
{
    if ((getNumReadSlotsAvailable())&&(getNumWriteSlotsAvailable()))
    {//There are images to consume and the downstream producer has space to produce.
        std::vector<Image**> imvRead=reserveReadSlot();
        std::vector<Image**> imvWrite=reserveWriteSlot();
        
        //Start stats measurement event.
        ProcessorStats_->tick();
        
        for (size_t imgNum=0; imgNum<ImagesPerSlot_; imgNum++)
        {
            Image const * const imReadUS = *(imvRead[imgNum]);
            Image * const imWriteDS = *(imvWrite[imgNum]);
            
            float const * const dataReadUS=(float const * const)imReadUS->data();
            float * const dataWriteDS=(float * const)imWriteDS->data();
            
            const ImageFormat imDSFormat=getDownstreamFormat(imgNum);
            const ImageFormat imUSFormat=getUpstreamFormat(imgNum);
            
            const size_t widthDS=imDSFormat.getWidth();
            const size_t heightDS=imDSFormat.getHeight();
            
            const size_t widthUS=imUSFormat.getWidth();
            const size_t heightUS=imUSFormat.getHeight();
            
            const size_t componentsPerPixel=imDSFormat.getComponentsPerPixel();
            
            const size_t componentsPerLineDS=componentsPerPixel * widthDS;
            const size_t componentsPerLineUS=componentsPerPixel * widthUS;
            
            size_t y=0;
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
                    for (y=0; y<heightUS; y++)
                    {
                        const size_t lineOffsetDS=y * componentsPerLineDS;
                        const size_t lineOffsetUS=y * componentsPerLineUS;
                        
                        for (size_t compNumDS=4; compNumDS<(componentsPerLineDS-4); compNumDS++)
                        {
                            float xFiltValue=( dataReadUS[lineOffsetUS + (compNumDS<<1)] +
                                              dataReadUS[lineOffsetUS + (compNumDS<<1) + 1] ) * (462.0f/2048.0f);
                            
                            xFiltValue+=( dataReadUS[lineOffsetUS + (compNumDS<<1)-1] +
                                         dataReadUS[lineOffsetUS + (compNumDS<<1) + 2] ) * (330.0f/2048.0f);
                            
                            xFiltValue+=( dataReadUS[lineOffsetUS + (compNumDS<<1)-2] +
                                         dataReadUS[lineOffsetUS + (compNumDS<<1) + 3] ) * (165.0f/2048.0f);
                            
                            xFiltValue+=( dataReadUS[lineOffsetUS + (compNumDS<<1)-3] +
                                         dataReadUS[lineOffsetUS + (compNumDS<<1) + 4] ) * (55.0f/2048.0f);
                            
                            xFiltValue+=( dataReadUS[lineOffsetUS + (compNumDS<<1)-4] +
                                         dataReadUS[lineOffsetUS + (compNumDS<<1) + 5] ) * (1.0f/2048.0f);
                            
                            xFiltData_[lineOffsetDS + compNumDS]=xFiltValue;
                        }
                    }
                }
            }
            
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
                    for (y=4; y<(heightDS-4); y++)
                    {
                        const size_t lineOffsetDS=y * componentsPerLineDS;
                        const size_t lineOffsetUS=(y<<1) * componentsPerLineDS;
                        
                        for (size_t compNum=0; compNum<componentsPerLineDS; compNum++)
                        {
                            float filtValue=( xFiltData_[lineOffsetUS + compNum]+
                                             xFiltData_[lineOffsetUS + componentsPerLineDS + compNum] ) * (462.0f/2048.0f);
                            
                            filtValue+=( xFiltData_[lineOffsetUS + compNum - componentsPerLineDS]+
                                        xFiltData_[lineOffsetUS + (componentsPerLineDS<<1) + compNum] ) * (330.0f/2048.0f);
                            
                            filtValue+=( xFiltData_[lineOffsetUS + compNum - (componentsPerLineDS<<1)]+
                                        xFiltData_[lineOffsetUS + ((componentsPerLineDS<<1)+componentsPerLineDS) + compNum] ) * (165.0f/2048.0f);
                            
                            filtValue+=( xFiltData_[lineOffsetUS + compNum - ((componentsPerLineDS<<1)+componentsPerLineDS)]+
                                        xFiltData_[lineOffsetUS + (componentsPerLineDS<<2) + compNum] ) * (55.0f/2048.0f);
                            
                            filtValue+=( xFiltData_[lineOffsetUS + compNum - (componentsPerLineDS<<2)]+
                                        xFiltData_[lineOffsetUS + ((componentsPerLineDS<<2)+componentsPerLineDS) + compNum] ) * (1.0f/2048.0f);
                            
                            dataWriteDS[lineOffsetDS+compNum]=filtValue;
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

