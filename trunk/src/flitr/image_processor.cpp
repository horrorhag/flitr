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

#include <flitr/image_processor.h>

using namespace flitr;
using std::tr1::shared_ptr;

void ImageProcessorThread::run()
{
    const uint32_t num_images = IP_->ImagesPerSlot_;
    std::vector<Image**> imvRead;
    std::vector<Image**> imvWrite;

    while (true)
    {

        if ((IP_->getNumReadSlotsAvailable())&&(IP_->getNumWriteSlotsAvailable()))
        {
            imvRead.clear();
            imvWrite.clear();

            imvRead = IP_->reserveReadSlot();
            imvWrite = IP_->reserveWriteSlot();


            IP_->ProcessorStats_->tick();

            unsigned int i=0;
            //#pragma omp parallel for
            for (i=0; i<num_images; i++)
            {
                Image const * const imRead = *(imvRead[i]);
                Image * const imWrite = *(imvWrite[i]);
                const ImageFormat imFormat=IP_->getFormat(i);


                //===

                ImageFormat::PixelFormat pixelFormat=imFormat.getPixelFormat();

                const uint32_t width=imFormat.getWidth();
                const uint32_t height=imFormat.getHeight();
                const uint32_t bytesPerPixel=imFormat.getBytesPerPixel();
                uint8_t const * const dataRead=imRead->data();
                uint8_t * const dataWrite=imWrite->data();

                uint32_t offset=0;

                //Do image copy and processing here...
                for (uint32_t y=0; y<height; y++)
                {
                    for (uint32_t x=0; x<width; x++)
                    {
                        switch (pixelFormat)
                        {
                        case ImageFormat::FLITR_PIX_FMT_Y_8 :
                            dataWrite[offset]=dataRead[offset]/3;
                            break;
                        case ImageFormat::FLITR_PIX_FMT_RGB_8 :
                            dataWrite[offset]=dataRead[offset];
                            dataWrite[offset+1]=dataRead[offset+1];
                            dataWrite[offset+2]=dataRead[offset+2];
                            break;
                        case ImageFormat::FLITR_PIX_FMT_Y_16 :
                            dataWrite[offset]=dataRead[offset];
                            dataWrite[offset+1]=dataRead[offset+1];
                            break;
                        //default:
                        }
                        offset+=bytesPerPixel;
                    }
                }

                //===

            }

            IP_->ProcessorStats_->tock();


            IP_->releaseWriteSlot();
            IP_->releaseReadSlot();

        } else {
            // wait a while for producers and consumers.
            Thread::microSleep(1000);
        }

        // check for exit
        if (ShouldExit_) {
            break;
        }
    }
}

ImageProcessor::ImageProcessor(ImageProducer& producer,
                               uint32_t images_per_slot, uint32_t buffer_size) :
    ImageConsumer(producer),
    ImagesPerSlot_(images_per_slot),
    buffer_size_(buffer_size)
{
    std::stringstream stats_name;
    stats_name << " ImageProcessor::process";
    ProcessorStats_ = std::tr1::shared_ptr<StatsCollector>(new StatsCollector(stats_name.str()));

    for (uint32_t i=0; i<images_per_slot; i++) {
        ImageFormat_.push_back(producer.getFormat(i));
    }
}

ImageProcessor::~ImageProcessor()
{
    Thread_->setExit();
    Thread_->join();
}

bool ImageProcessor::init()
{
    Thread_ = new ImageProcessorThread(this);
    Thread_->startThread();

    // Allocate storage
    SharedImageBuffer_ = shared_ptr<SharedImageBuffer>(
        new SharedImageBuffer(*this, buffer_size_, ImagesPerSlot_));
    SharedImageBuffer_->initWithStorage();

    return true;
}

