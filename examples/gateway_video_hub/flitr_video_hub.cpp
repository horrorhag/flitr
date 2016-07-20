#include "flitr_video_hub.h"

#include <flitr/ffmpeg_producer.h>
#include <flitr/multi_ffmpeg_consumer.h>
#include <flitr/fifo_consumer.h>
#include <flitr/multi_image_buffer_consumer.h>

#include <flitr/modules/flitr_image_processors/cnvrt_to_float/fip_cnvrt_to_y_f32.h>
#include <flitr/modules/flitr_image_processors/stabilise/fip_lk_stabilise.h>
#include <flitr/modules/flitr_image_processors/motion_detect/fip_motion_detect.h>
#include <flitr/modules/flitr_image_processors/cnvrt_to_8bit/fip_cnvrt_to_y_8.h>
#include <flitr/modules/flitr_image_processors/cnvrt_to_8bit/fip_cnvrt_to_rgb_8.h>

//====
flitr::VideoHub::VideoHub()
{}

//====
flitr::VideoHub::~VideoHub()
{
    //Stop the trigger threads of all image processors.
    for (auto ipp : _producerMap)
    {
        ImageProcessor *proc=dynamic_cast<ImageProcessor *>(ipp.second.get());
        
        if (proc)
        {
            proc->stopTriggerThread();
        }
    }
}

//====
bool flitr::VideoHub::init()
{
    return true;
}

//====
void flitr::VideoHub::stopAllThreads()
{
}

//====
bool flitr::VideoHub::createVideoFileProducer(const std::string &name, const std::string &filename)
{
    std::shared_ptr<flitr::FFmpegProducer> ip(new flitr::FFmpegProducer(filename, flitr::ImageFormat::FLITR_PIX_FMT_RGB_8, 2));
    
    if (!ip->init())
    {
        std::cerr << "Could not open " << filename << " SOURCE: " __FILE__ << " " << __LINE__ << "\n";
        return false;
    }
    
    //Add new map entry and store the image producer.
    _producerMap[name]=ip;
    
    return true;
}

//====
bool flitr::VideoHub::createRTSPProducer(const std::string &name, const std::string &url)
{
    std::shared_ptr<flitr::FFmpegProducer> ip(new flitr::FFmpegProducer(url, flitr::ImageFormat::FLITR_PIX_FMT_RGB_8, 2));
    
    if (!ip->init())
    {
        std::cerr << "Could not open " << url << " SOURCE: " __FILE__ << " " << __LINE__ << "\n";
        return false;
    }
    
    //Add new map entry and store the image producer.
    _producerMap[name]=ip;
    
    return true;
}

//====
bool flitr::VideoHub::createV4LProducer(const std::string &name, const std::string &device)
{
    return false;
}

//====
bool flitr::VideoHub::createImageStabProcess(const std::string &name, const std::string &producerName, const double outputFilter)
{
    const auto it=_producerMap.find(producerName);
    
    if (it!=_producerMap.end())
    {
        //==
        std::shared_ptr<FIPConvertToYF32> cnvrtToYF32(new FIPConvertToYF32(*(it->second), 1, 2));
        if (!cnvrtToYF32->init())
        {
            std::cerr << "Could not initialise the cnvrtToF32 processor "<< " SOURCE: " __FILE__ << " " << __LINE__ << "\n";
            return false;
        }
        
        cnvrtToYF32->startTriggerThread();
        
        //==
        std::shared_ptr<FIPLKStabilise> lkstabilise(new FIPLKStabilise(*cnvrtToYF32, 1, FIPLKStabilise::Mode::SUBPIXELSTAB, 2));
        if (!lkstabilise->init())
        {
            std::cerr << "Could not initialise the lkstabilise processor "<< " SOURCE: " __FILE__ << " " << __LINE__ << "\n";
            return false;
        }
        lkstabilise->setupOutputTransformBurn(outputFilter, outputFilter); //High pass filter output transform.
        lkstabilise->startTriggerThread();
        
        
        //==
        std::shared_ptr<FIPConvertToRGB8> cnvrtToRGB8(new FIPConvertToRGB8(*lkstabilise, 1, 0.95f, 2));
        if (!cnvrtToRGB8->init())
        {
            std::cerr << "Could not initialise the cnvrtToY8 processor "<< " SOURCE: " __FILE__ << " " << __LINE__ << "\n";
            return false;
        }
        cnvrtToRGB8->startTriggerThread();
        
        
        //Add new map entries and store the image producers.
        _producerMap[name+std::string("_cnvrtToF32")]=cnvrtToYF32;
        _producerMap[name+std::string("_lkstabiliseF32")]=lkstabilise;
        _producerMap[name]=cnvrtToRGB8;
        
        return true;
    }
    
    return false;
}

