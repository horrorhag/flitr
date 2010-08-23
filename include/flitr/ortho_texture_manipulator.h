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

#ifndef ORTHOTEXTUREMANIPULATOR
#define ORTHOTEXTUREMANIPULATOR 1

#include <osgGA/CameraManipulator>
#include <osg/Quat>

namespace flitr {

class OrthoTextureManipulator : public osgGA::CameraManipulator
{
  public:
    OrthoTextureManipulator(const double texture_width, const double texture_height);

    virtual const char* className() const { return "OrthoTextureManipulator"; }

    /** set the position of the matrix manipulator using a 4x4 Matrix.*/
    virtual void setByMatrix(const osg::Matrixd& matrix);

    /** set the position of the matrix manipulator using a 4x4 Matrix.*/
    virtual void setByInverseMatrix(const osg::Matrixd& matrix) { setByMatrix(osg::Matrixd::inverse(matrix)); }

    /** get the position of the manipulator as 4x4 Matrix.*/
    virtual osg::Matrixd getMatrix() const;

    /** get the position of the manipulator as a inverse matrix of the manipulator, typically used as a model view matrix.*/
    virtual osg::Matrixd getInverseMatrix() const;

    /** Get the FusionDistanceMode. Used by SceneView for setting up stereo convergence.*/
    virtual osgUtil::SceneView::FusionDistanceMode getFusionDistanceMode() const { return osgUtil::SceneView::USE_FUSION_DISTANCE_VALUE; }

    /** Get the FusionDistanceValue. Used by SceneView for setting up stereo convergence.*/
    virtual float getFusionDistanceValue() const { return _distance; }

    /** Attach a node to the manipulator. 
        Automatically detaches previously attached node.
        setNode(NULL) detaches previously nodes.
        Is ignored by manipulators which do not require a reference model.*/
    virtual void setNode(osg::Node*);

    /** Return node if attached.*/
    virtual const osg::Node* getNode() const;

    /** Return node if attached.*/
    virtual osg::Node* getNode();

    /** Move the camera to the default position. 
        May be ignored by manipulators if home functionality is not appropriate.*/
    virtual void home(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us);
    virtual void home(double);
        
    /** Start/restart the manipulator.*/
    virtual void init(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us);

    /** handle events, return true if handled, false otherwise.*/
    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us);

    /** Get the keyboard and mouse usage of this manipulator.*/
    virtual void getUsage(osg::ApplicationUsage& usage) const;

    /** set the mouse scroll wheel zoom delta.
     * Range -1.0 to +1.0,  -ve value inverts wheel direction and zero switches off scroll wheel. */
    void setScroolWheelZoomDelta(double zoomDelta) { _zoomDelta = zoomDelta; }

    /** get the mouse scroll wheel zoom delta. */
    double getScroolWheelZoomDelta() const { return _zoomDelta; }

    /** Set the center of the view. */
    void setCenter(const osg::Vec3d& center) { _center = center - _zoom_offset; }

    /** Get the center of the view. */
    const osg::Vec3d& getCenter() const { return _center; }

    /** Set the distance of the trackball. */
    void setDistance(double distance) { _distance = distance; }

    /** Get the distance of the trackball. */
    double getDistance() const { return _distance; }

    /** Set the 'allow throw' flag. Releasing the mouse button while moving the camera results in a throw. */
    void setAllowThrow(bool allowThrow) { _allowThrow = allowThrow; }

    /** Returns true if the camera can be thrown, false otherwise. This defaults to true. */
    bool getAllowThrow() const { return _allowThrow; }
  protected:

    virtual ~OrthoTextureManipulator();

    /** Reset the internal GUIEvent stack.*/
    void flushMouseEventStack();
    /** Add the current mouse GUIEvent to internal stack.*/
    void addMouseEvent(const osgGA::GUIEventAdapter& ea);

    void computePosition(const osg::Vec3& eye,const osg::Vec3& lv,const osg::Vec3& up);

    /** For the give mouse movement calculate the movement of the camera.
        Return true is camera has moved and a redraw is required.*/
    bool calcMovement();

    /** Check the speed at which the mouse is moving.
        If speed is below a threshold then return false, otherwise return true.*/
    bool isMouseMoving();

    bool initOrthoProjection(osgGA::GUIActionAdapter& us);
    void updateOrthoParams(osgGA::GUIActionAdapter& us);

    // Internal event stack comprising last two mouse events.
    osg::ref_ptr<const osgGA::GUIEventAdapter> _ga_t1;
    osg::ref_ptr<const osgGA::GUIEventAdapter> _ga_t0;

    osg::ref_ptr<osg::Node>       _node;

    bool _allowThrow;
    bool _thrown;

    /** The approximate amount of time it is currently taking to draw a frame.
     * This is used to compute the delta in translation/rotation during a thrown display update.
     * It allows us to match an delta in position/rotation independent of the rendering frame rate.
     */
    double _delta_frame_time; 

    /** The time the last frame started.
     * Used when _rate_sensitive is true so that we can match display update rate to rotation/translation rate.
     */
    double _last_frame_time;

    osg::Vec3d   _center;
    osg::Quat    _rotation;
    double       _distance;
    float        _zoomDelta;

    /// Used so that zoom happens in the middle of the window
    osg::Vec3d _zoom_offset;

    /// The aspect of the texture we are viewing, used to create accurate home pos
    double _texture_aspect;
    double _texture_width;
    double _texture_height;

    double _current_aspect;
    
    bool _init_ortho_params;
    double _ortho_from_view_width;
    double _ortho_from_view_height;
    double _ortho_from_view_left;
    double _ortho_from_view_right;
    double _ortho_from_view_top;
    double _ortho_from_view_bottom;
};

}

#endif //ORTHOTEXTUREMANIPULATOR

