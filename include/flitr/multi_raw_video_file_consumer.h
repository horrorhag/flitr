/* Framework for Live Image Transformation (FLITr)
 * Copyright (c) 2013 CSIR
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

#ifndef MULTI_RAW_VIDEO_FILE_CONSUMER_H
#define MULTI_RAW_VIDEO_FILE_CONSUMER_H 1

#include <flitr/stats_collector.h>
#include <flitr/image_consumer.h>
#include <flitr/metadata_writer.h>
#include <flitr/raw_video_file_writer.h>

#include <flitr/flitr_thread.h>

namespace flitr {

class MultiRawVideoFileConsumer;

/* This thread can be reused from the multi FFmpeg consumer */
class MultiRawVideoFileConsumerThread : public FThread {
  public: 
    MultiRawVideoFileConsumerThread(MultiRawVideoFileConsumer *consumer) :
        Consumer_(consumer),
        ShouldExit_(false) {}
    void run();
    void setExit() { ShouldExit_ = true; }
  private:
    MultiRawVideoFileConsumer *Consumer_;
    bool ShouldExit_;
};

/**
 * The MultiRawVideoFileConsumer class.
 *
 * This consumer makes use of the flitr::RawVideoFileWriter to write multiple
 * video streams to multiple FLITr Custom recordings.
 *
 * For more information about the custom FLITr recording format look at the
 * documentation on the flitr::RawVideoFileWriter.
 */
class FLITR_EXPORT MultiRawVideoFileConsumer : public ImageConsumer {
    friend class MultiRawVideoFileConsumerThread;
  public:
    MultiRawVideoFileConsumer(ImageProducer& producer, uint32_t images_per_slot);
    virtual ~MultiRawVideoFileConsumer();

    bool init();

    //!Open (for writing) one video file per image slot using the supplied basename file name.
    bool openFiles(std::string basename, const uint32_t frame_rate=FLITR_DEFAULT_VIDEO_FRAME_RATE);
    
    //!Open (for writing) one video file per image slot using the supplied basename file name and supplied postfixes.
    bool openFiles(std::string basename, std::vector<std::string> basename_postfixes, const uint32_t frame_rate=FLITR_DEFAULT_VIDEO_FRAME_RATE);

    //!Open (for writing) one video file per image slot using the supplied vector of file names.
    bool openFiles(std::vector<std::string> filenames, const uint32_t frame_rate=FLITR_DEFAULT_VIDEO_FRAME_RATE);

    bool startWriting();
    bool stopWriting();
    bool closeFiles();
		
  protected:
    std::vector<ImageFormat> ImageFormat_;
    uint32_t ImagesPerSlot_;
		
    MultiRawVideoFileConsumerThread *Thread_;
		
    std::vector<RawVideoFileWriter *> RawVideoFileWriters_;
    std::vector<MetadataWriter *> MetadataWriters_;

    std::mutex WritingMutex_;
    bool Writing_;

    std::shared_ptr<StatsCollector> MultiWriteStats_;

};

}

#endif //MULTI_RAW_VIDEO_FILE_CONSUMER_H
