#include <iostream>
#include <string>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/TrackballManipulator>
#include <osg/io_utils>

#include <flitr/ffmpeg_producer.h>
#include <flitr/multi_osg_consumer.h>
#include <flitr/textured_quad.h>
#include <flitr/manipulator_utils.h>

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
    const int ReadableTarget_;
};

#define USE_BACKGROUND_TRIGGER_THREAD 1

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

    int history_size = 8;

    shared_ptr<MultiOSGConsumer> osgc(new MultiOSGConsumer(*ffp,1,history_size));
    if (!osgc->init()) {
        std::cerr << "Could not init OSG consumer\n";
        exit(-1);
    }

    osg::Group *root_node = new osg::Group;
    
    int xoffset=0;
    for(int i=0; i<history_size; i++) {
        shared_ptr<TexturedQuad> quad(new TexturedQuad(osgc->getOutputTexture(0,i)));
        osg::Matrixd m;
        m.makeTranslate(osg::Vec3d(xoffset,0,0));
        quad->setTransform(m);
        root_node->addChild(quad->getRoot().get());
        xoffset += ffp->getFormat().getWidth();
    }

    osgViewer::Viewer viewer;
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.setSceneData(root_node);

    //viewer.setUpViewInWindow(480+40, 40, 640, 480);
    viewer.realize();
     osgGA::TrackballManipulator* tb = new osgGA::TrackballManipulator;
    viewer.setCameraManipulator(tb);
    adjustCameraManipulatorHomeForYUp(tb);
    
    while(!viewer.done()) {
#ifndef USE_BACKGROUND_TRIGGER_THREAD
        ffp->trigger();
#endif
        if (osgc->getNext()) {
            viewer.frame();
            OpenThreads::Thread::microSleep(40000);
        } else {
            OpenThreads::Thread::microSleep(5000);
        }
    }

#ifdef USE_BACKGROUND_TRIGGER_THREAD
    btt->setExit();
    btt->join();
#endif   

    return 0;
}
