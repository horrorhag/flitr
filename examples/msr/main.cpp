#include <iostream>
#include <string>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/TrackballManipulator>
#include <osg/io_utils>

#include <flitr/modules/flitr_image_processors/crop/fip_crop.h>
#include <flitr/modules/flitr_image_processors/flip/fip_flip.h>
#include <flitr/modules/flitr_image_processors/rotate/fip_rotate.h>
#include <flitr/modules/flitr_image_processors/transform2D/fip_transform2D.h>
#include <flitr/modules/flitr_image_processors/gaussian_filter/fip_gaussian_filter.h>
#include <flitr/modules/flitr_image_processors/photometric_equalise/fip_photometric_equalise.h>

#include <flitr/modules/flitr_image_processors/msr/fip_msr.h>

#include <flitr/modules/flitr_image_processors/cnvrt_to_float/fip_cnvrt_to_y_f32.h>
#include <flitr/modules/flitr_image_processors/cnvrt_to_float/fip_cnvrt_to_rgb_f32.h>
#include <flitr/modules/flitr_image_processors/cnvrt_to_8bit/fip_cnvrt_to_y_8.h>
#include <flitr/modules/flitr_image_processors/cnvrt_to_8bit/fip_cnvrt_to_rgb_8.h>

#include <flitr/modules/flitr_image_processors/average_image/fip_average_image.h>
#include <flitr/modules/flitr_image_processors/average_image/fip_average_image_iir.h>

#include <flitr/ffmpeg_producer.h>
#include <flitr/test_pattern_producer.h>
#include <flitr/multi_ffmpeg_consumer.h>
#include <flitr/multi_osg_consumer.h>
#include <flitr/textured_quad.h>
#include <flitr/manipulator_utils.h>
#include <flitr/ortho_texture_manipulator.h>

using std::shared_ptr;
using namespace flitr;



class KeyPressedHandler : public osgGA::GUIEventHandler
{
public:
    
    KeyPressedHandler(const float GFScale) :
    _GFScale(GFScale)
    {}
    
    ~KeyPressedHandler()
    {
    }
    
    bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        osgViewer::Viewer* viewer = dynamic_cast<osgViewer::Viewer*>(&aa);
        if (!viewer) return false;
        
        switch(ea.getEventType())
        {
            case(osgGA::GUIEventAdapter::KEYUP):
            {
                if (ea.getKey()==']')
                {
                    ++_GFScale;
                    std::cout << "GFScale_ = " << _GFScale << "\n";
                    std::cout.flush();
                } else
                    if (ea.getKey()=='[')
                    {
                        --_GFScale;
                        std::cout << "GFScale_ = " << _GFScale << "\n";
                        std::cout.flush();
                    }
                break;
            }
            default:
                break;
        }
        
        return false;
    }
    
    size_t _GFScale;
};


//!Class to read the video input from disk in the background.
class BackgroundTriggerThread : public FThread
{
public:
    BackgroundTriggerThread(ImageProducer* p) :
    _producer(p),
    _shouldExit(false) {}
    
    void run()
    {
        while(!_shouldExit)
        {
            if (!_producer->trigger()) FThread::microSleep(5000);
        }
    }
    
    void setExit() { _shouldExit = true; }
    
private:
    ImageProducer* _producer;
    bool _shouldExit;
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
    
    
    
    //===
    shared_ptr<FIPConvertToYF32> cnvrtToF32(new FIPConvertToYF32(*ip, 1, 2));
    if (!cnvrtToF32->init())
    {
        std::cerr << "Could not initialise the FIPConvertToYF32 processor.\n";
        exit(-1);
    }
    cnvrtToF32->startTriggerThread();
    
    
    
