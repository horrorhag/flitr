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

class SimpleShaderPass {
  public:
    SimpleShaderPass(osg::TextureRectangle *in_tex);
    ~SimpleShaderPass();
    osg::ref_ptr<osg::Group> getRoot() { return RootGroup_; }
    osg::ref_ptr<osg::TextureRectangle> getOutputTexture() { return OutTexture_; }
    void setShader(std::string filename);
	
  private:
    osg::ref_ptr<osg::Group> createTexturedQuad();
    void createOutputTexture();
    void setupCamera();

    osg::ref_ptr<osg::Group> RootGroup_;
    osg::ref_ptr<osg::Camera> Camera_;
    osg::ref_ptr<osg::TextureRectangle> InTexture_;
	osg::ref_ptr<osg::TextureRectangle> OutTexture_;

    int TextureWidth_;
    int TextureHeight_;

	osg::ref_ptr<osg::Program> FragmentProgram_;
    osg::ref_ptr<osg::StateSet> StateSet_;
};

#endif //SIMPLE_SHADER_PASS_H
