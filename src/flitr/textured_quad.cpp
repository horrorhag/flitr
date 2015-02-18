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

#include <iostream>

#include <osgDB/FileUtils>

#include <flitr/textured_quad.h>

using namespace flitr;

TexturedQuad::TexturedQuad(osg::Image* in_image)
{
    init();
    setTexture(in_image);
}

void TexturedQuad::setTexture(osg::Image* in_image)
{
    Width_ = in_image->s();
    Height_ = in_image->t();

    osg::ref_ptr<osg::TextureRectangle> trect = new osg::TextureRectangle;
    trect->setImage(in_image);

    replaceGeom(false);
    GeomStateSet_->setTextureAttributeAndModes(0, 
                                               trect.get(),
                                               osg::StateAttribute::ON);

    GeomStateSet_->setTextureAttribute(0, new osg::TexEnv(osg::TexEnv::DECAL));
}

TexturedQuad::TexturedQuad(osg::TextureRectangle* in_tex)
{
    init();
    setTexture(in_tex);
}

void TexturedQuad::setTexture(osg::TextureRectangle* in_tex)
{
	Width_  = in_tex->getTextureWidth();
    Height_ = in_tex->getTextureHeight();

    replaceGeom(false);
    GeomStateSet_->setTextureAttributeAndModes(0, 
                                               in_tex,
                                               osg::StateAttribute::ON);

    GeomStateSet_->setTextureAttribute(0, new osg::TexEnv(osg::TexEnv::DECAL));
}

TexturedQuad::TexturedQuad(osg::Texture2D* in_tex)
{
    init();
    setTexture(in_tex);
}

void TexturedQuad::setTexture(osg::Texture2D* in_tex)
{
	Width_  = in_tex->getTextureWidth();
    Height_ = in_tex->getTextureHeight();

    replaceGeom(true);
    GeomStateSet_->setTextureAttributeAndModes(0, 
                                               in_tex,
                                               osg::StateAttribute::ON);

    GeomStateSet_->setTextureAttribute(0, new osg::TexEnv(osg::TexEnv::DECAL));
}

void TexturedQuad::setShader(std::string filename)
{
    osg::ref_ptr<osg::Shader> fshader = new osg::Shader( osg::Shader::FRAGMENT ); 
    fshader->loadShaderSourceFromFile(filename);

    FragmentProgram_ = 0;
    FragmentProgram_ = new osg::Program;
    FragmentProgram_->setName(filename);
    FragmentProgram_->addShader(fshader.get());

    GeomStateSet_->setAttributeAndModes(FragmentProgram_.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE );
    GeomStateSet_->addUniform(new osg::Uniform("inputTexture", 0));
}

void TexturedQuad::init()
{
    OldWidth_ = -1;
    OldHeight_ = -1;
    OldUseNormalised_ = false;

    RootGroup_ = new osg::Group;
    RootGroup_->setName("flitr_textured_quad");
    MatrixTransform_ = new osg::MatrixTransform; // identity
    Geode_ = new osg::Geode();
    Geode_->setName("flitr_textured_quad");
    Geode_->setCullingActive(false);

    RootGroup_->addChild(MatrixTransform_.get());
    MatrixTransform_->addChild(Geode_.get());
}
 
void TexturedQuad::replaceGeom(bool use_normalised_coordinates)
{
    // initial Old* values will cause this to pass on first call at least
    if (Width_ == OldWidth_ &&
        Height_ == OldHeight_ &&
        use_normalised_coordinates == OldUseNormalised_) 
    {
        return;
    }
    OldWidth_ = Width_;
    OldHeight_ = Height_;
    OldUseNormalised_ = use_normalised_coordinates;

    Geode_->removeDrawables(0, Geode_->getNumDrawables());
    
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    colors->push_back(osg::Vec4(1.0f,1.0f,1.0f,1.0f));

    // texture coords flipped here
    osg::ref_ptr<osg::Vec2Array> tcoords = new osg::Vec2Array; // texture coords
    if (use_normalised_coordinates) {
        tcoords->push_back(osg::Vec2(0, 1));
        tcoords->push_back(osg::Vec2(1, 1));
        tcoords->push_back(osg::Vec2(1, 0));
        tcoords->push_back(osg::Vec2(0, 0));
    } else {
        tcoords->push_back(osg::Vec2(0, Height_));
        tcoords->push_back(osg::Vec2(Width_, Height_));
        tcoords->push_back(osg::Vec2(Width_, 0));
        tcoords->push_back(osg::Vec2(0, 0));
    }

    osg::ref_ptr<osg::Vec3Array> vcoords = new osg::Vec3Array; // vertex coords
    // we create the quad in the x-y-plane
    vcoords->push_back(osg::Vec3d(0, 0, 0));
    vcoords->push_back(osg::Vec3d(Width_, 0, 0));
    vcoords->push_back(osg::Vec3d(Width_, Height_, 0));
    vcoords->push_back(osg::Vec3d(0, Height_, 0));

    Geom_ = new osg::Geometry;
    Geom_->setName("flitr_textured_quad");
    Geom_->setVertexArray(vcoords.get());
    Geom_->setTexCoordArray(0,tcoords.get());
    osg::ref_ptr<osg::DrawArrays> da = new osg::DrawArrays(osg::PrimitiveSet::QUADS,0,4);
    da->setName("flitr_textured_quad");
    Geom_->addPrimitiveSet(da.get());
    Geom_->setColorArray(colors.get());
    Geom_->setColorBinding(osg::Geometry::BIND_OVERALL);
	GeomStateSet_ = Geom_->getOrCreateStateSet();
    GeomStateSet_->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    Geode_->addDrawable(Geom_.get());
}

