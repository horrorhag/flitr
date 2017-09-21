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
#include <stdint.h>
#include <flitr/modules/geometry_overlays/tape_overlay.h>

using namespace flitr;

TapeOverlay::TapeOverlay(double center_x, double center_y, double length, uint8_t majorTicks, uint8_t minorTicks,  bool indicator, Orientation layout) :
    GeometryOverlay(),
    _CenterX(center_x),
    _CenterY(center_y),
    _Length(length),
    _MajorTicks(majorTicks),
    _MinorTicks(minorTicks),
    _Offset(0),
    _Orientation(layout)
{
    _MajorTicks = _MajorTicks >= 3 ? _MajorTicks : 3;
    _MajorTicks = _MajorTicks % 2 ? _MajorTicks : _MajorTicks + 1;
//#if 0
   _MinorTicks = _MinorTicks > 0 ? (_MinorTicks % 2 ? _MinorTicks + 1 : _MinorTicks) : _MinorTicks;
//#endif
   //_MinorTicks = _MinorTicks > 0 ? _MajorTicks - 1 : 0;
    _Vertices = new osg::Vec3Array(2*(1+_MajorTicks+_MinorTicks));

    _MajorTickHeight = 10;
    _MinorTickHeight = 10;

    _MajorTickSpacing = _Length/(_MajorTicks - 1);

    makeTape(indicator);

    dirtyBound();

    setColour(osg::Vec4d(1,1,0,0.5));
}

TapeOverlay::~TapeOverlay()
{

}

void TapeOverlay::makeTape(bool indicator)
{
    _Geode = new osg::Geode();
    _Geom = new osg::Geometry();

    updateTape();

    _Geom->setVertexArray(_Vertices.get());

    _DrawArray = new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, 2*(1+_MajorTicks+_MinorTicks));

    _Geom->addPrimitiveSet(_DrawArray.get());
    _Geom->setUseDisplayList(false);
    _Geom->setCullingActive(false);

    _Geode->addDrawable(_Geom.get());
    _Geode->setCullingActive(false);

    _GeometryGroup->addChild(_Geode.get());
    _GeometryGroup->setCullingActive(false);
}

