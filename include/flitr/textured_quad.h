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

#define _USE_MATH_DEFINES

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
    
    //=== Geomtric transformations of quad ===//
    void setTransform(osg::Matrixd& m) { MatrixTransform_->setMatrix(m); }
    void resetTransform() { MatrixTransform_->setMatrix(osg::Matrixd()); }
    
    void translate(double x, double y)
    {
        osg::Matrixd m=MatrixTransform_->getMatrix();
        m*=osg::Matrixd::translate(osg::Vec3d(x, y, 0.0));
        MatrixTransform_->setMatrix(m);
    }
    
    void flipAroundHorizontal()
    {
        osg::Matrixd m=MatrixTransform_->getMatrix();
        m*=osg::Matrixd::translate(osg::Vec3d(-Width_*0.5, -Height_*0.5, 0.0));
        m*=osg::Matrixd::scale(osg::Vec3d(1.0, -1.0, 1.0));
        m*=osg::Matrixd::translate(osg::Vec3d(Width_*0.5, Height_*0.5, 0.0));
        MatrixTransform_->setMatrix(m);
    }
    
    void flipAroundVertical()
    {
        osg::Matrixd m=MatrixTransform_->getMatrix();
        m*=osg::Matrixd::translate(osg::Vec3d(-Width_*0.5, -Height_*0.5, 0.0));
        m*=osg::Matrixd::scale(osg::Vec3d(-1.0, 1.0, 1.0));
        m*=osg::Matrixd::translate(osg::Vec3d(Width_*0.5, Height_*0.5, 0.0));
        MatrixTransform_->setMatrix(m);
    }

    void rotateAroundCentre(double angle)
    {
        osg::Matrixd m=MatrixTransform_->getMatrix();
        m*=osg::Matrixd::translate(osg::Vec3d(-Width_*0.5, -Height_*0.5, 0.0));
        m*=osg::Matrixd::rotate(angle, osg::Vec3d(0.0, 0.0, -1.0));
        m*=osg::Matrixd::translate(osg::Vec3d(Width_*0.5, Height_*0.5, 0.0));
        MatrixTransform_->setMatrix(m);
    }
    //=== ===//
    
    
    int getWidth() {
        return Width_;
    }

    int getHeight() {
        return Height_;
    }

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

    
/*! Textured quad similar to TexturedQuad, but with multiple textures which may be masked/combined with the user applied shader.*/
class FLITR_EXPORT MultiTexturedQuad
{
  public:
    MultiTexturedQuad(const std::vector<osg::Image *> &inImageVec);
    MultiTexturedQuad(const std::vector<osg::TextureRectangle *> &inTexVec);
    MultiTexturedQuad(const std::vector<osg::Texture2D *> &inTexVec);
    ~MultiTexturedQuad();

    void setTextures(const std::vector<osg::Image *> &inImageVec);
    void setTextures(const std::vector<osg::TextureRectangle *> &inTexVec);
    void setTextures(const std::vector<osg::Texture2D *> &inTexVec);

    osg::ref_ptr<osg::Group> getRoot() { return RootGroup_; }
    
    //=== Geomtric transformations of quad ===//
    void setTransform(osg::Matrixd& m) { MatrixTransform_->setMatrix(m); }
    void resetTransform() { MatrixTransform_->setMatrix(osg::Matrixd()); }
    
    void translate(double x, double y)
    {
        osg::Matrixd m=MatrixTransform_->getMatrix();
        m*=osg::Matrixd::translate(osg::Vec3d(x, y, 0.0));
        MatrixTransform_->setMatrix(m);
    }
    
    void flipAroundHorizontal()
    {
        osg::Matrixd m=MatrixTransform_->getMatrix();
        m*=osg::Matrixd::translate(osg::Vec3d(-WidthVec_[0]*0.5, -HeightVec_[0]*0.5, 0.0));
        m*=osg::Matrixd::scale(osg::Vec3d(1.0, -1.0, 1.0));
        m*=osg::Matrixd::translate(osg::Vec3d(WidthVec_[0]*0.5, HeightVec_[0]*0.5, 0.0));
        MatrixTransform_->setMatrix(m);
    }
    
