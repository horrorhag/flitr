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

#ifndef FLITR_HUD_H
#define FLITR_HUD_H 1

#include <flitr/flitr_export.h>

#include <osg/Camera>
#include <osgGA/GUIEventHandler>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

namespace flitr {
    
    /**
     * A hud that spans the viewport.
     */
    class FLITR_EXPORT HUD : public osg::Camera
    {
    public:
        
        enum AnchorMode
        {
            HUD_ANCHOR_CENTER = 0,
            HUD_ANCHOR_BOTTOM_LEFT = 1
        };
        
        enum AspectMode
        {
            HUD_ASPECT_FIXED = 0, //Always show whole HUD in view.
            HUD_ASPECT_FIXED_FILL_X = 1, //Fill view in width with HUD.
            HUD_ASPECT_FIXED_FILL_Y = 2, //Fill view in height with HUD.
            HUD_ASPECT_STRETCHED = 3, //Stretch HUD to fill view in WIDTH and HEIGHT.
            HUD_ASPECT_TRACK_VIEW = 4, //Keep aspect of HUD the same as the view with dimensions in pixels.
        };
        
    private:
        
        /**
         * The resize handler makes sure that the HUD is resized when
         * the view it is attached to changes its size.
         *
         */
        struct ResizeHandler : public osgGA::GUIEventHandler
        {
            ResizeHandler(HUD *hud) :
            hud_(hud),
            old_view_width_(-1),
            old_view_height_(-1)
            {}
            
            bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter&)
            {
                if (ea.getHandled()) return false;
                
                switch(ea.getEventType())
                {
                    case(osgGA::GUIEventAdapter::FRAME):
                    {
                        double view_width = hud_->getViewport()->width();//In pixels.
                        double view_height = hud_->getViewport()->height();//In pixels.
                        
                        if ( (view_height!=0) &&
                            ((view_width != old_view_width_) ||
                             (view_height != old_view_height_))
                            )
                        {
                            old_view_width_ = view_width;
                            old_view_height_ = view_height;
                            
                            double view_aspect = view_width/view_height;
                            double ratio_change = view_aspect / hud_->aspect_;
                            
                            // The default choice is a stretched HUD; HUD_ASPECT_STRETCHED.
                            osg::Matrix proj = osg::Matrix::ortho2D(hud_->min_x_,
                                                                    hud_->max_x_,
                                                                    hud_->min_y_,
                                                                    hud_->max_y_);
                            
                            switch (hud_->aspect_mode_) {
                                case HUD_ASPECT_TRACK_VIEW: {
                                    // Keep aspect of HUD the same as the view with dimensions in pixels.
                                    hud_->setMinMax(0, view_width, 0, view_height);
                                    proj = osg::Matrix::ortho2D(0, view_width, 0, view_height);
                                    break;
                                }
                                case HUD_ASPECT_FIXED: {
                                    //Always show whole HUD in view.
                                    if (view_aspect > hud_->aspect_) {
                                        proj *= osg::Matrix::scale(1.0/ratio_change, 1.0, 1.0);
                                        
                                        if (hud_->anchor_mode_ == HUD_ANCHOR_BOTTOM_LEFT) {
                                            double w = hud_->width_;
                                            double shift = -0.5*(w - (w/ratio_change));
                                            osg::Matrixd shift_m;
                                            shift_m.makeTranslate(osg::Vec3d(shift, 0.0, 0.0));
                                            proj*=shift_m;
                                        }
                                    } else {
                                        proj *= osg::Matrix::scale(1.0, ratio_change, 1.0);
                                        
                                        if (hud_->anchor_mode_ == HUD_ANCHOR_BOTTOM_LEFT) {
                                            double h = hud_->height_;
                                            double shift = -0.5*(h - (h*ratio_change));
                                            osg::Matrixd shift_m;
                                            shift_m.makeTranslate(osg::Vec3d(0.0, shift, 0.0));
                                            proj*=shift_m;
                                        }
                                    }
                                    break;
                                }
                                case HUD_ASPECT_FIXED_FILL_X: {
                                    // Fill view in width with HUD.
                                    proj *= osg::Matrix::scale(1.0, ratio_change, 1.0);
                                    
                                    if (hud_->anchor_mode_ == HUD_ANCHOR_BOTTOM_LEFT) {
                                        double h = hud_->height_;
                                        double shift = -0.5*(h - (h*ratio_change));
                                        osg::Matrixd shift_m;
                                        shift_m.makeTranslate(osg::Vec3d(0.0, shift, 0.0));
                                        proj*=shift_m;
                                    }
                                    break;
                                }
                                case HUD_ASPECT_FIXED_FILL_Y: {
                                    // Fill view in height with HUD.
                                    proj *= osg::Matrix::scale(1.0/ratio_change, 1.0, 1.0);
                                    
                                    if (hud_->anchor_mode_ == HUD_ANCHOR_BOTTOM_LEFT) {
                                        double w = hud_->width_;
                                        double shift = -0.5*(w - (w/ratio_change));
                                        osg::Matrixd shift_m;
                                        shift_m.makeTranslate(osg::Vec3d(shift, 0.0, 0.0));
                                        proj*=shift_m;
                                    }
                                    break;
                                }
                                default:
                                    break;
                            }
                            
                            hud_->getProjectionMatrix() = proj;
                        }
                        
                        return false;
                    }
                    default:
                        break;
                }
                
                return false;
            }
            
            HUD* hud_;
            
            double old_view_width_;//In pixels.
            double old_view_height_;//In pixels.
        };
        
        friend struct ResizeHandler;
        
    public:
        HUD(osgViewer::View *view, const double minX=0.0, const double maxX=1.0, const double minY=0.0, const double maxY=1.0, const double near=-10.0, const double far=10.0);
        virtual ~HUD();
        
        osgGA::GUIEventHandler* getResizeHandler()
        {
            return resize_handler_.get();
        }
        
        void setAnchorMode(const AnchorMode m) { anchor_mode_ = m; }
        
        void setAspectMode(const AspectMode m) { aspect_mode_ = m; }
        
        void setMinMax(const double minX, const double maxX, const double minY, const double maxY)
        {
            if (maxX <= minX) return;
            if (maxY <= minY) return;
            
            min_x_ = minX;
            max_x_ = maxX;
            
            min_y_ = minY;
            max_y_ = maxY;
            
            width_ = max_x_ - min_x_;
            height_ = max_y_ - min_y_;
            
            aspect_ = width_/height_;
        }
        
        
    private:
        osg::ref_ptr<ResizeHandler> resize_handler_;
        
        double min_x_;
        double max_x_;
        double width_;
        
        double min_y_;
        double max_y_;
        double height_;
        
        double aspect_;
        
        AnchorMode anchor_mode_;
        AspectMode aspect_mode_;
    };
    
}

#endif //FLITR_HUD

