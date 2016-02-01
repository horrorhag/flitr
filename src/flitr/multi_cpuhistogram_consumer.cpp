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

#include <flitr/multi_cpuhistogram_consumer.h>
#include <flitr/image_producer.h>

using namespace flitr;

void MultiCPUHistogramConsumerThread::run()
{
    std::vector<Image**> imv;
    uint32_t imageStride=Consumer_->ImageStride_;
    uint32_t imageCount=0;
    
    while (true) {
        // check if image available
        imv.clear();
        imv = Consumer_->reserveReadSlot();
        
        if (imv.size() == Consumer_->ImagesPerSlot_)
        {
            
            if ((imageCount % imageStride)==0)
            {
                int imNum=0;
                
                {
#pragma omp parallel for
                    for (imNum=0; imNum<(int)Consumer_->ImagesPerSlot_; imNum++)
                    {// Calculate the histogram.
                        Image* im = *(imv[imNum]);
                        
                        OpenThreads::ScopedLock<OpenThreads::Mutex> wlock(*(Consumer_->CalcMutexes_[imNum]));
                        
                        std::vector<int32_t>* histogram=Consumer_->Histograms_[imNum].get();
                        
                        const uint32_t width=im->format()->getWidth();
                        const uint32_t height=im->format()->getHeight();
                        const uint32_t numPixels=width*height;
                        const uint32_t numComponents=im->format()->getComponentsPerPixel();
                        
                        unsigned char const * const data=(unsigned char const * const)im->data();
                        const uint32_t numBins=256;
                        const uint32_t pixelStride=Consumer_->PixelStride_;
                        
                        
                        // Zero the histogram.
                        for (uint32_t binNum=0; binNum<numBins; binNum++)
                        {
                            (*histogram)[binNum]=0;
                        }
                        
                        // Generate histogram.
                        const uint32_t numElements=numPixels*numComponents;
                        const uint32_t elementStride=pixelStride*numComponents;
                        
                        for (uint32_t i=0; i<numElements; i+=elementStride)
                        {
                            (*histogram)[data[i]]+=pixelStride;
                        }
                        
                        Consumer_->HistogramUpdatedVect_[imNum]=true;
                    }
                }
            }
            // indicate we are done with the image/s
            Consumer_->releaseReadSlot();
            imageCount++;
        } else
        {
            // wait a while for producers.
            Thread::microSleep(1000);
        }
        // check for exit
        if (ShouldExit_) {
            break;
        }
    }
}

const std::vector<uint8_t> MultiCPUHistogramConsumer::calcHistogramIdentityMap()
{
    std::vector<uint8_t> histoIdentityMap;
    histoIdentityMap.resize(256);
    
    for (uint32_t binNum=0; binNum<256; binNum++)
    {
        histoIdentityMap[binNum]=binNum;
    }
    
    return histoIdentityMap;
}

const std::vector<int32_t> MultiCPUHistogramConsumer::calcRefHistogramForEqualisation(uint32_t histoSum)
{
    std::vector<int32_t> refHisto;
    refHisto.resize(256);
    
    uint32_t binsTotal=0;
    
    for (uint32_t binNum=0; binNum<256; binNum++)
    {
        float targetBinsTotal=((binNum+1)/((float)256.0f))*histoSum;
        
        uint32_t binValue=(uint32_t)(targetBinsTotal - binsTotal + 0.5);
        
        refHisto[binNum]=binValue;
        
        binsTotal+=binValue;
    }
    
    return refHisto;
}

const std::vector<uint8_t> MultiCPUHistogramConsumer::calcHistogramMatchMap(const std::vector<int32_t> &inHisto, const std::vector<int32_t> &refHisto)
{
    std::vector<uint8_t> histoMatchMap;
    histoMatchMap.resize(256);
    
    float binTotal=0.0f;
    float refBinTotal=0.0f;
    
    uint32_t binNum=0;
    uint32_t refBinNum=0;
    
    for (; refBinNum<256; refBinNum++)
    {
        refBinTotal+=refHisto[refBinNum];
        
        while ((binTotal<refBinTotal)&&(binNum<256))
        {
            binTotal+=inHisto[binNum];
            histoMatchMap[binNum]=refBinNum;
            binNum++;
        }
    }
    for (; binNum<256; binNum++)
    {
        histoMatchMap[binNum]=255;
    }
    
    return histoMatchMap;
}

const std::vector<uint8_t> MultiCPUHistogramConsumer::calcHistogramStretchMap(const std::vector<int32_t> &inHisto, const uint32_t histoSum,
                                                                              const double ignoreBelow, const double ignoreAbove)
{
    std::vector<uint8_t> histoStretchMap;
    histoStretchMap.resize(256);
    
    uint32_t binTotal=0;
    uint32_t startBin=0;
    uint32_t endBin=255;
    
    for (uint32_t binNum=0; binNum<256; binNum++)
    {
        if ( (binTotal/((double)histoSum))<ignoreBelow )
        {
            startBin=binNum;
            //std::cout << " startBin ";
        }
        
        if ( (binTotal/((double)histoSum))<ignoreAbove )
        {
            endBin=binNum;
            //std::cout << " endBin " << (binTotal/((double)histoSum)) << " ";
        }
        
        binTotal+=inHisto[binNum];
        
        //std::cout << " stretch: " << binNum << ": " << binTotal << " " << histoSum << "\n";
        //std::cout.flush();
    }
    
    for (uint32_t binNum=0; binNum<startBin; binNum++)
    {//leading 0 entries.
        //std::cout << " leading: " << binNum << "\n";
        //std::cout.flush();
        histoStretchMap[binNum]=0;
    }
    for (uint32_t binNum=startBin; binNum<=endBin; binNum++)
    {
        
        //std::cout << " map: " << binNum << "\n";
        //std::cout.flush();
        histoStretchMap[binNum]=(endBin-startBin)==0 ? histoSum : (binNum-startBin)/((double)(endBin-startBin))*255;
    }
    for (uint32_t binNum=(endBin+1); binNum<=255; binNum++)
    {//trailing 255 entries.
        //std::cout << " trailing: " << binNum << "\n";
        //std::cout.flush();
        histoStretchMap[binNum]=255;
    }
    
    return histoStretchMap;
}

MultiCPUHistogramConsumer::MultiCPUHistogramConsumer(ImageProducer& producer,
                                                     uint32_t images_per_slot,
                                                     uint32_t pixel_stride,
                                                     uint32_t image_stride) :
ImageConsumer(producer),
ImagesPerSlot_(images_per_slot),
PixelStride_(pixel_stride),
ImageStride_(image_stride)
{
    for (uint32_t i=0; i<images_per_slot; i++)
    {
        ImageFormat_.push_back(producer.getFormat(i));
        
        CalcMutexes_.push_back(std::shared_ptr<OpenThreads::Mutex>(new OpenThreads::Mutex()));
        
        Histograms_.push_back(std::shared_ptr< std::vector<int32_t> >(new std::vector<int32_t>));
        Histograms_[i]->resize(256);
        
        HistogramUpdatedVect_.push_back(false);
    }
}

MultiCPUHistogramConsumer::~MultiCPUHistogramConsumer()
{
    Thread_->setExit();
    Thread_->join();
}


bool MultiCPUHistogramConsumer::init()
{
    ImageConsumer::init();
    
    Thread_ = new MultiCPUHistogramConsumerThread(this);
    Thread_->startThread();
    
    return true;
}
