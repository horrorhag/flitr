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

#define _USE_MATH_DEFINES
#include <cmath>

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

void TexturedQuad::flipTextureCoordsLeftToRight()
{
    std::swap((*tcoords_)[0], (*tcoords_)[1]);
    std::swap((*tcoords_)[2], (*tcoords_)[3]);
    //Geom_->setTexCoordArray(0,tcoords_.get());
}

void TexturedQuad::flipTextureCoordsTopToBottom()
{
    std::swap((*tcoords_)[0], (*tcoords_)[3]);
    std::swap((*tcoords_)[1], (*tcoords_)[2]);
    //Geom_->setTexCoordArray(0,tcoords_.get());
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
    RootGroup_ = new osg::Group;
    RootGroup_->setName("flitr_textured_quad");
    MatrixTransform_ = new osg::MatrixTransform; // identity
    Geode_ = new osg::Geode();
    Geode_->setName("flitr_textured_quad");
    Geode_->setCullingActive(false);
    
    tcoords_ = new osg::Vec2Array();
    
    RootGroup_->addChild(MatrixTransform_.get());
    MatrixTransform_->addChild(Geode_.get());
}

void TexturedQuad::replaceGeom(bool use_normalised_coordinates)
{
    Geode_->removeDrawables(0, Geode_->getNumDrawables());
    
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    colors->push_back(osg::Vec4(1.0f,1.0f,1.0f,1.0f));
    
    // texture coords flipped here
    tcoords_->clear();
    
    if (use_normalised_coordinates) {
        tcoords_->push_back(osg::Vec2(0, 1));
        tcoords_->push_back(osg::Vec2(1, 1));
        tcoords_->push_back(osg::Vec2(1, 0));
        tcoords_->push_back(osg::Vec2(0, 0));
    } else {
        tcoords_->push_back(osg::Vec2(0, Height_));
        tcoords_->push_back(osg::Vec2(Width_, Height_));
        tcoords_->push_back(osg::Vec2(Width_, 0));
        tcoords_->push_back(osg::Vec2(0, 0));
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
    Geom_->setTexCoordArray(0, tcoords_.get());
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

MultiTexturedQuad::MultiTexturedQuad(const std::vector<osg::Image *> &inImageVec)
{
    init(inImageVec.size());
    setTextures(inImageVec);
}

void MultiTexturedQuad::setTextures(const std::vector<osg::Image *> &inImageVec)
{
    WidthVec_.clear();
    HeightVec_.clear();
    
    std::vector<osg::TextureRectangle *> inTexVec;
    
    for (auto imgPtr : inImageVec)
    {
        WidthVec_.push_back(imgPtr->s());
        HeightVec_.push_back(imgPtr->t());
        
        inTexVec.push_back(new osg::TextureRectangle);
        inTexVec.back()->setImage(imgPtr);
        inTexVec.back()->setWrap(osg::TextureRectangle::WRAP_S, osg::TextureRectangle::CLAMP_TO_BORDER);
        inTexVec.back()->setWrap(osg::TextureRectangle::WRAP_T, osg::TextureRectangle::CLAMP_TO_BORDER);
        inTexVec.back()->setBorderWidth(10);
        inTexVec.back()->setBorderColor(osg::Vec4d(0.0, 0.0, 0.0, 1.0));
    }
    
    replaceGeom(false);
    
    int texID=0;
    
    for (auto & inTex : inTexVec)
    {
        GeomStateSet_->setTextureAttributeAndModes(texID,
                                                   inTex,
                                                   osg::StateAttribute::ON);
        
        GeomStateSet_->setTextureAttribute(texID, new osg::TexEnv(osg::TexEnv::DECAL));
        
        ++texID;
    }
}


MultiTexturedQuad::MultiTexturedQuad(const std::vector<osg::TextureRectangle *> &inTexVec)
{
    init(inTexVec.size());
    setTextures(inTexVec);
}

void MultiTexturedQuad::setTextures(const std::vector<osg::TextureRectangle *> &inTexVec)
{
    WidthVec_.clear();
    HeightVec_.clear();
    
    for (auto texPtr : inTexVec)
    {
        WidthVec_.push_back(texPtr->getTextureWidth());
        HeightVec_.push_back(texPtr->getTextureHeight());
    }
    
    replaceGeom(false);
    
    int texID=0;
    
    for (auto inTex : inTexVec)
    {
        GeomStateSet_->setTextureAttributeAndModes(texID,
                                                   inTex,
                                                   osg::StateAttribute::ON);
        
        GeomStateSet_->setTextureAttribute(texID, new osg::TexEnv(osg::TexEnv::DECAL));
        
        ++texID;
    }
}


MultiTexturedQuad::MultiTexturedQuad(const std::vector<osg::Texture2D *> &inTexVec)
{
    init(inTexVec.size());
    setTextures(inTexVec);
}

void MultiTexturedQuad::setTextures(const std::vector<osg::Texture2D *> &inTexVec)
{
    WidthVec_.clear();
    HeightVec_.clear();
    
    for (auto texPtr : inTexVec)
    {
        WidthVec_.push_back(texPtr->getTextureWidth());
        HeightVec_.push_back(texPtr->getTextureHeight());
    }
    
    replaceGeom(true);
    
    int texID=0;
    
    for (auto inTex : inTexVec)
    {
        GeomStateSet_->setTextureAttributeAndModes(texID,
                                                   inTex,
                                                   osg::StateAttribute::ON);
        
        GeomStateSet_->setTextureAttribute(texID, new osg::TexEnv(osg::TexEnv::DECAL));
        
        ++texID;
    }
}


void MultiTexturedQuad::setShader(const std::string &filename)
{
    osg::ref_ptr<osg::Shader> fshader = new osg::Shader( osg::Shader::FRAGMENT );
    fshader->loadShaderSourceFromFile(filename);
    
    FragmentProgram_ = 0;
    FragmentProgram_ = new osg::Program;
    FragmentProgram_->setName(filename);
    FragmentProgram_->addShader(fshader.get());
    
    GeomStateSet_->setAttributeAndModes(FragmentProgram_.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE );
    
    const size_t numTextures=WidthVec_.size();
    
    for (size_t textureID=0; textureID<numTextures; ++textureID)
    {
        std::string texUnifName=std::string("textureID") + std::to_string(textureID);
        GeomStateSet_->addUniform(new osg::Uniform(texUnifName.c_str(), int(textureID)));
        GeomStateSet_->addUniform(ColourMaskUniformVec_[textureID]);
    }
}

void MultiTexturedQuad::init(const size_t numTextures)
{
    RootGroup_ = new osg::Group;
    RootGroup_->setName("flitr_textured_quad");
    MatrixTransform_ = new osg::MatrixTransform; // identity
    Geode_ = new osg::Geode();
    Geode_->setName("flitr_textured_quad");
    Geode_->setCullingActive(false);
    
    ColourMaskUniformVec_.clear();
    
    for (size_t maskID=0; maskID<numTextures; ++maskID)
    {
        std::string maskUnifName=std::string("colourMask") + std::to_string(maskID);
        ColourMaskUniformVec_.push_back(new osg::Uniform(maskUnifName.c_str(), osg::Vec4f(1.0f, 1.0f, 1.0f, 1.0f)));
        
        texCoordVec_.push_back(new osg::Vec2Array());
    }
    
    RootGroup_->addChild(MatrixTransform_.get());
    MatrixTransform_->addChild(Geode_.get());
}

void MultiTexturedQuad::replaceGeom(bool use_normalised_coordinates)
{
    Geode_->removeDrawables(0, Geode_->getNumDrawables());
    
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    colors->push_back(osg::Vec4(1.0f,1.0f,1.0f,1.0f));
    
    osg::ref_ptr<osg::Vec3Array> vcoords = new osg::Vec3Array; // vertex coords
    
    int geomWidth=WidthVec_[0];
    int geomHeight=HeightVec_[0];
    
    // we create the quad in the x-y-plane
    vcoords->push_back(osg::Vec3d(0, 0, 0));
    vcoords->push_back(osg::Vec3d(geomWidth, 0, 0));
    vcoords->push_back(osg::Vec3d(geomWidth, geomHeight, 0));
    vcoords->push_back(osg::Vec3d(0, geomHeight, 0));
    
    Geom_ = new osg::Geometry;
    Geom_->setName("flitr_textured_quad");
    Geom_->setVertexArray(vcoords.get());
    
    
    
    const size_t numTextures=WidthVec_.size();
    
    for (size_t textureID=0; textureID<numTextures; ++textureID)
    {
        texCoordVec_[textureID]->clear();
        
        if (use_normalised_coordinates) {
            texCoordVec_[textureID]->push_back(osg::Vec2(0.0, 1.0));
            texCoordVec_[textureID]->push_back(osg::Vec2(1.0, 1.0));
            texCoordVec_[textureID]->push_back(osg::Vec2(1.0, 0.0));
            texCoordVec_[textureID]->push_back(osg::Vec2(0.0, 0.0));
        } else {
            texCoordVec_[textureID]->push_back(osg::Vec2(0.0, HeightVec_[textureID]));
            texCoordVec_[textureID]->push_back(osg::Vec2(WidthVec_[textureID], HeightVec_[textureID]));
            texCoordVec_[textureID]->push_back(osg::Vec2(WidthVec_[textureID], 0.0));
            texCoordVec_[textureID]->push_back(osg::Vec2(0.0, 0.0));
        }
        
        Geom_->setTexCoordArray(textureID, texCoordVec_[textureID].get());
    }
    
    
    osg::ref_ptr<osg::DrawArrays> da = new osg::DrawArrays(osg::PrimitiveSet::QUADS,0,4);
    da->setName("flitr_textured_quad");
    Geom_->addPrimitiveSet(da.get());
    Geom_->setColorArray(colors.get());
    Geom_->setColorBinding(osg::Geometry::BIND_OVERALL);
    GeomStateSet_ = Geom_->getOrCreateStateSet();
    GeomStateSet_->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    
    Geode_->addDrawable(Geom_.get());
}

MultiTexturedQuad::~MultiTexturedQuad()
{
}






