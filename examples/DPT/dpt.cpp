#include <iostream>
#include <string>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/TrackballManipulator>
#include <osg/io_utils>

#include <flitr/modules/geometry_overlays/points_overlay.h>
#include <flitr/modules/flitr_image_processors/dpt/fip_dpt.h>
//#include <flitr/modules/flitr_image_processors/median/fip_median.h>

#include <flitr/ffmpeg_producer.h>
#include <flitr/test_pattern_producer.h>
#include <flitr/multi_ffmpeg_consumer.h>
#include <flitr/multi_osg_consumer.h>
#include <flitr/textured_quad.h>
#include <flitr/manipulator_utils.h>
#include <flitr/ortho_texture_manipulator.h>

using std::shared_ptr;
using namespace flitr;

class BackgroundTriggerThread : public FThread {
public:
    BackgroundTriggerThread(ImageProducer* p) :
    Producer_(p),
    ShouldExit_(false) {}
    
    void run()
    {
        while(!ShouldExit_)
        {
            if (!Producer_->trigger()) FThread::microSleep(5000);
        }
    }
    
    void setExit() { ShouldExit_ = true; }
    
private:
    ImageProducer* Producer_;
    bool ShouldExit_;
};


#define USE_BACKGROUND_TRIGGER_THREAD 1


int main(int argc, char *argv[])
{
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " video_file filter_pulse_size\n";
        return 1;
    }
    
    shared_ptr<FFmpegProducer> ip(new FFmpegProducer(argv[1], ImageFormat::FLITR_PIX_FMT_Y_8));
    if (!ip->init()) {
        std::cerr << "Could not load " << argv[1] << "\n";
        exit(-1);
    }
    
#ifdef USE_BACKGROUND_TRIGGER_THREAD
    shared_ptr<BackgroundTriggerThread> btt(new BackgroundTriggerThread(ip.get()));
    btt->startThread();
#endif

/*
    shared_ptr<FIPMedian> median(new FIPMedian(*ip, 8, 1, 5));
    if (!median->init()) {
        std::cerr << "Could not initialise the median processor.\n";
        exit(-1);
    }
    median->startTriggerThread();
*/
    
    /*
    const uint32_t firstStagePulseSize=10;
    
    shared_ptr<FIPDPT> dpt0(new FIPDPT(*ip, firstStagePulseSize, 1, 4));
    if (!dpt0->init()) {
        std::cerr << "Could not initialise the dpt processor.\n";
        exit(-1);
    }
    dpt0->startTriggerThread();
    */
    
    //shared_ptr<FIPDPT> dpt1(new FIPDPT(*dpt0, std::max<uint32_t>(atoi(argv[2]), firstStagePulseSize), 1, 4));
    shared_ptr<FIPDPT> dpt1(new FIPDPT(*ip, atoi(argv[2]), 1, 4));
    if (!dpt1->init()) {
        std::cerr << "Could not initialise the dpt processor.\n";
        exit(-1);
    }
    dpt1->startTriggerThread();
    
    
    shared_ptr<MultiOSGConsumer> osgc(new MultiOSGConsumer(*dpt1, 1, 4));
    if (!osgc->init()) {
        std::cerr << "Could not init OSG consumer\n";
        exit(-1);
    }    
    
    shared_ptr<MultiOSGConsumer> osgcOrig(new MultiOSGConsumer(*ip, 1, 4));
    if (!osgcOrig->init()) {
        std::cerr << "Could not init osgcOrig consumer\n";
        exit(-1);
    }
    
    
    /*
     shared_ptr<MultiFFmpegConsumer> mffc(new MultiFFmpegConsumer(*cnvrtToM8,1));
     if (!mffc->init()) {
     std::cerr << "Could not init FFmpeg consumer\n";
     exit(-1);
     }
     std::stringstream filenameStringStream;
     filenameStringStream << argv[1] << "_improved";
     mffc->openFiles(filenameStringStream.str());
     */
    
    osg::Group *root_node = new osg::Group;
    
    shared_ptr<TexturedQuad> quad(new TexturedQuad(osgc->getOutputTexture()));
    root_node->addChild(quad->getRoot().get());
    {
        osg::Matrix translate;
        translate.makeTranslate(osg::Vec3d(osgcOrig->getOutputTexture()->getTextureWidth()/2,
                                           0.0,
                                           0.0));
        quad->setTransform(translate);
    }
    
    shared_ptr<TexturedQuad> quadOrig(new TexturedQuad(osgcOrig->getOutputTexture()));
    root_node->addChild(quadOrig->getRoot().get());
    {
        osg::Matrix translate;
        translate.makeTranslate(osg::Vec3d(-osgcOrig->getOutputTexture()->getTextureWidth()/2,
                                           0.0,
                                           0.0));
        quadOrig->setTransform(translate);
    }
    
    
    osgViewer::Viewer viewer;
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.setSceneData(root_node);
    
    viewer.setUpViewInWindow(100, 100, 640, 480);
    viewer.realize();
    
        OrthoTextureManipulator* om = new OrthoTextureManipulator(osgc->getOutputTexture()->getTextureWidth(), osgc->getOutputTexture()->getTextureHeight());
        viewer.setCameraManipulator(om);
    
    size_t numFrames=0;
    while((!viewer.done())/*&&(ffp->getCurrentImage()<(ffp->getNumImages()*0.9f))*/)
    {
#ifndef USE_BACKGROUND_TRIGGER_THREAD
        //Read from the video, but don't get more than n frames ahead.
        if (ip->getLeastNumReadSlotsAvailable()<5)
        {
            ip->trigger();
        }
#endif
        
        if (osgc->getNext())
        {
            osgcOrig->getNext();
            
            viewer.frame();
            numFrames++;
        }
        
        FThread::microSleep(1000);
    }
    
    //     mffc->stopWriting();
    //     mffc->closeFiles();
    
    
#ifdef USE_BACKGROUND_TRIGGER_THREAD
    btt->setExit();
    btt->join();
#endif
    
    //dpt0->stopTriggerThread();
    dpt1->stopTriggerThread();
    
    return 0;
}
