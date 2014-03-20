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
            
            int y=0;
            {
                {
                    for (y=0; y<heightUS; y++)
                    {
                        const size_t lineOffsetDS=y * componentsPerLineDS;
                        const size_t lineOffsetUS=y * componentsPerLineUS;
                        
                        for (size_t compNumDS=((size_t)3); compNumDS<(componentsPerLineDS-((size_t)3)); compNumDS++)
                        {
                            const size_t offsetUS=lineOffsetUS + (compNumDS<<1);
                            
                            float xFiltValue=( dataReadUS[offsetUS] +
                                              dataReadUS[offsetUS + 1] ) * (462.0f/2048.0f);//The const expr devisions will be compiled away!
                            
                            xFiltValue+=( dataReadUS[offsetUS-1] +
                                         dataReadUS[offsetUS + 2] ) * (330.0f/2048.0f);
                            
                            xFiltValue+=( dataReadUS[offsetUS-2] +
                                         dataReadUS[offsetUS + 3] ) * (165.0f/2048.0f);
                            
                            xFiltValue+=( dataReadUS[offsetUS-3] +
                                         dataReadUS[offsetUS + 4] ) * (55.0f/2048.0f);
                            
                            xFiltValue+=( dataReadUS[offsetUS-4] +
                                         dataReadUS[offsetUS + 5] ) * (11.0f/2048.0f);
                            
                            xFiltValue+=( dataReadUS[offsetUS-5] +
                                         dataReadUS[offsetUS + 6] ) * (1.0f/2048.0f);
                            
                            xFiltData_[lineOffsetDS + compNumDS]=xFiltValue;
                        }
                    }
                }
            }
            
            {
                {
                    for (y=3; y<(heightDS-3); y++)
                    {
                        const size_t lineOffsetDS=y * componentsPerLineDS;
                        const size_t lineOffsetFS=(y<<1) * componentsPerLineDS;
                        
                        for (size_t compNumDS=((size_t)3); compNumDS<componentsPerLineDS-((size_t)3); compNumDS++)
                        {
                            const size_t offsetFS=lineOffsetFS + compNumDS;
                            
                            float filtValue=( xFiltData_[offsetFS]+
                                             xFiltData_[offsetFS + componentsPerLineDS] ) * (462.0f/2048.0f);//The const expr devisions will be compiled away!
                            
                            filtValue+=( xFiltData_[offsetFS - componentsPerLineDS]+
                                        xFiltData_[offsetFS + (componentsPerLineDS<<1)] ) * (330.0f/2048.0f);
                            
                            filtValue+=( xFiltData_[offsetFS - (componentsPerLineDS<<1)]+
                                        xFiltData_[offsetFS + ((componentsPerLineDS<<1)+componentsPerLineDS)] ) * (165.0f/2048.0f);
                            
                            filtValue+=( xFiltData_[offsetFS - ((componentsPerLineDS<<1)+componentsPerLineDS)]+
                                        xFiltData_[offsetFS + (componentsPerLineDS<<2)] ) * (55.0f/2048.0f);
                            
                            filtValue+=( xFiltData_[offsetFS - (componentsPerLineDS<<2)]+
                                        xFiltData_[offsetFS + ((componentsPerLineDS<<2)+componentsPerLineDS)] ) * (11.0f/2048.0f);
                            
                            filtValue+=( xFiltData_[offsetFS - ((componentsPerLineDS<<2)+componentsPerLineDS)]+
                                        xFiltData_[offsetFS + ((componentsPerLineDS<<2)+(componentsPerLineDS<<1))] ) * (1.0f/2048.0f);
                            
                            dataWriteDS[lineOffsetDS+compNumDS]=filtValue;
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