TexturedQuad::~TexturedQuad()
{
}

//===============================
//===============================
//===============================

TexturedQuadAB::TexturedQuadAB(osg::Image* in_image_A, osg::Image* in_image_B)
{
    init();
    setTextures(in_image_A, in_image_B);
}

void TexturedQuadAB::setTextures(osg::Image* in_image_A, osg::Image* in_image_B)
{
    Width_A_ = in_image_A->s();
    Height_A_ = in_image_A->t();

    Width_B_ = in_image_B->s();
    Height_B_ = in_image_B->t();

    osg::ref_ptr<osg::TextureRectangle> trect_A = new osg::TextureRectangle;
    trect_A->setImage(in_image_A);
    trect_A->setWrap(osg::TextureRectangle::WRAP_S, osg::TextureRectangle::CLAMP_TO_BORDER);
    trect_A->setWrap(osg::TextureRectangle::WRAP_T, osg::TextureRectangle::CLAMP_TO_BORDER);
    trect_A->setBorderWidth(10);
    trect_A->setBorderColor(osg::Vec4d(0.0, 0.0, 0.0, 1.0));

    osg::ref_ptr<osg::TextureRectangle> trect_B = new osg::TextureRectangle;
    trect_B->setImage(in_image_B);
    trect_B->setWrap(osg::TextureRectangle::WRAP_S, osg::TextureRectangle::CLAMP_TO_BORDER);
    trect_B->setWrap(osg::TextureRectangle::WRAP_T, osg::TextureRectangle::CLAMP_TO_BORDER);
    trect_B->setBorderWidth(10);
    trect_B->setBorderColor(osg::Vec4d(0.0, 0.0, 0.0, 1.0));

    replaceGeom(false);

    GeomStateSet_->setTextureAttributeAndModes(0,
                                               trect_A.get(),
                                               osg::StateAttribute::ON);

    GeomStateSet_->setTextureAttribute(0, new osg::TexEnv(osg::TexEnv::DECAL));

    GeomStateSet_->setTextureAttributeAndModes(1,
                                               trect_B.get(),
                                               osg::StateAttribute::ON);

    GeomStateSet_->setTextureAttribute(1, new osg::TexEnv(osg::TexEnv::DECAL));
}

TexturedQuadAB::TexturedQuadAB(osg::TextureRectangle* in_tex_A, osg::TextureRectangle* in_tex_B)
{
    init();
    setTextures(in_tex_A, in_tex_B);
}

void TexturedQuadAB::setTextures(osg::TextureRectangle* in_tex_A, osg::TextureRectangle* in_tex_B)
{
    Width_A_  = in_tex_A->getTextureWidth();
    Height_A_ = in_tex_A->getTextureHeight();

    Width_B_  = in_tex_B->getTextureWidth();
    Height_B_ = in_tex_B->getTextureHeight();

    replaceGeom(false);

    GeomStateSet_->setTextureAttributeAndModes(0,
                                               in_tex_A,
                                               osg::StateAttribute::ON);

    GeomStateSet_->setTextureAttribute(0, new osg::TexEnv(osg::TexEnv::DECAL));

    GeomStateSet_->setTextureAttributeAndModes(1,
                                               in_tex_B,
                                               osg::StateAttribute::ON);

    GeomStateSet_->setTextureAttribute(1, new osg::TexEnv(osg::TexEnv::DECAL));
}

TexturedQuadAB::TexturedQuadAB(osg::Texture2D* in_tex_A, osg::Texture2D* in_tex_B)
{
    init();
    setTextures(in_tex_A, in_tex_B);
}

void TexturedQuadAB::setTextures(osg::Texture2D* in_tex_A, osg::Texture2D* in_tex_B)
{
    Width_A_  = in_tex_A->getTextureWidth();
    Height_A_ = in_tex_A->getTextureHeight();

    Width_B_  = in_tex_B->getTextureWidth();
    Height_B_ = in_tex_B->getTextureHeight();

    replaceGeom(true);

    GeomStateSet_->setTextureAttributeAndModes(0,
                                               in_tex_A,
                                               osg::StateAttribute::ON);

    GeomStateSet_->setTextureAttribute(0, new osg::TexEnv(osg::TexEnv::DECAL));

    GeomStateSet_->setTextureAttributeAndModes(1,
                                               in_tex_B,
                                               osg::StateAttribute::ON);

    GeomStateSet_->setTextureAttribute(1, new osg::TexEnv(osg::TexEnv::DECAL));

}

