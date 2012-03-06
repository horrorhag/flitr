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

#ifndef FLITR_PLOT2D_OVERLAY_H
#define FLITR_PLOT2D_OVERLAY_H 1

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
 * Plot2D overlay.
 */
class FLITR_EXPORT Plot2DOverlay : public GeometryOverlay
{
  public:
    /** 
     * Create a new plot2D overlay.
     * 
     *
     */
    Plot2DOverlay(const double x, const double y, const double width, const double height, 
                  const double axisU, const double axisV);

    virtual ~Plot2DOverlay() {};

    virtual void setAxisColour(osg::Vec4d newcol);

    virtual void addPoint(const double u, const double v);
    virtual void clearPoints();

  private:
    void create(const double x, const double y, const double width, const double height, 
                const double axisU, const double axisV);
    void update();

    osg::ref_ptr<osg::Vec3Array> _axisVertices;
    osg::ref_ptr<osg::Vec3Array> _plotVertices;

    std::vector< std::pair<double, double> > plot_;

    double x_;    
    double y_;    
    double width_;    
    double height_;    
    double axisU_;    
    double axisV_;    

    osg::ref_ptr<osg::Geode> _AxisGeode;
    osg::ref_ptr<osg::StateSet> _AxisGeometryStateSet;
    osg::ref_ptr<osg::Material> _AxisGeometryMaterial;
    osg::Vec4d _AxisGeometryColour;

    osg::ref_ptr<osg::Geode> _PlotGeode;
};

}

#endif // FLITR_PLOT2D_OVERLAY_H
