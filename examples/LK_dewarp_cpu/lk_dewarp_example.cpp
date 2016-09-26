#include <iostream>
#include <string>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/TrackballManipulator>
#include <osg/io_utils>

#include <flitr/modules/geometry_overlays/points_overlay.h>

#include <flitr/modules/flitr_image_processors/motion_detect/fip_motion_detect.h>
#include <flitr/modules/flitr_image_processors/crop/fip_crop.h>
#include <flitr/modules/flitr_image_processors/cnvrt_to_float/fip_cnvrt_to_y_f32.h>
#include <flitr/modules/flitr_image_processors/tonemap/fip_tonemap.h>
#include <flitr/modules/flitr_image_processors/cnvrt_to_8bit/fip_cnvrt_to_y_8.h>
#include <flitr/modules/flitr_image_processors/average_image/fip_average_image.h>
#include <flitr/modules/flitr_image_processors/photometric_equalise/fip_photometric_equalise.h>
#include <flitr/modules/flitr_image_processors/gaussian_downsample/fip_gaussian_downsample.h>
#include <flitr/modules/flitr_image_processors/gaussian_filter/fip_gaussian_filter.h>
#include <flitr/modules/flitr_image_processors/gradient_image/fip_gradient_image.h>
#include <flitr/modules/flitr_image_processors/unsharp_mask/fip_unsharp_mask.h>

#include <flitr/modules/flitr_image_processors/dewarp/fip_lk_dewarp.h>
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


//!Class to read the video input from disk in the background.
class BackgroundTriggerThread : public FThread
{
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
    if (argc != 2)
    {
        std::cout << "Usage: " << argv[0] << " video_file\n";
        return 1;
    }
    
    
    //===
    shared_ptr<FFmpegProducer> ip(new FFmpegProducer(argv[1], ImageFormat::FLITR_PIX_FMT_Y_8, 5));
    if (!ip->init())
    {
        std::cerr << "Could not load " << argv[1] << "\n";
        exit(-1);
    }
    
#ifdef USE_BACKGROUND_TRIGGER_THREAD
    shared_ptr<BackgroundTriggerThread> btt(new BackgroundTriggerThread(ip.get()));
    btt->startThread();
#endif
    
    
    //===
    shared_ptr<FIPConvertToYF32> cnvrtToYF32(new FIPConvertToYF32(*ip, 1, 2));
    if (!cnvrtToYF32->init())
    {
        std::cerr << "Could not initialise the cnvrtToYF32 processor.\n";
        exit(-1);
    }
    cnvrtToYF32->startTriggerThread();
    
    /*
     //===
     shared_ptr<FIPCrop> crop(new FIPCrop(*cnvrtToYF32, 1,
     100, //x0
     100, //y0
     1160, //width
     824, //height
     2));
     if (!crop->init())
     {
     std::cerr << "Could not initialise the crop processor.\n";
     exit(-1);
     }
     crop->startTriggerThread();
     */
    
    
    /*
     //===
     shared_ptr<FIPGaussianDownsample> gaussianDownSample0(new FIPGaussianDownsample(*crop, 1,
     2.0f,
     6,
     2));
     if (!gaussianDownSample0->init())
     {
     std::cerr << "Could not initialise the gaussianDownSample0 processor.\n";
     exit(-1);
     }
     gaussianDownSample0->startTriggerThread();
     
     
     //===
     shared_ptr<FIPGaussianDownsample> gaussianDownSample1(new FIPGaussianDownsample(*gaussianDownSample0, 1,
     2.0f,
     6,
     2));
     if (!gaussianDownSample1->init())
     {
     std::cerr << "Could not initialise the gaussianDownSample1 processor.\n";
     exit(-1);
     }
     gaussianDownSample1->startTriggerThread();
     */
    
    /*
     //===
     shared_ptr<FIPGaussianFilter> gaussianFilter(new FIPGaussianFilter(*crop, 1,
     1.0f,
     3,
     2));
     if (!gaussianFilter->init())
     {
     std::cerr << "Could not initialise the gaussianFilter processor.\n";
     exit(-1);
     }
     gaussianFilter->startTriggerThread();
     */
    
    
    //=== Photometric equalisation to improve accuracy of optical flow. ===
    //===  * Use setTargetAverage(...) to set the target average brightness at run-time.
    shared_ptr<FIPPhotometricEqualise> photometricEqualise(new FIPPhotometricEqualise(*cnvrtToYF32, 1,
                                                                                      0.6,//target average brightness.
                                                                                      2));//SharedImageBuffer size.
    
    if (!photometricEqualise->init())
    {
        std::cerr << "Could not initialise the photometricEqualise processor.\n";
        exit(-1);
    }
    photometricEqualise->startTriggerThread();
    
    
    
