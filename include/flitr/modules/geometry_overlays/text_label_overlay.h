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

#ifndef FLITR_TEXTLABEL_OVERLAY_H
#define FLITR_TEXTLABEL_OVERLAY_H 1

#include <flitr/modules/geometry_overlays/geometry_overlay.h>

#include <osg/ref_ptr>
#include <osgText/Text>
#include <osgDB/ReadFile>
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
 * A text label overlay at a given position.
 */
class FLITR_EXPORT TextLabelOverlay : public GeometryOverlay
{
  public:
    /** 
     * Create a new text label.
     * 
     * \param center_x The x position of the label center.
     * \param center_y The y position of the label center.
     */
    TextLabelOverlay(double centre_x, double centre_y, const std::string &text);
    ~TextLabelOverlay() {};

    /** 
     * Adjusts the position of the label.
     * 
     * \param x The new x position of the label center.
     * \param y The new y position of the label center.
     */
    void setCentre(double centre_x, double centre_y);
    
    void setText(const std::string &text);
    void setFont(const std::string font_file);
    void setSize(const double character_size);
    void setAlignment(const osgText::Text::AlignmentType type);
    virtual void setColour(osg::Vec4d newcol);

  private:
    void makeLabel();

    osg::ref_ptr<osgText::Text> _Text;
};

}

#endif // FLITR_TEXTLABEL_OVERLAY_H
