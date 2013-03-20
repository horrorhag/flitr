#define _USE_MATH_DEFINES
#include <cmath>
#include <flitr/modules/glsl_shader_passes/glsl_gaussian_filter_y_pass.h>

using namespace flitr;

GLSLGaussianFilterYPass::GLSLGaussianFilterYPass(osg::TextureRectangle *in_tex, bool read_back_to_CPU)
{
    TextureWidth_ = in_tex->getTextureWidth();
    TextureHeight_ = in_tex->getTextureHeight();

    RootGroup_ = new osg::Group;
    InTexture_ = in_tex;
    OutTexture_=0;
    OutImage_=0;

    Kernel1DImage_=new osg::Image();
    Kernel1DImage_->allocateImage(1, TextureHeight_, 1,
                                  GL_RED, GL_FLOAT);
    Kernel1DImage_->setInternalTextureFormat(GL_FLOAT_R_NV);
    Kernel1DImage_->setPixelFormat(GL_RED);
    Kernel1DImage_->setDataType(GL_FLOAT);

    Kernel1DTexture_=new osg::TextureRectangle();
    Kernel1DTexture_->setTextureSize(1, TextureHeight_);
    Kernel1DTexture_->setFilter(osg::TextureRectangle::MIN_FILTER,osg::TextureRectangle::NEAREST);
    Kernel1DTexture_->setFilter(osg::TextureRectangle::MAG_FILTER,osg::TextureRectangle::NEAREST);
    Kernel1DTexture_->setInternalFormat(GL_FLOAT_R_NV);
    Kernel1DTexture_->setSourceFormat(GL_RED);
    Kernel1DTexture_->setSourceType(GL_FLOAT);
    Kernel1DTexture_->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    Kernel1DTexture_->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
    Kernel1DTexture_->setImage(Kernel1DImage_.get());

    KernelSizeUniform_=osg::ref_ptr<osg::Uniform>(new osg::Uniform("kernelSize", (int)1));
    setStandardDeviation(2.0);//Make sure Kernel1DTexture is setup.

    createOutputTexture(read_back_to_CPU);

    Camera_ = new osg::Camera;

    setupCamera();

    Camera_->addChild(createTexturedQuad().get());

    RootGroup_->addChild(Camera_.get());

    setShader();
}

GLSLGaussianFilterYPass::~GLSLGaussianFilterYPass()
{
}

osg::ref_ptr<osg::Group> GLSLGaussianFilterYPass::createTexturedQuad()
{
    osg::ref_ptr<osg::Group> top_group = new osg::Group;

    osg::ref_ptr<osg::Geode> quad_geode = new osg::Geode;

    osg::ref_ptr<osg::Vec3Array> quad_coords = new osg::Vec3Array; // vertex coords
    // counter-clockwise
    quad_coords->push_back(osg::Vec3d(0, 0, -1));
    quad_coords->push_back(osg::Vec3d(1, 0, -1));
    quad_coords->push_back(osg::Vec3d(1, 1, -1));
    quad_coords->push_back(osg::Vec3d(0, 1, -1));

    osg::ref_ptr<osg::Vec2Array> quad_tcoords = new osg::Vec2Array; // texture coords
    quad_tcoords->push_back(osg::Vec2(0, 0));
    quad_tcoords->push_back(osg::Vec2(TextureWidth_, 0));
    quad_tcoords->push_back(osg::Vec2(TextureWidth_, TextureHeight_));
    quad_tcoords->push_back(osg::Vec2(0, TextureHeight_));

    osg::ref_ptr<osg::Geometry> quad_geom = new osg::Geometry;
    osg::ref_ptr<osg::DrawArrays> quad_da = new osg::DrawArrays(osg::PrimitiveSet::QUADS,0,4);

    osg::ref_ptr<osg::Vec4Array> quad_colors = new osg::Vec4Array;
    quad_colors->push_back(osg::Vec4(1.0f,1.0f,1.0f,1.0f));

    quad_geom->setVertexArray(quad_coords.get());
    quad_geom->setTexCoordArray(0, quad_tcoords.get());
    quad_geom->addPrimitiveSet(quad_da.get());
    quad_geom->setColorArray(quad_colors.get());
    quad_geom->setColorBinding(osg::Geometry::BIND_OVERALL);

    StateSet_ = quad_geom->getOrCreateStateSet();
    StateSet_->setMode(GL_LIGHTING,osg::StateAttribute::OFF);
    StateSet_->setMode(GL_DEPTH_TEST,osg::StateAttribute::OFF);

    StateSet_->setTextureAttributeAndModes(0, InTexture_.get(), osg::StateAttribute::ON);
    StateSet_->addUniform(new osg::Uniform("textureID0", 0));

    StateSet_->setTextureAttributeAndModes(1, Kernel1DTexture_.get(), osg::StateAttribute::ON);
    StateSet_->addUniform(new osg::Uniform("kernel1DTexture", 1));

    StateSet_->addUniform(KernelSizeUniform_);

    quad_geode->addDrawable(quad_geom.get());

    top_group->addChild(quad_geode.get());

    return top_group;
}

