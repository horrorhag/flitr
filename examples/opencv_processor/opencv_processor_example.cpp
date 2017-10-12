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

//# My Included Files
#include <include/flitr/v4l2_producer.h>
#include <flitr/modules/flitr_image_processors/cnvrt_to_float/fip_cnvrt_to_y_f32.h>
#include <flitr/modules/flitr_image_processors/cnvrt_to_8bit/fip_cnvrt_to_y_8.h>

#include <flitr/modules/flitr_image_processors/opencv_processor/fip_opencv_processor.h>
#include <flitr/modules/opencv_helpers/opencv_processors.h>

//# End my Included files

#include <flitr/ffmpeg_producer.h>
#include <flitr/test_pattern_producer.h>
#include <flitr/multi_ffmpeg_consumer.h>
#include <flitr/multi_osg_consumer.h>
#include <flitr/textured_quad.h>
#include <flitr/manipulator_utils.h>
#include <flitr/ortho_texture_manipulator.h>

using std::shared_ptr;
using std::cout; using std::endl;
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

//#define USE_BACKGROUND_TRIGGER_THREAD 0  //#Was 1, Natalie


int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " video_file\n";
        return 1;
    }

    //Video Producer
    shared_ptr<FFmpegProducer> ip(new FFmpegProducer(argv[1], ImageFormat::FLITR_PIX_FMT_Y_8,2));
    if (!ip->init()) {
        std::cerr << "Could not load " << argv[1] << "\n";
        exit(-1);
    }


#ifdef USE_BACKGROUND_TRIGGER_THREAD
    shared_ptr<BackgroundTriggerThread> btt(new BackgroundTriggerThread(ip.get()));
    btt->startThread();
#endif

    /// ============ OpenCV processors =============
    ///Background Subtraction
    OpenCVProcessors::ProcessorType procType = OpenCVProcessors::backgroundSubtraction;
    std::map<std::string, std::string> cvProcessorParams;
    cvProcessorParams["backgroundUpdateInterval"] = "50";
    shared_ptr<flitr::FIP_OpenCVProcessors> openCVProcessor(new flitr::FIP_OpenCVProcessors(*ip, 1, 4, procType, cvProcessorParams));
    if (!openCVProcessor->init()) {
        std::cerr << "Could not initialise the ConvertToOpenCV image processor.\n";
        exit(-1);
    }

    /*
    ///Default processor, laplace or histogramEqualization
    OpenCVProcessors::ProcessorType procType = OpenCVProcessors::histEqualisation; // or OpenCVProcessors::histEqualisation;
    shared_ptr<flitr::FIP_OpenCVProcessors> openCVProcessor(new flitr::FIP_OpenCVProcessors(*ip, 1, 4, procType));
    if (!openCVProcessor->init()) {
        std::cerr << "Could not initialise the ConvertToOpenCV image processor.\n";
        exit(-1);
    }
    */

    openCVProcessor->startTriggerThread();
    ///==================================================


    shared_ptr<MultiOSGConsumer> osgc(new MultiOSGConsumer(*openCVProcessor, 1, 1));
    if (!osgc->init()) {
        std::cerr << "Could not init OSG consumer\n";
        exit(-1);
    }


    shared_ptr<MultiOSGConsumer> osgcOrig(new MultiOSGConsumer(*ip, 1, 1));
    if (!osgcOrig->init()) {
        std::cerr << "Could not init osgcOrig consumer\n";
        exit(-1);
    }


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
    viewer.realize();  //Put this line back 11 May 17

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

    while((!viewer.done()) && (numFrames<2000) ) /*&&(ffp->getCurrentImage()<(ffp->getNumImages()*0.9f)) )*/
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
        FThread::microSleep(5000);
    }

    //     mffc->stopWriting();
    //     mffc->closeFiles();


#ifdef USE_BACKGROUND_TRIGGER_THREAD
    btt->setExit();
    btt->join();
#endif

    //transform->stopTriggerThread();
    //cnvrtToYF32->stopTriggerThread();
    openCVProcessor->stopTriggerThread();
    //cnvrtToY8->stopTriggerThread();

    return 0;
}