void TexturedQuadAB::setShader(const std::string &filename)
{
    osg::ref_ptr<osg::Shader> fshader = new osg::Shader( osg::Shader::FRAGMENT );
    fshader->loadShaderSourceFromFile(filename);

    FragmentProgram_ = 0;
    FragmentProgram_ = new osg::Program;
    FragmentProgram_->setName(filename);
    FragmentProgram_->addShader(fshader.get());

    GeomStateSet_->setAttributeAndModes(FragmentProgram_.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE );

    GeomStateSet_->addUniform(new osg::Uniform("textureID0", 0));
    GeomStateSet_->addUniform(new osg::Uniform("textureID1", 1));

    GeomStateSet_->addUniform(ColourMaskAUniform_);
    GeomStateSet_->addUniform(ColourMaskBUniform_);
}

void TexturedQuadAB::init()
{
    RootGroup_ = new osg::Group;
    RootGroup_->setName("flitr_textured_quad");
    MatrixTransform_ = new osg::MatrixTransform; // identity
    Geode_ = new osg::Geode();
    Geode_->setName("flitr_textured_quad");
    Geode_->setCullingActive(false);

    ColourMaskAUniform_=new osg::Uniform("colourMaskA", osg::Vec4f(1.0f, 1.0f, 1.0f, 1.0f));
    ColourMaskBUniform_=new osg::Uniform("colourMaskB", osg::Vec4f(1.0f, 1.0f, 1.0f, 1.0f));

    RootGroup_->addChild(MatrixTransform_.get());
    MatrixTransform_->addChild(Geode_.get());
}

void TexturedQuadAB::replaceGeom(bool use_normalised_coordinates)
{
    Geode_->removeDrawables(0, Geode_->getNumDrawables());

    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    colors->push_back(osg::Vec4(1.0f,1.0f,1.0f,1.0f));

    // texture coords flipped here
    osg::ref_ptr<osg::Vec2Array> tcoords_A = new osg::Vec2Array; // texture coords
    osg::ref_ptr<osg::Vec2Array> tcoords_B = new osg::Vec2Array; // texture coords

    double tex_scale_B=1.0;

    if (use_normalised_coordinates) {
        tcoords_A->push_back(osg::Vec2(0.0, 1.0));
        tcoords_A->push_back(osg::Vec2(1.0, 1.0));
        tcoords_A->push_back(osg::Vec2(1.0, 0.0));
        tcoords_A->push_back(osg::Vec2(0.0, 0.0));

        tcoords_B->push_back(osg::Vec2(1.0-tex_scale_B, tex_scale_B));
        tcoords_B->push_back(osg::Vec2(tex_scale_B, tex_scale_B));
        tcoords_B->push_back(osg::Vec2(tex_scale_B, 1.0-tex_scale_B));
        tcoords_B->push_back(osg::Vec2(1.0-tex_scale_B, 1.0-tex_scale_B));
    } else {
        tcoords_A->push_back(osg::Vec2(0.0, Height_A_));
        tcoords_A->push_back(osg::Vec2(Width_A_, Height_A_));
        tcoords_A->push_back(osg::Vec2(Width_A_, 0.0));
        tcoords_A->push_back(osg::Vec2(0.0, 0.0));

        tcoords_B->push_back(osg::Vec2(Width_B_*(1.0-tex_scale_B), Height_B_*tex_scale_B));
        tcoords_B->push_back(osg::Vec2(Width_B_*tex_scale_B, Height_B_*tex_scale_B));
        tcoords_B->push_back(osg::Vec2(Width_B_*tex_scale_B, Height_B_*(1.0-tex_scale_B)));
        tcoords_B->push_back(osg::Vec2(Width_B_*(1.0-tex_scale_B), Height_B_*(1.0-tex_scale_B)));
    }

    osg::ref_ptr<osg::Vec3Array> vcoords = new osg::Vec3Array; // vertex coords
    // we create the quad in the x-y-plane
    vcoords->push_back(osg::Vec3d(0, 0, 0));
    vcoords->push_back(osg::Vec3d(Width_A_, 0, 0));
    vcoords->push_back(osg::Vec3d(Width_A_, Height_A_, 0));
    vcoords->push_back(osg::Vec3d(0, Height_A_, 0));

    Geom_ = new osg::Geometry;
    Geom_->setName("flitr_textured_quad");
    Geom_->setVertexArray(vcoords.get());

    Geom_->setTexCoordArray(0, tcoords_A.get());
    Geom_->setTexCoordArray(1, tcoords_B.get());

    osg::ref_ptr<osg::DrawArrays> da = new osg::DrawArrays(osg::PrimitiveSet::QUADS,0,4);
    da->setName("flitr_textured_quad");
    Geom_->addPrimitiveSet(da.get());
    Geom_->setColorArray(colors.get());
    Geom_->setColorBinding(osg::Geometry::BIND_OVERALL);
    GeomStateSet_ = Geom_->getOrCreateStateSet();
    GeomStateSet_->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    Geode_->addDrawable(Geom_.get());
}

TexturedQuadAB::~TexturedQuadAB()
{
}






