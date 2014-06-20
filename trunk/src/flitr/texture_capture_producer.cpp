/* Framework for Live Image Transformation (FLITr) 
 * Copyright (c) 2010 CSIR
 *
 * This file is part of FLITr.
 *
 * FLITr is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * FLITr is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FLITr. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <flitr/texture_capture_producer.h>

using namespace flitr;

TextureCaptureProducer::TextureCaptureProducer(osg::TextureRectangle* input_texture,
                                               ImageFormat::PixelFormat output_pixel_format,
                                               uint32_t new_width,
                                               uint32_t new_height,
                                               bool keep_aspect,
                                               uint32_t buffer_size, bool blocking) :
    InputTexture_(input_texture),
    OutputFormat_(new_width, new_height, output_pixel_format),
    OutputTextureWidth_(new_width),
    OutputTextureHeight_(new_height),
    KeepAspect_(keep_aspect),
    BufferSize_(buffer_size),
    blocking_(blocking),
    enabled_(true),
    SaveNextImageTo_("")
{
    InputTextureWidth_ = InputTexture_->getTextureWidth();
    InputTextureHeight_ = InputTexture_->getTextureHeight();
}

bool TextureCaptureProducer::init()
{
    if (OutputFormat_.getPixelFormat()==ImageFormat::FLITR_PIX_FMT_ANY)
    {
        GLenum pixelFormat=InputTexture_->getSourceFormat();
        GLenum pixelType=InputTexture_->getSourceType();

        if (((pixelFormat==GL_RGB)||(pixelFormat==GL_RGBA))&&(pixelType==GL_UNSIGNED_BYTE))
        {
            OutputFormat_.setPixelFormat(ImageFormat::FLITR_PIX_FMT_RGB_8);
        } else
            if (((pixelFormat==GL_LUMINANCE)||(pixelFormat==GL_RED))&&(pixelType==GL_UNSIGNED_BYTE))
            {
                OutputFormat_.setPixelFormat(ImageFormat::FLITR_PIX_FMT_Y_8);
            } else
                if (((pixelFormat==GL_LUMINANCE)||(pixelFormat==GL_RED))&&(pixelType==GL_UNSIGNED_SHORT))
                {
                    OutputFormat_.setPixelFormat(ImageFormat::FLITR_PIX_FMT_Y_16);
                }
    }

    ImageFormat_.push_back(OutputFormat_);//Only one image per slot.

    SharedImageBuffer_ = std::shared_ptr<SharedImageBuffer>(new SharedImageBuffer(*this, BufferSize_, 1));
    SharedImageBuffer_->initWithStorage();

    RootGroup_ = new osg::Group;
    createTextures();
    
    Camera_ = new osg::Camera;
    setupCamera();
    Camera_->addChild(createTexturedQuad().get());

    RootGroup_->addChild(Camera_.get());

    //setShader("");


    return true;
}

void TextureCaptureProducer::createTextures()
{
    OutputTexture_ = new osg::TextureRectangle;
    OutputTexture_->setTextureSize(OutputTextureWidth_, OutputTextureHeight_);

    GLenum pixelFormat=GL_RGB;
    GLenum pixelType=GL_UNSIGNED_BYTE;

    switch (OutputFormat_.getPixelFormat())
    {
    case ImageFormat::FLITR_PIX_FMT_Y_8:
        pixelFormat=GL_LUMINANCE;
        pixelType=GL_UNSIGNED_BYTE;
        break;
    case ImageFormat::FLITR_PIX_FMT_RGB_8:
        pixelFormat=GL_RGB;
        pixelType=GL_UNSIGNED_BYTE;
        break;
    case ImageFormat::FLITR_PIX_FMT_Y_16:
        pixelFormat=GL_LUMINANCE;
        pixelType=GL_UNSIGNED_SHORT;
        break;
    default:
        std::cerr << "Pixel format " << OutputFormat_.getPixelFormat() << " not yet implemented in TextureCaptureProducer.\n";
        exit(-1);
    }

    OutputTexture_->setInternalFormat(pixelFormat);
    OutputTexture_->setSourceFormat(pixelFormat);
    OutputTexture_->setSourceType(pixelType);

    OutputTexture_->setFilter(osg::Texture2D::MIN_FILTER,osg::Texture2D::LINEAR);
    OutputTexture_->setFilter(osg::Texture2D::MAG_FILTER,osg::Texture2D::LINEAR);
}

void TextureCaptureProducer::setShader(std::string filename)
{
    osg::ref_ptr<osg::Shader> fshader = new osg::Shader( osg::Shader::FRAGMENT );
    fshader->loadShaderSourceFromFile(osgDB::findDataFile(filename));

    FragmentProgram_ = 0;
    FragmentProgram_ = new osg::Program;

    FragmentProgram_->addShader(fshader.get());

    StateSet_->setAttributeAndModes(FragmentProgram_.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE );
}

osg::ref_ptr<osg::Group> TextureCaptureProducer::createTexturedQuad()
{
    osg::ref_ptr<osg::Group> top_group = new osg::Group;
    
    osg::ref_ptr<osg::Geode> quad_geode = new osg::Geode;

    osg::ref_ptr<osg::Vec3Array> quad_coords = new osg::Vec3Array; // vertex coords
    // counter-clockwise
    quad_coords->push_back(osg::Vec3d(0, 0, -1));
    quad_coords->push_back(osg::Vec3d(InputTextureWidth_, 0, -1));
    quad_coords->push_back(osg::Vec3d(InputTextureWidth_, InputTextureHeight_, -1));
    quad_coords->push_back(osg::Vec3d(0, InputTextureHeight_, -1));

    osg::ref_ptr<osg::Vec2Array> quad_tcoords = new osg::Vec2Array; // texture coords

    quad_tcoords->push_back(osg::Vec2(0, 0));
    quad_tcoords->push_back(osg::Vec2(InputTextureWidth_, 0));
    quad_tcoords->push_back(osg::Vec2(InputTextureWidth_, InputTextureHeight_));
    quad_tcoords->push_back(osg::Vec2(0, InputTextureHeight_));
    
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
    StateSet_->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    StateSet_->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    StateSet_->setMode(GL_BLEND, osg::StateAttribute::OFF);

    // bind the input textures
    StateSet_->setTextureAttributeAndModes(0, InputTexture_.get(), osg::StateAttribute::ON);
    quad_geode->addDrawable(quad_geom.get());
    
    top_group->addChild(quad_geode.get());

    return top_group;
}

void TextureCaptureProducer::setupCamera()
{
    // clearing
    Camera_->setClearColor(osg::Vec4(0.0f,0.0f,0.0f,1.0f));
    Camera_->setClearMask(GL_COLOR_BUFFER_BIT);

    // projection and view
    if (KeepAspect_) {
        double in_aspect = (double)InputTextureWidth_/InputTextureHeight_;
        double out_aspect = (double)OutputTextureWidth_/OutputTextureHeight_;
        if (in_aspect > out_aspect) {
            double scale = in_aspect/out_aspect;
            double new_height = scale * InputTextureHeight_;
            double bottom = (double)InputTextureHeight_/2.0 - new_height/2.0;
            double top = bottom + new_height;
            Camera_->setProjectionMatrixAsOrtho(0,InputTextureWidth_,bottom,top,-100,100);
        } else {
            double scale = out_aspect/in_aspect;
            double new_width = scale * InputTextureWidth_;
            double left = (double)InputTextureWidth_/2.0 - new_width/2.0;
            double right = left + new_width;
            Camera_->setProjectionMatrixAsOrtho(left,right,0,InputTextureHeight_,-100,100);
        }
    } else {
        Camera_->setProjectionMatrixAsOrtho(0,InputTextureWidth_,0,InputTextureHeight_,-100,100);
    }

    Camera_->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
    Camera_->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    Camera_->setViewMatrix(osg::Matrix::identity());

    // viewport
    Camera_->setViewport(0, 0, OutputTextureWidth_, OutputTextureHeight_);

    Camera_->setRenderOrder(osg::Camera::PRE_RENDER);
    Camera_->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
    
    // attach the outputs
    Camera_->attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER),
                    OutputTexture_.get());

    Camera_->setPostDrawCallback(new TextureCaptureDrawCallback(this, blocking_));
}
