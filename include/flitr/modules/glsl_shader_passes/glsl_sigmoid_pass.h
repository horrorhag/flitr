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

#include <flitr/glsl_image_processor.h>

#include <flitr/flitr_export.h>
#include <flitr/modules/parameters/parameters.h>
#include <flitr/texture.h>

#include <osg/Switch>

namespace flitr {

class FLITR_EXPORT GLSLSigmoidPass : public GLSLImageProcessor
{
  public:
    GLSLSigmoidPass(flitr::TextureRectangle *in_tex, bool read_back_to_CPU = false);
    virtual ~GLSLSigmoidPass();

    osg::ref_ptr<osg::Group> getRoot() { return RootGroup_; }
    osg::ref_ptr<flitr::TextureRectangle> getOutputTexture() { return OutTexture_; }
    osg::ref_ptr<osg::Image> getOSGImage() { return OutImage_; }

    void setCentre(float value);
    float getCentre() const;
    void setSlope(float value);
    float getSlope() const;


    virtual int getNumberOfParms()
    {
        return 2;
    }

    virtual EParmType getParmType(int id)
    {
        return PARM_FLOAT;
    }

    virtual std::string getParmName(int id)
    {
        switch (id)
        {
        case 0 :return std::string("centre");
        case 1 :return std::string("slope");
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
        case 0 : return getCentre();
        case 1 : return getSlope();
        }
        return 0;
    }

    virtual bool getFloatRange(int id, float &low, float &high)
    {
        if (id==0)
        {
            low=0.0; high=1.0;
        }
        else if (id==1)
        {
            low=-100.0; high=100.0;
        }

        return true;
    }

    virtual bool setFloat(int id, float v)
    {
        switch (id)
        {
        case 0 : setCentre(v); break;
        case 1 : setSlope(v); break;
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

    osg::ref_ptr<osg::Switch> SwitchNode_;

    osg::ref_ptr<osg::Uniform> UniformCentre_;
    osg::ref_ptr<osg::Uniform> UniformSlope_;

    bool Enabled_;
};
}
#endif //GLSL_SIGMOID_PASS_H
