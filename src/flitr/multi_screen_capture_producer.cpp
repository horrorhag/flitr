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

#include <flitr/multi_screen_capture_producer.h>

using namespace flitr;

MultiScreenCaptureProducer::ScreenCaptureCallback::ScreenCaptureCallback(MultiScreenCaptureProducer* p, int w, int h, int index)
    : flipV_(false)
    , flipH_(false)
    , producer_(p)
    , index_(index)
{
    width_orig_ = w;//producer_->getFormat().getWidth();
    height_orig_ = h;//producer_->getFormat().getHeight();
    osg_image_ = new osg::Image();

}

void MultiScreenCaptureProducer::ScreenCaptureCallback::operator()(osg::RenderInfo& ri) const
{
    if (!producer_->shouldCaptureNow()) return;

    glReadBuffer(GL_BACK);
    osg::GraphicsContext* gc = ri.getState()->getGraphicsContext();
    if (!gc->getTraits()) return;
    if ((gc->getTraits()->width != width_orig_) ||
            (gc->getTraits()->height != height_orig_)) {
        logMessage(LOG_DEBUG) << "MultiScreenCaptureProducer: context size has changed since construction.\n";
        return;
    }

    readPixels();
}

void MultiScreenCaptureProducer::ScreenCaptureCallback::readPixels() const
{
    std::vector<Image**> imv = producer_->getSlot(index_);
//    if (imv.size() != 1) {
//        logMessage(LOG_CRITICAL) << "Dropping frames - no space in screen capture buffer\n";
//        return;
//    }
    flitr::Image* im = *(imv[index_]);
    osg_image_->setImage(width_orig_,
                         height_orig_,
                         1, GL_RGB, GL_RGB,  GL_UNSIGNED_BYTE,
                         im->data(),
                         osg::Image::NO_DELETE);

    osg_image_->readPixels(0, 0, width_orig_, height_orig_,
                           GL_RGB, GL_UNSIGNED_BYTE);

    if (flipV_) osg_image_->flipVertical();
    if (flipH_) osg_image_->flipHorizontal();

    std::string snapFileName=producer_->getNextSnapFileName();

    if (snapFileName!="")
    {
        osg_image_->flipVertical();//Flip image because image is upside down from video. Hopefully the snap is not done too often!
        osgDB::writeImageFile(*osg_image_, snapFileName);
        producer_->resetSnapFileName();
        osg_image_->flipVertical();//Flip back.
    }

//    producer_->releaseWriteSlot();
}

MultiScreenCaptureProducer::MultiScreenCaptureProducer(std::vector<osgViewer::View*> views, uint32_t buffer_size, uint32_t capture_every_nth) :
    views_(views),
    buffer_size_(buffer_size),
    capture_every_nth_(capture_every_nth),
    trigger_count_(0),
    do_capture_(false),
    nextSnapFileName_(""),
    req_bits_(0)
{}

bool MultiScreenCaptureProducer::init()
{
    for (size_t i=0;i<views_.size();i++)
    {
        osg::Viewport* vp = views_[i]->getCamera()->getViewport();
        double w = vp->width();
        double h = vp->height();

        ImageFormat imf(w, h, ImageFormat::FLITR_PIX_FMT_RGB_8);
        ImageFormat_.push_back(imf);

        cbs_.push_back(new ScreenCaptureCallback(this, w, h, i));
        views_[i]->getCamera()->setFinalDrawCallback(cbs_.back().get());
    }

    SharedImageBuffer_ = std::shared_ptr<SharedImageBuffer>(new SharedImageBuffer(*this, buffer_size_, views_.size()));
    SharedImageBuffer_->initWithStorage();

    return true;
}

std::vector<Image**> & MultiScreenCaptureProducer::getSlot(int index)
{
    if (req_bits_==(uint32_t)(1<<views_.size())-1)
    {
        releaseWriteSlot();
        req_bits_ = 0;
    }
    if (req_bits_==0)
    {
        imv_ = reserveWriteSlot();
    }
    req_bits_ |= (1<<index);
    return imv_;
}

bool MultiScreenCaptureProducer::shouldCaptureNow()
{
    if (!do_capture_) return false;
    
    trigger_count_++;
    if (trigger_count_ % capture_every_nth_ == 0) return true;
    
    return false;
}

void MultiScreenCaptureProducer::startCapture()
{
    trigger_count_ = 0;
    do_capture_ = true;
}

void MultiScreenCaptureProducer::stopCapture()
{
    do_capture_ = false;
}