void TapeOverlay::updateTape()
{
    _Vertices->clear();
    if (_Orientation == Orientation::HORIZONTAL) {
        _Vertices->push_back(osg::Vec3d(_CenterX-(_Length/2), _CenterY, 0));
        _Vertices->push_back(osg::Vec3d(_CenterX+(_Length/2), _CenterY, 0));

        double tickCenterX = _CenterX - _Offset*_MajorTickSpacing;

        for (uint8_t i=1; i<=(_MajorTicks-1)/2; ++i)
        {
            if (tickCenterX - i*_MajorTickSpacing > _CenterX - _Length/2) {
                _Vertices->push_back(osg::Vec3d(tickCenterX - i*_MajorTickSpacing, _CenterY + _MajorTickHeight, 0));
                _Vertices->push_back(osg::Vec3d(tickCenterX - i*_MajorTickSpacing, _CenterY                   , 0));
            } else {
                _Vertices->push_back(osg::Vec3d(tickCenterX, _CenterY , 0));
                _Vertices->push_back(osg::Vec3d(tickCenterX, _CenterY , 0));
            }

            if (tickCenterX + i*_MajorTickSpacing < _CenterX + _Length/2) {
                _Vertices->push_back(osg::Vec3d(tickCenterX + i*_MajorTickSpacing, _CenterY + _MajorTickHeight, 0));
                _Vertices->push_back(osg::Vec3d(tickCenterX + i*_MajorTickSpacing, _CenterY                   , 0));
            } else {
                _Vertices->push_back(osg::Vec3d(tickCenterX, _CenterY , 0));
                _Vertices->push_back(osg::Vec3d(tickCenterX, _CenterY , 0));
            }
        }
        _Vertices->push_back(osg::Vec3d(tickCenterX, _CenterY + _MajorTickHeight, 0));
        _Vertices->push_back(osg::Vec3d(tickCenterX, _CenterY                   , 0));

        if (_MinorTicks > 0) {
            double tickSpacing = _Length/_MinorTicks;
            for (uint8_t i=1; i<=_MinorTicks/2; ++i)
            {
                _Vertices->push_back(osg::Vec3d(tickCenterX - (i-0.5)*tickSpacing, _CenterY - _MinorTickHeight, 0));
                _Vertices->push_back(osg::Vec3d(tickCenterX - (i-0.5)*tickSpacing, _CenterY                   , 0));
                _Vertices->push_back(osg::Vec3d(tickCenterX + (i-0.5)*tickSpacing, _CenterY - _MinorTickHeight, 0));
                _Vertices->push_back(osg::Vec3d(tickCenterX + (i-0.5)*tickSpacing, _CenterY                   , 0));
            }
        }
    } else {
        _Vertices->push_back(osg::Vec3d(_CenterX, _CenterY-(_Length/2), 0));
        _Vertices->push_back(osg::Vec3d(_CenterX, _CenterY+(_Length/2), 0));

        double tickCenterY = _CenterY - _Offset*_MajorTickSpacing;

        for (uint8_t i=1; i<=(_MajorTicks-1)/2; ++i)
        {
            if (tickCenterY - i*_MajorTickSpacing > _CenterY - _Length/2) {
                _Vertices->push_back(osg::Vec3d(_CenterX - _MajorTickHeight, tickCenterY - i*_MajorTickSpacing,  0));
                _Vertices->push_back(osg::Vec3d(_CenterX                   , tickCenterY - i*_MajorTickSpacing,  0));
            } else {
                _Vertices->push_back(osg::Vec3d(_CenterX, tickCenterY, 0));
                _Vertices->push_back(osg::Vec3d(_CenterX, tickCenterY, 0));
            }

            if (tickCenterY + i*_MajorTickSpacing < _CenterY + _Length/2) {
                _Vertices->push_back(osg::Vec3d(_CenterX - _MajorTickHeight, tickCenterY + i*_MajorTickSpacing, 0));
                _Vertices->push_back(osg::Vec3d(_CenterX                   , tickCenterY + i*_MajorTickSpacing, 0));
            } else {
                _Vertices->push_back(osg::Vec3d(_CenterX, tickCenterY, 0));
                _Vertices->push_back(osg::Vec3d(_CenterX, tickCenterY, 0));
            }
        }
        _Vertices->push_back(osg::Vec3d(_CenterX - _MajorTickHeight, tickCenterY, 0));
        _Vertices->push_back(osg::Vec3d(_CenterX                   , tickCenterY, 0));

        if (_MinorTicks > 0) {
            double tickSpacing = _Length/_MinorTicks;
            for (uint8_t i=1; i<=_MinorTicks/2; ++i)
            {
                _Vertices->push_back(osg::Vec3d(_CenterX + _MinorTickHeight, tickCenterY - (i-0.5)*tickSpacing, 0));
                _Vertices->push_back(osg::Vec3d(_CenterX                   , tickCenterY - (i-0.5)*tickSpacing, 0));
                _Vertices->push_back(osg::Vec3d(_CenterX + _MinorTickHeight, tickCenterY + (i-0.5)*tickSpacing, 0));
                _Vertices->push_back(osg::Vec3d(_CenterX                   , tickCenterY + (i-0.5)*tickSpacing, 0));
            }
        }
    }
    _Vertices->dirty();
    dirtyBound();
}

void TapeOverlay::setCenter(double x, double y)
{
    _CenterX = x;
    _CenterY = y;
    updateTape();
}

void TapeOverlay::getCenter(double &x, double &y)
{
    x = _CenterX;
    y = _CenterY;
}

void TapeOverlay::setLength(double length)
{
    _Length = length;
    updateTape();
}

double TapeOverlay::getLength() const
{
    return _Length;
}

void TapeOverlay::setMajorTickHeight(double height)
{
    _MajorTickHeight = height;
    updateTape();
}

void TapeOverlay::setMinorTickHeight(double height)
{
    _MinorTickHeight = height;
    updateTape();
}

void TapeOverlay::setOffset(double offset)
{
    offset = offset > 0.5 ? offset - 2*0.5 : offset < -0.5 ? offset + 2*0.5 : offset;
    _Offset = offset;
    updateTape();
}

double TapeOverlay::getOffset() const
{
    return _Offset;
}

uint8_t TapeOverlay::getMajorTicks() const
{
    return _MajorTicks;
}

double TapeOverlay::getMajorTickSpacing() const
{
    return _MajorTickSpacing;
}

uint8_t TapeOverlay::getMinorTicks() const
{
    return _MinorTicks;
}

void TapeOverlay::setName(const std::string &name)
{
    /* Set the name on the base class */
    GeometryOverlay::setName(name);

    /* Set the name of the other osg::Nodes */
    if (_Geom.valid() == true)
        _Geom->setName(name);

    if (_Geode.valid() == true)
        _Geode->setName(name);

    if (_DrawArray.valid() == true)
        _DrawArray->setName(name);
}
