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
#include <flitr/modules/flitr_image_processors/cnvrt_to_float/fip_cnvrt_to_rgb_f32.h>
#include <flitr/modules/flitr_image_processors/cnvrt_to_8bit/fip_cnvrt_to_y_8.h>
#include <flitr/modules/flitr_image_processors/cnvrt_to_8bit/fip_cnvrt_to_rgb_8.h>
#include <flitr/modules/flitr_image_processors/gaussian_filter/fip_gaussian_filter.h>
#include <flitr/modules/flitr_image_processors/morphological_filter/fip_morphological_filter.h>

#include <flitr/ffmpeg_producer.h>
#include <flitr/test_pattern_producer.h>
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

#define USE_BACKGROUND_TRIGGER_THREAD 1

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cout << "Usage: " << argv[0] << " video_file\n";
        return 1;
    }
    
    shared_ptr<FFmpegProducer> ip(new FFmpegProducer(argv[1], ImageFormat::FLITR_PIX_FMT_Y_8, 2));
    if (!ip->init())
    {
        std::cerr << "Could not load " << argv[1] << "\n";
        exit(-1);
    }
    
#ifdef USE_BACKGROUND_TRIGGER_THREAD
    shared_ptr<BackgroundTriggerThread> btt(new BackgroundTriggerThread(ip.get()));
    btt->startThread();
#endif
    
    /*
     shared_ptr<FIPConvertToRGBF32> cnvrtToRGBF32(new FIPConvertToRGBF32(*ip, 1, 1));
     if (!cnvrtToRGBF32->init())
     {
     std::cerr << "Could not initialise the cnvrtToRGBF32 processor.\n";
     exit(-1);
     }
     cnvrtToRGBF32->startTriggerThread();
     */
    
    shared_ptr<FIPMorphologicalFilter> morphologicalFilt(new FIPMorphologicalFilter(*ip, 1,
                                                                                    15,//square structuring element size.
                                                                                    15, 255,//threshold, binaryMax
                                                                                    1));
    
    
    //===TOP HAT===//
    morphologicalFilt->addMorphoPass(flitr::FIPMorphologicalFilter::MorphoPass::ERODE);
    morphologicalFilt->addMorphoPass(flitr::FIPMorphologicalFilter::MorphoPass::DILATE);
    morphologicalFilt->addMorphoPass(flitr::FIPMorphologicalFilter::MorphoPass::SOURCE_MINUS);
    morphologicalFilt->addMorphoPass(flitr::FIPMorphologicalFilter::MorphoPass::THRESHOLD);
    //=== ===//
    
    //===BLACK HAT===//
    //morphologicalFilt->addMorphoPass(flitr::FIPMorphologicalFilter::MorphoPass::DILATE);
    //morphologicalFilt->addMorphoPass(flitr::FIPMorphologicalFilter::MorphoPass::ERODE);
    //morphologicalFilt->addMorphoPass(flitr::FIPMorphologicalFilter::MorphoPass::MINUS_SOURCE);
    //morphologicalFilt->addMorphoPass(flitr::FIPMorphologicalFilter::MorphoPass::THRESHOLD);
    //=== ===//
    
    
    if (!morphologicalFilt->init())
    {
        std::cerr << "Could not initialise the morphologicalFilt processor.\n";
        exit(-1);
    }
    
    morphologicalFilt->startTriggerThread();
    
    
    /*
     shared_ptr<FIPConvertToRGB8> cnvrtToRGB8(new FIPConvertToRGB8(*gaussFilt, 1, 0.95f, 1));
     if (!cnvrtToRGB8->init())
     {
     std::cerr << "Could not initialise the cnvrtToRGB8 processor.\n";
     exit(-1);
     }
     cnvrtToRGB8->startTriggerThread();
     */
    
    
    shared_ptr<MultiOSGConsumer> osgc(new MultiOSGConsumer(*morphologicalFilt, 1, 1));
    if (!osgc->init())
    {
        std::cerr << "Could not init OSG consumer\n";
        exit(-1);
    }
    
    
    shared_ptr<MultiOSGConsumer> osgcOrig(new MultiOSGConsumer(*ip, 1, 1));
    if (!osgcOrig->init())
    {
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
        
        bool renderFrame=false;
        
        if (osgc->getNext()) renderFrame=true;
        if (osgcOrig->getNext()) renderFrame=true;
        
        if (renderFrame)
        {
            viewer.frame();
            
            /*
             if (numFrames==15)
             {
             mffc->startWriting();
             }
             */
            
            numFrames++;
        }
        
        OpenThreads::Thread::microSleep(5000);
    }
    
    //     mffc->stopWriting();
    //     mffc->closeFiles();
    
    
#ifdef USE_BACKGROUND_TRIGGER_THREAD
    btt->setExit();
    btt->join();
#endif
    
    //cnvrtToRGBF32->stopTriggerThread();
    morphologicalFilt->stopTriggerThread();
    //cnvrtToRGB8->stopTriggerThread();
    
    return 0;
}
