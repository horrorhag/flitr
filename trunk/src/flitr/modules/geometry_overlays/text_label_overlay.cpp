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

#include <flitr/modules/geometry_overlays/text_label_overlay.h>

using namespace flitr;

TextLabelOverlay::TextLabelOverlay(double centre_x, double centre_y, const std::string &text) :
    GeometryOverlay()
{
    _Text = new osgText::Text;
    _Text->setFont("Vera.ttf");

    _Text->setAxisAlignment(osgText::Text::SCREEN);
    _Text->setAlignment(osgText::Text::CENTER_CENTER);
    _Text->setCharacterSizeMode(osgText::Text::SCREEN_COORDS);
    _Text->setCharacterSize(32.0f);
    _Text->setDataVariance(osg::Object::DYNAMIC);
    _Text->setBackdropType(osgText::Text::DROP_SHADOW_BOTTOM_RIGHT);

    _Text->setText(text);
    _Text->setPosition(osg::Vec3(centre_x, centre_y, 0.0));

    makeLabel();
    dirtyBound();

    setColour(osg::Vec4d(1.0,1.0,1.0,0.5));
}

void TextLabelOverlay::makeLabel()
{
    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
	
    geode->addDrawable(_Text.get());

    geode->setCullingActive(false);
	
	_GeometryGroup->addChild(geode.get());
}

void TextLabelOverlay::setCentre(double centre_x, double centre_y)
{
    _Text->setPosition(osg::Vec3(centre_x, centre_y, 0.0));
    dirtyBound();
}

void TextLabelOverlay::setText(const std::string &text)
{
    _Text->setText(text);
    dirtyBound();
}

void TextLabelOverlay::setFont(const std::string font_file)
{
    _Text->setFont(font_file);
}

void TextLabelOverlay::setSize(const double character_size)
{
    _Text->setCharacterSize(character_size);
}

void TextLabelOverlay::setAlignment(const osgText::Text::AlignmentType type)
{
    _Text->setAlignment(type);
}

void TextLabelOverlay::setColour(osg::Vec4d newcol)
{
    GeometryOverlay::setColour(newcol);

    _Text->setColor(newcol);
    _Text->setBackdropColor(osg::Vec4d(0.0, 0.0, 0.0, sqrt(sqrt(newcol._v[3]))));
}
