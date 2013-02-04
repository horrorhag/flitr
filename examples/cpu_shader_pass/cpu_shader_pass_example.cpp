#include <iostream>
#include <string>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/TrackballManipulator>
#include <osg/io_utils>

#include <flitr/ffmpeg_producer.h>
#include <flitr/multi_osg_consumer.h>
#include <flitr/multi_cpuhistogram_consumer.h>
#include <flitr/textured_quad.h>
#include <flitr/manipulator_utils.h>

#include <flitr/modules/cpu_shader_passes/cpu_shader_pass.h>
#include <flitr/modules/cpu_shader_passes/cpu_palette_remap_shader.h>
#include <flitr/modules/cpu_shader_passes/cpu_photometric_equalisation.h>

#include <flitr/modules/glsl_shader_passes/glsl_shader_pass.h>

using std::tr1::shared_ptr;
using namespace flitr;


class CPUSUM_Shader : public CPUShaderPass::CPUShader
{
public:
    CPUSUM_Shader(osg::Image* image) :
        Image_(image)
    {
    }

    virtual void operator () (osg::RenderInfo& renderInfo) const
    {//Reduce the image.
        const unsigned long width=Image_->s();
        const unsigned long height=Image_->t();
        const unsigned long numPixels=width*height;
        const unsigned long numComponents=osg::Image::computeNumComponents(Image_->getPixelFormat());

        unsigned char * const data=(unsigned char *)Image_->data();

        unsigned long pixelSum[numComponents];
        for (unsigned long i=0; i<numComponents; i++)
        {
            pixelSum[i]=0;
        }

        for (unsigned long i=0; i<(numPixels*numComponents); i++)
        {
            pixelSum[i%numComponents]+=data[i];
        }

        std::cout << "Summed image components: ";
        for (unsigned long i=0; i<numComponents; i++)
        {
            std::cout << pixelSum[i] << " ";
        }
        std::cout << "\n";
        std::cout.flush();

        Image_->dirty();
    }

    osg::Image* Image_;
};

class CPUGAUSFILT_Shader : public CPUShaderPass::CPUShader
{
public:
    CPUGAUSFILT_Shader(osg::Image* image) :
        Image_(image)
    {
    }

    virtual void operator () (osg::RenderInfo& renderInfo) const
    {//Filter the image; use a symmetric seperable filter kernel.
        const unsigned long width=Image_->s();
        const unsigned long height=Image_->t();
        const unsigned long numPixels=width*height;
        const unsigned long numComponents=osg::Image::computeNumComponents(Image_->getPixelFormat());

        unsigned char * const data=(unsigned char *)Image_->data();

        const unsigned long filtKernelDim=5;
        float filtKernel[filtKernelDim]={0.24f, 0.20f, 0.10f, 0.05f, 0.03f};
        //	    float filtKernel[filtKernelDim]={0.1f, 0.1f, 0.1f, 0.1f, 0.1f};

        const unsigned long indexTimesWidthTimesNumComponents[filtKernelDim]={0l*width*numComponents, 1l*width*numComponents, 2l*width*numComponents, 3l*width*numComponents, 4l*width*numComponents};
        const unsigned long indexTimesNumComponents[filtKernelDim]={0l*numComponents, 1l*numComponents, 2l*numComponents, 3l*numComponents, 4l*numComponents};

        unsigned char *filtFiltX=new unsigned char[numPixels*numComponents];

        {//Apply filter to image rows.
            for ( unsigned long y=0l; y<height; y++)
            {
                unsigned long offset=(filtKernelDim-1+y*width)*numComponents;

                for ( unsigned long x=(filtKernelDim-1)*numComponents; x<=((width-filtKernelDim+1)*numComponents-1); x++)
                {
                    filtFiltX[offset]=(data[offset+indexTimesNumComponents[0]]*filtKernel[0l]+
                                       (data[offset+indexTimesNumComponents[1]]+data[offset-indexTimesNumComponents[1]])*filtKernel[1l]+
                                       (data[offset+indexTimesNumComponents[2]]+data[offset-indexTimesNumComponents[2]])*filtKernel[2l]+
                                       (data[offset+indexTimesNumComponents[3]]+data[offset-indexTimesNumComponents[3]])*filtKernel[3l]+
                                       (data[offset+indexTimesNumComponents[4]]+data[offset-indexTimesNumComponents[4]])*filtKernel[4l]
                                       )+0.5;

                    offset++;
                }
            }
        }


        {//Apply filter to image columns.
            unsigned long offset=width*(filtKernelDim-1)*numComponents;
            for ( unsigned long y=filtKernelDim-1; y<=(height-filtKernelDim); y++)
                for ( unsigned long x=0l; x<width*numComponents; x++)
                {
                    data[offset]=(filtFiltX[offset]*filtKernel[0l]+
                                  (filtFiltX[offset+indexTimesWidthTimesNumComponents[1l]]+filtFiltX[offset-indexTimesWidthTimesNumComponents[1l]])*filtKernel[1l]+
                                  (filtFiltX[offset+indexTimesWidthTimesNumComponents[2l]]+filtFiltX[offset-indexTimesWidthTimesNumComponents[2l]])*filtKernel[2l]+
                                  (filtFiltX[offset+indexTimesWidthTimesNumComponents[3l]]+filtFiltX[offset-indexTimesWidthTimesNumComponents[3l]])*filtKernel[3l]+
                                  (filtFiltX[offset+indexTimesWidthTimesNumComponents[4l]]+filtFiltX[offset-indexTimesWidthTimesNumComponents[4l]])*filtKernel[4l]
                                  )+0.5;

                    offset++;
                }
        }

        delete [] filtFiltX;

        Image_->dirty();
    }

