#include <iostream>
#include <string>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/TrackballManipulator>
#include <osg/io_utils>

#include <flitr/modules/flitr_image_processors/flip/fip_flip.h>
#include <flitr/modules/flitr_image_processors/rotate/fip_rotate.h>
#include <flitr/modules/flitr_image_processors/transform2D/fip_transform2D.h>

#include <flitr/modules/flitr_image_processors/cnvrt_to_float/fip_cnvrt_to_y_f32.h>
#include <flitr/modules/flitr_image_processors/cnvrt_to_8bit/fip_cnvrt_to_y_8.h>
#include <flitr/modules/flitr_image_processors/cnvrt_to_8bit/fip_cnvrt_to_rgb_8.h>
#include <flitr/modules/flitr_image_processors/average_image/fip_average_image.h>
#include <flitr/modules/flitr_image_processors/photometric_equalise/fip_photometric_equalise.h>
#include <flitr/modules/flitr_image_processors/stabilise/fip_lk_stabilise.h>
#include <flitr/modules/flitr_image_processors/crop/fip_crop.h>
#include <flitr/modules/flitr_image_processors/unsharp_mask/fip_unsharp_mask.h>
#include <flitr/modules/flitr_image_processors/motion_detect/fip_motion_detect.h>
#include <flitr/modules/flitr_image_processors/morphological_filter/fip_morphological_filter.h>
#include <flitr/modules/flitr_image_processors/gaussian_filter/fip_gaussian_filter.h>

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
            if (!Producer_->trigger()) FThread::microSleep(1000);
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
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " video_file\n";
        return 1;
    }
    
    
    shared_ptr<FFmpegProducer> ip(new FFmpegProducer(argv[1], ImageFormat::FLITR_PIX_FMT_Y_8, 2));
    if (!ip->init()) {
        std::cerr << "Could not load " << argv[1] << "\n";
        exit(-1);
    }
    
    
#ifdef USE_BACKGROUND_TRIGGER_THREAD
    shared_ptr<BackgroundTriggerThread> btt(new BackgroundTriggerThread(ip.get()));
    btt->startThread();
#endif
    
    
    //==
    shared_ptr<FIPConvertToYF32> cnvrtToYF32(new FIPConvertToYF32(*ip, 1, 1));
    if (!cnvrtToYF32->init()) {
        std::cerr << "Could not initialise the cnvrtToF32 processor.\n";
        exit(-1);
    }
    cnvrtToYF32->startTriggerThread();
    
    
    //==
    const size_t photoMetEqWindow=128;
    std::shared_ptr<flitr::FIPLocalPhotometricEqualise> equaliseImage(new flitr::FIPLocalPhotometricEqualise(*cnvrtToYF32, 1,
                                                                                                             0.60, photoMetEqWindow,
                                                                                                             2));
    if (!equaliseImage->init()) {
        std::cerr << "Could not initialise the equaliseImage image processor.\n";
        exit(-1);
    }
    equaliseImage->startTriggerThread();
    
/*
    //==
    std::shared_ptr<flitr::FIPCrop> crop(new flitr::FIPCrop(*equaliseImage, 1,
                                                            photoMetEqWindow, photoMetEqWindow,
                                                            equaliseImage->getDownstreamFormat().getWidth()-(photoMetEqWindow<<1), equaliseImage->getDownstreamFormat().getHeight()-(photoMetEqWindow<<1),
                                                            2));
    if (!crop->init()) {
        std::cerr << "Could not initialise the crop image processor.\n";
        exit(-1);
    }
    crop->startTriggerThread();
*/
    
    
    //==
    std::shared_ptr<flitr::FIPLKStabilise> lkstabilise(new flitr::FIPLKStabilise(*equaliseImage, 1,
                                                                                 flitr::FIPLKStabilise::Mode::INTSTAB,
                                                                                 2));
    if (!lkstabilise->init())
    {
        std::cerr << "Could not initialise the lkstabilise processor.\n";
        exit(-1);
    }
    lkstabilise->startTriggerThread();
    lkstabilise->setupOutputTransformBurn(0.95f, 0.95f);
    
    
    //==
    shared_ptr<FIPConvertToY8> cnvrtTo8Bit(new FIPConvertToY8(*lkstabilise, 1, 0.99f, 1));
    if (!cnvrtTo8Bit->init()) {
        std::cerr << "Could not initialise the cnvrtTo8Bit processor.\n";
        exit(-1);
    }
    cnvrtTo8Bit->startTriggerThread();
    
    
    //==
    std::shared_ptr<FIPMotionDetect> motionDetect(new FIPMotionDetect(*cnvrtTo8Bit, 1,
                                                                      false, false, true, //showOverlays, produceOnlyMotionImages, forceRGBOutput
                                                                      3.5f, 2, //motionThreshold, detectionThreshold
                                                                      2));
    if (!motionDetect->init())
    {
        std::cerr << "Could not initialise the motion detection processor "<< " SOURCE: " __FILE__ << " " << __LINE__ << "\n";
        return false;
    }
    motionDetect->startTriggerThread();
    
    
    //==
    shared_ptr<MultiOSGConsumer> osgc(new MultiOSGConsumer(*motionDetect, 1, 1));
    //shared_ptr<MultiOSGConsumer> osgc(new MultiOSGConsumer(*morphologicalFilt, 1, 1));
    if (!osgc->init()) {
        std::cerr << "Could not init OSG consumer\n";
        exit(-1);
    }
    
    
    //==
    shared_ptr<MultiOSGConsumer> osgcOrig(new MultiOSGConsumer(*ip, 1, 1));
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
        translate.makeTranslate(osg::Vec3d(+osgcOrig->getOutputTexture()->getTextureWidth()/2,
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
    
    
    //=== Setup OSG Viewer ===//
    osgViewer::Viewer viewer;
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.setSceneData(root_node);
    
    viewer.setUpViewInWindow(100, 100, 1280, 480);
    viewer.realize();
    
    const int use_trackball = 0;
    if (use_trackball)
    {
        osgGA::TrackballManipulator* tb = new osgGA::TrackballManipulator;
        viewer.setCameraManipulator(tb);
        adjustCameraManipulatorHomeForYUp(tb);
    } else
    {
        OrthoTextureManipulator* om = new OrthoTextureManipulator(osgc->getOutputTexture()->getTextureWidth(), osgc->getOutputTexture()->getTextureHeight());
        viewer.setCameraManipulator(om);
    }
    //========================//
    
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
            
            /*
             if (numFrames==15)
             {
             mffc->startWriting();
             }
             */
            
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
    
    cnvrtToYF32->stopTriggerThread();
    equaliseImage->stopTriggerThread();
    //crop->stopTriggerThread();
    lkstabilise->stopTriggerThread();
    cnvrtTo8Bit->stopTriggerThread();
    motionDetect->stopTriggerThread();
    
    return 0;
}
