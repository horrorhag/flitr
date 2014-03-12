#include <iostream>
#include <string>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/TrackballManipulator>
#include <osg/io_utils>

#include <flitr/modules/geometry_overlays/points_overlay.h>
#include <flitr/modules/flitr_image_processors/crop/fip_crop.h>
#include <flitr/modules/flitr_image_processors/cnvrt_to_f32/fip_cnvrt_to_f32.h>
#include <flitr/modules/flitr_image_processors/tonemap/fip_tonemap.h>
#include <flitr/modules/flitr_image_processors/cnvrt_to_m8/fip_cnvrt_to_m8.h>
#include <flitr/modules/flitr_image_processors/average_image/fip_average_image.h>
#include <flitr/modules/flitr_image_processors/photometric_equalise/fip_photometric_equalise.h>
#include <flitr/modules/flitr_image_processors/gaussian_downsample/fip_gaussian_downsample.h>
#include <flitr/modules/flitr_image_processors/gaussian_filter/fip_gaussian_filter.h>
#include <flitr/modules/flitr_image_processors/gradient_image/fip_gradient_image.h>
#include <flitr/modules/flitr_image_processors/unsharp_mask/fip_unsharp_mask.h>

#include <flitr/modules/flitr_image_processors/dewarp/fip_lk_dewarp.h>

#include <flitr/ffmpeg_producer.h>
#include <flitr/test_pattern_producer.h>
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
            Thread::microSleep(5000);
        }
    }
    
    void setExit() { ShouldExit_ = true; }
    
private:
    ImageProducer* Producer_;
    bool ShouldExit_;
    const uint32_t ReadableTarget_;
};

#define USE_BACKGROUND_TRIGGER_THREAD 1

int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " video_file\n";
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
    
    shared_ptr<FIPConvertToF32> cnvrtToF32(new FIPConvertToF32(*ip, 1, 2));
    if (!cnvrtToF32->init()) {
        std::cerr << "Could not initialise the cnvrtToF32 processor.\n";
        exit(-1);
    }
    cnvrtToF32->startTriggerThread();
    
    shared_ptr<FIPCrop> crop(new FIPCrop(*cnvrtToF32, 1,
                                         0,
                                         0,
                                         ip->getFormat().getWidth(),
                                         ip->getFormat().getHeight(),
                                         2));
    if (!crop->init()) {
        std::cerr << "Could not initialise the crop processor.\n";
        exit(-1);
    }
    crop->startTriggerThread();
    
    /*
     shared_ptr<FIPGaussianFilter> gaussianFilter(new FIPGaussianFilter(*crop, 1, 2));
     if (!gaussianFilter->init()) {
     std::cerr << "Could not initialise the gaussianFilter processor.\n";
     exit(-1);
     }
     gaussianFilter->startTriggerThread();
     */
    
    shared_ptr<FIPLKDewarp> lkdewarp(new FIPLKDewarp(*crop, 1, true, 0.025f, 2));
    if (!lkdewarp->init()) {
        std::cerr << "Could not initialise the lkdewarp processor.\n";
        exit(-1);
    }
    lkdewarp->startTriggerThread();
    
    shared_ptr<FIPAverageImage> averageImage(new FIPAverageImage(*lkdewarp, 1, 0, 2));
    if (!averageImage->init()) {
        std::cerr << "Could not initialise the average image processor.\n";
        exit(-1);
    }
    averageImage->startTriggerThread();
    
    shared_ptr<FIPUnsharpMask> unsharpMask(new FIPUnsharpMask(*averageImage, 1, 15.0f, 2));
    if (!unsharpMask->init()) {
        std::cerr << "Could not initialise the unsharp mask processor.\n";
        exit(-1);
    }
    unsharpMask->startTriggerThread();
    
    shared_ptr<FIPTonemap> tonemap(new FIPTonemap(*unsharpMask, 1, 1.25f, 2));
    if (!tonemap->init()) {
        std::cerr << "Could not initialise the tonemap processor.\n";
        exit(-1);
    }
    tonemap->startTriggerThread();
    
    shared_ptr<FIPConvertToM8> cnvrtToM8(new FIPConvertToM8(*tonemap, 1, 0.95f, 2));
    if (!cnvrtToM8->init()) {
        std::cerr << "Could not initialise the cnvrtToM8 processor.\n";
        exit(-1);
    }
    cnvrtToM8->startTriggerThread();
    
    shared_ptr<MultiOSGConsumer> osgc(new MultiOSGConsumer(*cnvrtToM8, 1, 2));
    if (!osgc->init()) {
        std::cerr << "Could not init OSG consumer\n";
        exit(-1);
    }
    
    
    shared_ptr<MultiOSGConsumer> osgcOrig(new MultiOSGConsumer(*crop, 1));
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
    
    //=== Points overlay to test homography registration ===
    //Note that the points will be one frame behind input!!!
    PointsOverlay* pov = new PointsOverlay();
    pov->setPointSize(11);
    pov->setColour(osg::Vec4(1.0, 0.0, 0.0, 0.8));
    root_node->addChild(pov);
    //===
    
    osgViewer::Viewer viewer;
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.setSceneData(root_node);
    
    viewer.setUpViewInWindow(100, 100, 640, 480);
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
    
    size_t numFrames=0;
    float hx=0.0f,hy=0.0f;
    while((!viewer.done())/*&&(ffp->getCurrentImage()<(ffp->getNumImages()*0.9f))*/)
    {
        
#ifndef USE_BACKGROUND_TRIGGER_THREAD
        //Read from the video, but don't get more than n frames ahead.
        if (ip->getLeastNumReadSlotsAvailable()<5)
        {
            ip->trigger();
        }
#endif
        
        if ((osgc->getNext())||(osgcOrig->getNext()))
        {
            
            /*
             osg::Matrix translate;
             translate.makeTranslate(osg::Vec3d(+dewarp->getRigidHx(2)*4.0,
             -dewarp->getRigidHy(2)*4.0,
             0.0));
             quad->setTransform(translate);
             */
            
            viewer.frame();
            
            /*
             if (numFrames==15)
             {
             mffc->startWriting();
             }
             */
            
            numFrames++;
            
            
            //=== ===//
            /*
             if ((numFrames%100)==0)
             {
             lkdewarp->saveHVectVariance("dewarp.f32");
             }
             
             if (numFrames>10)
             {
             float dhx=0.0f,dhy=0.0f;
             lkdewarp->getLatestHVect(dhx, dhy);
             
             hx*=0.99f;
             hy*=0.99f;
             
             hx+=dhx;
             hy+=dhy;
             
             pov->clearVertices();
             pov->addVertex(osg::Vec3d(-hx,
             hy+osgc->getOutputTexture()->getTextureHeight()*0.5f, +0.1f));
             
             }
             */
            //=== ===//
            
        }
        
        OpenThreads::Thread::microSleep(1000);
    }
    
    //     mffc->stopWriting();
    //     mffc->closeFiles();
    
    
#ifdef USE_BACKGROUND_TRIGGER_THREAD
    btt->setExit();
    btt->join();
#endif
    
    cnvrtToF32->stopTriggerThread();
    //gaussianFilter->stopTriggerThread();
    lkdewarp->stopTriggerThread();
    averageImage->stopTriggerThread();
    unsharpMask->stopTriggerThread();
    tonemap->stopTriggerThread();
    cnvrtToM8->stopTriggerThread();
    
    return 0;
}
