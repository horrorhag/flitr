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
#include <flitr/modules/cpu_shader_passes/cpu_local_photometric_equalisation.h>

#include <flitr/modules/glsl_shader_passes/glsl_shader_pass.h>

using std::shared_ptr;
using namespace flitr;


//=== Example CPU shader: Average Image ===//
class CPUAVRGIMG_Shader : public CPUShaderPass::CPUShader
{
public:
    CPUAVRGIMG_Shader(osg::Image* image, const uint8_t base2NumImages) :
        Image_(image),
        base2NumImages_(base2NumImages),
        numImagesInWindow_(powf(2.0f, base2NumImages_)+0.5f),
        oldestImageInWindow_(0)
    {
        const size_t width=Image_->s();
        const size_t height=Image_->t();
        const size_t numPixels=width*height;
        const size_t componentsPerPixel=osg::Image::computeNumComponents(Image_->getPixelFormat());
        const size_t bytesPerPixel=componentsPerPixel;
        const size_t numValues=numPixels * componentsPerPixel;

        sumData_=new uint32_t[numValues];
        memset(sumData_, 0, numValues * sizeof(uint32_t));
        
        for (size_t imageInWindowNum=0; imageInWindowNum<numImagesInWindow_; imageInWindowNum++)
        {
            imageDataVec_.push_back(new unsigned char[numPixels * bytesPerPixel]);
        }
    }

    virtual ~CPUAVRGIMG_Shader()
    {
        for (size_t imageInWindowNum=0; imageInWindowNum<numImagesInWindow_; imageInWindowNum++)
        {
            delete [] imageDataVec_[imageInWindowNum];
        }

        imageDataVec_.clear();
        
        delete [] sumData_;
    }

    virtual void operator () (osg::RenderInfo& renderInfo) const
    {//Reduce the image.
        const size_t width=Image_->s();
        const size_t height=Image_->t();
        const size_t numPixels=width*height;
        const size_t componentsPerPixel=osg::Image::computeNumComponents(Image_->getPixelFormat());
        const size_t numValues=numPixels * componentsPerPixel;

        unsigned char * const data=(unsigned char *)Image_->data();

        //Assume data type uint8...
        for (size_t i=0; i<numValues; i++)
        {
            sumData_[i]+=data[i];
            sumData_[i]-=imageDataVec_[oldestImageInWindow_][i];
            imageDataVec_[oldestImageInWindow_][i]=data[i];
            data[i]=sumData_[i]>>base2NumImages_;
        }

        oldestImageInWindow_=(oldestImageInWindow_+1)%numImagesInWindow_;
        
        Image_->dirty();
    }

    osg::Image* Image_;

    const uint8_t base2NumImages_;
    const size_t numImagesInWindow_;

    mutable size_t oldestImageInWindow_;
    std::vector<unsigned char *> imageDataVec_;
    
    uint32_t *sumData_;
};
//=== ===


//=== Example CPU shader: Image SUM ===//
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
//=== ===

//=== Example CPU shader: Gausian Filter ===
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
//=== ===


int main(int argc, char *argv[])
{
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " video_file glsl_shader_file (Note: cpu pre- and post-shaders set in code in C++ main.cpp).\n";
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

/*
    MultiCPUHistogramConsumer mhc(*ffp, 1);
    if (!mhc.init()) {
        std::cerr << "Could not init histogram consumer.\n";
        exit(-1);
    }
*/

    //===
    shared_ptr<flitr::CPUShaderPass> cpuShaderPass(new flitr::CPUShaderPass(osgc->getOutputTexture()));
    cpuShaderPass->setGPUShader(argv[2]);
    //===

    //osg::ref_ptr<flitr::CPUPaletteRemap_Shader> prmCPUShader;
    //prmCPUShader=osg::ref_ptr<flitr::CPUPaletteRemap_Shader>(new flitr::CPUPaletteRemap_Shader(cpuShaderPass->getOutImage()));
    //cpuShaderPass->setPostRenderCPUShader(prmCPUShader);

    cpuShaderPass->setPostRenderCPUShader(new CPUAVRGIMG_Shader(cpuShaderPass->getOutImage(), 4));

    root_node->addChild(cpuShaderPass->getRoot().get());


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
    osgGA::TrackballManipulator* tb = new osgGA::TrackballManipulator;
    viewer.setCameraManipulator(tb);
    adjustCameraManipulatorHomeForYUp(tb);
    
    while(!viewer.done()) {

        ffp->trigger();
        /*
        if (mhc.isHistogramUpdated(0))
        {
            std::vector<int32_t> histogram=mhc.getHistogram(0);

            std::vector<uint8_t> histogramIdentityMap=MultiCPUHistogramConsumer::calcHistogramIdentityMap();//NoColourAdj.

            std::vector<uint8_t> histogramStretchMap=MultiCPUHistogramConsumer::calcHistogramStretchMap(histogram, mhc.getNumPixels(0), 0.1, 0.9);

            std::vector<uint8_t> histogramEqualiseMap=MultiCPUHistogramConsumer::calcHistogramMatchMap(histogram, MultiCPUHistogramConsumer::calcRefHistogramForEqualisation(mhc.getNumPixels(0)));

            prmCPUShader->setPaletteMap(histogramEqualiseMap);
        }
*/
        if (osgc->getNext()) {
            viewer.frame();
        }

    }

    return 0;
}
