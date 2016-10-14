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

#include <flitr/modules/geometry_overlays/lines_overlay.h>

#include <cassert>
#include <iostream>

using namespace flitr;

LinesOverlay::LinesOverlay()
{
    _Vertices = new osg::Vec3Array;
    _VertexColours = new osg::Vec4Array;

    makeGraph();
    dirtyBound();
}

LinesOverlay::~LinesOverlay()
{

}

void LinesOverlay::makeGraph()
{
    _Geode = new osg::Geode();
    _Geom  = new osg::Geometry();

    _Geom->setVertexArray(_Vertices.get());
    _Geom->setColorArray(_VertexColours.get());
    _Geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);

    _DrawArray = new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, _Vertices->size());

    _Geom->addPrimitiveSet(_DrawArray.get());
    _Geom->setUseDisplayList(false);
    _Geom->setCullingActive(false);
    _Geode->addDrawable(_Geom.get());
    _Geode->setCullingActive(false);

    _GeometryGroup->addChild(_Geode.get());
    _GeometryGroup->setCullingActive(false);
}

void LinesOverlay::setVertices(const osg::Vec3Array& vertices, const osg::Vec4Array& colours)
{
    _Vertices->clear();
    _VertexColours->clear();
    addVertices(vertices, colours);
}

void LinesOverlay::addVertices(const osg::Vec3Array& vertices, const osg::Vec4Array& colours)
{
    for(const osg::Vec3& vertex: vertices)
    {
        _Vertices->push_back(vertex);
    }
    /* Check if the points are even. If there are uneven points the last one should be
     * removed otherwise adding new lines will show strange behaviour.
     * The base class functions will make sure that this is the case when this function
     * is called, but if this class is derived from, that might not always be the case. */
    bool pointsEven = ((_Vertices->size() % 2) == 0);
    if(pointsEven == false)
    {
        std::cout << "LinesOverlay: Uneven number of vertices, last point getting removed... " << std::endl;
        _Vertices->pop_back();
        assert(pointsEven != false);
    }
    _Vertices->dirty();

    /* Add a colour for each vertex.
     * The base class functions will make sure that this is the case when this function
     * is called, but if this class is derived from, that might not always be the case. */
    osg::Vec4Array::const_iterator colourIter = colours.begin();
    while(_VertexColours->size() < _Vertices->size())
    {
        if(colourIter == colours.end())
        {
            _VertexColours->push_back(_GeometryColour);
        } else
            {
                _VertexColours->push_back(*colourIter);
                ++colourIter;
            }
    }

    assert(_VertexColours->size() == _Vertices->size());

    _VertexColours->dirty();
    _DrawArray->setCount(_Vertices->size());
    dirtyBound();
}

void LinesOverlay::setLines(const LinesOverlay::Lines& lines, LinesOverlay::Colours colours)
{
    _Vertices->clear();
    _VertexColours->clear();
    addLines(lines, colours);
}

void LinesOverlay::addLines(const Lines& lines, Colours colours)
{
    osg::ref_ptr<osg::Vec3Array> vertexArray = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colourArray = new osg::Vec4Array();

    /* Pad the colours vector to make sure that there is
     * at least a colour for each line. */
    while(colours.size() < lines.size())
    {
        colours.push_back(_GeometryColour);
    }

    const size_t nrLines = lines.size();
    for(size_t idx = 0; idx < nrLines; ++idx)
    {
        const Line& line = lines.at(idx);
        vertexArray->push_back(line.first);
        vertexArray->push_back(line.second);

        const osg::Vec4& colour = colours.at(idx);
        colourArray->push_back(colour);
        colourArray->push_back(colour);
    }

    addVertices(*vertexArray, *colourArray);
}

void LinesOverlay::setLine(const LinesOverlay::Line& line, const Colour& colour)
{
    setLines({line}, {colour});
}

void LinesOverlay::setLine(const LinesOverlay::Line& line)
{
    setLine(line, _GeometryColour);
}

void LinesOverlay::addLine(const LinesOverlay::Line& line, const Colour& colour)
{
    addLines({line}, {colour});
}

void LinesOverlay::addLine(const LinesOverlay::Line& line)
{
    addLine({line}, _GeometryColour);
}

void LinesOverlay::setName(const std::string& name)
{
    /* Set the name on the base class */
    GeometryOverlay::setName(name);

    /* Set the name of the other osg::Nodes that makes up this overlay */
    if (_Geom.valid() == true)
        _Geom->setName(name);

    if (_Geode.valid() == true)
        _Geode->setName(name);

    if (_DrawArray.valid() == true)
        _DrawArray->setName(name);
}
