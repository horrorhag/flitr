
#include <flitr/modules/xml_config/xml_config.h>
#include <flitr/modules/flitr_image_processors/flip/fip_flip.h>
#include <flitr/modules/flitr_image_processors/rotate/fip_rotate.h>
#include <flitr/modules/flitr_image_processors/transform2D/fip_transform2D.h>

#include <flitr/modules/flitr_image_processors/cnvrt_to_float/fip_cnvrt_to_y_f32.h>
#include <flitr/modules/flitr_image_processors/cnvrt_to_float/fip_cnvrt_to_rgb_f32.h>
#include <flitr/modules/flitr_image_processors/cnvrt_to_8bit/fip_cnvrt_to_y_8.h>
#include <flitr/modules/flitr_image_processors/cnvrt_to_8bit/fip_cnvrt_to_rgb_8.h>
#include <flitr/modules/flitr_image_processors/gaussian_filter/fip_gaussian_filter.h>
#include <flitr/modules/flitr_image_processors/morphological_filter/fip_morphological_filter.h>

#include <flitr/ffmpeg_producer.h>
#include <flitr/test_pattern_producer.h>
#include <flitr/multi_ffmpeg_consumer.h>

#include <iostream>
#include <string>
#include <atomic>





int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::cout << "Usage: " << argv[0] << " video_file" << " config_file\n";
        return 1;
    }

    
    //=====================//
    std::string configFileName(argv[2]);
    
    std::shared_ptr<flitr::XMLConfig> cfg(new flitr::XMLConfig());
    
    if (!cfg->init(configFileName))
    {
        std::cerr << "Could not open configuration file." << std::endl;
        return 1;
    }
    
    //const auto channelElementVec=cfg->getAttributeVector("channel_setup");
    //const size_t numChannels=channelElementVec.size();
    //=====================//
    
    
    //=====================//
    std::shared_ptr<flitr::FFmpegProducer> ip(new flitr::FFmpegProducer(argv[1], flitr::ImageFormat::FLITR_PIX_FMT_Y_8, 6));
    if (!ip->init())
    {
        std::cerr << "Could not load " << argv[1] << "\n";
        exit(-1);
    }
    //=====================//

    
    size_t frameNum=0;
    
    while(1)
    {
        if (ip->getLeastNumReadSlotsAvailable()<3)
        {
            ip->trigger();
            std::cout << frameNum << "\n";
        } else
        {
            FThread::microSleep(1000);
        }
    }
    
    return 0;
}
