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
#include <flitr/ortho_texture_manipulator.h>

#include "cuda_histogram_equalisation_pass.h"

using std::tr1::shared_ptr;
using namespace flitr;

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

    osg::Group* root_node = new osg::Group;


    shared_ptr<MultiOSGConsumer> osgc(new MultiOSGConsumer(*ffp,1));
    if (!osgc->init()) {
        std::cerr << "Could not init OSG consumer\n";
        exit(-1);
    }

    // Test CUDA histogram equalization
    ImageFormat imgFormat = ffp->getFormat();
    CUDAHistogramEqualisationPass* CUDAHistEq = new CUDAHistogramEqualisationPass(osgc->getOutputTexture(0,0), imgFormat);
    root_node->addChild(CUDAHistEq->getRoot().get());
    
    //shared_ptr<TexturedQuad> quad(new TexturedQuad(osgc->getOutputTexture()));
    shared_ptr<TexturedQuad> quad(new TexturedQuad(CUDAHistEq->getOutputTexture()));

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
       
        ImageFormat imf = ffp->getFormat();
        OrthoTextureManipulator* om = new OrthoTextureManipulator(imf.getWidth(), imf.getHeight());
        viewer.setCameraManipulator(om);
    }

    while(!viewer.done()) {
        ffp->trigger();
        if (osgc->getNext()) {
            viewer.frame();
        } else {
            OpenThreads::Thread::microSleep(5000);
        }
    }

    return 0;
}
