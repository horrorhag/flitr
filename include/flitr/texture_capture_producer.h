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
#include <osgDB/Registry>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>

#include <OpenThreads/Thread>

#include <flitr/flitr_export.h>

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


class FLITR_EXPORT TextureCaptureProducer : public ImageProducer {

    struct TextureCaptureDrawCallback : public osg::Camera::DrawCallback
    {
        TextureCaptureDrawCallback(TextureCaptureProducer* producer, bool blocking) :
            producer_(producer),
            blocking_(blocking)
        {}
        
        virtual void operator () (osg::RenderInfo& renderInfo) const
        {
            if (producer_->enabled_)
            {
                std::vector<Image**> imv;
                do {
                    imv = producer_->reserveWriteSlot();

                    if (imv.size()!=1)
                    {
                        if (blocking_)
                        {//sleep and try again i.e. block viewer's frame.
                          OpenThreads::Thread::microSleep(1000);
                        } else
                        {//drop this frame i.e. do not block viewer's frame.
                            logMessage(LOG_CRITICAL) << "Dropping frames - no space in texture capture buffer\n";
                            logMessage(LOG_CRITICAL).flush();
                            return;
                        }
                    }
                } while (imv.size() != 1);


                Image* im = *(imv[0]);
                osg::TextureRectangle* trect_ = producer_->OutputTexture_.get();

                renderInfo.getState()->applyTextureAttribute(0, //unit
                                                             trect_);

                GLenum pixelFormat=GL_RGB;
                GLenum pixelType=GL_UNSIGNED_BYTE;

                switch (producer_->OutputFormat_.getPixelFormat())
                {
                case ImageFormat::FLITR_PIX_FMT_Y_8:
                    pixelFormat=GL_LUMINANCE;
                    pixelType=GL_UNSIGNED_BYTE;
                    break;
                case ImageFormat::FLITR_PIX_FMT_RGB_8:
                    pixelFormat=GL_RGB;
                    pixelType=GL_UNSIGNED_BYTE;
                    break;
                case ImageFormat::FLITR_PIX_FMT_Y_16:
                    pixelFormat=GL_LUMINANCE;
                    pixelType=GL_UNSIGNED_SHORT;
                    break;
                case ImageFormat::FLITR_PIX_FMT_Y_F32:
                    pixelFormat=GL_RED;
                    pixelType=GL_FLOAT;
                    break;
                default:
                    std::cerr << "Pixel format " << producer_->OutputFormat_.getPixelFormat() << " not yet implemented in TextureCaptureProducer.\n";
                    exit(-1);
                }

                glGetTexImage(trect_->getTextureTarget(), 0, //level
                              pixelFormat, pixelType, im->data());

                // fill other vars
                if (producer_->CreateMetadataFunction_)
                {
                    im->setMetadata(producer_->CreateMetadataFunction_());

                    //if (im->metadata())
                    //{
                        //std::cout << " TextureCaptureProducer frame: " << im->metadata()->getString();
                        //std::cout.flush();
                    //}
                } else
                {
                    std::shared_ptr<DefaultTextureCaptureMetadata> meta(new DefaultTextureCaptureMetadata());
                    meta->PCTimeStamp_ = currentTimeNanoSec();
                    im->setMetadata(meta);
                }



                if (producer_->SaveNextImageTo_!="")
                {
                    osg::ref_ptr<osg::Image> image=osg::ref_ptr<osg::Image>(new osg::Image());
                    image->setImage(im->format()->getWidth(), im->format()->getHeight(), 1,
                                    pixelFormat, pixelFormat, pixelType, im->data(), osg::Image::NO_DELETE);

                    osg::ref_ptr<osg::Image> imageFlipped=osg::ref_ptr<osg::Image>(new osg::Image(*image,osg::CopyOp::DEEP_COPY_ALL));
                    imageFlipped->flipVertical();

                    osgDB::writeImageFile(*imageFlipped, producer_->SaveNextImageTo_);
                    producer_->SaveNextImageTo_="";
                }

                producer_->releaseWriteSlot();
            }
        }
        TextureCaptureProducer* producer_;
        bool blocking_;//blocking the viewer frame method as opposed to dropping image frames.
    };

public:
    TextureCaptureProducer(
            osg::TextureRectangle* input_texture,
            ImageFormat::PixelFormat output_pixel_format,
            uint32_t new_width,
            uint32_t new_height,
            bool keep_aspect,
            uint32_t buffer_size,
            bool blocking=false);

    bool init();

    osg::ref_ptr<osg::Group> getRoot() { return RootGroup_; }
    osg::ref_ptr<osg::TextureRectangle> getOutputTexture() { return OutputTexture_; }

    void setEnable(bool value) {enabled_=value;}

    void setShader(std::string filename);

    void saveNextImage(std::string filename) {SaveNextImageTo_=filename;}

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
    bool blocking_;

    bool enabled_;
    std::string SaveNextImageTo_;
};

}

#endif // TEXTURE_CAPTURE_PRODUCER_H
