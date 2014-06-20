#include <flitr/ffmpeg_producer.h>
#include <flitr/multi_osg_consumer.h>
#include <flitr/textured_quad.h>
#include <flitr/manipulator_utils.h>
#include <flitr/ortho_texture_manipulator.h>
#include <flitr/modules/geometry_overlays/points_overlay.h>

#include <osg/Vec3>
#include <osg/Vec4>
#include <osg/Quat>
#include <osg/Matrix>

#include <osgDB/FileUtils>
#include <osgDB/ReadFile>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <osgGA/TrackballManipulator>
#include <osg/io_utils>

#include <iostream>
#include <sstream>
#include <cstdlib>

using std::shared_ptr;
using namespace flitr;

void updatePoints(PointsOverlay* po, int width, int height, int num)
{
    osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array();
    
    int i=0;
    for (; i<num/2; ++i) {
        double x = 0.5 + (rand() % width);
        double y = 0.5 + (rand() % height);
        double z = 0.1;
        va->push_back(osg::Vec3d(x,y,z));
    }

    po->setVertices(*va);

    for (; i<num; ++i) {
        double x = 0.5 + (rand() % width);
        double y = 0.5 + (rand() % height);
        double z = 0.1;
        po->addVertex(osg::Vec3d(x,y,z));
    }
}

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

    shared_ptr<MultiOSGConsumer> osgc(new MultiOSGConsumer(*ffp,1));
    if (!osgc->init()) {
        std::cerr << "Could not init OSG consumer\n";
        exit(-1);
    }

    osg::Group *root_node = new osg::Group;
    
    shared_ptr<TexturedQuad> quad(new TexturedQuad(osgc->getOutputTexture()));
    root_node->addChild(quad->getRoot().get());

    osgViewer::Viewer viewer;
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    
    ImageFormat imf = ffp->getFormat();
    double w = imf.getWidth();
    double h = imf.getHeight();
    
    PointsOverlay* pov = new PointsOverlay();
    pov->setPointSize(11);
    pov->setColour(osg::Vec4(1.0, 1.0, 0.2, 0.8));
    root_node->addChild(pov);

    viewer.setSceneData(root_node);

    OrthoTextureManipulator* om = new OrthoTextureManipulator(imf.getWidth(), imf.getHeight());
    viewer.setCameraManipulator(om);

    // disable all culling
    //viewer.getCamera()->setCullingMode(0);

    const int num_points = 1000;
    while(!viewer.done())
    {
        ffp->trigger();
        if (osgc->getNext()) {
            updatePoints(pov, w, h, num_points);
            viewer.frame();
        } else {
            OpenThreads::Thread::microSleep(5000);
        }
    }

    return 0;
}
