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

#include <flitr/modules/geometry_overlays/geometry_overlay.h>

#include <boost/cstdint.hpp>

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
                  const double axisU, const double axisV, const boost::uint32_t numPlots=1);

    virtual ~Plot2DOverlay() {};

    virtual void setAxisColour(osg::Vec4d newcol);

    //Note: One may use the inherited setColour(...) method if all plots should have the smae colour.
    virtual void setPlotColour(osg::Vec4d newcol, uint32_t plotNum=0);

    virtual void addPoint(const double u, const double v, bool autoUpdate=true, uint32_t plotNum=0);
    virtual void clearPoints(bool autoUpdate=true, uint32_t plotNum=0);
    void update();

  private:
    void create(const double x, const double y, const double width, const double height, 
                const double axisU, const double axisV);


    std::vector< osg::ref_ptr<osg::Vec3Array> > _plotVertices;

    std::vector< std::vector< std::pair<double, double> > > plots_;

    double x_;    
    double y_;    
    double width_;    
    double height_;    
    double axisU_;    
    double axisV_;    

    osg::ref_ptr<osg::Geode> _AxisFrntGeode;
    osg::ref_ptr<osg::StateSet> _AxisFrntGeometryStateSet;
    osg::ref_ptr<osg::Material> _AxisFrntGeometryMaterial;
    osg::Vec4d _AxisFrntGeometryColour;
    osg::ref_ptr<osg::Vec3Array> _axisFrntVertices;

    osg::ref_ptr<osg::Geode> _AxisBckGeode;
    osg::ref_ptr<osg::StateSet> _AxisBckGeometryStateSet;
    osg::ref_ptr<osg::Material> _AxisBckGeometryMaterial;
    osg::Vec4d _AxisBckGeometryColour;
    osg::ref_ptr<osg::Vec3Array> _axisBckVertices;

    std::vector< osg::ref_ptr<osg::Geode> > _PlotGeodes;
    std::vector< osg::ref_ptr<osg::Material> > _PlotGeometryMaterials;
};

}

#endif // FLITR_PLOT2D_OVERLAY_H