//====
bool flitr::VideoHub::createMotionDetectProcess(const std::string &name, const std::string &producerName,
                                                const bool showOverlays, const bool produceOnlyMotionImages)
{
    const auto it=_producerMap.find(producerName);
    
    if (it!=_producerMap.end())
    {
        //==
        std::shared_ptr<FIPMotionDetect> motionDetect(new FIPMotionDetect(*(it->second), 1, 2));
        if (!motionDetect->init())
        {
            std::cerr << "Could not initialise the lkstabilise processor "<< " SOURCE: " __FILE__ << " " << __LINE__ << "\n";
            return false;
        }
        motionDetect->startTriggerThread();
        
        //Add new map entry and store the image producer.
        _producerMap[name]=motionDetect;
        
        return true;
    }
    
    return false;
}

//====
bool flitr::VideoHub::createVideoFileConsumer(const std::string &name, const std::string &producerName,
                                              const std::string &fileName, const double frameRate)
{
    const auto it=_producerMap.find(producerName);
    
    if (it!=_producerMap.end())
    {
        std::shared_ptr<MultiFFmpegConsumer> mffc(new MultiFFmpegConsumer(*(it->second), 1));
        
        if (!mffc->init())
        {
            std::cerr << "Could not initialise MultiFFmpegConsumer" << " SOURCE: " __FILE__ << " " << __LINE__ << "\n";
            return false;
        }
        
        //Add new map entry and store the image consumer.
        _consumerMap[name]=mffc;
        
        mffc->openFiles(fileName, frameRate);
        mffc->startWriting();
        
        return true;
    }
    
    return false;
}

//====
bool flitr::VideoHub::createRTSPConsumer(const std::string &name, const std::string &producerName,
                                         const int port)
{
    const auto it=_producerMap.find(producerName);
    
    if (it!=_producerMap.end())
    {
        return false;
    }
    
    return false;
}

//====
bool flitr::VideoHub::createWebRTCConsumer(const std::string &name, const std::string &producerName,
                                           const std::string &fifoName)
{
    const auto it=_producerMap.find(producerName);
    
    if (it!=_producerMap.end())
    {
        std::shared_ptr<flitr::FifoConsumer> fifo(new flitr::FifoConsumer(*(it->second), fifoName));
        
        if (!fifo->init())
        {
            std::cerr << "Could not initialise WebRTC FifoConsumer" << " SOURCE: " __FILE__ << " " << __LINE__ << "\n";
            return false;
        }
        
        //Add new map entry and store the image consumer.
        _consumerMap[name]=fifo;
        
        return true;
    }
    
    return false;
}

//====
bool flitr::VideoHub::createImageBufferConsumer(const std::string &name, const std::string &producerName)
{
    const auto it=_producerMap.find(producerName);
    
    if (it!=_producerMap.end())
    {
        std::shared_ptr<flitr::MultiImageBufferConsumer> imageBufferConsumer(new flitr::MultiImageBufferConsumer(*(it->second), 1));
        
        if (!imageBufferConsumer->init())
        {
            std::cerr << "Could not initialise ImageBufferConsumer" << " SOURCE: " __FILE__ << " " << __LINE__ << "\n";
            return false;
        }
        
        //Add new map entry and store the image consumer.
        _consumerMap[name]=imageBufferConsumer;
        
        return true;
    }
    
    return false;
}

//====
bool flitr::VideoHub::imageBufferConsumerSetBuffer(const std::string &consumerName, uint8_t * const buffer)
{
    const auto it=_consumerMap.find(consumerName);
    
    if (it!=_consumerMap.end())
    {
        MultiImageBufferConsumer *mibc=dynamic_cast<MultiImageBufferConsumer *>(it->second.get());
        
        if (mibc!=nullptr)
        {
            std::vector<uint8_t *> bufferVec;
            bufferVec.push_back(buffer);
            
            mibc->setBufferVec(bufferVec);
            
            return true;
        }
        
        return false;
    }
    
    return false;
}

//====
bool flitr::VideoHub::imageBufferConsumerHold(const std::string &consumerName, const bool bufferHold)
{
    const auto it=_consumerMap.find(consumerName);
    
    if (it!=_consumerMap.end())
    {
        MultiImageBufferConsumer *mibc=dynamic_cast<MultiImageBufferConsumer *>(it->second.get());
        
        if (mibc!=nullptr)
        {
            mibc->setBufferHold(bufferHold);
            
            return true;
        }
        
        return false;
    }
    
    return false;
}

//====
std::shared_ptr<flitr::ImageProducer> flitr::VideoHub::getProducer(const std::string &producerName)
{
    std::shared_ptr<flitr::ImageProducer> ipp;
    
    const auto it=_producerMap.find(producerName);
    
    if (it!=_producerMap.end())
    {
        ipp=it->second;
    }
    
    return ipp;
}

//====
flitr::VideoHubImageFormat flitr::VideoHub::getImageFormat(const std::string &producerName) const
{
    VideoHubImageFormat imgFrmt;
    
    const auto it=_producerMap.find(producerName);
    
    if (it!=_producerMap.end())
    {
        const ImageFormat flitrImgFrmt=it->second->getFormat();
        imgFrmt._width=flitrImgFrmt.getWidth();
        imgFrmt._height=flitrImgFrmt.getHeight();
        imgFrmt._pixelFormat=ImageFormat::FLITR_PIX_FMT_RGB_8;
        imgFrmt._bytesPerPixel=3;
    }
    
    return imgFrmt;
}
