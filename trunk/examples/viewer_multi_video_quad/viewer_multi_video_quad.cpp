#include <iostream>
#include <string>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/TrackballManipulator>
#include <osg/io_utils>

#include <flitr/multi_ffmpeg_producer.h>
#include <flitr/smulti_ffmpeg_producer.h>
#include <flitr/smulti_raw_video_file_producer.h>
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
    const uint32_t ReadableTarget_;
};

//#define USE_BACKGROUND_TRIGGER_THREAD 1

int main(int argc, char *argv[])
{
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " video_file(s)\n";
        return 1;
    }

    std::vector<std::string> fnames;
    for (int i=1; i<argc; i++) {
        fnames.push_back(argv[i]);
    }

    int num_videos = argc-1;

    shared_ptr<ImageProducer> mffp;

    shared_ptr<SMultiFFmpegProducer> mffpFFmpeg(new SMultiFFmpegProducer(fnames, ImageFormat::FLITR_PIX_FMT_ANY));
    shared_ptr<SMultiRawVideoFileProducer> mffpFvf(new SMultiRawVideoFileProducer(fnames, ImageFormat::FLITR_PIX_FMT_ANY));
    if (fnames[0].find(".fvf")!=std::string::npos)
    {
        mffp = mffpFvf;
    }
    else
    {
        mffp = mffpFFmpeg;
    }

    if (!mffp->init()) {
        std::cerr << "Could not load input files\n";
        exit(-1);
    }

#ifdef USE_BACKGROUND_TRIGGER_THREAD
    shared_ptr<BackgroundTriggerThread> btt(new BackgroundTriggerThread(mffp.get()));
    btt->startThread();
#endif

    shared_ptr<MultiOSGConsumer> osgc(new MultiOSGConsumer(*mffp, num_videos));
    if (!osgc->init()) {
        std::cerr << "Could not init OSG consumer\n";
        exit(-1);
    }

    osg::Group *root_node = new osg::Group;
    
    int xoffset=0;
    for(int i=0; i<num_videos; i++) {
        shared_ptr<TexturedQuad> quad(new TexturedQuad(osgc->getOutputTexture(i)));
        osg::Matrixd m;
        m.makeTranslate(osg::Vec3d(xoffset,0,0));
        quad->setTransform(m);
        root_node->addChild(quad->getRoot().get());
        xoffset += mffp->getFormat(i).getWidth();
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
    
    uint32_t frameRate;
    if (fnames[0].find(".fvf")!=std::string::npos)
    {
       frameRate = mffpFvf->getFrameRate(0);
    } else
    {
       frameRate = mffpFFmpeg->getFrameRate(0);
    }

    double framePeriodNS=1.0e9 / frameRate;
    double nextFrameTimeNS=currentTimeNanoSec()+framePeriodNS;

    while(!viewer.done()) {
#ifndef USE_BACKGROUND_TRIGGER_THREAD
        //Read from the video, but don't get more than 10 frames ahead.
        if (mffp->getLeastNumReadSlotsAvailable()<10)
        {
            mffp->trigger();
        }
#endif
        if (currentTimeNanoSec()>=nextFrameTimeNS)
        {
            if (osgc->getNext()) {
                viewer.frame();
                nextFrameTimeNS+=framePeriodNS;
            }
        }
        else
        {
            OpenThreads::Thread::microSleep(5000);
        }
    }

#ifdef USE_BACKGROUND_TRIGGER_THREAD
    btt->setExit();
    btt->join();
#endif   

    return 0;
}
