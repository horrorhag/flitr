#include <iostream>
#include <string>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/TrackballManipulator>
#include <osg/io_utils>

#include <flitr/ffmpeg_producer.h>
#include <flitr/multi_ffmpeg_consumer.h>
#include <flitr/multi_osg_consumer.h>
#include <flitr/textured_quad.h>
#include <flitr/manipulator_utils.h>

#include <flitr/modules/glsl_shader_passes/glsl_shader_pass.h>

#include <flitr/texture_capture_producer.h>

using std::shared_ptr;
using namespace flitr;

int main(int argc, char *argv[])
{
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " video_file shader_file\n";
        return 1;
    }

    //shared_ptr<FFmpegProducer> ffp(new FFmpegProducer(argv[1], ImageFormat::FLITR_PIX_FMT_Y_16));
    //shared_ptr<FFmpegProducer> ffp(new FFmpegProducer(argv[1], ImageFormat::FLITR_PIX_FMT_Y_8));
    shared_ptr<FFmpegProducer> ffp(new FFmpegProducer(argv[1], ImageFormat::FLITR_PIX_FMT_ANY));//Produced format is the same as the video pixel format!

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
    
    shared_ptr<GLSLShaderPass> ssp(new GLSLShaderPass(osgc->getOutputTexture()));
    ssp->setShader(argv[2]);
    root_node->addChild(ssp->getRoot().get());


    shared_ptr<TexturedQuad> quad(new TexturedQuad(ssp->getOutputTexture()));
    root_node->addChild(quad->getRoot().get());


    //=== Video recording code ===//
    shared_ptr<TextureCaptureProducer> tcp(new TextureCaptureProducer(ssp->getOutputTexture(),
                                                                      ImageFormat::FLITR_PIX_FMT_ANY, //Produced format same as captured format!
//                                                                      ImageFormat::FLITR_PIX_FMT_Y_8, //Produced format is Y_8!
//                                                                      ImageFormat::FLITR_PIX_FMT_RGB_8, //Produced format is RGB_8!
//                                                                      ImageFormat::FLITR_PIX_FMT_Y_16, //Produced format is Y_16!
                                                                      ssp->getOutputTexture()->getTextureWidth(),
                                                                      ssp->getOutputTexture()->getTextureHeight(),
                                                                      true,//keep image aspect.
                                                                      3,//producer buffer sze.
                                                                      true));//blocking operation.
    if (!tcp->init()) {
        std::cerr << "Could not init TextureCaptureProducer.\n";
        exit(-1);
    }
    root_node->addChild(tcp->getRoot().get());

    shared_ptr<MultiFFmpegConsumer> mfc(new MultiFFmpegConsumer(*tcp, 1));

    if (!mfc->init()) {
        std::cerr << "Could not init MultiFFmpegConsumer.\n";
        exit(-1);
    }

    mfc->setCodec(flitr::FLITR_RAWVIDEO_CODEC);
    mfc->setContainer(flitr::FLITR_AVI_CONTAINER);

    mfc->openFiles("output", 20);
    mfc->startWriting();

    //=== ===//

    osgViewer::Viewer viewer;
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.setSceneData(root_node);

    viewer.setUpViewInWindow(40, 40, 640, 480);
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

    mfc->stopWriting();
    mfc->closeFiles();

    return 0;
}