void GLSLGaussianFilterYPass::setupCamera()
{
    // clearing
    bool need_clear = false;
    if (need_clear) {
        Camera_->setClearColor(osg::Vec4(0.1f,0.1f,0.3f,1.0f));
        Camera_->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    } else {
        Camera_->setClearMask(0);
        Camera_->setImplicitBufferAttachmentMask(0, 0);
    }

    // projection and view
    Camera_->setProjectionMatrixAsOrtho(0,1,0,1,-100,100);
    Camera_->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
    Camera_->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    Camera_->setViewMatrix(osg::Matrix::identity());

    // viewport
    Camera_->setViewport(0, 0, TextureWidth_, TextureHeight_);

    Camera_->setRenderOrder(osg::Camera::PRE_RENDER);
    Camera_->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);

    // attach the texture and use it as the color buffer.
    Camera_->attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER), OutTexture_.get());

    if (OutImage_!=0)
    {
        Camera_->attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER), OutImage_.get());
    }
}

void GLSLGaussianFilterYPass::createOutputTexture(bool read_back_to_CPU)
{
    OutTexture_ = new osg::TextureRectangle;

    OutTexture_->setTextureSize(TextureWidth_, TextureHeight_);
    OutTexture_->setInternalFormat(InTexture_->getInternalFormat());
    OutTexture_->setSourceFormat(InTexture_->getSourceFormat());
    OutTexture_->setSourceType(InTexture_->getSourceType());
    OutTexture_->setFilter(osg::Texture2D::MIN_FILTER,osg::Texture2D::LINEAR);
    OutTexture_->setFilter(osg::Texture2D::MAG_FILTER,osg::Texture2D::LINEAR);

    if (read_back_to_CPU)
    {
        OutImage_ = new osg::Image;
        OutImage_->allocateImage(TextureWidth_, TextureHeight_, 1,
                                 InTexture_->getInternalFormat(), InTexture_->getInternalFormatType());
    }
}

void GLSLGaussianFilterYPass::setShader()
{
    osg::ref_ptr<osg::Shader> fshader = new osg::Shader( osg::Shader::FRAGMENT,
                                                         "uniform sampler2DRect textureID0;\n"
                                                         "uniform sampler2DRect kernel1DTexture;\n"

                                                         "uniform int kernelSize;\n"

                                                         "void main(void)\n"
                                                         "{\n"
                                                         "    vec2 texCoord = gl_TexCoord[0].xy;\n"

                                                         "    float kernelValue=texture2DRect(kernel1DTexture, vec2(0.5, 0.5)).r;\n"
                                                         "    float kernelValueSum=kernelValue;\n"

                                                         "    vec4 filteredPixel=texture2DRect(textureID0, texCoord)*kernelValue;\n"

                                                         "    for (int r=1; r<kernelSize; r++)\n"
                                                         "    {\n"
                                                         "      kernelValue=texture2DRect(kernel1DTexture, vec2(0.5, r+0.5));\n"
                                                         "      kernelValueSum+=kernelValue*2.0;\n"

                                                         "      filteredPixel+=(texture2DRect(textureID0, texCoord-vec2(0,r))+texture2DRect(textureID0, texCoord+vec2(0,r)))*kernelValue;\n"
                                                         "    }\n"

                                                         "    gl_FragColor = filteredPixel/kernelValueSum;\n"
                                                         "}\n"
                                                         );

    FragmentProgram_ = 0;
    FragmentProgram_ = new osg::Program;

    FragmentProgram_->addShader(fshader.get());

    StateSet_->setAttributeAndModes(FragmentProgram_.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE );
}

void GLSLGaussianFilterYPass::setStandardDeviation(float sd)
{
    StandardDeviation_=sd;
    int kernelSize=ceil(StandardDeviation_*3.0f)+0.5;
    if (kernelSize>TextureWidth_) kernelSize=TextureWidth_;

    KernelSizeUniform_->set(kernelSize);

    float * const data=(float * const)Kernel1DImage_->data();

    data[0]=1.0;

    for (int r=0; r<kernelSize; r++)
    {
        data[r]=(1.0/sqrt(2.0*M_PI*StandardDeviation_*StandardDeviation_))*exp(-r*r/(2.0*StandardDeviation_*StandardDeviation_));
    }

    Kernel1DImage_->dirty();
}

