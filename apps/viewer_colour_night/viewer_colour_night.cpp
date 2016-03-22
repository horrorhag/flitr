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
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " video_file0  video_file1 video_file2\n";
        return 1;
    }

    
    std::vector<std::string> fileVec;
    
    for (size_t arg=1; arg<argc; ++arg)
    {
        fileVec.emplace_back(argv[arg]);
    }
    
    
    shared_ptr<MultiLibTiffProducer> ip(new MultiLibTiffProducer(fileVec));
    
    if (!ip->init()) {
        std::cerr << "Could not load " << argv[1] << "\n";
        exit(-1);
    }

    
    shared_ptr<MultiOSGConsumer> osgc(new MultiOSGConsumer(*ip, fileVec.size()));
    
    if (!osgc->init()) {
        std::cerr << "Could not init OSG consumer\n";
        exit(-1);
    }

    
    osg::Group *root_node = new osg::Group;
    
    std::vector<osg::TextureRectangle *> inTexVec;
    inTexVec.push_back(osgc->getOutputTexture(0));
    inTexVec.push_back(osgc->getOutputTexture(1));
    inTexVec.push_back(osgc->getOutputTexture(2));
    
    shared_ptr<MultiTexturedQuad> multiTexQuad(new MultiTexturedQuad(inTexVec));
    multiTexQuad->setColourMask(osg::Vec4f(1.0, 0.0, 0.0, 0.0), 0);
    multiTexQuad->setColourMask(osg::Vec4f(0.0, 1.0, 0.0, 0.0), 1);
    multiTexQuad->setColourMask(osg::Vec4f(0.0, 0.0, 1.0, 0.0), 2);
    multiTexQuad->setShader("fuse.frag");
    root_node->addChild(multiTexQuad->getRoot().get());
    
    
    osgViewer::Viewer viewer;
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.setSceneData(root_node);

    viewer.setUpViewInWindow(480+40, 40, 640, 480);
    viewer.realize();

    const int use_trackball = 0;
    if (use_trackball) {
        osgGA::TrackballManipulator* tb = new osgGA::TrackballManipulator;
        viewer.setCameraManipulator(tb);
        adjustCameraManipulatorHomeForYUp(tb);
    } else {

        ImageFormat imf = ip->getFormat(0);
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
            ip->seek(0xFFFFFFFF);//Seek to end of file for live multipage view...
            //OR
            //ip->trigger();//Read every slot...

            if (osgc->getNext())
            {
                viewer.frame();
                nextFrameTimeNS+=framePeriodNS;
            }
        }
        else
        {
            OpenThreads::Thread::microSleep(1000);
        }
    }

    return 0;
}
