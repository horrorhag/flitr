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

#ifndef TEXTURE_CAPTURE_PRODUCER_H
#define TEXTURE_CAPTURE_PRODUCER_H 1

#include <flitr/image_producer.h>
#include <flitr/image_format.h>
#include <flitr/high_resolution_time.h>
#include <flitr/image_metadata.h>

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Texture2D>
#include <osg/TextureRectangle>
#include <osg/Switch>
#include <osg/AlphaFunc>
#include <osgDB/FileUtils>

namespace flitr {


class DefaultTextureCaptureMetadata : public flitr::ImageMetadata {
public:
    DefaultTextureCaptureMetadata() : ImageMetadata() {}

    virtual ~DefaultTextureCaptureMetadata() {}

    virtual bool writeToStream(std::ostream& s) const
    {
        s.write((char *)&PCTimeStamp_, sizeof(PCTimeStamp_));
        return true;
    }

    virtual bool readFromStream(std::istream& s) const
    {
        s.read((char *)&PCTimeStamp_, sizeof(PCTimeStamp_));
        return true;
    }

    virtual DefaultTextureCaptureMetadata* clone() const
    {
        return new DefaultTextureCaptureMetadata(*this);
    }

    virtual uint32_t getSizeInBytes() const
    {// size when packed in stream.
        return sizeof(PCTimeStamp_);
    }

    virtual std::string getString() const
    {
       std::stringstream rValueStream;
       rValueStream << PCTimeStamp_ << "\n";
       rValueStream.flush();
       return rValueStream.str();
    }

    /// Nanoseconds since epoch on the PC when frame was captured.
    uint64_t PCTimeStamp_;
};


class TextureCaptureProducer : public ImageProducer {

    struct TextureCaptureDrawCallback : public osg::Camera::DrawCallback
    {
        TextureCaptureDrawCallback(TextureCaptureProducer* producer) :
            producer_(producer)
        {}
        
        virtual void operator () (osg::RenderInfo& renderInfo) const
        {
            if (producer_->enabled_)
            {
                std::vector<Image**> imv = producer_->reserveWriteSlot();
                if (imv.size() != 1) {
                    logMessage(LOG_CRITICAL) << "Dropping frames - no space in texture capture buffer\n";
                    logMessage(LOG_CRITICAL).flush();
                    return;
                }

                Image* im = *(imv[0]);
                osg::TextureRectangle* trect_ = producer_->OutputTexture_.get();

                renderInfo.getState()->applyTextureAttribute(0, //unit
                                                             trect_);

                if (producer_->OutputFormat_.getPixelFormat() == ImageFormat::FLITR_PIX_FMT_Y_8) {
                    glGetTexImage(trect_->getTextureTarget(),
                                  0, //level
                                  GL_LUMINANCE,
                                  GL_UNSIGNED_BYTE,
                                  im->data());
                } else {
                    glGetTexImage(trect_->getTextureTarget(),
                                  0, //level
                                  GL_RGB,
                                  GL_UNSIGNED_BYTE,
                                  im->data());
                }


                // fill other vars
                if (producer_->CreateMetadataFunction_)
                {
                    im->setMetadata(producer_->CreateMetadataFunction_());

                    if (im->metadata())
                    {
                        //std::cout << " TextureCaptureProducer frame: " << im->metadata()->getString();
                        //std::cout.flush();
                    }
                } else
                {
                    std::tr1::shared_ptr<DefaultTextureCaptureMetadata> meta(new DefaultTextureCaptureMetadata());
                    meta->PCTimeStamp_ = currentTimeNanoSec();
                    im->setMetadata(meta);
                }


                producer_->releaseWriteSlot();
            }
        }
        TextureCaptureProducer* producer_;
    };

public:
    TextureCaptureProducer(
            osg::TextureRectangle* input_texture,
            ImageFormat::PixelFormat output_pixel_format,
            uint32_t new_width,
            uint32_t new_height,
            bool keep_aspect,
            uint32_t buffer_size);

    bool init();

    osg::ref_ptr<osg::Group> getRoot() { return RootGroup_; }
    osg::ref_ptr<osg::TextureRectangle> getOutputTexture() { return OutputTexture_; }

    void setEnable(bool value) {enabled_=value;}

    void setShader(std::string filename);
private:
    osg::ref_ptr<osg::Group> createTexturedQuad();
    void setupCamera();
    void createTextures();

    osg::ref_ptr<osg::TextureRectangle> InputTexture_;
    osg::ref_ptr<osg::TextureRectangle> OutputTexture_;
    ImageFormat OutputFormat_;

    osg::ref_ptr<osg::Group> RootGroup_;
    osg::ref_ptr<osg::Camera> Camera_;
    osg::ref_ptr<osg::Program> FragmentProgram_;
    osg::ref_ptr<osg::StateSet> StateSet_;

    uint32_t InputTextureWidth_;
    uint32_t InputTextureHeight_;
    uint32_t OutputTextureWidth_;
    uint32_t OutputTextureHeight_;

    bool KeepAspect_;

    uint32_t BufferSize_;

    bool enabled_;
};

}

#endif // TEXTURE_CAPTURE_PRODUCER_H
