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

#include <osg/Quat>
#include <osg/Notify>
#include <osg/BoundsChecking>
#include <osgViewer/View>

#include <osg/io_utils>
#include <iostream>

#include <flitr/ortho_texture_manipulator.h>

using namespace osg;
using namespace osgGA;
using namespace flitr;

OrthoTextureManipulator::OrthoTextureManipulator(const double texture_width, const double texture_height)
{
    _init_ortho_params = true;

    _texture_aspect = texture_width/texture_height;
    _texture_width = texture_width;
    _texture_height = texture_height;

    _allowThrow = true;
    _thrown = false;

    _distance = 1.0f;
    _center = osg::Vec3d(0,0,0);
    _zoomDelta = 0.1f;
}

OrthoTextureManipulator::~OrthoTextureManipulator()
{
}

void OrthoTextureManipulator::initOrthoProjection(GUIActionAdapter& us)
{
    osgViewer::View* view = dynamic_cast<osgViewer::View*>(&us);
    if (view) {
        osg::Viewport* vp = view->getCamera()->getViewport();
        double view_aspect = vp->width()/vp->height();
        osg::Vec3d top_left(0,0,0);
        osg::Vec3d bottom_right(_texture_width, _texture_height, 0);
        osg::Vec3d center = (bottom_right + top_left)*0.5f;
        osg::Vec3d dx(bottom_right.x()-center.x(), 0.0f, 0.0f);
        osg::Vec3d dy(0.0f, top_left.y()-center.y(), 0.0f);
        double ratio = _texture_aspect/view_aspect;
        if (ratio>1.0f) {
            // use width as the control on size
            bottom_right = center + dx - dy * ratio;
            top_left = center - dx + dy * ratio;
        } else {
            // use height as the control on size
            bottom_right = center + dx / ratio - dy;
            top_left = center - dx / ratio + dy;
        }

        
        view->getCamera()->setProjectionMatrixAsOrtho(top_left.x(), bottom_right.x(), top_left.y(), bottom_right.y(), -1e6, 1e6);
        view->getCamera()->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
        view->getCamera()->setCullingActive(false);
        //std::cerr << "VP " << vp->x() << " " << vp->y() << " " << vp->width() << " " << vp->height() << "\n";
    }
}

void OrthoTextureManipulator::updateOrthoParams(GUIActionAdapter& us)
{
    osgViewer::View* view = dynamic_cast<osgViewer::View*>(&us);
    
    if (view) {
        double zNear;
        double zFar;
        view->getCamera()->getProjectionMatrixAsOrtho(_ortho_from_view_left,  _ortho_from_view_right,
                                                      _ortho_from_view_bottom,  _ortho_from_view_top,
                                                      zNear,  zFar);

        _ortho_from_view_width = _ortho_from_view_right - _ortho_from_view_left;
        _ortho_from_view_height = _ortho_from_view_top - _ortho_from_view_bottom;
        _zoom_offset = osg::Vec3d(
            _ortho_from_view_left + _ortho_from_view_width/2.0, 
            _ortho_from_view_bottom + _ortho_from_view_height/2.0,
            0 
            );
        if (_ortho_from_view_height > 0) {
            _current_aspect = _ortho_from_view_width/_ortho_from_view_height;
        }
    }
}

void OrthoTextureManipulator::setNode(osg::Node* node)
{
    _node = node;
}

const osg::Node* OrthoTextureManipulator::getNode() const
{
    return _node.get();
}

osg::Node* OrthoTextureManipulator::getNode()
{
    return _node.get();
}

void OrthoTextureManipulator::home(double /*currentTime*/)
{
    if (getAutoComputeHomePosition()) computeHomePosition();
    computePosition(_homeEye, _homeCenter, _homeUp);
    _thrown = false;
}

void OrthoTextureManipulator::home(const GUIEventAdapter& ea, GUIActionAdapter& us)
{
    home(ea.getTime());
    us.requestRedraw();
    us.requestContinuousUpdate(false);
}

void OrthoTextureManipulator::init(const GUIEventAdapter&, GUIActionAdapter& )
{
    flushMouseEventStack();
}

