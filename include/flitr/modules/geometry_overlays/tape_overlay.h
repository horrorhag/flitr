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

#ifndef FLITR_TAPE_OVERLAY_H
#define FLITR_TAPE_OVERLAY_H 1

#include <flitr/modules/geometry_overlays/geometry_overlay.h>

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

#include <stdint.h>

namespace flitr {

/**
 * Tape overlay with a given center, length and ticks.
 */
class FLITR_EXPORT TapeOverlay : public GeometryOverlay
{
  public:
    /**
     * Create a new tape overlay.
     *
     * \param center_x The x position of the tape center.
     * \param center_y The y position of the tape center.
     * \param length Length of the tape.
     * \param majorTicks Number of major ticks on the tape.
     * \param minorTicks Number of minor ticks on the tape.
     * \param indicator If the boresight indicator arrow should be displayed or not.
     */
    TapeOverlay(double center_x, double center_y, double length, uint8_t majorTicks = 3, uint8_t minorTicks = 0,  bool indicator=false);
    virtual ~TapeOverlay();

    /**
     * Adjust the position of the tape.
     * \param x The x position of the tape center.
     * \param y The y position of the tape center.
     */
    void setCenter(double x, double y);
    /**
     * Get the current center position of the tape.
     * \param x The x position of the tape center.
     * \param y The y position of the tape center.
     */
    void getCenter(double &x, double &y);

    /**
     * Set the length of the tape.
     */
    void setLength(double length);

    /**
     * Get the length of the tape.
     */
    double getLength() const;

    /**
     * Adjust the height of the major ticks.
     */
    void setMajorTickHeight(double height);

    /**
     * Adjust the height of the minor ticks.
     */
    void setMinorTickHeight(double height);

    /**
     * Adjust the offset value (as a fraction of tick interval) of center tick from the center of the tape.
     */
    void setOffset(double offset);

    /**
     * Returns the offset value of the ticks from the center of the tape.
     */
    double getOffset() const;

    /**
     * Returns the number of major ticks configured for the tape.
     *
     * This may be the number set in the constructor, or a
     * modified number of ticks to make the tape easier to draw.
     */
    uint8_t getMajorTicks() const;

    double getMajorTickSpacing() const;

    /**
     * Returns the number of minor ticks configured for the tape.
     *
     * This may be the number set in the constructor, or a
     * modified number of ticks to make the tape easier to draw.
     */
    uint8_t getMinorTicks() const;

    /**
     * Set the name of the overlay along with the nodes that make up
     * the overlay.
     *
     * This is normally used when geometry intersections are required.
     *
     * \param name New name of the overlay
     */
    virtual void setName(const std::string& name);

  private:
    void makeTape(bool filled);
    void updateTape();
    osg::ref_ptr<osg::Vec3Array> _Vertices;
    osg::ref_ptr<osg::Geode> _Geode;
    osg::ref_ptr<osg::Geometry> _Geom;
    osg::ref_ptr<osg::DrawArrays> _DrawArray;

    double _CenterX;
    double _CenterY;
    double _Length;
    uint8_t _MajorTicks;
    uint8_t _MinorTicks;
    double _MajorTickHeight;
    double _MajorTickSpacing;
    double _MinorTickHeight;
    double _Offset;
};

}

#endif // FLITR_TAPE_OVERLAY_H
