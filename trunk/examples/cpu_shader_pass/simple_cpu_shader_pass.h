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
public:

    class CPUShaderPass : public osg::Camera::DrawCallback
    {
    };

    class CPUExample_ShaderPass : public CPUShaderPass
    {
    public:
        CPUExample_ShaderPass(osg::Image* image) :
            Image_(image)
        {
        }
        
        virtual void operator () (osg::RenderInfo& renderInfo) const
        {
            const unsigned long width=Image_->s();
            const unsigned long height=Image_->t();
            const unsigned long numPixels=width*height;
            const unsigned long numComponents=osg::Image::computeNumComponents(Image_->getPixelFormat());

            unsigned char * const data=(unsigned char *)Image_->data();

            for (unsigned long i=0; i<(numPixels*numComponents); i++)
            {
                data[i]=(unsigned char)(pow(data[i]/(float)255.0f, 2.0f)*255.0+0.5);
            }

            std::cout << "*";
            std::cout.flush();
            Image_->dirty();
        }

        osg::Image* Image_;
    };


public:
    SimpleCPUShaderPass(osg::ref_ptr<osg::Image> in_img, bool read_back_to_CPU = false);
    SimpleCPUShaderPass(osg::ref_ptr<osg::TextureRectangle> in_tex, bool read_back_to_CPU = false);
    ~SimpleCPUShaderPass();

    osg::ref_ptr<osg::Group> getRoot() { return RootGroup_; }

    osg::ref_ptr<osg::TextureRectangle> getOutputTexture() { return OutTexture_; }

    osg::ref_ptr<osg::Image> getOSGImage() { return OutImage_; }

    osg::ref_ptr<osg::Image> getInImage() { return InImage_; }
    osg::ref_ptr<osg::Image> getOutImage() { return OutImage_; }

    void setPreDrawCPUShader(CPUShaderPass *cpuShaderPass);

    void setPostDrawCPUShader(CPUShaderPass *cpuShaderPass);

    void setGPUShader(std::string filename);

private:
    osg::ref_ptr<osg::Group> createTexturedQuad();
    void setupCamera();

    osg::ref_ptr<osg::Group> RootGroup_;
    osg::ref_ptr<osg::Camera> Camera_;

    osg::ref_ptr<osg::Image> InImage_;
    osg::ref_ptr<osg::TextureRectangle> InTexture_;

    osg::ref_ptr<osg::Image> OutImage_;
    osg::ref_ptr<osg::TextureRectangle> OutTexture_;

    unsigned long TextureWidth_;
    unsigned long TextureHeight_;

    osg::ref_ptr<osg::Program> FragmentProgram_;
    osg::ref_ptr<osg::StateSet> StateSet_;
};

#endif //SIMPLE_CPU_SHADER_PASS_H
