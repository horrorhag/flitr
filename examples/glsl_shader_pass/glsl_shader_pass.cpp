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

#include "simple_shader_pass.h"
#include "render_to_video.h"

using std::tr1::shared_ptr;
using namespace flitr;

int main(int argc, char *argv[])
{
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " video_file shader_file\n";
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
    
    shared_ptr<SimpleShaderPass> ssp(new SimpleShaderPass(osgc->getOutputTexture()));
    ssp->setShader(argv[2]);
    root_node->addChild(ssp->getRoot().get());

    shared_ptr<TexturedQuad> quad(new TexturedQuad(ssp->getOutputTexture()));
    root_node->addChild(quad->getRoot().get());

    #if 0
    shared_ptr<RenderToVideo> rtv(new RenderToVideo(ssp->getOutputTexture(), "glsl_shader_pass.avi"));
    root_node->addChild(rtv->getRoot().get());
    #endif    

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
