#include <iostream>
#include <string>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/TrackballManipulator>
#include <osg/io_utils>

#include <flitr/modules/flitr_image_processors/cnvrt_to_f32/fip_cnvrt_to_f32.h>
#include <flitr/modules/flitr_image_processors/average_image/fip_average_image.h>
#include <flitr/modules/flitr_image_processors/photometric_equalise/fip_photometric_equalise.h>
#include <flitr/modules/flitr_image_processors/gaussian_downsample/fip_gaussian_downsample.h>
#include <flitr/modules/flitr_image_processors/gradient_image/fip_gradient_image.h>

#include <flitr/modules/flitr_image_processors/dewarp/fip_dewarp.h>

#include <flitr/ffmpeg_producer.h>
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

#define USE_BACKGROUND_TRIGGER_THREAD 1

int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " video_file\n";
        return 1;
    }
    
    shared_ptr<FFmpegProducer> ffp(new FFmpegProducer(argv[1], ImageFormat::FLITR_PIX_FMT_Y_8));
    if (!ffp->init()) {
        std::cerr << "Could not load " << argv[1] << "\n";
        exit(-1);
        
    }
    
    
#ifdef USE_BACKGROUND_TRIGGER_THREAD
    shared_ptr<BackgroundTriggerThread> btt(new BackgroundTriggerThread(ffp.get()));
    btt->startThread();
