#include <iostream>
#include <string>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/TrackballManipulator>
#include <osg/io_utils>

#include <flitr/modules/geometry_overlays/points_overlay.h>
#include <flitr/modules/flitr_image_processors/classification_RCNN/fip_classification_RCNN.h>


#include <flitr/ffmpeg_producer.h>
#include <flitr/test_pattern_producer.h>
#include <flitr/multi_ffmpeg_consumer.h>
#include <flitr/multi_osg_consumer.h>
#include <flitr/textured_quad.h>
#include <flitr/manipulator_utils.h>
#include <flitr/ortho_texture_manipulator.h>

#include <flitr/modules/flitr_image_processors/cnvrt_to_float/fip_cnvrt_to_y_f32.h>
#include <flitr/modules/flitr_image_processors/cnvrt_to_float/fip_cnvrt_to_rgb_f32.h>
#include <flitr/modules/flitr_image_processors/cnvrt_to_8bit/fip_cnvrt_to_y_8.h>

/////////////////////////////////////////

#include <iosfwd>
#include <memory>
#include <utility>
#include <vector>
#include <stdio.h>



/*Caffe and OpenCV*/
//#include <caffe.pb.h>
#ifdef FLITR_USE_CAFFE
    #include <caffe/caffe.hpp>
    #include <caffe/proto/caffe.pb.h>
#endif


//#using std::shared_ptr;
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

//Do not USE_BACKGROUND_TRIGGER_THREAD for opencv
//#define USE_BACKGROUND_TRIGGER_THREAD 0

int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " video_file\n";
        return 1;
    }

    shared_ptr<FFmpegProducer> ip(new FFmpegProducer(argv[1], ImageFormat::FLITR_PIX_FMT_RGB_8));
    if (!ip->init()) {
        std::cerr << "Could not load " << argv[1] << "\n";
        exit(-1);
    }


#ifdef USE_BACKGROUND_TRIGGER_THREAD
    shared_ptr<BackgroundTriggerThread> btt(new BackgroundTriggerThread(ip.get()));
    btt->startThread();
#endif

    std::shared_ptr<FIPClassifyRCNN> classi(new FIPClassifyRCNN(*ip, 1, 4)); //# 2,4 from Jaco
    if (!classi->init()) {
        std::cerr << "Could not initialise the classification processor.\n";
        exit(-1);
    }
    classi->startTriggerThread();



    shared_ptr<MultiOSGConsumer> osgc(new MultiOSGConsumer(*classi, 1, 1));
    if (!osgc->init()) {
        std::cerr << "Could not init OSG consumer\n";
        exit(-1);
    }

    /*
    shared_ptr<MultiOSGConsumer> osgcOrig(new MultiOSGConsumer(*ip, 1, 4));
    if (!osgcOrig->init()) {
        std::cerr << "Could not init osgcOrig consumer\n";
        exit(-1);
    }
*/


    osg::Group *root_node = new osg::Group;

    shared_ptr<TexturedQuad> quad(new TexturedQuad(osgc->getOutputTexture()));
    root_node->addChild(quad->getRoot().get());


    osgViewer::Viewer viewer;
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.setSceneData(root_node);

    viewer.setUpViewInWindow(100, 100, 640, 480);
    viewer.realize();
        OrthoTextureManipulator* om = new OrthoTextureManipulator(osgc->getOutputTexture()->getTextureWidth(), osgc->getOutputTexture()->getTextureHeight());
        viewer.setCameraManipulator(om);

    size_t numFrames=0;
    while((!viewer.done()) && (numFrames<40) ) /*&&(ffp->getCurrentImage()<(ffp->getNumImages()*0.9f)) )*/
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
            osgc->getNext(); //#osgcOrig->getNext();

            viewer.frame();
            numFrames++;
        }else{
            FThread::microSleep(5000);  //@20000 worked
        }
    }

    //     mffc->stopWriting();
    //     mffc->closeFiles();


#ifdef USE_BACKGROUND_TRIGGER_THREAD
    btt->setExit();
    btt->join();
#endif

    //dpt0->stopTriggerThread();
    classi->stopTriggerThread();

    return 0;
}
