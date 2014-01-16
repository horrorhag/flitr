#include <iostream>
#include <string>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/TrackballManipulator>
#include <osg/io_utils>

#include <flitr/modules/geometry_overlays/points_overlay.h>
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
#include <flitr/modules/cpu_shader_passes/cpu_uniform_noise_shader.h>

#include <flitr/modules/glsl_shader_passes/glsl_shader_pass.h>
#include <flitr/modules/glsl_shader_passes/glsl_downsample_x_pass.h>
#include <flitr/modules/glsl_shader_passes/glsl_downsample_y_pass.h>
#include <flitr/modules/glsl_shader_passes/glsl_translate_pass.h>
#include <flitr/modules/glsl_shader_passes/glsl_gaussian_filter_x_pass.h>
#include <flitr/modules/glsl_shader_passes/glsl_gaussian_filter_y_pass.h>
#include <flitr/modules/glsl_shader_passes/glsl_keep_history_pass.h>

using std::tr1::shared_ptr;
using namespace flitr;

int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " video_file\n";
        return 1;
    }

    //#define USE_TEST_PATTERN 1
#ifdef USE_TEST_PATTERN
    int dsFactor=4;
    bool jitter=false;

    shared_ptr<TestPatternProducer> ip(new TestPatternProducer(512*dsFactor, 512*dsFactor, ImageFormat::FLITR_PIX_FMT_Y_8, 0.5, 8));
    if (!ip->init()) {
        std::cerr << "Could not instantiate the test pattern producer.\n";
        exit(-1);
    }
