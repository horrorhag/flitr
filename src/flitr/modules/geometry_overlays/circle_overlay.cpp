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

#include <flitr/modules/geometry_overlays/circle_overlay.h>

#include <stdio.h>
#include <iostream>

using namespace flitr;

CircleOverlay::CircleOverlay(double center_x, double center_y, double radius, unsigned int numPoints, bool filled) :
    GeometryOverlay(),
    _CenterX(center_x),
    _CenterY(center_y),
    _Radius(radius),
    _NumberPoints((static_cast<unsigned int>(numPoints) / static_cast<unsigned int>(2)) * 2) /* Getting even number of points */
{
    /* Create the Vertices array with the specified number of points for the circle.
     * The Number of points are increased with one to resolve a destruction issue with the overlay.
     * This is a hack, without the '+1' this overlay causes a segmentation fault if destructed
     * with the following message: double free or corruption */
    _Vertices = new osg::Vec3Array(_NumberPoints + 1);

    makeQuad(filled);

    dirtyBound();

    setColour(osg::Vec4d(1,1,0,0.5));
}

CircleOverlay::~CircleOverlay()
{
}

void CircleOverlay::makeQuad(bool filled)
{
    _Geode = new osg::Geode();
    _Geom = new osg::Geometry();

    updateQuad();
    
    _Geom->setVertexArray(_Vertices.get());
    
    if (!filled) {
        _DrawArray = new osg::DrawArrays(osg::PrimitiveSet::LINE_LOOP, 0, _NumberPoints);
    } else {
        _DrawArray = new osg::DrawArrays(osg::PrimitiveSet::POLYGON, 0, _NumberPoints);
    }
    _Geom->addPrimitiveSet(_DrawArray.get());
    _Geom->setUseDisplayList(false);
    _Geom->setCullingActive(false);

    _Geode->addDrawable(_Geom.get());
    _Geode->setCullingActive(false);

    _GeometryGroup->addChild(_Geode.get());
    _GeometryGroup->setCullingActive(false);
}

void CircleOverlay::updateQuad()
{
    /* Draw a circle using the normal circle formula. This function
     * is optimised as far as possible since it can be called many
     * times for a large number of points.
     *
     * The vector is not cleared each time but the values are updated
     * in place in the vector. This is to reduce the overhead with
     * pushin values into the vector. It also allows one to add the points
     * from both sides at once to create the circle. */
    double range = _NumberPoints / 2.0;
    double begin = _CenterY - _Radius;
    double end = _CenterY + _Radius;
    double step = (end - begin) / range;
    double y;
    double temp;
    double x1;
    double x2;
    int index;
    /* It is only needed to calculate half the number of points since
     * 2 points are acros each other on the y axis. */
    for(y = begin, index = 0; index <= range; y += step, ++index) {
        /* Step 1. Calculate the square root. The values are multiplied with
         * each other since the pow() function can be slower (benchmarked
         * using Qt). */
        temp = sqrt((_Radius * _Radius) - (y - _CenterY)*(y - _CenterY));
        /* Step 2. Calculate the 2 points on the y axis based on the value
         * calculated in step 2. */
        x1 = _CenterX + temp;
        x2 = _CenterX - temp;
        /* Step 3. Update the point from the beginning of the array. */
        (*_Vertices)[index] = osg::Vec3d(x1, y, 0);
        /* Step 4. Update the adjecent point from the end of the array. */
        (*_Vertices)[_NumberPoints - index] = osg::Vec3d(x2, y, 0);
    }
    /* If the last value of x is equal to the end of the circle (_CenterX +
     * end) then in Step 1 above the square root of 0 (zero) would be
     * calculated. The result of this is NaN# and thus the values set for
     * the last points calculated (middel of the circle) will also be NaN.
     * In this case these values are set to 0 to make sure that the circle
     * is drawn correctly.
     *
     * This check is done outside of the for loop again for optimisation
     * since checking this in each iteration can be time consuming and it
     * will only happen with the last points.
     *
     * Without this there is a gap in the circle.
     *
     * # - MinGW 3.81 */
    if(isnan(temp) == true) {
        (*_Vertices)[index] = osg::Vec3d(_CenterX, y, 0);
        (*_Vertices)[_NumberPoints - index] = osg::Vec3d(_CenterX, y, 0);
    }
    _Vertices->dirty();

    dirtyBound();
}

void CircleOverlay::setCenter(double x, double y)
{
    _CenterX = x;
    _CenterY = y;
    updateQuad();
}

void CircleOverlay::getCenter(double &x, double &y)
{
    x = _CenterX;
    y = _CenterY;
}

void CircleOverlay::setRadius(double radius)
{
    _Radius = radius;
    updateQuad();
}

double CircleOverlay::getRadius() const
{
    return _Radius;
}

void CircleOverlay::setName(const std::string &name)
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