    osg::Image* Image_;
};



int main(int argc, char *argv[])
{
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " video_file glsl_shader_file (Note: cpu pre- and post-shaders in C++ main.cpp).\n";
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

    MultiCPUHistogramConsumer mhc(*ffp, 1);
    if (!mhc.init()) {
        std::cerr << "Could not init histogram consumer.\n";
        exit(-1);
    }


    //===
    shared_ptr<flitr::CPUShaderPass> gfp(new flitr::CPUShaderPass(osgc->getOutputTexture()));
    //OR
    //shared_ptr<SimpleCPUShaderPass> gfp(new SimpleCPUShader(osgc->getOSGImage(), true));
    //===

    gfp->setGPUShader(argv[2]);
        //gfp->setPostRenderCPUShader(new CPUGAUSFILT_Shader(gfp->getOutImage()));
        gfp->setPostRenderCPUShader(new CPUPhotometricEqualisation_Shader(gfp->getOutImage(), 0.5, 0.025));

        osg::ref_ptr<flitr::CPUPaletteRemap_Shader> prmCPUShader;
        prmCPUShader=osg::ref_ptr<flitr::CPUPaletteRemap_Shader>(new flitr::CPUPaletteRemap_Shader(gfp->getOutImage()));
        //gfp->setPostRenderCPUShader(prmCPUShader);
    root_node->addChild(gfp->getRoot().get());


    //shared_ptr<TexturedQuad> quad(new TexturedQuad(osgc->getOutputTexture()));
    shared_ptr<TexturedQuad> quad(new TexturedQuad(gfp->getOutputTexture()));
    //shared_ptr<TexturedQuad> quad(new TexturedQuad(gfp->getOutImage()));

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

        if (mhc.isHistogramUpdated(0))
        {
            std::vector<int32_t> histogram=mhc.getHistogram(0);

            std::vector<uint8_t> histogramIdentityMap=MultiCPUHistogramConsumer::calcHistogramIdentityMap();//NoColourAdj.

            std::vector<uint8_t> histogramStretchMap=MultiCPUHistogramConsumer::calcHistogramStretchMap(histogram, mhc.getNumPixels(0), 0.1, 0.9);

            std::vector<uint8_t> histogramEqualiseMap=MultiCPUHistogramConsumer::calcHistogramMatchMap(histogram, MultiCPUHistogramConsumer::calcRefHistogramForEqualisation(mhc.getNumPixels(0)));

            prmCPUShader->setPaletteMap(histogramStretchMap);
        }

        if (osgc->getNext()) {
            viewer.frame();
        }

    }

    return 0;
}
