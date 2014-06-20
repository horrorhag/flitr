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

#include <flitr/multi_raw_video_file_consumer.h>
#include <flitr/image_producer.h>

using namespace flitr;

void MultiRawVideoFileConsumerThread::run()
{
    size_t num_writers = Consumer_->ImagesPerSlot_;
    std::vector<Image**> imv;

    while (true) {
        // check if image available
        imv.clear();
        imv = Consumer_->reserveReadSlot();
        if (imv.size() >= num_writers) { // allow selection of some sources                       
            OpenThreads::ScopedLock<OpenThreads::Mutex> wlock(Consumer_->WritingMutex_);
            if (Consumer_->Writing_) {
                Consumer_->MultiWriteStats_->tick();

                {
                int i=0;
                #pragma omp parallel for
                for (i=0; i<(int)num_writers; i++) {
                    if (Consumer_->RawVideoFileWriters_[i])
                    {
                        Image* im = *(imv[i]);

                        Consumer_->RawVideoFileWriters_[i]->writeVideoFrame(im->data());
                        Consumer_->MetadataWriters_[i]->writeFrame(*im);
                    }
                }
                }

                Consumer_->MultiWriteStats_->tock();
            } else {
                // just discard
            }
            // indicate we are done with the image/s
            Consumer_->releaseReadSlot();
        } else {
            // wait a while for producers.
            Thread::microSleep(1000);
        }
        // check for exit
        if (ShouldExit_) {
            break;
        }
    }
}

MultiRawVideoFileConsumer::MultiRawVideoFileConsumer(ImageProducer& producer,
                                         uint32_t images_per_slot) :
    ImageConsumer(producer),
    ImagesPerSlot_(images_per_slot),
    Writing_(false)
{
    std::stringstream write_stats_name;
    write_stats_name << " MultiRawVideoFileConsumer::write";
    MultiWriteStats_ = std::shared_ptr<StatsCollector>(new StatsCollector(write_stats_name.str()));

    for (uint32_t i=0; i<images_per_slot; i++) {
        ImageFormat_.push_back(producer.getFormat(i));
    }
}

MultiRawVideoFileConsumer::~MultiRawVideoFileConsumer()
{
    closeFiles();

    Thread_->setExit();
    Thread_->join();
}

bool MultiRawVideoFileConsumer::init()
{
    RawVideoFileWriters_.resize(ImagesPerSlot_);
    MetadataWriters_.resize(ImagesPerSlot_);

    Thread_ = new MultiRawVideoFileConsumerThread(this);
    Thread_->startThread();
    
    return true;
}

bool MultiRawVideoFileConsumer::openFiles(std::string basename, const uint32_t frame_rate)
{
    std::vector<std::string> postfixes;

    for (unsigned int i=0; i<ImagesPerSlot_; i++)
    {
        char c_count[16];
        sprintf(c_count, "%02d", i+1);
        postfixes.push_back(std::string(c_count));
    }

    return openFiles(basename,postfixes,frame_rate);
}

bool MultiRawVideoFileConsumer::openFiles(std::string basename, std::vector<std::string> basename_postfixes, const uint32_t frame_rate)
{
    if (basename_postfixes.size()==ImagesPerSlot_)
    {
        std::vector<std::string> filenames;

        for (unsigned int i=0; i<ImagesPerSlot_; i++)
        {
            filenames.push_back(basename + "_" + basename_postfixes[i]);
        }

        return openFiles(filenames,frame_rate);
    } else
    {
        return false;
    }
}

bool MultiRawVideoFileConsumer::openFiles(std::vector<std::string> filenames, const uint32_t frame_rate)
{
    if (filenames.size()==ImagesPerSlot_)
    {
        for (unsigned int i=0; i<ImagesPerSlot_; i++)
        {
            if (filenames[i]!="")
            {
                std::string video_filename(filenames[i] + ".fvf");
                std::string metadata_filename(filenames[i] + ".meta");

                RawVideoFileWriters_[i] = new RawVideoFileWriter(video_filename, ImageFormat_[i], frame_rate);
                MetadataWriters_[i] = new MetadataWriter(metadata_filename);
            } else
            {//If the filename is "" then the recording is disbaled.
                RawVideoFileWriters_[i] = 0;
                MetadataWriters_[i] = 0;
            }
        }
        return true;
    } else
    {
        return false;
    }
}


bool MultiRawVideoFileConsumer::startWriting()
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> wlock(WritingMutex_);
    Writing_ = true;
    return true;
}

bool MultiRawVideoFileConsumer::stopWriting()
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> wlock(WritingMutex_);
    Writing_ = false;
    return true;
}

bool MultiRawVideoFileConsumer::closeFiles()
{
    stopWriting();
    
    for (unsigned int i=0; i<ImagesPerSlot_; i++) {
        if (RawVideoFileWriters_[i] != 0) {
            delete RawVideoFileWriters_[i];
            RawVideoFileWriters_[i] = 0;
        }
        if (MetadataWriters_[i] != 0) {
            delete MetadataWriters_[i];
            MetadataWriters_[i] = 0;
        }
    }
    return true;
}
