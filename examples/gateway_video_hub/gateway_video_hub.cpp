#include "flitr_video_hub.h"

#include <flitr/multi_osg_consumer.h>
#include <flitr/textured_quad.h>
#include <flitr/manipulator_utils.h>
#include <flitr/ortho_texture_manipulator.h>
#include <flitr/ffmpeg_producer.h>
#include <flitr/multi_ffmpeg_consumer.h>
#include <flitr/multi_webrtc_consumer.h>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <iostream>
#include <string>



int main(int argc, char *argv[])
{

    
    flitr::VideoHub videoHub;
    //videoHub.createV4LProducer("v4linput", "/dev/video0");
    videoHub.createVideoFileProducer("input", "/Users/bduvenhage/Desktop/nikon_compressed.mp4");
    
    videoHub.createImageStabProcess("istab", "input");
    //ToDo: High pass filter output transform - lkstabilise->burnOutputTransform(0.975f, 0.975f);
    
    videoHub.createVideoFileConsumer("output", "istab", "/Users/bduvenhage/Desktop/output.avi", 20);
    //videoHub.createWebRTCConsumer("webrtcstream", "istab", "146.64.246.11:8000");
    
    
    
    //=============================//
    //=== OSG and Viewer things ===//
    std::shared_ptr<flitr::MultiOSGConsumer> osgc(new flitr::MultiOSGConsumer(*(videoHub.getProducer("istab")), 1, 1));
    if (!osgc->init()) {
        std::cerr << "Could not init OSG consumer\n";
        exit(-1);
    }
    
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
    //=============================//
    //=============================//
    
    
    while(!viewer.done())
    {
        videoHub.getProducer("input")->trigger();
        
        if (osgc->getNext())
        {
            viewer.frame();
        }
        
        OpenThreads::Thread::microSleep(1000);
    }
    
    return 0;
}
