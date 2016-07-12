#include <iostream>
#include <string>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <flitr/ffmpeg_producer.h>
#include <flitr/multi_ffmpeg_consumer.h>
#include <flitr/multi_osg_consumer.h>
#include <flitr/textured_quad.h>
#include <flitr/manipulator_utils.h>
#include <flitr/ortho_texture_manipulator.h>
#include <flitr/multi_webrtc_consumer.h>



int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " video_file\n";
        return 1;
    }
    
    
    std::shared_ptr<flitr::FFmpegProducer> ip(new flitr::FFmpegProducer(argv[1], flitr::ImageFormat::FLITR_PIX_FMT_Y_8, 2));
    if (!ip->init()) {
        std::cerr << "Could not load " << argv[1] << "\n";
        exit(-1);
    }
    
    
    std::shared_ptr<flitr::MultiOSGConsumer> osgc(new flitr::MultiOSGConsumer(*ip, 1, 1));
    if (!osgc->init()) {
        std::cerr << "Could not init OSG consumer\n";
        exit(-1);
    }

    
    std::shared_ptr<flitr::MultiWebRTCConsumer> webrtcCons(new flitr::MultiWebRTCConsumer(*ip, 1));
    if (!webrtcCons->init()) {
        std::cerr << "Could not init WebRTC consumer\n";
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
    
    std::shared_ptr<flitr::TexturedQuad> quad(new flitr::TexturedQuad(osgc->getOutputTexture()));
    root_node->addChild(quad->getRoot().get());
    {
        osg::Matrix translate;
        translate.makeTranslate(osg::Vec3d(0.0, //left/right
                                           0.0, //up/down
                                           0.0));
        quad->setTransform(translate);
    }
    
    
    osgViewer::Viewer viewer;
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.setSceneData(root_node);
    viewer.setUpViewInWindow(100, 100, 640, 480);
    viewer.realize();
    
    flitr::OrthoTextureManipulator* om = new flitr::OrthoTextureManipulator(osgc->getOutputTexture()->getTextureWidth(), osgc->getOutputTexture()->getTextureHeight());
    viewer.setCameraManipulator(om);
    
    
    while(!viewer.done())
    {
        //Read from the video, but don't get more than n frames ahead.
        if (ip->getLeastNumReadSlotsAvailable()<5)
        {
            ip->trigger();
        }
        
        if (osgc->getNext())
        {
            viewer.frame();
        }
        
        OpenThreads::Thread::microSleep(1000);
    }
    
    //     mffc->stopWriting();
    //     mffc->closeFiles();
    
    
    //transform->stopTriggerThread();
    
    return 0;
}
