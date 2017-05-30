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

#include <flitr/log_message.h>
#include <flitr/osg_image_producer.h>

#include <osgDB/ReadFile>

using std::shared_ptr;
using namespace flitr;

flitr::ImageFormat::PixelFormat convertGLtoFlitr(GLenum format);

OsgImageProducer::OsgImageProducer(const std::string& file_name, const uint32_t buffer_size /*= FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS*/)
    : file_name_(file_name)
    , buffer_size_(buffer_size)
    , osg_image_(0)
{
}

OsgImageProducer::~OsgImageProducer()
{

}

bool OsgImageProducer::init()
{
    osg_image_ = osgDB::readImageFile(file_name_);
    if (!osg_image_)
    {
        logMessage(LOG_CRITICAL) << "OsgImageProducer could not load image file " << file_name_ << ".\n";
        return false;
    }

    osg_image_->flipVertical();

    ImageFormat imf(osg_image_->s(), osg_image_->t(), convertGLtoFlitr(osg_image_->getPixelFormat()));
    ImageFormat_.push_back(imf);

    // Allocate storage
    SharedImageBuffer_ = shared_ptr<SharedImageBuffer>(new SharedImageBuffer(*this, buffer_size_, 1));
    SharedImageBuffer_->initWithStorage();

    return true;
}

bool OsgImageProducer::trigger() 
{ 
    std::vector<Image**> aImages;
    aImages = reserveWriteSlot();
    if (aImages.size() == 1)
    {
        Image& out_image = *(*(aImages[0]));
        uint8_t* buffer = out_image.data();
        uint8_t*  data = (uint8_t*)osg_image_->data();
        memcpy(buffer, data, this->getFormat().getBytesPerImage());
    }
    releaseWriteSlot();

    return true; 
}

flitr::ImageFormat::PixelFormat convertGLtoFlitr(GLenum format)
{
    switch (format)
    {
    case(GL_LUMINANCE):
        return flitr::ImageFormat::PixelFormat::FLITR_PIX_FMT_Y_8;
    case(GL_RGB):
        return flitr::ImageFormat::PixelFormat::FLITR_PIX_FMT_RGB_8;
    case(GL_RGBA):
        return flitr::ImageFormat::PixelFormat::FLITR_PIX_FMT_RGBA;
    default:
        logMessage(LOG_INFO) << "GL to FLITr pixel conversion: No suitable format conversion for" <<format <<" .\n";
        return flitr::ImageFormat::PixelFormat::FLITR_PIX_FMT_RGB_8;
    }
    logMessage(LOG_INFO) << "GL to FLITr pixel conversion: No suitable format conversion for" << format << " .\n";
    return flitr::ImageFormat::PixelFormat::FLITR_PIX_FMT_RGB_8;
}