    //=== Simple Gaussian noise filter ===
    shared_ptr<FIPGaussianFilter> gaussFilt(new FIPGaussianFilter(*cnvrtToF32, 1, //Mono-colour input will take less time to process!
                                                                  2.0, //filter radius
                                                                  5, //kernel width
                                                                  0, //integral image approximation passes
                                                                  2));
    if (!gaussFilt->init())
    {
        std::cerr << "Could not initialise the gaussFilt processor.\n";
        exit(-1);
    }
    gaussFilt->startTriggerThread();
    
    
    
    /*
     //===
     shared_ptr<FIPPhotometricEqualise> photometricEqualise(new FIPPhotometricEqualise(*cnvrtToF32, 1,
     0.5, //target average
     2));//Buffer size.
     
     if (!photometricEqualise->init())
     {
     std::cerr << "Could not initialise the photometricEqualise processor.\n";
     exit(-1);
     }
     photometricEqualise->startTriggerThread();
     */
    
    
    //=== The multi-scale retinex algorithm ===
    shared_ptr<FIPMSR> msr(new FIPMSR(*gaussFilt, 1, 1));
    if (!msr->init())
    {
        std::cerr << "Could not initialise the msr image processor.\n";
        exit(-1);
    }
    msr->setGFScale(15); //Sets the divider of the image width to calculate the MSR Gaussian scale.
    msr->setNumGaussianScales(1); //Sets the number of Gaussian scales to use. MSR typically uses 3, but 1 is faster.
    msr->startTriggerThread();
    
    
    
    //=== Convert back to 8 bit. ===
    shared_ptr<FIPConvertToRGB8> cnvrtToRGB8(new FIPConvertToRGB8(*msr, 1, 0.95f, 2));
    if (!cnvrtToRGB8->init())
    {
        std::cerr << "Could not initialise the cnvrtToRGB8 processor.\n";
        exit(-1);
    }
    cnvrtToRGB8->startTriggerThread();
    
    
    
    //=== OSG display
    shared_ptr<MultiOSGConsumer> osgc(new MultiOSGConsumer(*msr, 1));
    if (!osgc->init())
    {
        std::cerr << "Could not init OSG consumer\n";
        exit(-1);
    }
    
    
    //=== OSG display
    shared_ptr<MultiOSGConsumer> osgcOrig(new MultiOSGConsumer(*ip, 1));
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
        //quad->flipAroundHorizontal();
        //quad->rotateAroundCentre(5.0/180.0*M_PI);
        quad->translate(osgc->getOutputTexture()->getTextureWidth()>>1, 0);
    }
    
    shared_ptr<TexturedQuad> quadOrig(new TexturedQuad(osgcOrig->getOutputTexture()));
    root_node->addChild(quadOrig->getRoot().get());
    {
        osg::Matrix translate;
        translate.makeTranslate(osg::Vec3d(-osgcOrig->getOutputTexture()->getTextureWidth()/2,
                                           0.0,
                                           0.0));
        //quadOrig->flipAroundHorizontal();
        //quadOrig->rotateAroundCentre(5.0/180.0*M_PI);
        quadOrig->translate(-osgc->getOutputTexture()->getTextureWidth()>>1, 0);
    }
    
    
    osgViewer::Viewer viewer;
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.addEventHandler(new osgViewer::StatsHandler);
    
    KeyPressedHandler *kbHandler=new KeyPressedHandler(msr->getGFScale());
    viewer.addEventHandler(kbHandler);
    
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
        //if (osgcOrig->getNext()) renderFrame=true;
        
        msr->setGFScale(kbHandler->_GFScale);
        
        if (renderFrame)
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
        
        OpenThreads::Thread::microSleep(1000);
    }
    
    //     mffc->stopWriting();
    //     mffc->closeFiles();
    
    
#ifdef USE_BACKGROUND_TRIGGER_THREAD
    btt->setExit();
    btt->join();
#endif
    
    cnvrtToF32->stopTriggerThread();
    msr->stopTriggerThread();
    //cnvrtToRGB8->stopTriggerThread();
    
    return 0;
}
