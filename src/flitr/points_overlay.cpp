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

#include <flitr/points_overlay.h>

using namespace flitr;

PointsOverlay::PointsOverlay() :
    GeometryOverlay(),
    _PointSize(2.0)
{
    _Vertices = new osg::Vec3Array;
    _VertexColours = new osg::Vec4Array;

    makeGraph();
    dirtyBound();

    setColour(osg::Vec4d(0,1,0,0.5));
}

void PointsOverlay::makeGraph()
{
    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry();
    
    geom->setVertexArray(_Vertices.get());
    geom->setColorArray(_VertexColours.get());
    geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    _DrawArray = new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, _Vertices->size());

    setPointSize(_PointSize);

    geom->addPrimitiveSet(_DrawArray.get());
    geom->setUseDisplayList(false);
    geode->addDrawable(geom.get());
    geode->setCullingActive(false);
    
    _GeometryGroup->addChild(geode.get());
}

void PointsOverlay::setVertices(osg::Vec3Array& v)
{
    *_Vertices = v;
    _Vertices->dirty();

    _VertexColours->clear();

    int numVertices=_Vertices->size();
    for (int i=0; i<numVertices; i++)
    {
        _VertexColours->push_back(_GeometryColour);
    }

    _VertexColours->dirty();

    _DrawArray->setCount(_Vertices->size());

    dirtyBound();
}

void PointsOverlay::setVertices(osg::Vec3Array& v, osg::Vec4Array& vc)
{
    *_Vertices = v;
    _Vertices->dirty();

    *_VertexColours = vc;
    _VertexColours->dirty();

    _DrawArray->setCount(_Vertices->size());

    dirtyBound();
}

void PointsOverlay::addVertex(osg::Vec3d v)
{
    addVertex(v, _GeometryColour);
}

void PointsOverlay::addVertex(const osg::Vec3d &v, const osg::Vec4d &vc)
{
    _Vertices->push_back(v);
    _Vertices->dirty();

    _VertexColours->push_back(vc);
    _VertexColours->dirty();

    _DrawArray->setCount(_Vertices->size());

    dirtyBound();
}

void PointsOverlay::clearVertices()
{
    _Vertices->clear();
    _Vertices->dirty();

    _VertexColours->clear();
    _VertexColours->dirty();

    _DrawArray->setCount(_Vertices->size());

    dirtyBound();
}
