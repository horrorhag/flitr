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

#ifndef FLITR_TEXTURED_QUAD
#define FLITR_TEXTURED_QUAD 1

#include <flitr/flitr_export.h>

#include <osg/ref_ptr>
#include <osg/Group>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/TextureRectangle>
#include <osg/Texture2D>
#include <osg/TexEnv>
#include <osg/MatrixTransform>

namespace flitr {

class FLITR_EXPORT TexturedQuad
{
  public:
    TexturedQuad(osg::Image* in_image);
    TexturedQuad(osg::TextureRectangle* in_tex);
    TexturedQuad(osg::Texture2D* in_tex);

    ~TexturedQuad();

    void setTexture(osg::Image* in_image);
    void setTexture(osg::TextureRectangle* in_tex);
    void setTexture(osg::Texture2D* in_tex);

    osg::ref_ptr<osg::Group> getRoot() { return RootGroup_; }
    void setTransform(osg::Matrixd& m) { MatrixTransform_->setMatrix(m); }

    void flipTextureCoordsLeftToRight();
    void flipTextureCoordsTopToBottom();

    void setShader(std::string filename);

  private:
    void init();
    void replaceGeom(bool use_normalised_coordinates);
    osg::ref_ptr<osg::Group> RootGroup_;
    osg::ref_ptr<osg::MatrixTransform> MatrixTransform_;
    osg::ref_ptr<osg::Geode> Geode_;
	osg::ref_ptr<osg::Geometry> Geom_;
    osg::ref_ptr<osg::StateSet> GeomStateSet_;
    osg::ref_ptr<osg::Program> FragmentProgram_;

    osg::ref_ptr<osg::Vec2Array> tcoords_;

	int Width_;
	int Height_;
};

/*! Textured quad similar to TexturedQuad, but with two textures which may be masked/combined with the user applied shader.*/
class FLITR_EXPORT TexturedQuadAB
{
  public:
    TexturedQuadAB(osg::Image* in_image_A, osg::Image* in_image_B);
    TexturedQuadAB(osg::TextureRectangle* in_tex_A, osg::TextureRectangle* in_tex_B);
    TexturedQuadAB(osg::Texture2D* in_tex_A, osg::Texture2D* in_tex_B);
    ~TexturedQuadAB();

    void setTextures(osg::Image* in_image_A, osg::Image* in_image_B);
    void setTextures(osg::TextureRectangle* in_tex_A, osg::TextureRectangle* in_tex_B);
    void setTextures(osg::Texture2D* in_tex_A, osg::Texture2D* in_tex_B);

    osg::ref_ptr<osg::Group> getRoot() { return RootGroup_; }
    void setTransform(osg::Matrixd& m) { MatrixTransform_->setMatrix(m); }

    void setShader(const std::string &filename);

    void setColourMaskA(const osg::Vec4f &colourMask)
    {
        ColourMaskAUniform_->set(colourMask);
    }

    void setColourMaskB(const osg::Vec4f &colourMask)
    {
        ColourMaskBUniform_->set(colourMask);
    }

  private:
    void init();

    void replaceGeom(bool use_normalised_coordinates);

    osg::ref_ptr<osg::Group> RootGroup_;
    osg::ref_ptr<osg::MatrixTransform> MatrixTransform_;

    osg::ref_ptr<osg::Geode> Geode_;
    osg::ref_ptr<osg::Geometry> Geom_;
    osg::ref_ptr<osg::StateSet> GeomStateSet_;
    osg::ref_ptr<osg::Program> FragmentProgram_;

    osg::ref_ptr<osg::Uniform> ColourMaskAUniform_;
    osg::ref_ptr<osg::Uniform> ColourMaskBUniform_;

    int Width_A_;
    int Height_A_;

    int Width_B_;
    int Height_B_;
};
}

#endif //FLITR_TEXTURED_QUAD
