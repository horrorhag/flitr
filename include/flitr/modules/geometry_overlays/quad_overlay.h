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

#ifndef FLITR_QUAD_OVERLAY_H
#define FLITR_QUAD_OVERLAY_H 1

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

namespace flitr {

/**
 * Filled or outline quad with a given center, width and height.
 */
class FLITR_EXPORT QuadOverlay : public GeometryOverlay
{
  public:
    /** 
     * Create a new quad overlay.
     * 
     * \param center_x The x position of the quad center.
     * \param center_y The y position of the quad center.
     * \param width Width of the quad.
     * \param height Height of the quad.
     * \param filled Whether the quad should be filled or not.
     *
     */
    QuadOverlay(double center_x, double center_y, double width, double height, bool filled=false);
    virtual ~QuadOverlay() {};

    /** 
     * Adjusts the position of the quad.
     * 
     * \param x The new x position of the quad center.
     * \param y The new y position of the quad center.
     */
    void setCenter(double x, double y);
    void getCenter(double &x, double &y);

    /** 
     * Adjusts the width of the quad.
     * 
     * \param width New width.
     */
    void setWidth(double width);

    /** 
     * Adjusts the height of the quad.
     * 
     * \param height New height.
     */
    void setHeight(double height);

    double getWidth() const;
    double getHeight() const;

    /**
     * Set the name of the overlay along with the nodes that make up
     * the overlay
     *
     * \param name New name of the overlay
     */
    virtual void setName(const std::string& name);

  private:
    void makeQuad(bool filled);
    void updateQuad();
    osg::ref_ptr<osg::Vec3Array> _Vertices;
    osg::ref_ptr<osg::Geode> _Geode;
    osg::ref_ptr<osg::Geometry> _Geom;
    osg::ref_ptr<osg::DrawArrays> _DrawArray;

    double _CenterX;
    double _CenterY;
    double _Width;
    double _Height;
};

}

#endif // FLITR_QUAD_OVERLAY_H
