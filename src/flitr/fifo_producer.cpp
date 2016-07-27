#include <flitr/fifo_producer.h>

#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>


flitr::FifoProducer::FifoProducer(const std::string fifoName, const ImageFormat imgFormat, uint32_t bufferSize) :
_fifoName(fifoName),
_shouldExit(false),
_bufferSize(bufferSize)
{
    ImageFormat_.push_back(imgFormat);
}

flitr::FifoProducer::~FifoProducer()
{
    _shouldExit=true;
    _readThread.join();
}

bool flitr::FifoProducer::init()
{
    // Allocate storage
    SharedImageBuffer_ = std::shared_ptr<SharedImageBuffer>(new SharedImageBuffer(*this, _bufferSize, 1));
    SharedImageBuffer_->initWithStorage();
    
    std::thread t(&FifoProducer::readThread, this);
    _readThread.swap(t);
    
    return true;
}


void flitr::FifoProducer::readThread()
{
    //signal(SIGPIPE,SIG_IGN);//Ignore SIGPIPE.
    
    const uint32_t bytesPerImage=ImageFormat_[0].getBytesPerImage();
    uint8_t * const fifoData=new uint8_t[bytesPerImage];
    
    int fd=-1;
    
    while(!_shouldExit)
    {
        if (fd==-1)
        {
            fd = open(_fifoName.c_str(), O_RDONLY | O_NONBLOCK);
            if (fd>=0) fcntl(fd, F_SETFL, O_NONBLOCK);
        }
        
        if (fd == -1)
        {
            usleep(1000);
            continue;
        }

        
        uint32_t bytesRead=0;
        
        while ((bytesRead < bytesPerImage) && (!_shouldExit))
        {
            int r=read(fd, fifoData + bytesRead, bytesPerImage - bytesRead);
            
            if (r<0)
            {
                if (errno == EAGAIN)
                {
                    usleep(100);
                    continue;
                } else
                {
                    close(fd);
                    fd=-1;
                    break;
                }
            } else
            {
                bytesRead+=r;
            }
        }
        
        
        std::vector<Image**> imvec = reserveWriteSlot();
        
        if (imvec.size() == 0)
        {
            // no storage available
            logMessage(LOG_CRITICAL) << ": dropping frames - no space in buffer\n";
            continue;
        }
        
        uint8_t *imgData=(*imvec[0])->data();
        
        memcpy(imgData, fifoData, bytesPerImage);
        
        // Notify that new data has been written to buffer.
        releaseWriteSlot();
    }
    
    close(fd);
    
    delete [] fifoData;
}

