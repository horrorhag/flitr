#ifndef SIMPLE_SHADER_PASS_H
#define SIMPLE_SHADER_PASS_H 1

#include <osg/ref_ptr>
#include <osg/Group>
#include <osg/Camera>
#include <osg/MatrixTransform>
#include <osg/Projection>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Texture2D>
#include <osg/TextureRectangle>
#include <osgCuda/Texture>

class SimpleShaderPass {
  public:
    SimpleShaderPass(osgCuda::TextureRectangle *in_tex);
    ~SimpleShaderPass();
    osg::ref_ptr<osg::Group> getRoot() { return RootGroup_; }
    osg::ref_ptr<osgCuda::TextureRectangle> getOutputTexture() { return OutTexture_; }
    void setShader(std::string filename);
	
  private:
    osg::ref_ptr<osg::Group> createTexturedQuad();
    void createOutputTexture();
    void setupCamera();

    osg::ref_ptr<osg::Group> RootGroup_;
    osg::ref_ptr<osg::Camera> Camera_;
    osg::ref_ptr<osgCuda::TextureRectangle> InTexture_;
	osg::ref_ptr<osgCuda::TextureRectangle> OutTexture_;

    int TextureWidth_;
    int TextureHeight_;

	osg::ref_ptr<osg::Program> FragmentProgram_;
    osg::ref_ptr<osg::StateSet> StateSet_;
};

#endif //SIMPLE_SHADER_PASS_H