#else
    int dsFactor=1;
    bool jitter=false;

    shared_ptr<FFmpegProducer> ip(new FFmpegProducer(argv[1], ImageFormat::FLITR_PIX_FMT_ANY));
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


    //=== GLSL Jitter pass ===
    shared_ptr<flitr::GLSLTranslatePass> translate(new flitr::GLSLTranslatePass(osgc->getOutputTexture()));
    root_node->addChild(translate->getRoot().get());
    //===


    //=== GLSL Gaussian filter before downsample ===
    shared_ptr<flitr::GLSLGaussianFilterXPass> gfX(new flitr::GLSLGaussianFilterXPass(translate->getOutputTexture()));
    gfX->setRadius(8.0*dsFactor);//*dsFactor because it is before downsample pass.
    root_node->addChild(gfX->getRoot().get());

    shared_ptr<flitr::GLSLGaussianFilterYPass> gfY(new flitr::GLSLGaussianFilterYPass(gfX->getOutputTexture()));
    gfY->setRadius(8.0*dsFactor);
    root_node->addChild(gfY->getRoot().get());
    //===


    //=== GLSL Downsample pass ===
    shared_ptr<flitr::GLSLDownsampleXPass> dsX(new flitr::GLSLDownsampleXPass(gfY->getOutputTexture(), dsFactor));
    root_node->addChild(dsX->getRoot().get());


    shared_ptr<flitr::GLSLDownsampleYPass> dsY(new flitr::GLSLDownsampleYPass(dsX->getOutputTexture(), dsFactor));
    root_node->addChild(dsY->getRoot().get());
    //===


    //=== CPU Photometric calibration pass ===
    shared_ptr<flitr::CPUShaderPass> gfp(new flitr::CPUShaderPass(dsY->getOutputTexture()));
    gfp->setGPUShader("copy.frag");
    gfp->setPostRenderCPUShader(new CPUPhotometricEqualisation_Shader(gfp->getOutImage(), 0.5, 0.01));
    root_node->addChild(gfp->getRoot().get());
    //===


    //=== Add uniform pixel noise after downsample ===
    shared_ptr<flitr::CPUShaderPass> noisePass(new flitr::CPUShaderPass(gfp->getOutputTexture()));
    noisePass->setGPUShader("copy.frag");
    noisePass->setPostRenderCPUShader(new CPUUniformNoise_Shader(noisePass->getOutImage(), 0.0));
    root_node->addChild(noisePass->getRoot().get());
    //===


    //=== GPS+CPU Lucas-Kanade ===
    unsigned long roi_dim=256;
    std::vector< std::pair<int,int> > roiVec;

    roiVec.push_back(std::pair<int,int>(noisePass->getOutputTexture()->getTextureWidth()/2-roi_dim/2,
                                        noisePass->getOutputTexture()->getTextureHeight()/2-roi_dim/2));
    //roiVec.push_back(std::pair<int,int>(5, 5));
    //roiVec.push_back(std::pair<int,int>(noisePass->getOutputTexture()->getTextureWidth()-roi_dim-5, 5));
    //roiVec.push_back(std::pair<int,int>(noisePass->getOutputTexture()->getTextureWidth()-roi_dim-5, noisePass->getOutputTexture()->getTextureHeight()-roi_dim-5));
    //roiVec.push_back(std::pair<int,int>(5, noisePass->getOutputTexture()->getTextureHeight()-roi_dim-5));

    ImageStabiliserMultiLK *iStab=new ImageStabiliserMultiLK(noisePass->getOutputTexture(),
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
                                                             0.5, 1);

    iStab->init(root_node);
    iStab->setAutoSwapCurrentPrevious(true); //Do not compare successive frames with each other. E.g. compare all frames with specific reference frame.
    //==================


    //=== GLSL history pass to delay the iStab output by one frame to sync with points overlay. ===
    shared_ptr<GLSLKeepHistoryPass> hp(new GLSLKeepHistoryPass(iStab->getOutputTexture(), 2));
    root_node->addChild(hp->getRoot().get());
    //===


    //=== View the result texture delayed by one frame to match points overlay ===
    shared_ptr<TexturedQuad> quad(new TexturedQuad(hp->getOutputTexture(1)));
    root_node->addChild(quad->getRoot().get());
    //===


    //=== Points overlay to test homography registration ===
    //Note that the points will be one frame behind input!!!
    PointsOverlay* pov = new PointsOverlay();
    pov->setPointSize(11);
    pov->setColour(osg::Vec4(1.0, 0.0, 0.0, 0.8));
    root_node->addChild(pov);
    //===


    osgViewer::Viewer viewer;
    //viewer.setCamera( cam );
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.setSceneData(root_node);
    viewer.realize();
    //ImageFormat imf = ip->getFormat();
    OrthoTextureManipulator* om = new OrthoTextureManipulator(iStab->getOutputTexture()->getTextureWidth(), iStab->getOutputTexture()->getTextureHeight());
    viewer.setCameraManipulator(om);


    unsigned long numFramesDone=0;
    osg::Vec2f oldJitterTranslation=translate->getTranslation();
    osg::Vec2f currentJitterTranslation=translate->getTranslation();
    osg::Vec2f deltaJitterTranslation(0.0f, 0.0f);


    osg::Vec2f deltaErrorSum(0.0f, 0.0f);
    float squaredDeltaErrorSum=0.0f;
    uint32_t numDeltaErrors=0;


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

            quad->setTexture(hp->getOutputTexture(1));

            viewer.frame();


            pov->clearVertices();
            for (uint32_t i=0; i<iStab->getNumPyramids(); i++)
            {
                osg::Vec2f rc=iStab->getTransformedROI_Centre(i);
                pov->addVertex(osg::Vec3d(rc._v[0], iStab->getOutputTexture()->getTextureHeight() - rc._v[1] - 1, 0.1));
            }


            osg::Matrixd iStabDeltaTransform=iStab->getDeltaTransformationMatrix();
            osg::Vec2f iStabDeltaTranslation(-iStabDeltaTransform(3,0), -iStabDeltaTransform(3,2));

            oldJitterTranslation=currentJitterTranslation;
            currentJitterTranslation=translate->getTranslation()/dsFactor;
            deltaJitterTranslation=currentJitterTranslation - oldJitterTranslation;


            if (numFramesDone>2)
            {
                osg::Vec2f deltaTranslationError=deltaJitterTranslation - iStabDeltaTranslation;

                deltaErrorSum+=deltaTranslationError;

                squaredDeltaErrorSum+=deltaTranslationError * deltaTranslationError;
                numDeltaErrors++;

                std::cout << "ES: " << deltaErrorSum/numDeltaErrors << " RMSE: " << sqrtf(squaredDeltaErrorSum/numDeltaErrors) << " (" << deltaTranslationError << " , " << deltaJitterTranslation << " | " << iStabDeltaTranslation << ")\n";
                std::cout.flush();
            }


            numFramesDone++;
            if (jitter)
            {
                translate->setTranslation(osg::Vec2f((rand()/((float)RAND_MAX)-0.5f)*2.0f*4.0f*dsFactor,
                                                     (rand()/((float)RAND_MAX)-0.5f)*2.0f*4.0f*dsFactor));
            }
        }
    }


    return 0;
}