    void flipAroundVertical()
    {
        osg::Matrixd m=MatrixTransform_->getMatrix();
        m*=osg::Matrixd::translate(osg::Vec3d(-WidthVec_[0]*0.5, -HeightVec_[0]*0.5, 0.0));
        m*=osg::Matrixd::scale(osg::Vec3d(-1.0, 1.0, 1.0));
        m*=osg::Matrixd::translate(osg::Vec3d(WidthVec_[0]*0.5, HeightVec_[0]*0.5, 0.0));
        MatrixTransform_->setMatrix(m);
    }
    
    void rotateAroundCentre(double angle)
    {
        osg::Matrixd m=MatrixTransform_->getMatrix();
        m*=osg::Matrixd::translate(osg::Vec3d(-WidthVec_[0]*0.5, -HeightVec_[0]*0.5, 0.0));
        m*=osg::Matrixd::rotate(angle, osg::Vec3d(0.0, 0.0, -1.0));
        m*=osg::Matrixd::translate(osg::Vec3d(WidthVec_[0]*0.5, HeightVec_[0]*0.5, 0.0));
        MatrixTransform_->setMatrix(m);
    }
    //=== ===//
    
    
    //=== Transformations of texture coords ===//
    void translateTexture(const float x, const float y, const size_t textureID)
    {
        for (size_t cn=0; cn<4; ++cn)
        {
            osg::Vec2f &currentCoord=(*(texCoordVec_[textureID]))[cn];
            
            currentCoord-=osg::Vec2f(x, y);
        }
        
        Geom_->setTexCoordArray(textureID, texCoordVec_[textureID].get());
    }
    
    void scaleTexture(const float cx, const float cy,
                            const float sx, const float sy,
                            const size_t textureID)
    {
        for (size_t cn=0; cn<4; ++cn)
        {
            osg::Vec2f &currentCoord=(*(texCoordVec_[textureID]))[cn];
            
            currentCoord-=osg::Vec2f(cx, cy);
            currentCoord=osg::componentDivide(currentCoord, osg::Vec2f(sx, sy));
            currentCoord+=osg::Vec2f(cx, cy);
        }
        
        Geom_->setTexCoordArray(textureID, texCoordVec_[textureID].get());
    }

    void rotateTexture(const float cx, const float cy,
                             const float thetaDeg,
                             const size_t textureID)
    {
        const float thetaRad=-(thetaDeg/180.0) * M_PI;
        const osg::Vec2f A(cosf(thetaRad), sinf(thetaRad));
        const osg::Vec2f B(-A._v[1], A._v[0]);
        
        for (size_t cn=0; cn<4; ++cn)
        {
            osg::Vec2f &currentCoord=(*(texCoordVec_[textureID]))[cn];
            
            currentCoord-=osg::Vec2f(cx, cy);
            currentCoord=A * (currentCoord._v[0]) + B * (currentCoord._v[1]);
            currentCoord+=osg::Vec2f(cx, cy);
        }
        
        Geom_->setTexCoordArray(textureID, texCoordVec_[textureID].get());
    }
    //=== ===//
    
    
    void setShader(const std::string &filename);

    void setColourMask(const osg::Vec4f &colourMask, const size_t maskID)
    {
        if (maskID < ColourMaskUniformVec_.size())
        {
            ColourMaskUniformVec_[maskID]->set(colourMask);
        }
    }
    
    
  private:
    void init(const size_t numTextures);

    void replaceGeom(bool useNormalisedCoordinates);

    osg::ref_ptr<osg::Group> RootGroup_;
    osg::ref_ptr<osg::MatrixTransform> MatrixTransform_;

    osg::ref_ptr<osg::Geode> Geode_;
    osg::ref_ptr<osg::Geometry> Geom_;
    osg::ref_ptr<osg::StateSet> GeomStateSet_;
    osg::ref_ptr<osg::Program> FragmentProgram_;

    std::vector<osg::ref_ptr<osg::Vec2Array>> texCoordVec_;

    std::vector<osg::ref_ptr<osg::Uniform>> ColourMaskUniformVec_;
    std::vector<int> WidthVec_;
    std::vector<int> HeightVec_;
};
}

#endif //FLITR_TEXTURED_QUAD
