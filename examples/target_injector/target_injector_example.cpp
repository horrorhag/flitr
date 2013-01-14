#include <iostream>
#include <string>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/TrackballManipulator>
#include <osg/io_utils>

#include <flitr/modules/target_injector/target_injector.h>
#include <flitr/ffmpeg_producer.h>
#include <flitr/multi_ffmpeg_consumer.h>
#include <flitr/multi_osg_consumer.h>
#include <flitr/textured_quad.h>
#include <flitr/manipulator_utils.h>
#include <flitr/ortho_texture_manipulator.h>

using std::tr1::shared_ptr;
using namespace flitr;

class BackgroundTriggerThread : public OpenThreads::Thread {
public:
    BackgroundTriggerThread(ImageProducer* p) :
        Producer_(p),
        ShouldExit_(false),
        ReadableTarget_(5) {}

    void run()
    {
        while(!ShouldExit_) {
            bool triggerred = false;
            while (Producer_->getLeastNumReadSlotsAvailable() < ReadableTarget_) {
                Producer_->trigger();
                triggerred = true;
            }
            if (!triggerred) {
                Thread::microSleep(5000);
            }
        }
    }

    void setExit() { ShouldExit_ = true; }

private:
    ImageProducer* Producer_;
    bool ShouldExit_;
    const uint32_t ReadableTarget_;
};

//#define USE_BACKGROUND_TRIGGER_THREAD 1

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

#ifdef USE_BACKGROUND_TRIGGER_THREAD
    shared_ptr<BackgroundTriggerThread> btt(new BackgroundTriggerThread(ffp.get()));
    btt->startThread();
#endif

    shared_ptr<TargetInjector> targetInjector(new TargetInjector(*ffp, 1));
    if (!targetInjector->init()) {
        std::cerr << "Could not initialise the target injector.\n";
        exit(-1);
    }


    targetInjector->setTargetBrightness(10.0f);

    targetInjector->addTarget(TargetInjector::SyntheticTarget(targetInjector->getDownstreamFormat(0).getWidth()*0.5f-targetInjector->getDownstreamFormat(0).getHeight()*0.5f, 0.5f,
                                                              1.0f, 1.0f, 0.1f,
                                                              0.8f, 0.4f,
                                                              0.0f, 0.0f));

    targetInjector->addTarget(TargetInjector::SyntheticTarget(targetInjector->getDownstreamFormat(0).getWidth()*0.5f-targetInjector->getDownstreamFormat(0).getHeight()*0.5f, targetInjector->getDownstreamFormat(0).getHeight()*0.45f,
                                                              1.0f, 0.0f, 0.1f,
                                                              0.8f, 0.4f,
                                                              0.0f, 0.0f));
    targetInjector->startTriggerThread();


    shared_ptr<MultiOSGConsumer> osgc(new MultiOSGConsumer(*targetInjector,1));
    if (!osgc->init()) {
        std::cerr << "Could not init OSG consumer\n";
        exit(-1);
    }

    shared_ptr<MultiFFmpegConsumer> mffc(new MultiFFmpegConsumer(*targetInjector,1));
    if (!mffc->init()) {
        std::cerr << "Could not init FFmpeg consumer\n";
        exit(-1);
    }
    std::stringstream filenameStringStream;
    filenameStringStream << argv[1] << "_ti";
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

    const int use_trackball = 0;
    if (use_trackball) {
        osgGA::TrackballManipulator* tb = new osgGA::TrackballManipulator;
        viewer.setCameraManipulator(tb);
        adjustCameraManipulatorHomeForYUp(tb);
    } else {

        ImageFormat imf = ffp->getFormat();
        OrthoTextureManipulator* om = new OrthoTextureManipulator(imf.getWidth(), imf.getHeight());
        viewer.setCameraManipulator(om);
    }

    while((!viewer.done())&&(ffp->getCurrentImage()<(ffp->getNumImages()*0.9f))) {
#ifndef USE_BACKGROUND_TRIGGER_THREAD
        //Read from the video, but don't get more than 10 frames ahead.
        if (ffp->getLeastNumReadSlotsAvailable()<10)
        {
            ffp->trigger();
        }
#endif

        if (!(targetInjector->isTriggerThreadStarted()))
        {
            targetInjector->trigger();
        }

        if (osgc->getNext()) {
            viewer.frame();
        }
    }

    mffc->stopWriting();
    mffc->closeFiles();

#ifdef USE_BACKGROUND_TRIGGER_THREAD
    btt->setExit();
    btt->join();
#endif   

    return 0;
}
