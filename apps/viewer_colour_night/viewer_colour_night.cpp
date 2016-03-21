#include <iostream>
#include <string>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/TrackballManipulator>
#include <osg/io_utils>

#include <flitr/ffmpeg_producer.h>
#include <flitr/libtiff_producer.h>
#include <flitr/multi_osg_consumer.h>
#include <flitr/textured_quad.h>
#include <flitr/manipulator_utils.h>
#include <flitr/ortho_texture_manipulator.h>

using std::shared_ptr;
using namespace flitr;

int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " video_file\n";
        return 1;
    }

    shared_ptr<LibTiffProducer> ip(new LibTiffProducer(argv[1]));
    if (!ip->init()) {
        std::cerr << "Could not load " << argv[1] << "\n";
        exit(-1);
    }

    shared_ptr<MultiOSGConsumer> osgc(new MultiOSGConsumer(*ip,1));
    if (!osgc->init()) {
        std::cerr << "Could not init OSG consumer\n";
        exit(-1);
    }

    osg::Group *root_node = new osg::Group;
    
    shared_ptr<TexturedQuad> quad(new TexturedQuad(osgc->getOutputTexture()));
    //quad->flipTextureCoordsLeftToRight();
    //quad->flipTextureCoordsTopToBottom();
    root_node->addChild(quad->getRoot().get());

    osgViewer::Viewer viewer;
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.setSceneData(root_node);

    //viewer.setUpViewInWindow(480+40, 40, 640, 480);
    viewer.realize();

    const int use_trackball = 0;
    if (use_trackball) {
        osgGA::TrackballManipulator* tb = new osgGA::TrackballManipulator;
        viewer.setCameraManipulator(tb);
        adjustCameraManipulatorHomeForYUp(tb);
    } else {

        ImageFormat imf = ip->getFormat();
        OrthoTextureManipulator* om = new OrthoTextureManipulator(imf.getWidth(), imf.getHeight());
        viewer.setCameraManipulator(om);
    }

    uint32_t frameRate=10;
    double framePeriodNS=1.0e9 / frameRate;
    double nextFrameTimeNS=currentTimeNanoSec()+framePeriodNS;

    while(!viewer.done())
    {
        if (currentTimeNanoSec()>=nextFrameTimeNS)
        {
            const uint32_t numTifPages=ip->getNumImages();
            
            //ip->seek(numTifPages>=2 ? numTifPages-2 : 0);
            ip->trigger();
            
            std::cout << numTifPages << "\n";
            std::cout.flush();
            
            if (osgc->getNext())
            {
                viewer.frame();
                nextFrameTimeNS+=framePeriodNS;
            }
        }
        else
        {
            OpenThreads::Thread::microSleep(5000);
        }
    }

#ifdef USE_BACKGROUND_TRIGGER_THREAD
    btt->setExit();
    btt->join();
#endif   

    return 0;
}
