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

#ifndef MULTI_FFMPEG_CONSUMER_H
#define MULTI_FFMPEG_CONSUMER_H 1

#include <flitr/ffmpeg_utils.h>
#include <flitr/image_consumer.h>
#include <flitr/ffmpeg_writer.h>
#include <flitr/metadata_writer.h>

#include <OpenThreads/Thread>
#include <OpenThreads/Mutex>

namespace flitr {

class MultiFFmpegConsumer;

class MultiFFmpegConsumerThread : public OpenThreads::Thread {
  public: 
    MultiFFmpegConsumerThread(MultiFFmpegConsumer *consumer) :
        Consumer_(consumer),
        ShouldExit_(false) {}
    void run();
    void setExit() { ShouldExit_ = true; }
  private:
    MultiFFmpegConsumer *Consumer_;
    bool ShouldExit_;
};

class FLITR_EXPORT MultiFFmpegConsumer : public ImageConsumer {
    friend class MultiFFmpegConsumerThread;
  public:
    MultiFFmpegConsumer(ImageProducer& producer, uint32_t images_per_slot);
    virtual ~MultiFFmpegConsumer();

    bool setCodec(VideoCodec codec, int32_t bit_rate=-1);
    bool setContainer(VideoContainer container);

    bool init();

    bool openFiles(std::string basename, const uint32_t frame_rate=FLITR_DEFAULT_VIDEO_FRAME_RATE);
    bool openFiles(std::string basename, std::vector<std::string> basename_postfixes, const uint32_t frame_rate=FLITR_DEFAULT_VIDEO_FRAME_RATE);
    bool openFiles(std::vector<std::string> filenames, const uint32_t frame_rate=FLITR_DEFAULT_VIDEO_FRAME_RATE);

    bool startWriting();
    bool stopWriting();
    uint32_t closeFiles();
    
    /** 
    * Returns the number of frames written in the file.
    * 
    * \return Number of frames.
    */
    uint32_t getNumImages() const;

  protected:
    std::vector<ImageFormat> ImageFormat_;
    uint32_t ImagesPerSlot_;

    VideoCodec Codec_;
    int32_t BitRate_;
    VideoContainer Container_;
        
    MultiFFmpegConsumerThread *Thread_;
        
    std::vector<FFmpegWriter *> FFmpegWriters_;
    std::vector<MetadataWriter *> MetadataWriters_;

    OpenThreads::Mutex WritingMutex_;
    bool Writing_;

    std::shared_ptr<StatsCollector> MultiWriteStats_;

};

}

#endif //MULTI_FFMPEG_CONSUMER_H
