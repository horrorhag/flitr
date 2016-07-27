#ifndef FIFO_PRODUCER_H
#define FIFO_PRODUCER_H 1


#include <flitr/image_format.h>
#include <flitr/image_producer.h>
#include <flitr/log_message.h>
#include <flitr/high_resolution_time.h>

#include <thread>

namespace flitr {


class FifoProducer : public flitr::ImageProducer
{
  public:
	FifoProducer(const std::string fifoName, ImageFormat imgFormat, uint32_t bufferSize = FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS);
    
    ~FifoProducer();
    
    bool init();
    
  private:
    void readThread();

    std::thread _readThread;
    
    std::string _fifoName;
    bool _shouldExit;
    uint32_t _bufferSize;
};

} // namespace flitr


#endif //FIFO_PRODUCER_H
