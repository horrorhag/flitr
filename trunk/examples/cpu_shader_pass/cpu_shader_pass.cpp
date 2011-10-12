#include <iostream>
#include <string>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/TrackballManipulator>
#include <osg/io_utils>

#include <flitr/ffmpeg_producer.h>
#include <flitr/multi_osg_consumer.h>
#include <flitr/textured_quad.h>
#include <flitr/manipulator_utils.h>

#include "simple_cpu_shader_pass.h"
#include <examples/glsl_shader_pass/simple_shader_pass.h>

using std::tr1::shared_ptr;
using namespace flitr;


int main(int argc, char *argv[])
{
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " video_file glsl_shader_file (Note: cpu pre- and post-shaders in simple_cpu_shader_pass.h).\n";
        return 1;
    }

    shared_ptr<FFmpegProducer> ffp(new FFmpegProducer(argv[1], ImageFormat::FLITR_PIX_FMT_Y_8));
    if (!ffp->init()) {
        std::cerr << "Could not load " << argv[1] << "\n";
        exit(-1);
    }

    shared_ptr<MultiOSGConsumer> osgc(new MultiOSGConsumer(*ffp,1));
    if (!osgc->init()) {
        std::cerr << "Could not init OSG consumer.\n";
        exit(-1);
    }

    osg::Group *root_node = new osg::Group;

//=== 
/*
    shared_ptr<SimpleShaderPass> ssp(new SimpleShaderPass(osgc->getOutputTexture(), true));
    ssp->setShader("copy.frag");
    root_node->addChild(ssp->getRoot().get());

    shared_ptr<SimpleCPUShaderPass> gfp(new SimpleCPUShaderPass(ssp->getOSGImage(), true));
*/
//OR
    shared_ptr<SimpleCPUShaderPass> gfp(new SimpleCPUShaderPass(osgc->getOSGImage(), true));
//===
    gfp->setShader(argv[2]);
    root_node->addChild(gfp->getRoot().get());


    shared_ptr<TexturedQuad> quad(new TexturedQuad(gfp->getOutputTexture()));
    root_node->addChild(quad->getRoot().get());

    osgViewer::Viewer viewer;
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.setSceneData(root_node);

    //viewer.setUpViewInWindow(480+40, 40, 640, 480);
    viewer.realize();
    osgGA::TrackballManipulator* tb = new osgGA::TrackballManipulator;
    viewer.setCameraManipulator(tb);
    adjustCameraManipulatorHomeForYUp(tb);
    
    while(!viewer.done()) {
        ffp->trigger();
        if (osgc->getNext()) {
            viewer.frame();
        }
    }

    return 0;
}
