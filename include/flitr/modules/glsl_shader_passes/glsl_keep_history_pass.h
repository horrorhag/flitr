#ifndef GLSL_KEEP_HISTORY_PASS_H
#define GLSL_KEEP_HISTORY_PASS_H 1

#include <osg/ref_ptr>
#include <osg/Group>
#include <osg/Camera>
#include <osg/MatrixTransform>
#include <osg/Projection>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Texture2D>
#include <osg/TextureRectangle>

class GLSLKeepHistoryPass {
  public:
    GLSLKeepHistoryPass(osg::TextureRectangle* in_tex, int hist_size);
    ~GLSLKeepHistoryPass();
    osg::ref_ptr<osg::Group> getRoot() { return RootGroup_; }
    osg::ref_ptr<osg::TextureRectangle> getOutputTexture(int age=0);
    void setShader(std::string filename);
	
  private:
    osg::ref_ptr<osg::Group> createTexturedQuad();
    void createOutputTextures();
    void setupCamera();
    void frameDone();

    osg::ref_ptr<osg::Group> RootGroup_;
    osg::ref_ptr<osg::Camera> Camera_;
    osg::ref_ptr<osg::TextureRectangle> InTexture_;
    std::vector< osg::ref_ptr<osg::TextureRectangle> > OutTextures_;

    int TextureWidth_;
    int TextureHeight_;

	osg::ref_ptr<osg::Program> FragmentProgram_;
    osg::ref_ptr<osg::StateSet> StateSet_;

    int HistorySize_;
    int OverwriteIndex_;
    int ValidHistory_;
    bool FirstPass_;

    struct CameraCullCallback : public osg::NodeCallback 
    {
        CameraCullCallback(GLSLKeepHistoryPass* hp) : hp_(hp) {}
        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);
      protected:
        GLSLKeepHistoryPass* hp_;
    };
    
    struct CameraPostDrawCallback : public osg::Camera::DrawCallback
    {
        CameraPostDrawCallback(GLSLKeepHistoryPass* hp) : hp_(hp) {}
        virtual void operator()(osg::RenderInfo& ri) const;
      protected:
        GLSLKeepHistoryPass* hp_;
    };
};

#endif //GLSL_KEEP_HISTORY_PASS_H
