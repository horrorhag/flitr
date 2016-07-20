#ifndef FIFO_CONSUMER_H
#define FIFO_CONSUMER_H

#include <thread>

#include <flitr/image_producer.h>
#include <flitr/image_consumer.h>

#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace flitr {

class FifoConsumer : public flitr::ImageConsumer {
  public:
    FifoConsumer(flitr::ImageProducer& producer, std::string fifo_name) :
        flitr::ImageConsumer(producer),
        total_frame_size_((producer.getFormat().getBytesPerImage())),
        should_exit_(false),
        fifo_name_(fifo_name),
        im_(producer.getFormat())
    {
        std::thread t(&FifoConsumer::writeThread, this);
        write_thread_.swap(t);
    }
    ~FifoConsumer()
    {
        should_exit_ = true;
        write_thread_.join();
    }
    // just prevent overflow by discarding images
    void clearQueue()
    {
        std::vector<flitr::Image**> imv;
        uint8_t ims_per_slot = 1;
        // sync access to the buffer
        uint32_t num_avail = getNumReadSlotsAvailable();
        // discard all but...
        uint32_t num_to_leave = 1;
        if (num_avail > num_to_leave) {
            for (uint32_t i=0; i<(num_avail-num_to_leave); i++) {
                imv = reserveReadSlot();
                if (imv.size() >= ims_per_slot) {
                    //fprintf(stderr, "discarded frame...\n");
                    releaseReadSlot();
                }
            }
        }
    }

//    "( videotestsrc ! video/x-raw-yuv,width=320,height=240,framerate=10/1 ! x264enc ! queue ! rtph264pay name=pay0 pt=96 ! audiotestsrc ! audio/x-raw-int,rate=8000 ! alawenc ! rtppcmapay name=pay1 pt=97 )"

    void writeThread()
    {
        signal(SIGPIPE,SIG_IGN);

        while (!should_exit_) {
            int fd;
            //printf("waiting for readers\n");
            fd = open(fifo_name_.c_str(), O_WRONLY | O_NONBLOCK);
            if (fd == -1) {
                usleep(100000);
                clearQueue();
                continue;
            }
            //int fl = fcntl(fd, F_GETFL, 0);
            //fcntl(fd, F_SETFL, fl | O_NONBLOCK);
            fcntl(fd, F_SETFL, O_NONBLOCK);
            flitr::logMessage(flitr::LOG_CRITICAL) << "Reader connected to " << fifo_name_ << "\n";

            uint8_t count=0;
            while(1) {                
                clearQueue();
                // get an image
                std::vector<flitr::Image**> imv = reserveReadSlot();
                flitr::Image *imp;
                if (imv.size()!=0) {
                    imp = *(imv[0]);
                    // copy, otherwise when we block on write we'll stall the producer
                    im_ = *imp; 
                    releaseReadSlot();
                } else {
                    // no frame, wait and try again
                    usleep(1000);
                    continue;
                }
                int total_w=0;
                int num_w;
                int borked_pipe=0;
                do {
                    if ((num_w = write(fd, (char*)&(im_.data()[total_w]), total_frame_size_-total_w)) == -1) {
                        if (errno == EAGAIN) {
                            usleep(100);
                            clearQueue();
                            continue;
                        } else {
                            flitr::logMessage(flitr::LOG_DEBUG) << "Write error on " << fifo_name_ << "\n";
                            //perror("write");
                            borked_pipe=1;
                            break;
                        }
                    } else {
                        //printf("wrote %d bytes\n", num_w);
                        total_w += num_w;
                    }
                } while(!borked_pipe && (total_w != total_frame_size_));
                if (borked_pipe) {
                    break;
                }
                count++;
                //std::cerr << (int)count << "\n";
            }
            close(fd);
            flitr::logMessage(flitr::LOG_CRITICAL) << "Closed " << fifo_name_ << "\n";
        }
    }
  private:
    std::thread write_thread_;
    int total_frame_size_;
    bool should_exit_;
    std::string fifo_name_;
    flitr::Image im_;
};

}

#endif
