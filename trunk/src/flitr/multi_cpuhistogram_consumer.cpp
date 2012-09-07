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
    uint32_t num_writers = Consumer_->ImagesPerSlot_;
    std::vector<Image**> imv;
    
    while (true) {
        // check if image available
        imv.clear();
        imv = Consumer_->reserveReadSlot();

        if (imv.size() == Consumer_->ImagesPerSlot_)
        {

            for (int imNum=0; imNum<Consumer_->ImagesPerSlot_; imNum++)
            {// Calculate the histogram.
                Image* im = *(imv[imNum]);

                OpenThreads::ScopedLock<OpenThreads::Mutex> wlock(*(Consumer_->CalcMutexes_[imNum]));

                std::vector<int32_t>* histogram=Consumer_->Histograms_[imNum].get();

                const uint32_t width=im->format()->getWidth();
                const uint32_t height=im->format()->getHeight();
                const uint32_t numPixels=width*height;
                const uint32_t numComponents=im->format()->getComponentsPerPixel();

                unsigned char * const data=(unsigned char *)im->data();
                const uint32_t numBins=histogram->size();
                const uint32_t pixelStride=1;

                // Zero the histogram.
                for (uint32_t binNum=0; binNum<=numBins; binNum++)
                {
                    (*histogram)[binNum]=0;
                }

                // Generate histogram.
                for (uint32_t i=0; i<(numPixels*numComponents); i+=(pixelStride*numComponents))
                {
                    if ((i%numComponents)==0)//Only considers the first component at the moment.
                    {
                        (*histogram)[data[i]]+=pixelStride;
                    }
                }
            }

            // indicate we are done with the image/s
            Consumer_->releaseReadSlot();
        } else
        {
            // wait a while for producers.
            Thread::microSleep(10000);
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

    float cuma=0.0f;
    float reqCuma=0.0f;
    uint32_t cumaBin=0;

    uint32_t binNum;
    for (binNum=0; binNum<256; binNum++)
    {
        reqCuma+=refHisto[binNum];

        while ((cuma<reqCuma)&&(cumaBin<256))
        {
            cuma+=inHisto[cumaBin];
//                        std::cout << cumaBin << " => " << binNum << " " << cuma << "/" << reqCuma << "\n";
//                        std::cout.flush();
            histoMatchMap[cumaBin]=binNum;
            cumaBin++;
        }
    }
    for (; cumaBin<256; cumaBin++)
    {
        histoMatchMap[cumaBin]=255;
    }

    return histoMatchMap;
}



MultiCPUHistogramConsumer::MultiCPUHistogramConsumer(ImageProducer& producer,
                                                     uint32_t images_per_slot) :
    ImageConsumer(producer),
    ImagesPerSlot_(images_per_slot)
{
    for (uint32_t i=0; i<images_per_slot; i++)
    {
        ImageFormat_.push_back(producer.getFormat(i));

        CalcMutexes_.push_back(std::tr1::shared_ptr<OpenThreads::Mutex>(new OpenThreads::Mutex()));

        Histograms_.push_back(std::tr1::shared_ptr< std::vector<int32_t> >(new std::vector<int32_t>));
        Histograms_[i]->resize(256);
    }
}

MultiCPUHistogramConsumer::~MultiCPUHistogramConsumer()
{
    Thread_->setExit();
    Thread_->join();
}


bool MultiCPUHistogramConsumer::init()
{
    Thread_ = new MultiCPUHistogramConsumerThread(this);
    Thread_->startThread();
    
    return true;
}