void OrthoTextureManipulator::getUsage(osg::ApplicationUsage& usage) const
{
    usage.addKeyboardMouseBinding("OrthoTextureManipulator: Space","Reset the viewing position to home");
}

bool OrthoTextureManipulator::handle(const GUIEventAdapter& ea, GUIActionAdapter& us)
{
    switch(ea.getEventType())
    {
      case(GUIEventAdapter::FRAME):
      {
          if (_init_ortho_params) {
              initOrthoProjection(us);
              updateOrthoParams(us);
              home(ea,us);
              _init_ortho_params = false;
          }

          double current_frame_time = ea.getTime();

          _delta_frame_time = current_frame_time - _last_frame_time;
          _last_frame_time = current_frame_time;

          if (_thrown && _allowThrow)
          {
              if (calcMovement()) us.requestRedraw();
          }
          return false;
      }
      default:
        break;
    }

    if (ea.getHandled()) return false;

    switch(ea.getEventType())
    {
      case(GUIEventAdapter::PUSH):
      {
          updateOrthoParams(us);
          flushMouseEventStack();
          addMouseEvent(ea);
          if (calcMovement()) us.requestRedraw();
          us.requestContinuousUpdate(false);
          _thrown = false;
          return true;
      }

      case(GUIEventAdapter::RELEASE):
      {
          if (ea.getButtonMask()==0)
          {
            
              double timeSinceLastRecordEvent = _ga_t0.valid() ? (ea.getTime() - _ga_t0->getTime()) : DBL_MAX;
              if (timeSinceLastRecordEvent>0.02) flushMouseEventStack();

              if (isMouseMoving())
              {
                  if (calcMovement())
                  {
                      us.requestRedraw();
                      us.requestContinuousUpdate(true);
                      _thrown = _allowThrow;
                  }
              }
              else
              {
                  flushMouseEventStack();
                  addMouseEvent(ea);
                  if (calcMovement()) us.requestRedraw();
                  us.requestContinuousUpdate(false);
                  _thrown = false;
              }

          }
          else
          {
              flushMouseEventStack();
              addMouseEvent(ea);
              if (calcMovement()) us.requestRedraw();
              us.requestContinuousUpdate(false);
              _thrown = false;
          }
          return true;
      }

      case(GUIEventAdapter::DRAG):
      case(GUIEventAdapter::SCROLL):
      {
          addMouseEvent(ea);
          if (calcMovement()) us.requestRedraw();
          us.requestContinuousUpdate(false);
          _thrown = false;
          return true;
      }

      case(GUIEventAdapter::MOVE):
      {
          return false;
      }

      case(GUIEventAdapter::KEYDOWN):
      {
          if (ea.getKey()== GUIEventAdapter::KEY_Space)
          {
              updateOrthoParams(us);
              flushMouseEventStack();
              _thrown = false;
              home(ea,us);
              return true;
          }
          return false;
      }
      default:
        return false;
    }
}

bool OrthoTextureManipulator::isMouseMoving()
{
    if (_ga_t0.get()==NULL || _ga_t1.get()==NULL) return false;

    static const float velocity = 0.1f;

    float dx = _ga_t0->getXnormalized()-_ga_t1->getXnormalized();
    float dy = _ga_t0->getYnormalized()-_ga_t1->getYnormalized();
    float len = sqrtf(dx*dx+dy*dy);
    float dt = _ga_t0->getTime()-_ga_t1->getTime();

    return (len>dt*velocity);
}

void OrthoTextureManipulator::flushMouseEventStack()
{
    _ga_t1 = NULL;
    _ga_t0 = NULL;
}

void OrthoTextureManipulator::addMouseEvent(const GUIEventAdapter& ea)
{
    _ga_t1 = _ga_t0;
    _ga_t0 = &ea;
}

void OrthoTextureManipulator::setByMatrix(const osg::Matrixd& matrix)
{
}

osg::Matrixd OrthoTextureManipulator::getMatrix() const
{
    osg::Matrixd m = getInverseMatrix();
    m.invert(m);
    return m;
}

