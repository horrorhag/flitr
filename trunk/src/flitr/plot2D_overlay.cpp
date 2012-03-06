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

#include <flitr/plot2D_overlay.h>

using namespace flitr;

Plot2DOverlay::Plot2DOverlay(const double x, const double y, const double width, const double height, 
                const double axisU, const double axisV) :
    GeometryOverlay(),
    x_(x),
    y_(y),
    width_(width),
    height_(height),
    axisU_(axisU),
    axisV_(axisV)
{
    _axisVertices = new osg::Vec3Array;
    _plotVertices = new osg::Vec3Array;

    create(x_, y_, width_, height_, axisU_, axisV_);

    dirtyBound();

    setColour(osg::Vec4d(0.7,1.0,0.7,0.5));
    setAxisColour(osg::Vec4d(0.3,0.3,0.3,0.75));
}

void Plot2DOverlay::create(const double x, const double y, const double width, const double height, 
                const double axisU, const double axisV)
{
//=====================//
//=== Plot geometry ===//
    _PlotGeode = new osg::Geode();
    _GeometryGroup->addChild(_PlotGeode.get());
    _PlotGeode->setCullingActive(false);
	
    update();
    
	
//=====================//

//=====================//
//=== Axis geometry ===//
    _AxisGeode = new osg::Geode();
    _GeometryGroup->addChild(_AxisGeode.get());
    _AxisGeode->setCullingActive(false);

    osg::ref_ptr<osg::Geometry> axisGeometry = new osg::Geometry();

    _AxisGeometryStateSet = _AxisGeode->getOrCreateStateSet();
    _AxisGeometryMaterial = new osg::Material();
    _AxisGeometryColour=osg::Vec4d(0.3, 0.3, 0.3, 0.75);

    //Create axis.
    _axisVertices->push_back(osg::Vec3d(x - 0.05*width,         y - 0.05*height,          0.0));
    _axisVertices->push_back(osg::Vec3d(x + 0.05*width + width, y - 0.05*height,          0.0));
    _axisVertices->push_back(osg::Vec3d(x + 0.05*width + width, y + 0.05*height + height, 0.0));
    _axisVertices->push_back(osg::Vec3d(x - 0.05*width,         y + 0.05*height + height, 0.0));

    axisGeometry->setVertexArray(_axisVertices.get());
    
    osg::ref_ptr<osg::DrawArrays> axisDA=new osg::DrawArrays(osg::PrimitiveSet::QUADS,0, _axisVertices->size());
    axisGeometry->addPrimitiveSet(axisDA.get());
    axisGeometry->setUseDisplayList(false);

    _AxisGeode->addDrawable(axisGeometry.get());
	
    _axisVertices->dirty();
    dirtyBound();
//=====================//
}

void Plot2DOverlay::update()
{
    _PlotGeode->removeDrawables(0,1);
    _plotVertices->clear();

    osg::ref_ptr<osg::Geometry> plotGeometry=new osg::Geometry();

    //Create the plot.
    std::vector< std::pair<double, double> >::const_iterator plotIterator=plot_.begin();
    for (; plotIterator!=plot_.end(); plotIterator++)
    {
      _plotVertices->push_back(osg::Vec3d(plotIterator->first*width_/axisU_+x_, plotIterator->second*height_/axisV_+y_, 0));
    }

    plotGeometry->setVertexArray(_plotVertices.get());

    osg::ref_ptr<osg::DrawArrays> plotDA=new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP,0,_plotVertices->size());
    plotGeometry->addPrimitiveSet(plotDA.get());
    plotGeometry->setUseDisplayList(false);

    _PlotGeode->addDrawable(plotGeometry.get());
    
    _plotVertices->dirty();
    dirtyBound();
}

void Plot2DOverlay::setAxisColour(osg::Vec4d newcol)
{
    _AxisGeometryColour = newcol;
    _AxisGeometryMaterial->setAmbient(osg::Material::FRONT_AND_BACK, _AxisGeometryColour);
    _AxisGeometryMaterial->setDiffuse(osg::Material::FRONT_AND_BACK, _AxisGeometryColour);

    _AxisGeometryStateSet->setAttributeAndModes(_AxisGeometryMaterial.get(), osg::StateAttribute::ON);
}


void Plot2DOverlay::addPoint(const double u, const double v) 
{
  plot_.push_back(std::pair<double, double>(u, v));
  update();
}


void Plot2DOverlay::clearPoints() 
{
  plot_.clear();
  update();
}

