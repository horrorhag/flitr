#include "simple_shader_pass.h"

SimpleShaderPass::SimpleShaderPass(osgCuda::TextureRectangle *in_tex)
{
    TextureWidth_ = in_tex->getTextureWidth();
    TextureHeight_ = in_tex->getTextureHeight();

    RootGroup_ = new osg::Group;
    InTexture_ = in_tex;
   
    createOutputTexture();

    Camera_ = new osg::Camera;
    
    setupCamera();

    Camera_->addChild(createTexturedQuad().get());

    RootGroup_->addChild(Camera_.get());

    //setShader("");
}

SimpleShaderPass::~SimpleShaderPass()
{
}

osg::ref_ptr<osg::Group> SimpleShaderPass::createTexturedQuad()
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

    quad_geode->addDrawable(quad_geom.get());
    
    top_group->addChild(quad_geode.get());

    return top_group;
}

void SimpleShaderPass::setupCamera()
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
}

void SimpleShaderPass::createOutputTexture()
{
    OutTexture_ = new osgCuda::TextureRectangle;
    
    OutTexture_->setTextureSize(TextureWidth_, TextureHeight_);
    OutTexture_->setInternalFormat(GL_LUMINANCE);
    OutTexture_->setFilter(osg::Texture2D::MIN_FILTER,osg::Texture2D::NEAREST);
    OutTexture_->setFilter(osg::Texture2D::MAG_FILTER,osg::Texture2D::NEAREST);
    
    // hdr
    //OutTexture_->setInternalFormat(GL_RGBA16F_ARB);
    //OutTexture_->setSourceFormat(GL_RGBA);
    //OutTexture_->setSourceType(GL_FLOAT);
}

void SimpleShaderPass::setShader(std::string filename)
{
    osg::ref_ptr<osg::Shader> fshader = new osg::Shader( osg::Shader::FRAGMENT ); 
    fshader->loadShaderSourceFromFile(filename);

    FragmentProgram_ = 0;
    FragmentProgram_ = new osg::Program;

    FragmentProgram_->addShader(fshader.get());

    StateSet_->setAttributeAndModes(FragmentProgram_.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE );
}