#endif
    
    
    shared_ptr<FIPConvertToF32> cnvrtToF32(new FIPConvertToF32(*ffp, 1));
    if (!cnvrtToF32->init()) {
        std::cerr << "Could not initialise the cnvrtToF32 processor.\n";
        exit(-1);
    }
    cnvrtToF32->startTriggerThread();
    
    
    shared_ptr<FIPPhotometricEqualise> photometricEqualise(new FIPPhotometricEqualise(*cnvrtToF32, 1, 0.25f));
    if (!photometricEqualise->init()) {
        std::cerr << "Could not initialise the photometric equalise processor.\n";
        exit(-1);
    }
    photometricEqualise->startTriggerThread();
    
    
    std::vector< shared_ptr<FIPGaussianDownsample> > gaussianDownSampleVec;
    std::vector< shared_ptr<FIPGradientXImage> > gaussianDownSampleDDXVec;
    std::vector< shared_ptr<FIPGradientXImage> > gaussianDownSampleDDX2Vec;
    std::vector< shared_ptr<FIPGradientYImage> > gaussianDownSampleDDYVec;
    std::vector< shared_ptr<FIPGradientYImage> > gaussianDownSampleDDY2Vec;
    
    ImageProducer *upstreamProducer=photometricEqualise.get();
    
    //===
    const size_t numLevels=2;
    for (size_t levelNum=0; levelNum<numLevels; levelNum++)
    {
        gaussianDownSampleVec.push_back(shared_ptr<FIPGaussianDownsample>(new FIPGaussianDownsample(*upstreamProducer, 1)));
        if (!gaussianDownSampleVec.back()->init()) {
            std::cerr << "Could not initialise the gaussian downsample processor.\n";
            exit(-1);
        }
        gaussianDownSampleVec.back()->startTriggerThread();
        
        gaussianDownSampleDDXVec.push_back(shared_ptr<FIPGradientXImage>(new FIPGradientXImage(*(gaussianDownSampleVec.back()), 1)));
        if (!gaussianDownSampleDDXVec.back()->init()) {
            std::cerr << "Could not initialise the gaussian downsample ddx processor.\n";
            exit(-1);
        }
        gaussianDownSampleDDXVec.back()->startTriggerThread();
        
        gaussianDownSampleDDX2Vec.push_back(shared_ptr<FIPGradientXImage>(new FIPGradientXImage(*(gaussianDownSampleDDXVec.back()), 1)));
        if (!gaussianDownSampleDDX2Vec.back()->init()) {
            std::cerr << "Could not initialise the gaussian downsample ddx2 processor.\n";
            exit(-1);
        }
        gaussianDownSampleDDX2Vec.back()->startTriggerThread();

        gaussianDownSampleDDYVec.push_back(shared_ptr<FIPGradientYImage>(new FIPGradientYImage(*(gaussianDownSampleVec.back()), 1)));
        if (!gaussianDownSampleDDYVec.back()->init()) {
            std::cerr << "Could not initialise the gaussian downsample ddy processor.\n";
            exit(-1);
        }
        gaussianDownSampleDDYVec.back()->startTriggerThread();
        
        gaussianDownSampleDDY2Vec.push_back(shared_ptr<FIPGradientYImage>(new FIPGradientYImage(*(gaussianDownSampleDDYVec.back()), 1)));
        if (!gaussianDownSampleDDY2Vec.back()->init()) {
            std::cerr << "Could not initialise the gaussian downsample ddy2 processor.\n";
            exit(-1);
        }
        gaussianDownSampleDDY2Vec.back()->startTriggerThread();
        
        upstreamProducer=gaussianDownSampleVec.back().get();
    }
    //===
    
    
    
    
    /*
     shared_ptr<FIPAverageImage> averageImage(new FIPAverageImage(*(gaussianDownSampleVec.back()), 1, 4));
     if (!averageImage->init()) {
     std::cerr << "Could not initialise the average image processor.\n";
     exit(-1);
     }
     averageImage->startTriggerThread();
     
     
     shared_ptr<FIPDewarp> dewarp(new FIPDewarp(*averageImage, 1));
     if (!dewarp->init()) {
     std::cerr << "Could not initialise the dewarp processor.\n";
     exit(-1);
     }
     dewarp->startTriggerThread();
     */
    
    //shared_ptr<MultiOSGConsumer> osgc(new MultiOSGConsumer(*averageImage, 1));
    //shared_ptr<MultiOSGConsumer> osgc(new MultiOSGConsumer(*dewarp, 1));
    shared_ptr<MultiOSGConsumer> osgc(new MultiOSGConsumer(*(gaussianDownSampleVec.back()), 1));
    if (!osgc->init()) {
        std::cerr << "Could not init OSG consumer\n";
        exit(-1);
    }
    
    /*
     shared_ptr<MultiFFmpegConsumer> mffc(new MultiFFmpegConsumer(*averageImage,1));
     if (!mffc->init()) {
     std::cerr << "Could not init FFmpeg consumer\n";
     exit(-1);
     }
     std::stringstream filenameStringStream;
     filenameStringStream << argv[1] << "_ti";
     mffc->openFiles(filenameStringStream.str());
     mffc->startWriting();
     */
    osg::Group *root_node = new osg::Group;
    
    shared_ptr<TexturedQuad> quad(new TexturedQuad(osgc->getOutputTexture()));
    root_node->addChild(quad->getRoot().get());
    
    osgViewer::Viewer viewer;
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.setSceneData(root_node);
    
    //viewer.setUpViewInWindow(100, 100, 640, 480);
    viewer.realize();
    
    const int use_trackball = 0;
    if (use_trackball)
    {
        osgGA::TrackballManipulator* tb = new osgGA::TrackballManipulator;
        viewer.setCameraManipulator(tb);
        adjustCameraManipulatorHomeForYUp(tb);
    } else
    {
        ImageFormat imf = ffp->getFormat();
        OrthoTextureManipulator* om = new OrthoTextureManipulator(osgc->getOutputTexture()->getTextureWidth(), osgc->getOutputTexture()->getTextureHeight());
        viewer.setCameraManipulator(om);
    }
    
    while((!viewer.done())/*&&(ffp->getCurrentImage()<(ffp->getNumImages()*0.9f))*/)
    {
#ifndef USE_BACKGROUND_TRIGGER_THREAD
        //Read from the video, but don't get more than 10 frames ahead.
        if (ffp->getLeastNumReadSlotsAvailable()<10)
        {
            ffp->trigger();
        }
#endif
        
        if (osgc->getNext()) {
            viewer.frame();
            //OpenThreads::Thread::microSleep(500000);
        }
    }
    /*
     mffc->stopWriting();
     mffc->closeFiles();
     */
    
#ifdef USE_BACKGROUND_TRIGGER_THREAD
    btt->setExit();
    btt->join();
#endif   
    
    return 0;
}
