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

#include <flitr/geometry_overlay.h>

using namespace flitr;

GeometryOverlay::GeometryOverlay()
{
    getOrCreateStateSet()->setMode(GL_LIGHTING,osg::StateAttribute::OFF);
    getOrCreateStateSet()->setMode(GL_BLEND,osg::StateAttribute::ON);

    _CoordinateTransform = new osg::MatrixTransform();
    _DepthTransform = new osg::MatrixTransform();
    _GeometryGroup = new osg::Group();

    addChild(_DepthTransform.get());
    _DepthTransform->addChild(_CoordinateTransform.get());
    _CoordinateTransform->addChild(_GeometryGroup.get());
    
    _GeometryMaterial = new osg::Material();
    _GeometryStateSet = _GeometryGroup->getOrCreateStateSet();
    _GeometryStateSet->setAttributeAndModes(_GeometryMaterial.get(), osg::StateAttribute::ON);
    
    _GeometryLinewidth = new osg::LineWidth();
    _GeometryLinewidth->setWidth(1);
    _GeometryStateSet->setAttributeAndModes(_GeometryLinewidth.get(),osg::StateAttribute::ON);

    setColour(osg::Vec4d(1,1,0,0.5));

    setAllChildrenOn();

    setDepth(0.1);
}

void GeometryOverlay::setDepth(double z)
{
    osg::Matrixd d;
    d.makeTranslate(0,0,z);
    _DepthTransform->setMatrix(d);

    dirtyBound();
}

void GeometryOverlay::setLineWidth(double w)
{
    _GeometryLinewidth->setWidth(w);
}

void GeometryOverlay::setColour(osg::Vec4d newcol)
{
    _GeometryColour = newcol;
    _GeometryMaterial->setAmbient(osg::Material::FRONT_AND_BACK, _GeometryColour);
    _GeometryMaterial->setDiffuse(osg::Material::FRONT_AND_BACK, _GeometryColour);
}

void GeometryOverlay::setShow(bool show)
{
    if (show) {
        setAllChildrenOn();
    } else {
        setAllChildrenOff();
    }
}

void GeometryOverlay::setCoordinateTransform(osg::Matrixd m)
{
    _CoordinateTransform->setMatrix(m);

    dirtyBound();
}

void GeometryOverlay::flipVerticalCoordinates(double im_height)
{
    osg::Matrixd r;
    osg::Matrixd t;
    t.makeTranslate(osg::Vec3d(0,im_height,0));
    r.makeRotate(osg::DegreesToRadians(180.0), osg::Vec3d(1,0,0));
    // if viewed from a coordinate's perspective, translate up z and then flip around x
    setCoordinateTransform(r*t);
}
