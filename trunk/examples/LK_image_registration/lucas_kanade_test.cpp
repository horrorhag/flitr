#include <iostream>
#include <string>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/TrackballManipulator>
#include <osg/io_utils>

#include <flitr/test_pattern_producer.h>
#include <flitr/ffmpeg_producer.h>
#include <flitr/multi_osg_consumer.h>
#include <flitr/textured_quad.h>
#include <flitr/manipulator_utils.h>
#include <flitr/ortho_texture_manipulator.h>

#include <flitr/modules/lucas_kanade/ImageStabiliserBiLK.h>
#include <flitr/modules/lucas_kanade/ImageStabiliserNLK.h>

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
    shared_ptr<TestPatternProducer> ip(new TestPatternProducer(1024, 768, ImageFormat::FLITR_PIX_FMT_Y_8, 0.1));
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


    unsigned long roi_dim=256;
    //==================
    /*
    ImageStabiliserBiLK *iStab=new ImageStabiliserBiLK(osgc->getOutputTexture(0, 0),
                                  25, ip->getFormat().getHeight()/2-roi_dim/2,
                                  ip->getFormat().getWidth()-25-roi_dim, ip->getFormat().getHeight()/2-roi_dim/2,
                                  roi_dim, roi_dim,
                                  true,//Indicate ROI?
                                  true,//Do GPU Pyramid construction.
                                  true,//Do GPU LK iteration.
                                  2,//Number of GPU h-vector reduction levels.
                                  false,//Read output back to a CPU image?
                                  false,//Bilinear output filter.
                                  1,//Output scale factor.
                                  1.0f,//Output crop factor.
                                  0.0, 1);
*/
    //------------------
    std::vector< std::pair<int,int> > roiVec;
    roiVec.push_back(std::pair<int,int>(25, ip->getFormat().getHeight()/2-roi_dim/2));
    roiVec.push_back(std::pair<int,int>(ip->getFormat().getWidth()-25-roi_dim, ip->getFormat().getHeight()/2-roi_dim/2));
    ImageStabiliserNLK *iStab=new ImageStabiliserNLK(osgc->getOutputTexture(0, 0),
                                                     roiVec,
                                                     roi_dim, roi_dim,
                                                     true,//Indicate ROI?
                                                     false,//Do GPU Pyramid construction.
                                                     false,//Do GPU LK iteration.
                                                     0,//Number of GPU h-vector reduction levels.
                                                     false,//Read output back to a CPU image?
                                                     false,//Bilinear output filter.
                                                     1,//Output scale factor.
                                                     1.0f,//Output crop factor.
                                                     0.0, 1);
    //==================

    iStab->init(root_node);

    iStab->setAutoSwapCurrentPrevious(true); //Do not compare successive frames with each other. E.g. compare all frames with specific reference frame.


    shared_ptr<TexturedQuad> quad(new TexturedQuad(iStab->getOutputTexture()));
    root_node->addChild(quad->getRoot().get());

    //=== ===//
    /*
    const int width( 800 ), height( 450 );
    const std::string version( "3.1" );
    osg::ref_ptr< osg::GraphicsContext::Traits > traits = new osg::GraphicsContext::Traits();
    traits->x = 20; traits->y = 30;
    traits->width = width; traits->height = height;
    traits->windowDecoration = true;
    traits->doubleBuffer = true;
    traits->glContextVersion = version;
    osg::ref_ptr< osg::GraphicsContext > gc = osg::GraphicsContext::createGraphicsContext( traits.get() );
    if( !gc.valid() )
    {
        osg::notify( osg::FATAL ) << "Unable to create OpenGL v" << version << " context." << std::endl;
        return( 1 );
    }

    // Create a Camera that uses the above OpenGL context.
    osg::Camera* cam = new osg::Camera;
    cam->setGraphicsContext( gc.get() );
    // Must set perspective projection for fovy and aspect.
    cam->setProjectionMatrix( osg::Matrix::perspective( 30., (double)width/(double)height, 1., 100. ) );
    // Unlike OpenGL, OSG viewport does *not* default to window dimensions.
    cam->setViewport( new osg::Viewport( 0, 0, width, height ) );
    */

    osgViewer::Viewer viewer;
    //viewer.setCamera( cam );
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.setSceneData(root_node);

    // for non GL3/GL4 and non GLES2 platforms we need enable the osg_ uniforms that the shaders will use,
    // you don't need thse two lines on GL3/GL4 and GLES2 specific builds as these will be enable by default.
    //gc->getState()->setUseModelViewAndProjectionUniforms(true);
    //gc->getState()->setUseVertexAttributeAliasing(true);

    viewer.realize();
    //=== ===//


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

            numFramesDone++;
        }
    }


    return 0;
}


