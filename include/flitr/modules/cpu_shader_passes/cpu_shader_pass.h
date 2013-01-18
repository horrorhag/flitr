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

#ifndef CPU_SHADER_PASS_H
#define CPU_SHADER_PASS_H q

#include <flitr/flitr_export.h>

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
#include <iostream>

namespace flitr {

class FLITR_EXPORT CPUShaderPass {
public:

    class FLITR_EXPORT CPUShader : public osg::Camera::DrawCallback
    {
    };

    class FLITR_EXPORT CPUExample_Shader : public CPUShader
    {
    public:
        CPUExample_Shader(osg::Image* image) :
            Image_(image)
        {
        }
        
        virtual void operator () (osg::RenderInfo& renderInfo) const
        {
            const unsigned long width=Image_->s();
            const unsigned long height=Image_->t();
            const unsigned long numPixels=width*height;
            const unsigned long numComponents=osg::Image::computeNumComponents(Image_->getPixelFormat());

            unsigned char * const data=(unsigned char *)Image_->data();

            for (unsigned long i=0; i<(numPixels*numComponents); i++)
            {
                data[i]=(unsigned char)(pow(data[i]/(float)255.0f, 2.0f)*255.0+0.5);
            }

            std::cout << "*";
            std::cout.flush();
            Image_->dirty();
        }

        osg::Image* Image_;
    };


public:
    CPUShaderPass(osg::ref_ptr<osg::Image> in_img);
    CPUShaderPass(osg::ref_ptr<osg::TextureRectangle> in_tex);
    ~CPUShaderPass();

    osg::ref_ptr<osg::Group> getRoot() { return RootGroup_; }

    osg::ref_ptr<osg::TextureRectangle> getOutputTexture() { return OutTexture_; }

    osg::ref_ptr<osg::Image> getOSGImage() { return OutImage_; }


    osg::ref_ptr<osg::Image> getOutImage() { return OutImage_; }


    void setPostRenderCPUShader(CPUShader *cpuShader);

    void setGPUShader(std::string filename);

private:
    osg::ref_ptr<osg::Image> getInImage() { return InImage_; }
    void setPreRenderCPUShader(CPUShader *cpuShader);

    osg::ref_ptr<osg::Group> createTexturedQuad();
    void setupCamera();

    osg::ref_ptr<osg::Group> RootGroup_;
    osg::ref_ptr<osg::Camera> Camera_;

    osg::ref_ptr<osg::Image> InImage_;
    osg::ref_ptr<osg::TextureRectangle> InTexture_;

    osg::ref_ptr<osg::Image> OutImage_;
    osg::ref_ptr<osg::TextureRectangle> OutTexture_;

    unsigned long TextureWidth_;
    unsigned long TextureHeight_;

    osg::ref_ptr<osg::Program> FragmentProgram_;
    osg::ref_ptr<osg::StateSet> StateSet_;
};

}
#endif //CPU_SHADER_PASS_H
