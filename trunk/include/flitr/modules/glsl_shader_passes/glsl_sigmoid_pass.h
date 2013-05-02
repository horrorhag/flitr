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

#ifndef GLSL_SIGMOID_PASS_H
#define GLSL_SIGMOID_PASS_H 1

#include <iostream>
#include <string>

#include <flitr/flitr_export.h>
#include <flitr/modules/parameters/parameters.h>
#include <flitr/texture.h>


#include <osg/ref_ptr>
#include <osg/Group>
#include <osg/Switch>
#include <osg/Camera>
#include <osg/MatrixTransform>
#include <osg/Projection>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Texture2D>
#include <osg/TextureRectangle>
#include <osg/Image>

namespace flitr {

class FLITR_EXPORT GLSLSigmoidPass : public Parameters
{
  public:
    GLSLSigmoidPass(flitr::TextureRectangle *in_tex, bool read_back_to_CPU = false);
    ~GLSLSigmoidPass();

    osg::ref_ptr<osg::Group> getRoot() { return RootGroup_; }
    osg::ref_ptr<flitr::TextureRectangle> getOutputTexture() { return OutTexture_; }
    osg::ref_ptr<osg::Image> getOSGImage() { return OutImage_; }

    void setOffset(float value);
    float getOffset() const;
    void setSlope(float value);
    float getSlope() const;
    void setScale(float value);
    float getScale() const;


    virtual int getNumberOfParms()
    {
        return 3;
    }

    virtual EParmType getParmType(int id)
    {
        return PARM_FLOAT;
    }

    virtual std::string getParmName(int id)
    {
        switch (id)
        {
        case 0 :return std::string("offset");
        case 1 :return std::string("slope");
        case 2 :return std::string("scale");
        }
        return std::string("???");
    }

    virtual std::string getTitle()
    {
        return "Sigmoid Adjust Pass";
    }

    virtual float getFloat(int id)
    {
        switch (id)
        {
        case 0 : return getOffset();
        case 1 : return getSlope();
        case 2 : return getScale();
        }
        return 0;
    }

    virtual bool getFloatRange(int id, float &low, float &high)
    {
        if (id==0)
        {
            low=-10.0; high=0;
        }
        else if (id==1)
        {
            low=-10.0; high=10.0;
        }
        else if (id==2)
        {
            low=0.01; high=10.0;
        }

        return true;
    }

    virtual bool setFloat(int id, float v)
    {
        switch (id)
        {
        case 0 : setOffset(v); break;
        case 1 : setSlope(v); break;
        case 2 : setScale(v); break;
        }
        return true;
    }

    virtual void enable(bool state=true)
    {
        Enabled_ = state;
        SwitchNode_->setSingleChildOn(Enabled_ ? 0 : 1);
    }

    virtual bool isEnabled()
    {
        return Enabled_;
    }


  private:
    void setShader();

    osg::ref_ptr<osg::Group> createTexturedQuad();
    void createOutputTexture(bool read_back_to_CPU);
    void setupCamera();

    osg::ref_ptr<osg::Group> RootGroup_;
    osg::ref_ptr<osg::Switch> SwitchNode_;
    osg::ref_ptr<osg::Camera> Camera_;
    osg::ref_ptr<flitr::TextureRectangle> InTexture_;
    osg::ref_ptr<flitr::TextureRectangle> OutTexture_;
    osg::ref_ptr<osg::Image> OutImage_;

    int TextureWidth_;
    int TextureHeight_;

	osg::ref_ptr<osg::Program> FragmentProgram_;
    osg::ref_ptr<osg::StateSet> StateSet_;

    osg::ref_ptr<osg::Uniform> UniformOffset_;
    osg::ref_ptr<osg::Uniform> UniformSlope_;
    osg::ref_ptr<osg::Uniform> UniformScale_;

    bool Enabled_;
};
}
#endif //GLSL_SIGMOID_PASS_H
