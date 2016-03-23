#include <iostream>
#include <string>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/TrackballManipulator>
#include <osg/io_utils>

#include <flitr/image_multiplexer.h>

#include <flitr/ffmpeg_producer.h>
#include <flitr/multi_ffmpeg_consumer.h>
#include <flitr/multi_osg_consumer.h>
#include <flitr/textured_quad.h>
#include <flitr/manipulator_utils.h>
#include <flitr/ortho_texture_manipulator.h>

using std::shared_ptr;
using namespace flitr;


class BackgroundTriggerThread : public OpenThreads::Thread {
public:
    BackgroundTriggerThread(ImageProducer* p) :
        Producer_(p),
        ShouldExit_(false) {}

    void run()
    {
        while(!ShouldExit_)
        {
            if (!Producer_->trigger()) Thread::microSleep(5000);
        }
    }

    void setExit() { ShouldExit_ = true; }

private:
    ImageProducer* Producer_;
    bool ShouldExit_;
};


int main(int argc, char *argv[])
{
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " video_file_1 video_file_2\n";
        return 1;
    }

    shared_ptr<FFmpegProducer> ffp01(new FFmpegProducer(argv[1], ImageFormat::FLITR_PIX_FMT_Y_8));
    if (!ffp01->init()) {
        std::cerr << "Could not load " << argv[1] << "\n";
        exit(-1);
    }
    shared_ptr<BackgroundTriggerThread> btt01(new BackgroundTriggerThread(ffp01.get()));
    btt01->startThread();

    shared_ptr<FFmpegProducer> ffp02(new FFmpegProducer(argv[2], ImageFormat::FLITR_PIX_FMT_RGB_8));
    if (!ffp02->init()) {
        std::cerr << "Could not load " << argv[2] << "\n";
        exit(-1);
    }
    shared_ptr<BackgroundTriggerThread> btt02(new BackgroundTriggerThread(ffp02.get()));
    btt02->startThread();


    shared_ptr<ImageMultiplexer> imageMultiplexer(new ImageMultiplexer(1024, 768,
                                                                       flitr::ImageFormat::FLITR_PIX_FMT_RGB_8,
                                                                       1));
    imageMultiplexer->addUpstreamProducer(*ffp01);
    imageMultiplexer->addUpstreamProducer(*ffp02);

    if (!imageMultiplexer->init()) {
        std::cerr << "Could not initialise the ImageMultiplexer.\n";
        exit(-1);
    }


    imageMultiplexer->startTriggerThread();


    shared_ptr<MultiOSGConsumer> osgc(new MultiOSGConsumer(*imageMultiplexer,1));
    if (!osgc->init()) {
        std::cerr << "Could not init OSG consumer\n";
        exit(-1);
    }

    shared_ptr<MultiFFmpegConsumer> mffc(new MultiFFmpegConsumer(*imageMultiplexer,1));
    if (!mffc->init()) {
        std::cerr << "Could not init FFmpeg consumer\n";
        exit(-1);
    }
    std::stringstream filenameStringStream;
    filenameStringStream << "result";
    mffc->openFiles(filenameStringStream.str());
    mffc->startWriting();

    osg::Group *root_node = new osg::Group;
    
    shared_ptr<TexturedQuad> quad(new TexturedQuad(osgc->getOutputTexture()));
    root_node->addChild(quad->getRoot().get());

    osgViewer::Viewer viewer;
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.setSceneData(root_node);

    //viewer.setUpViewInWindow(480+40, 40, 640, 480);
    viewer.realize();

        //ImageFormat imf = ffp01->getFormat();
        OrthoTextureManipulator* om = new OrthoTextureManipulator(imageMultiplexer->getDownstreamFormat().getWidth(),
                                                                  imageMultiplexer->getDownstreamFormat().getHeight());
        viewer.setCameraManipulator(om);

    uint64_t viewerFrame=0;
    while(!viewer.done())
    {
        if (!(imageMultiplexer->isTriggerThreadStarted()))
        {
            imageMultiplexer->trigger();
        }

        imageMultiplexer->setSingleSource((viewerFrame>>5)%imageMultiplexer->getNumSources());

        if (osgc->getNext())
        {
            viewer.frame();
            viewerFrame++;
        }

    }

    mffc->stopWriting();
    mffc->closeFiles();

    btt01->setExit();
    btt01->join();

    btt02->setExit();
    btt02->join();

    return 0;
}
