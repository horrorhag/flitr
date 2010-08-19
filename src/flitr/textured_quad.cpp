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

TexturedQuad::TexturedQuad(osg::Image *in_image)
{
	Width_ = in_image->s();
    Height_ = in_image->t();

    osg::ref_ptr<osg::TextureRectangle> trect = new osg::TextureRectangle;
    trect->setImage(in_image);
        
    buildGraph(false);

    GeomStateSet_->setTextureAttributeAndModes(0, 
                                               trect.get(),
                                               osg::StateAttribute::ON);
}

TexturedQuad::TexturedQuad(osg::TextureRectangle *in_tex)
{
	Width_  = in_tex->getTextureWidth();
    Height_ = in_tex->getTextureHeight();

    buildGraph(false);

    GeomStateSet_->setTextureAttributeAndModes(0, 
                                               in_tex,
                                               osg::StateAttribute::ON);
}

TexturedQuad::TexturedQuad(osg::Texture2D *in_tex)
{
	Width_  = in_tex->getTextureWidth();
    Height_ = in_tex->getTextureHeight();

    buildGraph(true);

    GeomStateSet_->setTextureAttributeAndModes(0, 
                                               in_tex,
                                               osg::StateAttribute::ON);
}

void TexturedQuad::buildGraph(bool use_normalised_coordinates)
{
    RootGroup_ = new osg::Group;

    MatrixTransform_ = new osg::MatrixTransform; // identity
    RootGroup_->addChild(MatrixTransform_.get());

	// create quad to display image on
    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    
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
    Geom_->setVertexArray(vcoords.get());
    Geom_->setTexCoordArray(0,tcoords.get());
    osg::ref_ptr<osg::DrawArrays> da = new osg::DrawArrays(osg::PrimitiveSet::QUADS,0,4);
    Geom_->addPrimitiveSet(da.get());
    Geom_->setColorArray(colors.get());
    Geom_->setColorBinding(osg::Geometry::BIND_OVERALL);
	GeomStateSet_ = Geom_->getOrCreateStateSet();
    GeomStateSet_->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    geode->addDrawable(Geom_.get());
  
    MatrixTransform_->addChild(geode.get());
}

TexturedQuad::~TexturedQuad()
{
}
