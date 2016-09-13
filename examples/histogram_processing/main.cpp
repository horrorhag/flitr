#include <iostream>
#include <string>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osg/io_utils>

#include <flitr/ffmpeg_producer.h>
#include <flitr/multi_osg_consumer.h>
#include <flitr/multi_cpuhistogram_consumer.h>
#include <flitr/textured_quad.h>
#include <flitr/manipulator_utils.h>
#include <flitr/modules/cpu_shader_passes/cpu_palette_remap_shader.h>
#include <flitr/ortho_texture_manipulator.h>


using std::shared_ptr;
using namespace flitr;


int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cout << "Usage: " << argv[0] << " video_file.\n";
        return 1;
    }

    shared_ptr<FFmpegProducer> ffp(new FFmpegProducer(argv[1], ImageFormat::FLITR_PIX_FMT_Y_8));
    if (!ffp->init())
    {
        std::cerr << "Could not load " << argv[1] << "\n";
        exit(-1);
    }

    shared_ptr<MultiOSGConsumer> osgc(new MultiOSGConsumer(*ffp,1));
    if (!osgc->init())
    {
        std::cerr << "Could not init OSG consumer.\n";
        exit(-1);
    }

    osg::Group *root_node = new osg::Group;

    
    
    //=== Histogram palette remap things ===//
    MultiCPUHistogramConsumer mhc(*ffp, 1,
                                  3,  //pixel stride
                                  5); //image stride
    if (!mhc.init())
    {
        std::cerr << "Could not init histogram consumer.\n";
        exit(-1);
    }
    
    shared_ptr<flitr::CPUShaderPass> cpuShaderPass(new flitr::CPUShaderPass(osgc->getOutputTexture()));
    cpuShaderPass->setGPUShader("copy.frag");

    osg::ref_ptr<flitr::CPUPaletteRemap_Shader> prmCPUShader;
    prmCPUShader=osg::ref_ptr<flitr::CPUPaletteRemap_Shader>(new flitr::CPUPaletteRemap_Shader(cpuShaderPass->getOutImage()));
    cpuShaderPass->setPostRenderCPUShader(prmCPUShader);

    root_node->addChild(cpuShaderPass->getRoot().get());
    //=== ===//

    //shared_ptr<TexturedQuad> quad(new TexturedQuad(osgc->getOutputTexture()));
    shared_ptr<TexturedQuad> quad(new TexturedQuad(cpuShaderPass->getOutputTexture()));
    //shared_ptr<TexturedQuad> quad(new TexturedQuad(cpuShaderPass->getOutImage()));
    root_node->addChild(quad->getRoot().get());
    

    osgViewer::Viewer viewer;
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.setSceneData(root_node);
    viewer.setUpViewInWindow(480+40, 40, 640, 480);
    viewer.realize();
    OrthoTextureManipulator* om = new OrthoTextureManipulator(osgc->getOutputTexture()->getTextureWidth(), osgc->getOutputTexture()->getTextureHeight());
    viewer.setCameraManipulator(om);
    
    
    while(!viewer.done())
    {
        ffp->trigger();

        if (mhc.isHistogramUpdated(0))
        {
            std::vector<int32_t> histogram=mhc.getHistogram(0);

            std::vector<uint8_t> histogramIdentityMap=MultiCPUHistogramConsumer::calcHistogramIdentityMap();//NoColourAdj.

            std::vector<uint8_t> histogramStretchMap=MultiCPUHistogramConsumer::calcHistogramStretchMap(histogram, mhc.getNumPixels(0), 0.05, 0.95);

            std::vector<uint8_t> histogramEqualiseMap=MultiCPUHistogramConsumer::calcHistogramMatchMap(histogram, MultiCPUHistogramConsumer::calcRefHistogramForEqualisation(mhc.getNumPixels(0)));

            
            //prmCPUShader->setPaletteMap(histogramIdentityMap);
            //prmCPUShader->setPaletteMap(histogramStretchMap);
            prmCPUShader->setPaletteMap(histogramEqualiseMap);
        }

        if (osgc->getNext())
        {
            viewer.frame();
        }
    }

    return 0;
}
