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

#ifndef FLITR_LINES_OVERLAY_H
#define FLITR_LINES_OVERLAY_H 1

#include <flitr/modules/geometry_overlays/geometry_overlay.h>

namespace flitr {

    /**
     * Array of lines of specified colours drawn as an overlay.
     *
     * A line is defined as a start and end point, each with an optional
     * colour. If at any point the colour of the line is not specified, the
     * default colour set on the overlay (GeometryOverlay::setColour())
     * will be used for the line colour.
     *
     * A few types are defined as part of this class to make working with
     * the class a bit easier. These types are used as input types to
     * most of the functions on the class.
     *
     * The width of the lines are defined by the width set on the overlay
     * using the GeometryOverlay::setLineWidth()
     */
    class LinesOverlay : public GeometryOverlay
    {
    public:

        /**
         * Line defined by a 3D start and end point
         */
        typedef std::pair<osg::Vec3, osg::Vec3> Line;

        /**
         * Vector of multiple Line elements.
         */
        typedef std::vector<Line> Lines;

        /**
         * Colour defined by a 4D vector.
         */
        typedef osg::Vec4 Colour;

        /**
         * Vector of multiple Colour elements.
         */
        typedef std::vector<Colour> Colours;

        /**
         * Constructs the overlay.
         *
         * The overlay will be constructed with no lines drawn.
         */
        LinesOverlay();
        ~LinesOverlay();

        /**
         * Draw the supplied lines and remove all other lines.
         *
         * \param[in] lines Vector of lines that must be drawn..
         * \param[in] colours Optional colours that must be used for the lines. If the
         *      number of \a colours is less than the number of \a lines, the remaining
         *      lines will be drawn in the colour set on the geometry.
         */
        void setLines(const Lines &lines, Colours colours = Colours());

        /**
         * Draw the supplied lines but keep all other lines.
         *
         * \param[in] lines Vector of lines that must be drawn..
         * \param[in] colours Optional colours that must be used for the lines. If the
         *      number of \a colours is less than the number of \a lines, the remaining
         *      lines will be drawn in the colour set on the geometry.
         */
        void addLines(const Lines &lines, Colours colours = Colours());

        /**
         * Convenience function to draw a single line and remove all other lines.
         *
         * \sa setLine(Line)
         *
         * \param[in] line Single line that must be drawn.
         * \param[in] colour Colour that must be used for the line.
         */
        void setLine(const Line& line, const Colour &colour);

        /**
         * Convenience function to draw a single line and remove all other lines.
         *
         * The line will be drawn in the colour set on the geometry with the
         * GeometryOverlay::setColour() function.
         *
         * \sa setLine(Line, Colour)
         *
         * \param[in] line Single line that must be drawn.
         */
        void setLine(const Line& line);

        /**
         * Convenience function to add a single line but keep all other lines.
         *
         * \sa addLine(Line)
         *
         * \param[in] line Single line that must be drawn.
         * \param[in] colour Colour that must be used for the line.
         */
        void addLine(const Line& line, const Colour &colour);

        /**
         * Convenience function to add a single line but keep all other lines.
         *
         * The line will be drawn in the colour set on the geometry with the
         * GeometryOverlay::setColour() function.
         *
         * \sa addLine(Line, Colour)
         *
         * \param[in] line Single line that must be drawn.
         */
        void addLine(const Line& line);

        /* GeometryOverlay */
        void setName(const std::string& name) override;
    protected:
        /**
         * Protected method that draws the vertices and removes all other vertices.
         *
         * \sa addVertices()
         * \param[in] vertices Vertices that must be drawn.
         * \param[in] colours Colours that the vertices must be drawn in.
         */
        void setVertices(const osg::Vec3Array &vertices, const osg::Vec4Array &colours);
        /**
         * Protected method that draws the vertices but keeps all other vertices.
         *
         * The number of \a vertices must be an even number. If this is not the case, the last
         * vertex in the array will be removed. Thus each pair of vertices are one line.
         *
         * The colours passed are applied to each point in the array of vertices from
         * first to last. If the number of colours are less than the number of vertices then
         * the default colour set on the overlay will be used for the rest of the points.
         *
         * \param[in] vertices Vertices that must be drawn.
         * \param[in] colours Colours that the vertices must be drawn in.
         */
        void addVertices(const osg::Vec3Array &vertices, const osg::Vec4Array &colours);
    private:
        /**
         * Initialisation function that is called by the constructor to set up the graph.
         */
        void makeGraph();
        osg::ref_ptr<osg::Vec3Array> _Vertices;
        osg::ref_ptr<osg::Vec4Array> _VertexColours;
        osg::ref_ptr<osg::DrawArrays> _DrawArray;
        osg::ref_ptr<osg::Geode> _Geode;
        osg::ref_ptr<osg::Geometry> _Geom;
    };

} // namespace flitr

#endif // FLITR_LINES_OVERLAY_H
