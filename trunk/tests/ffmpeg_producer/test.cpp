#include <iostream>
#include <string>

#include <flitr/ffmpeg_producer.h>
#include <flitr/image_consumer.h>

using std::tr1::shared_ptr;
using namespace flitr;

class TestConsumer : public ImageConsumer {
  public:
    TestConsumer(ImageProducer& producer) :
        ImageConsumer(producer)
    {
        
    }
    bool init()
    {
        return true;
    }
    
    bool reserveOne()
    {
        std::vector<Image**> iv = reserveReadSlot();
        return (iv.size()!=0);
    }
    bool releaseOne()
    {
        releaseReadSlot();
        return true;
    }
};

int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " video_file\n";
        return 1;
    }
    
    shared_ptr<FFmpegProducer> ffp(new FFmpegProducer(argv[1], ImageFormat::FLITR_PIX_FMT_Y_8));
    if (!ffp->init()) {
        std::cerr << "Could not load " << argv[1] << "\n";
        exit(-1);
    }

    std::cout << argv[1] << " has " << ffp->getNumImages() << " frames.\n";

    shared_ptr<TestConsumer> tc(new TestConsumer(*ffp));
    tc->init();

    for (uint32_t i=0; i<ffp->getNumImages(); ++i) {
        ffp->trigger();
        if (!tc->reserveOne()) {
            std::cerr << "Expected to be able to read after trigger.\n";
            exit(-1);
        }
        tc->releaseOne();
    }
    
    std::cout << "Read all frames.\n";
    
    return 0;
}
