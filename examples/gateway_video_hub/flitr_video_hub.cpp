#include "flitr_video_hub.h"

#include <flitr/ffmpeg_producer.h>
#include <flitr/multi_ffmpeg_consumer.h>
#include <flitr/multi_webrtc_consumer.h>

#include <flitr/modules/flitr_image_processors/cnvrt_to_float/fip_cnvrt_to_y_f32.h>

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
bool flitr::VideoHub::createRTSPProducer(std::string &name, const std::string &url)
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
bool flitr::VideoHub::createV4LProducer(std::string &name, const std::string &device)
{
    return false;
}

//====
bool flitr::VideoHub::createImageStabProcess(const std::string &name, const std::string &producerName)
{
    const auto it=_producerMap.find(producerName);
    
    if (it!=_producerMap.end())
    {
        /*
         //==
         shared_ptr<FIPConvertToYF32> cnvrtToYF32(new FIPConvertToYF32(*ip, 1, 2));
         if (!cnvrtToYF32->init())
         {
         std::cerr << "Could not initialise the cnvrtToF32 processor.\n";
         exit(-1);
         }
         
         cnvrtToYF32->startTriggerThread();
         
         //==
         shared_ptr<FIPLKStabilise> lkstabilise(new FIPLKStabilise(*cnvrtToYF32, 1,
         FIPLKStabilise::Mode::SUBPIXELSTAB,
         2));
         if (!lkstabilise->init())
         {
         std::cerr << "Could not initialise the lkstabilise processor.\n";
         exit(-1);
         }
         lkstabilise->startTriggerThread();
         
         
         //==
         shared_ptr<FIPConvertToY8> cnvrtToY8(new FIPConvertToY8(*lkstabilise, 1, 0.95f, 2));
         if (!cnvrtToY8->init())
         {
         std::cerr << "Could not initialise the cnvrtToY8 processor.\n";
         exit(-1);
         }
         cnvrtToY8->startTriggerThread();
         
         */
        return false;
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
        return false;
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
        std::shared_ptr<MultiFFmpegConsumer> mffc(new MultiFFmpegConsumer(*(it->second),1));
        
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
                                           const std::string &streamName)
{
    const auto it=_producerMap.find(producerName);
    
    if (it!=_producerMap.end())
    {
        std::shared_ptr<flitr::MultiWebRTCConsumer> ic(new flitr::MultiWebRTCConsumer(*(it->second), 1));
        
        if (!ic->init())
        {
            std::cerr << "Could not initialise MultiWebRTCConsumer" << " SOURCE: " __FILE__ << " " << __LINE__ << "\n";
            return false;
        }
        
        //Add new map entry and store the image consumer.
        _consumerMap[name]=ic;
        
        ic->openConnection(streamName); //Connection should be closed on destruct.
        
        return true;
    }
    
    return false;
}


//====
bool flitr::VideoHub::createImageBufferConsumer(const std::string &name, const std::string &producerName,
                                                uint8_t * const buffer)
{
    const auto it=_producerMap.find(producerName);
    
    if (it!=_producerMap.end())
    {
        return false;
    }
    
    return false;
}

//====
void flitr::VideoHub::trigger()
{
    for (auto &ip : _producerMap)
    {
        ip.second->trigger();
    }
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
    }
    
    return imgFrmt;
}
