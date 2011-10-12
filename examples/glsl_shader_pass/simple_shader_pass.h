#ifndef SIMPLE_SHADER_PASS_H
#define SIMPLE_SHADER_PASS_H 1
#include <iostream>
#include <string>

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

class SimpleShaderPass {
  public:
    SimpleShaderPass(osg::TextureRectangle *in_tex, bool read_back_to_CPU = false);
    ~SimpleShaderPass();
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
    osg::ref_ptr<osg::TextureRectangle> InTexture_;
	osg::ref_ptr<osg::TextureRectangle> OutTexture_;
    osg::ref_ptr<osg::Image> OutImage_;

    int TextureWidth_;
    int TextureHeight_;

	osg::ref_ptr<osg::Program> FragmentProgram_;
    osg::ref_ptr<osg::StateSet> StateSet_;
};

#endif //SIMPLE_SHADER_PASS_H
