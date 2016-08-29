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

#ifndef MULTI_CPUHISTOGRAM_CONSUMER_H
#define MULTI_CPUHISTOGRAM_CONSUMER_H 1

#include <flitr/ffmpeg_utils.h>
#include <flitr/image_consumer.h>
#include <flitr/ffmpeg_writer.h>
#include <flitr/metadata_writer.h>

#include <flitr/flitr_thread.h>

namespace flitr {

class MultiCPUHistogramConsumer;

class MultiCPUHistogramConsumerThread : public FThread {
  public: 
    MultiCPUHistogramConsumerThread(MultiCPUHistogramConsumer *consumer) :
        Consumer_(consumer),
        ShouldExit_(false) {}
    void run();
    void setExit() { ShouldExit_ = true; }
  private:
    MultiCPUHistogramConsumer *Consumer_;
    bool ShouldExit_;
};

class FLITR_EXPORT MultiCPUHistogramConsumer : public ImageConsumer {
    friend class MultiCPUHistogramConsumerThread;
  public:

    MultiCPUHistogramConsumer(ImageProducer& producer, uint32_t images_per_slot, uint32_t pixel_stride=1, uint32_t image_stride=1);

    virtual ~MultiCPUHistogramConsumer();

    bool init();

    uint32_t getNumPixels(uint32_t im_number) const
    {
        return ImageFormat_[im_number].getWidth() * ImageFormat_[im_number].getHeight();
    }

    std::vector<int32_t> getHistogram(uint32_t im_number)
    {
        std::lock_guard<std::mutex> scopedLock(*(CalcMutexes_[im_number]));

        HistogramUpdatedVect_[im_number]=false;
        return *(Histograms_[im_number]);
    }

    bool isHistogramUpdated(uint32_t im_number) const
    {
        std::lock_guard<std::mutex> scopedLock(*(CalcMutexes_[im_number]));

        return HistogramUpdatedVect_[im_number];
    }

    static const std::vector<uint8_t> calcHistogramIdentityMap();
    static const std::vector<int32_t> calcRefHistogramForEqualisation(uint32_t histoSum);
    static const std::vector<uint8_t> calcHistogramMatchMap(const std::vector<int32_t> &inHisto, const std::vector<int32_t> &refHisto);
    static const std::vector<uint8_t> calcHistogramStretchMap(const std::vector<int32_t> &inHisto, const uint32_t histoSum,
                                                              const double ignoreBelow=0.0, const double ignoreAbove=1.0);

  private:
    std::vector<ImageFormat> ImageFormat_;
    const uint32_t ImagesPerSlot_;
    const uint32_t PixelStride_;

    const uint32_t ImageStride_;

    MultiCPUHistogramConsumerThread *Thread_;
    std::vector< std::shared_ptr<std::mutex> > CalcMutexes_;

    std::vector< std::shared_ptr< std::vector<int32_t> > > Histograms_;
    std::vector<bool> HistogramUpdatedVect_;

};

}

#endif //MULTI_CPUHISTOGRAM_CONSUMER_H
