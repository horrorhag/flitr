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

#ifndef GLSL_DOWNSAMPLE_PASS_H
#define GLSL_DOWNSAMPLE_PASS_H 1
#include <iostream>
#include <string>

#include <flitr/flitr_export.h>
#include <flitr/modules/parameters/parameters.h>
#include <flitr/texture.h>

#include <osg/ref_ptr>
#include <osg/Group>
#include <osg/Camera>
#include <osg/MatrixTransform>
#include <osg/Projection>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Texture2D>
#include <osg/TextureRectangle>
#include <osg/Image>

namespace flitr {

class FLITR_EXPORT GLSLDownsamplePass
{
  public:
    GLSLDownsamplePass(flitr::TextureRectangle *in_tex, int dsfactor, bool read_back_to_CPU = false);
    ~GLSLDownsamplePass();
    osg::ref_ptr<osg::Group> getRoot() { return RootGroup_; }
    osg::ref_ptr<flitr::TextureRectangle> getOutputTexture() { return OutTexture_; }
    osg::ref_ptr<osg::Image> getOSGImage() { return OutImage_; }

    int getDSFactor() const
    {
        int rValue=0;

        UniformDSFactor_->get(rValue);

        return rValue;
    }
	

  private:
    void setShader();
    osg::ref_ptr<osg::Group> createTexturedQuad();
    void createOutputTexture(bool read_back_to_CPU);
    void setupCamera();

    osg::ref_ptr<osg::Group> RootGroup_;
    osg::ref_ptr<osg::Camera> Camera_;
    osg::ref_ptr<flitr::TextureRectangle> InTexture_;
    osg::ref_ptr<flitr::TextureRectangle> OutTexture_;
    osg::ref_ptr<osg::Image> OutImage_;

    int TextureWidth_;
    int TextureHeight_;

	osg::ref_ptr<osg::Program> FragmentProgram_;
    osg::ref_ptr<osg::StateSet> StateSet_;

    osg::ref_ptr<osg::Uniform> UniformDSFactor_;
};
}
#endif //GLSL_DOWNSAMPLE_PASS_H