osg::Matrixd OrthoTextureManipulator::getInverseMatrix() const
{
    return osg::Matrixd::translate(-_center) * 
        osg::Matrixd::translate(-_zoom_offset) * 
        osg::Matrixd::scale(_distance, _distance, _distance) * 
        osg::Matrixd::translate(_zoom_offset) * 
        osg::Matrixd::rotate(_rotation.inverse());
}

void OrthoTextureManipulator::computePosition(const osg::Vec3& eye, const osg::Vec3& center, const osg::Vec3& up)
{
    // we ignore the incoming params and just look along -z
    osg::Matrix rotation_matrix; // identity
    _rotation = rotation_matrix.getRotate().inverse();

    _center = osg::Vec3d(0,0,0);

    if (_current_aspect < _texture_aspect) {
        _distance = (_ortho_from_view_width/_texture_width);
    } else {
        _distance = (_ortho_from_view_height/_texture_height);
    }
}

bool OrthoTextureManipulator::calcMovement()
{
    // mouse scroll is only a single event
    if (_ga_t0.get()==NULL) return false;

    float dx=0.0f;
    float dy=0.0f;
    unsigned int buttonMask=osgGA::GUIEventAdapter::NONE;

    if (_ga_t0->getEventType()==GUIEventAdapter::SCROLL)
    {
        switch (_ga_t0->getScrollingMotion()) {
        case osgGA::GUIEventAdapter::SCROLL_UP:
            dy = _zoomDelta;
            break;
        case osgGA::GUIEventAdapter::SCROLL_DOWN:
            dy = -_zoomDelta;
            break;
        case osgGA::GUIEventAdapter::SCROLL_LEFT:
        case osgGA::GUIEventAdapter::SCROLL_RIGHT:
            // pass
            break;
        case osgGA::GUIEventAdapter::SCROLL_2D:
            // normalize scrolling delta
            dx = _ga_t0->getScrollingDeltaX() / ((_ga_t0->getXmax()-_ga_t0->getXmin()) * 0.5f);
            dy = _ga_t0->getScrollingDeltaY() / ((_ga_t0->getYmax()-_ga_t0->getYmin()) * 0.5f);

            dx *= _zoomDelta;
            dy *= _zoomDelta;
            break;
        default:
            break;
        }
        buttonMask=GUIEventAdapter::SCROLL;
    } 
    else 
    {

        if (_ga_t1.get()==NULL) return false;
        dx = _ga_t0->getXnormalized()-_ga_t1->getXnormalized();
        dy = _ga_t0->getYnormalized()-_ga_t1->getYnormalized();
        float distance = sqrtf(dx*dx + dy*dy);
        
        // return if movement is too fast, indicating an error in event values or change in screen.
        if (distance>0.5)
        {
            return false;
        }
        
        // return if there is no movement.
        if (distance==0.0f)
        {
            return false;
        }

        buttonMask = _ga_t1->getButtonMask();
    }

    double throwScale =  (_thrown && _ga_t0.valid() && _ga_t1.valid()) ?
            _delta_frame_time / (_ga_t0->getTime() - _ga_t1->getTime()) :
            1.0;

    if (buttonMask==GUIEventAdapter::MIDDLE_MOUSE_BUTTON || buttonMask==GUIEventAdapter::LEFT_MOUSE_BUTTON ||
        buttonMask==(GUIEventAdapter::LEFT_MOUSE_BUTTON|GUIEventAdapter::RIGHT_MOUSE_BUTTON))
    {
        // pan
        float xscale = -_ortho_from_view_width / (_distance*2.0) * throwScale;
        float yscale = -_ortho_from_view_height / (_distance*2.0) * throwScale;
        
        osg::Matrix rotation_matrix;
        rotation_matrix.makeRotate(_rotation);

        osg::Vec3 dv(dx*xscale,dy*yscale,0.0f);

        _center += dv*rotation_matrix;
        
        return true;
    }
    else if ((buttonMask==GUIEventAdapter::RIGHT_MOUSE_BUTTON) || (buttonMask==GUIEventAdapter::SCROLL))
    {
        // zoom
        dy *= -1;
        float scale = 1.0f + dy * throwScale;
        _distance *= scale;
        return true;
    }

    return false;
}
