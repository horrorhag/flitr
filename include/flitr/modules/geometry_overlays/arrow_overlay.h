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

#ifndef FLITR_ARROW_OVERLAY_H
#define FLITR_ARROW_OVERLAY_H 1

#include <flitr/modules/geometry_overlays/geometry_overlay.h>

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

#define _USE_MATH_DEFINES
#include <cmath>

namespace flitr {

/**
 * Filled or outlined arrow with a given tip position, width and height.
 *
 * The arrow will always be positioned upwards.
 */
class FLITR_EXPORT ArrowOverlay : public GeometryOverlay
{
  public:
    /**
     * Create a new arrow overlay.
     *
     * \param tip_x The x position of the arrow tip point.
     * \param tip_y The y position of the arrow tip point.
     * \param width Width of the base of the arrow head.
     * \param height Height of the arrow, from tip point to end of tail.
     * \param filled If the arrow head should be filled or not.
     */
    ArrowOverlay(double tip_x, double tip_y, double headWidth, double headHeight, double tailWidth, double tailLength, bool filled=false, double rotationDegrees=0);
    virtual ~ArrowOverlay();

    /**
     * Set the tip point of the arrow.
     * \param x The x position of the arrow tip point.
     * \param y The y position of the arrow tip point.
     */
    void setTipPoint(double x, double y);
    /**
     * Get the current tip point position of the arrow.
     * \param x The x position of the arrow tip point.
     * \param y The y position of the arrow tip point.
     */
    void getTipPoint(double &x, double &y);

    /**
     * Set the width or base of the arrow head.
     */
    void setHeadWidth(double width);
    /**
     * Set the height of the arrow head.
     */
    void setHeadHeight(double height);
    /**
     * Set the thickness of the arrow tail.
     */
    void setTailWidth(double width);
    /**
     * Set the length of the arrow tail.
     */
    void setTailLength(double length);
    /**
     * Set the rotation of the arrow in degrees.
     */
    void setRotation(double rotationDegrees);

    /**
     * Get the width or base of the arrow head.
     */
    double getHeadWidth() const;
    /**
     * Get the height of the arrow head.
     */
    double getHeadHeight() const;
    /**
     * Get the thickness of the arrow tail.
     */
    double getTailWidth() const;
    /**
     * Get the length of the arrow tail.
     */
    double getTailLength() const;

    /**
     * Set the name of the overlay along with the nodes that make up
     * the overlay.
     *
     * This is normally used when geometry intersections are required.
     *
     * \param name New name of the overlay
     */
    virtual void setName(const std::string& name);

  private:
    void makeArrow(bool filled);
    void updateArrow();
    osg::ref_ptr<osg::Vec3Array> _Vertices;
    osg::ref_ptr<osg::Geode> _Geode;
    osg::ref_ptr<osg::Geometry> _Geom;
    osg::ref_ptr<osg::DrawArrays> _DrawArray;

    double _TipX;
    double _TipY;
    double _HeadWidth;
    double _HeadHeight;
    double _TailWidth;
    double _TailLength;

    double _RotationDegrees;
};

}

#endif // FLITR_ARROW_OVERLAY_H
