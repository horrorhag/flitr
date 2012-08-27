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

#ifndef MULTI_FFMPEG_PRODUCER_H
#define MULTI_FFMPEG_PRODUCER_H 1

#include <boost/tr1/memory.hpp>

#include <OpenThreads/Thread>
#include <OpenThreads/Mutex>
#include <OpenThreads/Barrier>

#include <flitr/image_producer.h>
#include <flitr/image_consumer.h>
#include <flitr/ffmpeg_producer.h>

namespace flitr {

class MultiFFmpegProducer;

class MultiFFmpegProducerThread : public OpenThreads::Thread {
  public: 
    MultiFFmpegProducerThread(MultiFFmpegProducer *producer, uint32_t thread_index) :
        Parent_(producer),
        Index_(thread_index),
        ShouldExit_(false) {}
    void run();
    void setExit() { ShouldExit_ = true; }
  private:
    MultiFFmpegProducer *Parent_;
    uint32_t Index_;
    bool ShouldExit_;
};

/**
 * Producer that reads from multiple input videos and places a frame
 * from each one into a shared slot.
 * 
 */
class FLITR_EXPORT MultiFFmpegProducer : public ImageProducer {
    friend class MultiFFmpegProducerThread;
  public:
    /** 
     * Creates the producer.
     * 
     * \param filenames Vector of video filenames.
     * \param out_pix_fmt The output pixel format to be used for all videos.
     * 
     */ 
    // \todo maybe change this so pix formats can be requested per input file
    MultiFFmpegProducer(std::vector<std::string> filenames, ImageFormat::PixelFormat out_pix_fmt, uint32_t buffer_size=FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS);
    ~MultiFFmpegProducer();

    bool setAutoLoadMetaData(std::tr1::shared_ptr<ImageMetadata> defaultMetadata);

    /** 
     * Perform initialisation that might fail on file opens.
     * 
     * \return True on success. 
     */
    bool init();

    /** 
     * Reads the next frame from all files and make it available. If
     * a file reaches the end, reading is resumed from the start.
     * 
     * \return True if a frame was successfully read from all files.
     */
    bool trigger();
    
    /** 
     * Reads a specified frame from the files.
     *
     * \param position Frame number to read (0 based).
     * 
     * \return True if a frame was successfully read.
     */
    bool seek(uint32_t position);

    /** 
     * Callback performed by the shared buffer when all consumers are
     * done with a read slot. This allows us to release the slots held
     * by our internal child consumers.
     * 
     */
    void releaseReadSlotCallback();
    
    /** 
     * Returns the number of frames in the longest file.
     * 
     * \return Number of frames.
     */
    uint32_t getNumImages() { return NumImages_; }
     
    /** 
     * Returns the number of the last correctly read image.
     * 
     * \return 0 to getNumImages()-1 for valid frames or -1 when not
     * successfully triggered.
     */
    int32_t getCurrentImage() { return CurrentImage_; }

    /**
     * Get the frame rate of the video at position imageNum in the slot.
     *
     * \return Frame rate.
     */
    uint32_t getFrameRate(const int imageNum) const {return Producers_[imageNum]->getFrameRate();}

  private:
    /// Child producers
    std::vector< std::tr1::shared_ptr<FFmpegProducer> > Producers_;
    /// Child consumers
    std::vector< std::tr1::shared_ptr<ImageConsumer> > Consumers_;
    /// Threads for triggering the producers in parallel.
    std::vector< MultiFFmpegProducerThread* > ProducerThreads_;
    /// Sync creation of the threads.
    OpenThreads::Barrier ProducerThreadBarrier_;

    uint32_t NumImages_;
    uint32_t CurrentImage_;
    uint32_t ImagesPerSlot_;
    uint32_t SeekPos_;
    std::vector< bool > SeekOK_;

    std::vector<Image**> OutputImageVector_;


    uint32_t buffer_size_;
};

}
#endif //MULTI_FFMPEG_PRODUCER_H
