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

#include <flitr/modules/glsl_shader_passes/glsl_keep_history_pass.h>

using std::shared_ptr;
using namespace flitr;

int main(int argc, char *argv[])
{
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " video_file\n";
        return 1;
    }

    shared_ptr<FFmpegProducer> ffp(new FFmpegProducer(argv[1], ImageFormat::FLITR_PIX_FMT_Y_8));
    if (!ffp->init()) {
        std::cerr << "Could not load " << argv[1] << "\n";
        exit(-1);
    }

    int history_size = 10;

    shared_ptr<MultiOSGConsumer> osgc(new MultiOSGConsumer(*ffp,1));
    if (!osgc->init()) {
        std::cerr << "Could not init OSG consumer\n";
        exit(-1);
    }

    osg::Group *root_node = new osg::Group;

    shared_ptr<GLSLKeepHistoryPass> hp(new GLSLKeepHistoryPass(osgc->getOutputTexture(), history_size));
    if (argc > 2) {
        hp->setShader(argv[2]);
    }
    root_node->addChild(hp->getRoot().get());
    
    int xoffset=0;
    std::vector< shared_ptr<TexturedQuad> > quad_vec;
    for(int i=0; i<history_size; i++) {
        quad_vec.push_back(shared_ptr<TexturedQuad>(new TexturedQuad(hp->getOutputTexture(i))));
        osg::Matrixd m;
        m.makeTranslate(osg::Vec3d(xoffset,0,0));
        quad_vec[i]->setTransform(m);
        root_node->addChild(quad_vec[i]->getRoot().get());
        xoffset += ffp->getFormat().getWidth();
    }

    osgViewer::Viewer viewer;
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.setSceneData(root_node);

    //viewer.setUpViewInWindow(480+40, 40, 640, 480);
    viewer.realize();
    osgGA::TrackballManipulator* tb = new osgGA::TrackballManipulator;
    viewer.setCameraManipulator(tb);
    adjustCameraManipulatorHomeForYUp(tb);
    
    int frame_count = 0;
    while(!viewer.done()) {
        ffp->trigger();
        if (osgc->getNext()) {
            // rewire so the latest texture is on the left
            for (int i=0; i< history_size; i++) {
                quad_vec[i]->setTexture(hp->getOutputTexture(i));
            }
            viewer.frame();
            frame_count++;
            OpenThreads::Thread::microSleep(40000);
        } else {
            OpenThreads::Thread::microSleep(50);
        }
    }

    return 0;
}
