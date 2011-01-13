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
#include <osg/MatrixTransform>

namespace flitr {

class FLITR_EXPORT TexturedQuad {
  public:
    TexturedQuad(osg::Image *in_image);
    TexturedQuad(osg::TextureRectangle *in_tex);
    TexturedQuad(osg::Texture2D *in_tex);
    ~TexturedQuad();
    osg::ref_ptr<osg::Group> getRoot() { return RootGroup_; }
    void setTransform(osg::Matrixd& m) { MatrixTransform_->setMatrix(m); }

  private:
    void buildGraph(bool use_normalised_coordinates);
    osg::ref_ptr<osg::Group> RootGroup_;
    osg::ref_ptr<osg::MatrixTransform> MatrixTransform_;
	osg::ref_ptr<osg::Geometry> Geom_;
    osg::ref_ptr<osg::StateSet> GeomStateSet_;

	int Width_;
	int Height_;
};

}

#endif //FLITR_TEXTURED_QUAD
