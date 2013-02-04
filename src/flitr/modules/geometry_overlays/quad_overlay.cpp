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

#include <flitr/modules/geometry_overlays/quad_overlay.h>

using namespace flitr;

QuadOverlay::QuadOverlay(double center_x, double center_y, double width, double height, bool filled) :
    GeometryOverlay(),
    _CenterX(center_x),
    _CenterY(center_y),
    _Width(width),
    _Height(height)
{
    _Vertices = new osg::Vec3Array;

    makeQuad(filled);

    dirtyBound();

    setColour(osg::Vec4d(1,1,0,0.5));
}

void QuadOverlay::makeQuad(bool filled)
{
    _Geode = new osg::Geode();
    _Geom = new osg::Geometry();

    updateQuad();
    
    _Geom->setVertexArray(_Vertices.get());
    
    if (!filled) {
        _DrawArray= new osg::DrawArrays(osg::PrimitiveSet::LINE_LOOP,0,4);
    } else {
        _DrawArray = new osg::DrawArrays(osg::PrimitiveSet::QUADS,0,4);
    }
    _Geom->addPrimitiveSet(_DrawArray.get());
    _Geom->setUseDisplayList(false);
    _Geode->addDrawable(_Geom.get());
    _Geode->setCullingActive(false);

    _GeometryGroup->addChild(_Geode.get());
}

void QuadOverlay::updateQuad()
{
    _Vertices->clear();
    _Vertices->push_back(osg::Vec3d(_CenterX-(_Width/2), _CenterY-(_Height/2), 0));
    _Vertices->push_back(osg::Vec3d(_CenterX+(_Width/2), _CenterY-(_Height/2), 0));
    _Vertices->push_back(osg::Vec3d(_CenterX+(_Width/2), _CenterY+(_Height/2), 0));
    _Vertices->push_back(osg::Vec3d(_CenterX-(_Width/2), _CenterY+(_Height/2), 0));
    _Vertices->dirty();

    dirtyBound();
}

void QuadOverlay::setCenter(double x, double y)
{
    _CenterX = x;
    _CenterY = y;
    updateQuad();
}

void QuadOverlay::getCenter(double &x, double &y)
{
    x=_CenterX;
    y=_CenterY;
}

void QuadOverlay::setWidth(double width)
{
    _Width = width;
    updateQuad();
}

void QuadOverlay::setHeight(double height)
{
    _Height = height;
    updateQuad();
}

double QuadOverlay::getWidth() const
{
    return _Width;
}
double QuadOverlay::getHeight() const
{
    return _Height;
}

void QuadOverlay::setName(const std::string &name)
{
    /* Set the name on the base class */
    GeometryOverlay::setName(name);

    /* Set the name of the other osg::Nodes */
    if (_Geom)//calls ref_ptr::valid()
        _Geom->setName(name);

    if (_Geode)//calls ref_ptr::valid()
        _Geode->setName(name);

    if (_DrawArray)//calls ref_ptr::valid()
        _DrawArray->setName(name);
}

