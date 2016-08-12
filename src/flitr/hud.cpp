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

#include <flitr/hud.h>

flitr::HUD::HUD(osgViewer::View *view, const double minX, const double maxX, const double minY, const double maxY, const double near, const double far) :
    anchor_mode_(HUD_ANCHOR_CENTER),
    aspect_mode_(HUD_ASPECT_STRETCHED)
{
	setMinMax(minX, maxX, minY, maxY);
	
	//this->setProjectionMatrix(osg::Matrix::ortho2D(min_x_, max_x_, min_y_, max_y_));
    this->setProjectionMatrixAsOrtho(min_x_, max_x_, min_y_, max_y_, near, far);
    this->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
	
    resize_handler_ = new ResizeHandler(this);

    // set the view matrix    
    this->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    this->setViewMatrix(osg::Matrix::identity());

    // only clear the depth buffer
    this->setClearMask(GL_DEPTH_BUFFER_BIT);

    // draw subgraph after main camera view.
    this->setRenderOrder(osg::Camera::POST_RENDER);

    // we don't want the camera to grab event focus from the viewers main camera(s).
    this->setAllowEventFocus(false);

    setGraphicsContext(view->getCamera()->getGraphicsContext());
    osg::Viewport *vp=view->getCamera()->getViewport();
    setViewport(vp->x(), vp->y(), vp->width(), vp->height());

    view->addSlave(this, false);
    
    view->addEventHandler(getResizeHandler());
    
    this->setCullingMode(0);
}

flitr::HUD::~HUD()
{
}