    //=== Optical flow based image stabilisation to nearest integer...
    //===  * Use setupOutputTransformBurn(...) to accommodate a panning camera. Smaller values are more accommodating.
    shared_ptr<FIPLKStabilise> lkstabilise(new FIPLKStabilise(*photometricEqualise, 1,
                                                              FIPLKStabilise::Mode::INTSTAB, //Stabilise to nearest int.
                                                              //FIPLKStabilise::Mode::SUBPIXELSTAB, //Stabilise to sub-pixel accuracy (resamples the image).
                                                              2));
    if (!lkstabilise->init())
    {
        std::cerr << "Could not initialise the lkstabilise processor.\n";
        exit(-1);
    }
    lkstabilise->setupOutputTransformBurn(0.8, 0.8); //High pass filter output transform.
    lkstabilise->startTriggerThread();
    
    
    
    
    //=== Optical flow based image dewarp...
    shared_ptr<FIPLKDewarp> lkdewarp(new FIPLKDewarp(*lkstabilise, 1,
                                                     0.95f, //Internal average image longevity.
                                                     2));  //Buffer size.
    if (!lkdewarp->init())
    {
        std::cerr << "Could not initialise the lkdewarp processor.\n";
        exit(-1);
    }
    lkdewarp->startTriggerThread();
    
    
    
    //=== Post dewarp average image to reduce noise/artifacts...
    shared_ptr<FIPAverageImage> averageImage(new FIPAverageImage(*lkdewarp, 1,
                                                                 2,   //Exponent of average window length e.g. 5 -> window length = 2^5 = 32
                                                                 2));
    if (!averageImage->init())
    {
        std::cerr << "Could not initialise the average image processor.\n";
        exit(-1);
    }
    averageImage->startTriggerThread();
    
    
    
    //=== Sharpening/de-blur pass. ===//
    //===  * Use setFilterRadius(...) to at runtime adjust the filter radius to best sharpen the image.
    shared_ptr<FIPUnsharpMask> unsharpMask(new FIPUnsharpMask(*averageImage, 1,
                                                              20.0f, //unsharp mask gain.
                                                              1.49f, //unsharp mask filter radius.
                                                              2));
    if (!unsharpMask->init())
    {
        std::cerr << "Could not initialise the unsharp mask processor.\n";
        exit(-1);
    }
    unsharpMask->startTriggerThread();
    
    
    
    /*
     //===
     shared_ptr<FIPTonemap> tonemap(new FIPTonemap(*unsharpMask, 1, 0.8f, 2));
     if (!tonemap->init())
     {
     std::cerr << "Could not initialise the tonemap processor.\n";
     exit(-1);
     }
     tonemap->startTriggerThread();
     */
    
    
    
    //=== Convert float32 image back to 8 bit...
    shared_ptr<FIPConvertToY8> cnvrtToY8(new FIPConvertToY8(*unsharpMask, 1,
                                                            1.0f,
                                                            2));
    if (!cnvrtToY8->init())
    {
        std::cerr << "Could not initialise the cnvrtToM8 processor.\n";
        exit(-1);
    }
    cnvrtToY8->startTriggerThread();
    
    
    
    
    shared_ptr<MultiOSGConsumer> osgc(new MultiOSGConsumer(*averageImage, 1));
    if (!osgc->init())
    {
        std::cerr << "Could not init OSG consumer\n";
        exit(-1);
    }
    
    
    shared_ptr<MultiOSGConsumer> osgcOrig(new MultiOSGConsumer(*ip, 1));
    if (!osgcOrig->init())
    {
        std::cerr << "Could not init osgcOrig consumer\n";
        exit(-1);
    }
    
    
    /*
     shared_ptr<MultiFFmpegConsumer> mffc(new MultiFFmpegConsumer(*cnvrtToY8,1));
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
        //quad->flipAroundHorizontal();
        //quad->rotateAroundCentre(5.0/180.0*M_PI);
        quad->translate(osgc->getOutputTexture()->getTextureWidth()>>1, 0);
    }
    
    shared_ptr<TexturedQuad> quadOrig(new TexturedQuad(osgcOrig->getOutputTexture()));
    root_node->addChild(quadOrig->getRoot().get());
    {
        //quadOrig->flipAroundHorizontal();
        //quadOrig->rotateAroundCentre(5.0/180.0*M_PI);
        quadOrig->translate(-osgc->getOutputTexture()->getTextureWidth()>>1, 0);
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
    
    while((!viewer.done())/*&&(ffp->getCurrentImage()<(ffp->getNumImages()*0.9f))*/)
    {
        
#ifndef USE_BACKGROUND_TRIGGER_THREAD
        //Read from the video, but don't get more than n frames ahead.
        if (ip->getLeastNumReadSlotsAvailable()<5)
        {
            ip->trigger();
        }
#endif
        
        //unsharpMask->setFilterRadius(numFrames * 0.01 + 0.5);
        
        if (osgc->getNext())
        {
            osgcOrig->getNext();
            
            viewer.frame();
            
            numFrames++;
        }
        
        FThread::microSleep(1000);
    }
    
    //mffc->stopWriting();
    //mffc->closeFiles();
    
    
#ifdef USE_BACKGROUND_TRIGGER_THREAD
    btt->setExit();
    btt->join();
#endif
    
    cnvrtToYF32->stopTriggerThread();
    //crop->stopTriggerThread();
    //gaussianDownSample0->stopTriggerThread();
    //gaussianDownSample1->stopTriggerThread();
    //gaussianFilter->stopTriggerThread();
    photometricEqualise->stopTriggerThread();
    lkstabilise->stopTriggerThread();
    lkdewarp->stopTriggerThread();
    averageImage->stopTriggerThread();
    unsharpMask->stopTriggerThread();
    //tonemap->stopTriggerThread();
    cnvrtToY8->stopTriggerThread();
    
    return 0;
}
