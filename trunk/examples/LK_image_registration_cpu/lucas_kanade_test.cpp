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

#include <flitr/modules/flitr_image_processors/simulate/fip_camera_shake.h>
#include <flitr/modules/flitr_image_processors/stabilise/fip_lk_stabilise.h>

#include <flitr/ffmpeg_producer.h>
#include <flitr/test_pattern_producer.h>
#include <flitr/multi_ffmpeg_consumer.h>
#include <flitr/multi_osg_consumer.h>
#include <flitr/textured_quad.h>
#include <flitr/manipulator_utils.h>
#include <flitr/ortho_texture_manipulator.h>


using std::shared_ptr;
using namespace flitr;

class BackgroundTriggerThread : public OpenThreads::Thread
{
public:
    BackgroundTriggerThread(ImageProducer* p) :
    Producer_(p),
    ShouldExit_(false),
    ReadableTarget_(10) {}
    
    void run()
    {
        while(!ShouldExit_)
        {
            bool triggerred = false;
            while (Producer_->getLeastNumReadSlotsAvailable() < ReadableTarget_)
            {
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
    if (argc != 2)
    {
        std::cout << "Usage: " << argv[0] << " video_file\n";
        return 1;
    }
    
    
    //==
    shared_ptr<FFmpegProducer> ip(new FFmpegProducer(argv[1], ImageFormat::FLITR_PIX_FMT_Y_8));
    if (!ip->init())
    {
        std::cerr << "Could not load " << argv[1] << "\n";
        exit(-1);
    }

#ifdef USE_BACKGROUND_TRIGGER_THREAD
    shared_ptr<BackgroundTriggerThread> btt(new BackgroundTriggerThread(ip.get()));
    btt->startThread();
#endif
    
    
    //==
    shared_ptr<FIPConvertToF32> cnvrtToF32(new FIPConvertToF32(*ip, 1, 2));
    if (!cnvrtToF32->init())
    {
        std::cerr << "Could not initialise the cnvrtToF32 processor.\n";
        exit(-1);
    }
    cnvrtToF32->startTriggerThread();
    
/*
    //==
    shared_ptr<FIPCameraShake> cameraShake(new FIPCameraShake(*cnvrtToF32, 1,
                                                              3.0f, 30, //Kernel width is required to be >> than SD.
                                                              2));
    if (!cameraShake->init())
    {
        std::cerr << "Could not initialise the cameraShake processor.\n";
        exit(-1);
    }
    cameraShake->startTriggerThread();
*/
    
    
    /*
     //==
     shared_ptr<FIPGaussianDownsample> gaussianDownsample(new FIPGaussianDownsample(*cameraShake, 1,
     2.0f,
     6,
     2));
     if (!gaussianDownsample->init())
     {
     std::cerr << "Could not initialise the gaussianDownSample processor.\n";
     exit(-1);
     }
     gaussianDownsample->startTriggerThread();
     */
     
     /*
     //==
     shared_ptr<FIPGaussianFilter> gaussianFilter0(new FIPGaussianFilter(*crop, 1,
     1.0f,
     4,
     2));
     if (!gaussianFilter0->init())
     {
     std::cerr << "Could not initialise the gaussianFilter processor.\n";
     exit(-1);
     }
     gaussianFilter0->startTriggerThread();
    */
    
/*
    //==
    shared_ptr<FIPCrop> crop(new FIPCrop(*cameraShake, 1,
                                         ip->getFormat().getWidth()/4,
                                         160,
                                         ip->getFormat().getWidth()/2,
                                         640,
                                         2));
    if (!crop->init())
    {
        std::cerr << "Could not initialise the crop processor.\n";
        exit(-1);
    }
    crop->startTriggerThread();
*/
    
    //==
    shared_ptr<FIPLKStabilise> lkstabilise(new FIPLKStabilise(*cnvrtToF32, 1,
                                                              FIPLKStabilise::Mode::INTSTAB,
                                                              2));
    if (!lkstabilise->init())
    {
        std::cerr << "Could not initialise the lkstabilise processor.\n";
        exit(-1);
    }
    lkstabilise->startTriggerThread();
    
    
/*
    //==
    shared_ptr<FIPConvertToM8> cnvrtToM8(new FIPConvertToM8(*lkstabilise, 1, 0.95f, 2));
    if (!cnvrtToM8->init())
    {
        std::cerr << "Could not initialise the cnvrtToM8 processor.\n";
        exit(-1);
    }
    cnvrtToM8->startTriggerThread();
*/    
    
    
    //==
    shared_ptr<MultiOSGConsumer> osgc(new MultiOSGConsumer(*lkstabilise, 1, 2));
    if (!osgc->init())
    {
        std::cerr << "Could not init OSG consumer\n";
        exit(-1);
    }
    
    
    //==
    //    shared_ptr<MultiOSGConsumer> osgcOrig(new MultiOSGConsumer(*gaussianDownsample, 1));
    //    if (!osgcOrig->init())
    //    {
    //        std::cerr << "Could not init osgcOrig consumer\n";
    //        exit(-1);
    //    }
    
    
/*
    //==
    shared_ptr<MultiFFmpegConsumer> mffc(new MultiFFmpegConsumer(*cnvrtToM8,1));
    if (!mffc->init())
    {
        std::cerr << "Could not init FFmpeg consumer\n";
        exit(-1);
    }
    std::stringstream filenameStringStream;
    filenameStringStream << argv[1] << "_improved";
    mffc->openFiles(filenameStringStream.str());
    mffc->startWriting();
*/
    
    
    osg::Group *root_node = new osg::Group;
    
    shared_ptr<TexturedQuad> quad(new TexturedQuad(osgc->getOutputTexture()));
    root_node->addChild(quad->getRoot().get());
    {
        //        osg::Matrix translate;
        //        translate.makeTranslate(osg::Vec3d(osgc->getOutputTexture()->getTextureWidth()/2,
        //                                           0.0,
        //                                           0.0));
        //        quad->setTransform(translate);
    }
    
    //    shared_ptr<TexturedQuad> quadOrig(new TexturedQuad(osgcOrig->getOutputTexture()));
    //    root_node->addChild(quadOrig->getRoot().get());
    //    {
    //        osg::Matrix translate;
    //        translate.makeTranslate(osg::Vec3d(-osgcOrig->getOutputTexture()->getTextureWidth()+osgc->getOutputTexture()->getTextureWidth()/2,
    //                                           0.0,
    //                                           0.0));
    //        quadOrig->setTransform(translate);
    //    }
    
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

    float errSq[16];
    float errCounter[16];
    for (size_t i=0; i<15; ++i)
    {
        errSq[i]=0.0f;
        errCounter[i]=0;
    }

    
    while((!viewer.done())/*&&(ffp->getCurrentImage()<(ffp->getNumImages()*0.9f))*/&&(numFrames<1000))
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
            viewer.frame();
/*
            float simHx, simHy;
            size_t simFrameNumber;
            float lkHx, lkHy;
            size_t lkFrameNumber;
            
            lkstabilise->getLatestHVect(lkHx, lkHy, lkFrameNumber);
            
            cameraShake->getLatestHVect(simHx, simHy, simFrameNumber);
            
            //simHx*=0.5f;
            //simHy*=0.5f;
            
            if (numFrames>0)
            {
                const float H=sqrtf(simHx*simHx + simHy*simHy);
                const float ieSq=(lkHx-simHx)*(lkHx-simHx) + (lkHy-simHy)*(lkHy-simHy);
                
                const size_t hIndex=(H/16.0f)*16;
                
                if (hIndex<=15)
                {
                    errSq[hIndex]+=ieSq;
                    ++errCounter[hIndex];
                }
                
                std::cout << "LK_" << lkFrameNumber << "=(" << lkHx << " " << lkHy << ")\n";
                std::cout << "SIM_" << simFrameNumber << "=(" << simHx << " " << simHy << ")\n";
                std::cout << "[H=" << H << ", err=" << sqrtf(ieSq) << "]\n";
                
                for (size_t i=0; i<15; ++i)
                {
                    std::cout << "[" << ((errCounter[i]>0) ? sqrtf(errSq[i]/errCounter[i]) : 0.0f) << "]";
                }
                std::cout << "\n";
                std::cout.flush();
            }
*/
            
            numFrames++;
        }
        
        OpenThreads::Thread::microSleep(3000);
    }
    
    //mffc->stopWriting();
    //mffc->closeFiles();
    
    
#ifdef USE_BACKGROUND_TRIGGER_THREAD
    btt->setExit();
    btt->join();
#endif
    
    cnvrtToF32->stopTriggerThread();
    //crop->stopTriggerThread();
    //cameraShake->stopTriggerThread();
    //gaussianDownsample->stopTriggerThread();
    //gaussianFilter0->stopTriggerThread();
    lkstabilise->stopTriggerThread();
    //cnvrtToM8->stopTriggerThread();
    
    return 0;
}
