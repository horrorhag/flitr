#include <flitr/modules/glsl_shader_passes/homography_shader_pass.h>

using namespace flitr;

HomographyShaderPass::HomographyShaderPass(flitr::TextureRectangle *in_tex, bool read_back_to_CPU)
{
    TextureWidth_ = in_tex->getTextureWidth();
    TextureHeight_ = in_tex->getTextureHeight();

    RootGroup_ = new osg::Group;
    InTexture_ = in_tex;
    OutTexture_=0;
    OutImage_=0;

    Homography_.makeIdentity();
    QuadCoords_ = new osg::Vec3Array; // vertex coords


    createOutputTexture(read_back_to_CPU);

    Camera_ = new osg::Camera;
    
    setupCamera();

    Camera_->addChild(createTexturedQuad().get());

    RootGroup_->addChild(Camera_.get());

}

HomographyShaderPass::~HomographyShaderPass()
{
}

void HomographyShaderPass::setHomography(const osg::Matrix in_homog)
{
    Homography_=in_homog;

    osg::Vec4d homogCoord0(Homography_ * osg::Vec4f(0.0f, 0.0f, 1.0f, 0.0f));
    osg::Vec4d homogCoord1(Homography_ * osg::Vec4f(TextureWidth_, 0.0f, 1.0f, 0.0f));
    osg::Vec4d homogCoord2(Homography_ * osg::Vec4f(TextureWidth_, TextureHeight_, 1.0f, 0.0f));
    osg::Vec4d homogCoord3(Homography_ * osg::Vec4f(0.0f, TextureHeight_, 1.0f, 0.0f));

    QuadCoords_->clear();

    QuadCoords_->push_back(osg::Vec3d(homogCoord0._v[0], homogCoord0._v[1], 0));
    QuadCoords_->push_back(osg::Vec3d(homogCoord1._v[0], homogCoord1._v[1], 0));
    QuadCoords_->push_back(osg::Vec3d(homogCoord2._v[0], homogCoord2._v[1], 0));
    QuadCoords_->push_back(osg::Vec3d(homogCoord3._v[0], homogCoord3._v[1], 0));

    QuadCoords_->dirty();

    //std::cout << "dirty " << homogCoord0 << " " << Homography_;
    //std::cout.flush();
}

osg::ref_ptr<osg::Group> HomographyShaderPass::createTexturedQuad()
{
    osg::ref_ptr<osg::Group> top_group = new osg::Group;
    
    osg::ref_ptr<osg::Geode> quad_geode = new osg::Geode;

    // counter-clockwise
    QuadCoords_->push_back(osg::Vec3f(0.0f, 0.0f, 0.0f));
    QuadCoords_->push_back(osg::Vec3f(TextureWidth_, 0.0f, 0.0f));
    QuadCoords_->push_back(osg::Vec3f(TextureWidth_, TextureHeight_, 0.0f));
    QuadCoords_->push_back(osg::Vec3f(0.0f, TextureHeight_, 0.0f));

    osg::ref_ptr<osg::Vec2Array> quad_tcoords = new osg::Vec2Array; // texture coords
    quad_tcoords->push_back(osg::Vec2(0, 0));
    quad_tcoords->push_back(osg::Vec2(TextureWidth_, 0));
    quad_tcoords->push_back(osg::Vec2(TextureWidth_, TextureHeight_));
    quad_tcoords->push_back(osg::Vec2(0, TextureHeight_));

    osg::ref_ptr<osg::Geometry> quad_geom = new osg::Geometry;
    osg::ref_ptr<osg::DrawArrays> quad_da = new osg::DrawArrays(osg::PrimitiveSet::QUADS,0,4);

    osg::ref_ptr<osg::Vec4Array> quad_colors = new osg::Vec4Array;
    quad_colors->push_back(osg::Vec4(1.0f,1.0f,1.0f,1.0f));

    quad_geom->setVertexArray(QuadCoords_.get());
    quad_geom->setTexCoordArray(0, quad_tcoords.get());
    quad_geom->addPrimitiveSet(quad_da.get());
    quad_geom->setColorArray(quad_colors.get());
    quad_geom->setColorBinding(osg::Geometry::BIND_OVERALL);
    quad_geom->setUseDisplayList(false);

    StateSet_ = quad_geom->getOrCreateStateSet();
    StateSet_->setMode(GL_LIGHTING,osg::StateAttribute::OFF);
    StateSet_->setMode(GL_DEPTH_TEST,osg::StateAttribute::OFF);
    StateSet_->setTextureAttributeAndModes(0, InTexture_.get(), osg::StateAttribute::ON);
    
    StateSet_->addUniform(new osg::Uniform("textureID0", 0));

    quad_geode->addDrawable(quad_geom.get());
    
    top_group->addChild(quad_geode.get());

    osg::ref_ptr<osg::Shader> fshader = new osg::Shader( osg::Shader::FRAGMENT,
                                                         "uniform sampler2DRect textureID0;\n"
                                                         "void main(void)\n"
                                                         "{\n"
                                                         "    vec2 texCoord = gl_TexCoord[0].xy;\n"
                                                         "    vec4 c  = texture2DRect(textureID0, texCoord);\n"
                                                         "    gl_FragColor = c;\n"
                                                         "}\n"
                                                         );

    FragmentProgram_ = 0;
    FragmentProgram_ = new osg::Program;

    FragmentProgram_->addShader(fshader.get());

    StateSet_->setAttributeAndModes(FragmentProgram_.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE );

    return top_group;
}

void HomographyShaderPass::setupCamera()
{
    // clearing
    bool need_clear = true;
    if (need_clear) {
        Camera_->setClearColor(osg::Vec4(0.0f,0.0f,0.0f,1.0f));
        Camera_->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    } else {
        Camera_->setClearMask(0);
        Camera_->setImplicitBufferAttachmentMask(0, 0);
    }

    // projection and view
    Camera_->setProjectionMatrixAsOrtho(0,TextureWidth_,0,TextureHeight_,-100,100);
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

void HomographyShaderPass::createOutputTexture(bool read_back_to_CPU)
{
    OutTexture_ = new flitr::TextureRectangle;
    
    OutTexture_->setTextureSize(TextureWidth_, TextureHeight_);
    OutTexture_->setInternalFormat(GL_RGBA);
    OutTexture_->setFilter(osg::Texture2D::MIN_FILTER,osg::Texture2D::NEAREST);
    OutTexture_->setFilter(osg::Texture2D::MAG_FILTER,osg::Texture2D::NEAREST);

    if (read_back_to_CPU)
    {
        OutImage_ = new osg::Image;
        OutImage_->allocateImage(TextureWidth_, TextureHeight_, 1,
                                 GL_RGBA, GL_UNSIGNED_BYTE);
    }
    
    // hdr
    //OutTexture_->setInternalFormat(GL_RGBA16F_ARB);
    //OutTexture_->setSourceFormat(GL_RGBA);
    //OutTexture_->setSourceType(GL_FLOAT);
}


