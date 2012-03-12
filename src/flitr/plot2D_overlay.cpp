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
    _axisBckVertices = new osg::Vec3Array;
    _axisFrntVertices = new osg::Vec3Array;

    _plotVertices = new osg::Vec3Array;

    create(x_, y_, width_, height_, axisU_, axisV_);

    dirtyBound();

    setColour(osg::Vec4d(0.7,1.0,0.7,0.5));
    setAxisColour(osg::Vec4d(0.7,1.0,0.7,0.75));
}

void Plot2DOverlay::create(const double x, const double y, const double width, const double height, 
                const double axisU, const double axisV)
{
//=====================//
//=== Plot geometry ===//
    _PlotGeode = new osg::Geode();
    _GeometryGroup->addChild(_PlotGeode.get());
    _PlotGeode->setCullingActive(false);
	
    update();//Update also called when plot modified.
    
	
//=====================//

//=====================//
//=== Axis geometry ===//
    _AxisBckGeode = new osg::Geode();
    _AxisBckGeode->setCullingActive(false);

    osg::ref_ptr<osg::Geometry> axisBckGeometry = new osg::Geometry();

    _AxisBckGeometryStateSet = _AxisBckGeode->getOrCreateStateSet();
    _AxisBckGeometryMaterial = new osg::Material();
    _AxisBckGeometryStateSet->setAttributeAndModes(_AxisBckGeometryMaterial.get(), osg::StateAttribute::ON);

    _axisBckVertices->push_back(osg::Vec3d(x - 0.05*width,         y - 0.05*height,          0.0));
    _axisBckVertices->push_back(osg::Vec3d(x + 0.05*width + width, y - 0.05*height,          0.0));
    _axisBckVertices->push_back(osg::Vec3d(x + 0.05*width + width, y + 0.05*height + height, 0.0));
    _axisBckVertices->push_back(osg::Vec3d(x - 0.05*width,         y + 0.05*height + height, 0.0));

    axisBckGeometry->setVertexArray(_axisBckVertices.get());
    
    osg::ref_ptr<osg::DrawArrays> axisDABck=new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, _axisBckVertices->size());
    axisBckGeometry->addPrimitiveSet(axisDABck.get());

    axisBckGeometry->setUseDisplayList(false);

    _AxisBckGeode->addDrawable(axisBckGeometry.get());
    _axisBckVertices->dirty();


	
    _AxisFrntGeode = new osg::Geode();
    _AxisFrntGeode->setCullingActive(false);

    osg::ref_ptr<osg::Geometry> axisFrntGeometry = new osg::Geometry();

    _AxisFrntGeometryStateSet = _AxisFrntGeode->getOrCreateStateSet();
    _AxisFrntGeometryMaterial = new osg::Material();
    _AxisFrntGeometryStateSet->setAttributeAndModes(_AxisFrntGeometryMaterial.get(), osg::StateAttribute::ON);

    _axisFrntVertices->push_back(osg::Vec3d(x + 0.0*width, y,                   0.0));
    _axisFrntVertices->push_back(osg::Vec3d(x + 0.0*width, y + height,          0.0));
    _axisFrntVertices->push_back(osg::Vec3d(x + 0.25*width, y,                   0.0));
    _axisFrntVertices->push_back(osg::Vec3d(x + 0.25*width, y + height,          0.0));
    _axisFrntVertices->push_back(osg::Vec3d(x + 0.5*width, y,                   0.0));
    _axisFrntVertices->push_back(osg::Vec3d(x + 0.5*width, y + height,          0.0));
    _axisFrntVertices->push_back(osg::Vec3d(x + 0.75*width, y,                   0.0));
    _axisFrntVertices->push_back(osg::Vec3d(x + 0.75*width, y + height,          0.0));
    _axisFrntVertices->push_back(osg::Vec3d(x + 1.0*width, y,                   0.0));
    _axisFrntVertices->push_back(osg::Vec3d(x + 1.0*width, y + height,          0.0));

    _axisFrntVertices->push_back(osg::Vec3d(x,         y + 0.0*height,          0.0));
    _axisFrntVertices->push_back(osg::Vec3d(x + width, y + 0.0*height,          0.0));
    _axisFrntVertices->push_back(osg::Vec3d(x,         y + 0.25*height,          0.0));
    _axisFrntVertices->push_back(osg::Vec3d(x + width, y + 0.25*height,          0.0));
    _axisFrntVertices->push_back(osg::Vec3d(x,         y + 0.5*height,          0.0));
    _axisFrntVertices->push_back(osg::Vec3d(x + width, y + 0.5*height,          0.0));
    _axisFrntVertices->push_back(osg::Vec3d(x,         y + 0.75*height,          0.0));
    _axisFrntVertices->push_back(osg::Vec3d(x + width, y + 0.75*height,          0.0));
    _axisFrntVertices->push_back(osg::Vec3d(x,         y + 1.0*height,          0.0));
    _axisFrntVertices->push_back(osg::Vec3d(x + width, y + 1.0*height,          0.0));

    axisFrntGeometry->setVertexArray(_axisFrntVertices.get());
    
    osg::ref_ptr<osg::DrawArrays> axisDAFrnt=new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, _axisFrntVertices->size());
    axisFrntGeometry->addPrimitiveSet(axisDAFrnt.get());

    axisFrntGeometry->setUseDisplayList(false);

    _AxisFrntGeode->addDrawable(axisFrntGeometry.get());
    _axisFrntVertices->dirty();

    _GeometryGroup->addChild(_AxisFrntGeode.get());
    _GeometryGroup->addChild(_AxisBckGeode.get());

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
    _AxisFrntGeometryColour = newcol;
    _AxisFrntGeometryMaterial->setAmbient(osg::Material::FRONT_AND_BACK, _AxisFrntGeometryColour);
    _AxisFrntGeometryMaterial->setDiffuse(osg::Material::FRONT_AND_BACK, _AxisFrntGeometryColour);

    _AxisBckGeometryColour = newcol * 0.5;
    _AxisBckGeometryColour._v[3]=newcol._v[3];
    _AxisBckGeometryMaterial->setAmbient(osg::Material::FRONT_AND_BACK, _AxisBckGeometryColour);
    _AxisBckGeometryMaterial->setDiffuse(osg::Material::FRONT_AND_BACK, _AxisBckGeometryColour);
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

