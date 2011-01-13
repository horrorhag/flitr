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

#ifndef FLITR_GEOMETRY_OVERLAY_H
#define FLITR_GEOMETRY_OVERLAY_H 1

#include <flitr/flitr_export.h>

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
#include <osg/Point>

namespace flitr {

/**
 * Base for geometric overlays to be drawn onto textures. 
 */
class FLITR_EXPORT GeometryOverlay : public osg::Switch
{
  public:
    GeometryOverlay();
    virtual ~GeometryOverlay() {};
    virtual void setColour(osg::Vec4d newcol);

    /** 
     * Coordinates of the overlay would be transformed by the given
     * matrix before being drawn.
     * 
     * \param m Matrix to typically contain scale and translation. 
     */
    virtual void setCoordinateTransform(osg::Matrixd m);

    /** 
     * Utility function to set the coordinate system for the common
     * case where an image with (0,0) at the top-left is drawn onto a
     * quad in the X-Y plane with (0,0) at the bottom left.
     * 
     * \param im_height The image height used to flip the values.
     */
    virtual void flipVerticalCoordinates(double im_height);
    
    /** 
     * Adjust the depth the geometry is drawn at. Can be used to layer
     * multiple overlays. In the default state positive z is towards
     * the camera. The default depth value is set to 0.1. The
     * transform in z is applied after the coordinate transform.
     * 
     * \param z Value to adjust the depth of the geometry with.
     */
    virtual void setDepth(double z);
    
    virtual void setLineWidth(double w);

    virtual void setPointSize(double s);

    /**
     *  \brief Toggle the display of the quad on and off. 
     *  \param show True to turn on.
     */
    void setShow(bool show);

  protected:
    /// Matrix to make sure our coordinate system matches the geometry being draw on.
    osg::ref_ptr<osg::MatrixTransform> _CoordinateTransform;
    osg::ref_ptr<osg::MatrixTransform> _DepthTransform;
    
    osg::ref_ptr<osg::Group> _GeometryGroup;
    osg::ref_ptr<osg::StateSet> _GeometryStateSet;
    osg::ref_ptr<osg::Material> _GeometryMaterial;
    osg::ref_ptr<osg::LineWidth> _GeometryLineWidth;
    osg::ref_ptr<osg::Point> _GeometryPointSize;
    osg::Vec4d _GeometryColour;
};

}

#endif // FLITR_GEOMETRY_OVERLAY_H
