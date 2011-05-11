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

#ifndef FLITR_POINTS_OVERLAY_H
#define FLITR_POINTS_OVERLAY_H 1

#include <flitr/geometry_overlay.h>

#include <osg/ref_ptr>
#include <osg/Node>
#include <osg/Group>
#include <osg/Switch>
#include <osg/Geometry>
#include <osg/PositionAttitudeTransform>
#include <osg/MatrixTransform>
#include <osg/Projection>
#include <osg/Geode>
#include <osg/Vec4d>
#include <osg/Material>
#include <osg/LineWidth>

namespace flitr {

/**
 * Array of points of the same colour drawn as an overlay.
 */
class FLITR_EXPORT PointsOverlay : public GeometryOverlay
{
  public:
    PointsOverlay();
    ~PointsOverlay() {};

    /** 
     * Draw points at all locations given.
     * 
     * \param v Array of 3D points.
     */
    void setVertices(osg::Vec3Array& v);

    /** 
     * Add a single point to the existing list.
     * 
     * \param v Single 3D location.
     */
    void addVertex(osg::Vec3d v);

    /** 
     * Clear the list of points to draw.
     */
    void clearVertices();

  private:
    void makeGraph();
    osg::ref_ptr<osg::Vec3Array> _Vertices;
    osg::ref_ptr<osg::DrawArrays> _DrawArray;
    double _PointSize;
};

}

#endif // FLITR_POINTS_OVERLAY_H
