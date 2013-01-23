#include <iostream>
#include <string>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/TrackballManipulator>
#include <osg/io_utils>

#include <flitr/points_overlay.h>
#include <flitr/test_pattern_producer.h>
#include <flitr/ffmpeg_producer.h>
#include <flitr/multi_osg_consumer.h>
#include <flitr/textured_quad.h>
#include <flitr/manipulator_utils.h>
#include <flitr/ortho_texture_manipulator.h>

#include <flitr/modules/lucas_kanade/ImageStabiliserMultiLK.h>

#include <flitr/modules/cpu_shader_passes/cpu_shader_pass.h>
#include <flitr/modules/cpu_shader_passes/cpu_palette_remap_shader.h>
#include <flitr/modules/cpu_shader_passes/cpu_photometric_equalisation.h>

#include <flitr/modules/glsl_shader_passes/glsl_shader_pass.h>

using std::tr1::shared_ptr;
using namespace flitr;

int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " video_file\n";
        return 1;
    }

#define USE_TEST_PATTERN 1
#ifdef USE_TEST_PATTERN
    shared_ptr<TestPatternProducer> ip(new TestPatternProducer(1024, 768, ImageFormat::FLITR_PIX_FMT_Y_8, 0.5, 6));
    if (!ip->init()) {
        std::cerr << "Could not instantiate the test pattern producer.\n";
        exit(-1);
    }
#else
    shared_ptr<FFmpegProducer> ip(new FFmpegProducer(argv[1], ImageFormat::FLITR_PIX_FMT_Y_8));
    if (!ip->init()) {
        std::cerr << "Could not load " << argv[1] << "\n";
        exit(-1);
    }
#endif


    shared_ptr<MultiOSGConsumer> osgc(new MultiOSGConsumer(*ip,1));
    if (!osgc->init()) {
        std::cerr << "Could not init OSG consumer\n";
        exit(-1);
    }


    osg::Group *root_node = new osg::Group;

    //===
    shared_ptr<flitr::CPUShaderPass> gfp(new flitr::CPUShaderPass(osgc->getOutputTexture()));
    gfp->setGPUShader("/home/ballon/flitr/share/flitr/shaders/copy.frag");
    gfp->setPostRenderCPUShader(new CPUPhotometricCalibration_Shader(gfp->getOutImage(), 0.5, 0.025));
    root_node->addChild(gfp->getRoot().get());


    //==================
    unsigned long roi_dim=256;

    std::vector< std::pair<int,int> > roiVec;

    roiVec.push_back(std::pair<int,int>(25, 25));
    roiVec.push_back(std::pair<int,int>(ip->getFormat().getWidth()-roi_dim-25, 25));
    roiVec.push_back(std::pair<int,int>(ip->getFormat().getWidth()-roi_dim-25, ip->getFormat().getHeight()-roi_dim-25));
    roiVec.push_back(std::pair<int,int>(25, ip->getFormat().getHeight()-roi_dim-25));

    ImageStabiliserMultiLK *iStab=new ImageStabiliserMultiLK(gfp->getOutputTexture(),
                                                             roiVec,
                                                             roi_dim, roi_dim,
                                                             true,//Indicate ROI?
                                                             true,//Do GPU Pyramid construction.
                                                             true,//Do GPU LK iteration.
                                                             2,//Number of GPU h-vector reduction levels.
                                                             false,//Read output back to a CPU image?
                                                             false,//Bilinear output filter.
                                                             1,//Output scale factor.
                                                             1.0f,//Output crop factor.
                                                             0.3, 1);
    //==================

    iStab->init(root_node);

    iStab->setAutoSwapCurrentPrevious(true); //Do not compare successive frames with each other. E.g. compare all frames with specific reference frame.


    //shared_ptr<TexturedQuad> quad(new TexturedQuad(gfp->getOutputTexture()));
    shared_ptr<TexturedQuad> quad(new TexturedQuad(iStab->getOutputTexture()));
    root_node->addChild(quad->getRoot().get());


    PointsOverlay* pov = new PointsOverlay();
    pov->setPointSize(11);
    pov->setColour(osg::Vec4(1.0, 0.0, 0.0, 0.8));
    root_node->addChild(pov);


    osgViewer::Viewer viewer;
    //viewer.setCamera( cam );
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.setSceneData(root_node);

    viewer.realize();


    ImageFormat imf = ip->getFormat();
    OrthoTextureManipulator* om = new OrthoTextureManipulator(imf.getWidth(), imf.getHeight());
    viewer.setCameraManipulator(om);

    unsigned long numFramesDone=0;
    while(!viewer.done())
    {
        //Read from the video, but don't get more than 10 frames ahead.
        if (ip->getLeastNumReadSlotsAvailable()<10)
        {
            ip->trigger();
        }

        if (osgc->getNext()) {
            if (numFramesDone>=2)
            {
                iStab->triggerInput();
            }

            viewer.frame();

            pov->clearVertices();
            for (int i=0; i<iStab->getNumPyramids(); i++)
            {
                osg::Vec2f rc=iStab->getTransformedROI_Centre(i);
                pov->addVertex(osg::Vec3d(rc._v[0], ip->getFormat().getHeight() - rc._v[1] - 1, 0.1));
            }

            numFramesDone++;
        }
    }


    return 0;
}


