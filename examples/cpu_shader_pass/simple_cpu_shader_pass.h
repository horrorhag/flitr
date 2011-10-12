#ifndef SIMPLE_CPU_SHADER_PASS_H
#define SIMPLE_CPU_SHADER_PASS 1

#include <osg/ref_ptr>
#include <osg/Group>
#include <osg/Camera>
#include <osg/MatrixTransform>
#include <osg/Projection>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Texture2D>
#include <osg/TextureRectangle>
#include <osg/Image>
#include <iostream>


class SimpleCPUShaderPass {
    class CPUPostDrawCallback : public osg::Camera::DrawCallback
    {
        public:
        CPUPostDrawCallback(osg::Image* image) :
            Image_(image)
        {
        }
        
        virtual void operator () (const osg::Camera& /*camera*/) const
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
        }
    
        osg::Image* Image_;
    };

    class CPUPreDrawCallback : public osg::Camera::DrawCallback
    {
        public:
        CPUPreDrawCallback(osg::Image* image) :
            Image_(image)
        {
        }
        
        virtual void operator () (const osg::Camera& /*camera*/) const
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
        }
    
        osg::Image* Image_;
    };

  public:
    SimpleCPUShaderPass(osg::Image *in_img, bool read_back_to_CPU = false);
    ~SimpleCPUShaderPass();

    osg::ref_ptr<osg::Group> getRoot() { return RootGroup_; }

    osg::ref_ptr<osg::TextureRectangle> getOutputTexture() { return OutTexture_; }

    osg::ref_ptr<osg::Image> getOSGImage() { return OutImage_; }

    void setShader(std::string filename);
	
  private:
    osg::ref_ptr<osg::Group> createTexturedQuad();
    void createOutputTexture(bool read_back_to_CPU);
    void setupCamera();

    osg::ref_ptr<osg::Group> RootGroup_;
    osg::ref_ptr<osg::Camera> Camera_;
    osg::ref_ptr<osg::Image> InImage_;
    osg::ref_ptr<osg::TextureRectangle> InTexture_;

    osg::ref_ptr<osg::TextureRectangle> OutTexture_;
    osg::ref_ptr<osg::Image> OutImage_;

    unsigned long TextureWidth_;
    unsigned long TextureHeight_;

    osg::ref_ptr<osg::Program> FragmentProgram_;
    osg::ref_ptr<osg::StateSet> StateSet_;
};

#endif //SIMPLE_CPU_SHADER_PASS_H
