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

#ifndef SCREEN_CAPTURE_PRODUCER_H
#define SCREEN_CAPTURE_PRODUCER_H 1

#include <osgViewer/ViewerBase>

#include <flitr/image_producer.h>
#include <flitr/image_format.h>

namespace flitr {

/**
 * Simple producer that reads pixels after every rendered OSG frame.
 * 
 */
class ScreenCaptureProducer : public ImageProducer {
  public:
    class ScreenCaptureCallback : public osg::Camera::DrawCallback
    {
      public:
        ScreenCaptureCallback(ScreenCaptureProducer* p) :
            producer_(p)
        {
            width_orig_ = producer_->getFormat().getWidth();
            height_orig_ = producer_->getFormat().getHeight();
            osg_image_ = new osg::Image();
        }
        virtual void operator()(osg::RenderInfo& ri) const
        {
            if (!producer_->captureNow()) return;
            glReadBuffer(GL_BACK);
            osg::GraphicsContext* gc = ri.getState()->getGraphicsContext();
            if (!gc->getTraits()) return;
            if ((gc->getTraits()->width != width_orig_) ||
                (gc->getTraits()->height != height_orig_)) {
                logMessage(LOG_DEBUG) << "ScreenCaptureProducer: context size has changed since construction.\n";
                return;
            }
            readPixels();
        }
      private:
        void readPixels() const
        {
            std::vector<Image**> imv = producer_->reserveWriteSlot();
			if (imv.size() != 1) {
				logMessage(LOG_CRITICAL) << "Dropping frames - no space in screen capture buffer\n";
                return;
			}
            flitr::Image* im = *(imv[0]);
            osg_image_->setImage(width_orig_,
                                 height_orig_,
                                 1, GL_RGB, GL_RGB,  GL_UNSIGNED_BYTE,
                                 im->data(),
                                 osg::Image::NO_DELETE);
            osg_image_->readPixels(0, 0, width_orig_, height_orig_,
                                   GL_RGB, GL_UNSIGNED_BYTE);
            producer_->releaseWriteSlot();
        }
        ScreenCaptureProducer* producer_;
        osg::ref_ptr<osg::Image> osg_image_;
        int32_t width_orig_;
        int32_t height_orig_;
    };

    ScreenCaptureProducer(osgViewer::View& view, uint32_t buffer_size, uint32_t capture_every_nth) :
        view_(view),
        buffer_size_(buffer_size),
        capture_every_nth_(capture_every_nth),
        trigger_count_(0),
        do_capture_(false)
    {}
    /** 
     * The init method is used after construction to be able to return
     * success or failure of initialisation.
     * 
     * \return True if successful.
     */
    bool init()
    {
        osg::Viewport* vp = view_.getCamera()->getViewport();
        double w = vp->width();
        double h = vp->height();
        
        ImageFormat imf(w, h, ImageFormat::FLITR_PIX_FMT_RGB_8);
        ImageFormat_.push_back(imf);
        
        SharedImageBuffer_ = std::tr1::shared_ptr<SharedImageBuffer>(new SharedImageBuffer(*this, buffer_size_, 1));
        SharedImageBuffer_->initWithStorage();

        cb_ = new ScreenCaptureCallback(this);
        view_.getCamera()->setFinalDrawCallback(cb_.get());
    }
    bool captureNow()
    {
        if (!do_capture_) return false;
 
        trigger_count_++;
        if (trigger_count_ % capture_every_nth_ == 0) return true;

        return false;
    }
    void startCapture()
    {
        trigger_count_ = 0;
        do_capture_ = true;
    }
    void stopCapture()
    {
        do_capture_ = false;
    }
  private:
    osgViewer::View& view_;
    osg::ref_ptr<ScreenCaptureCallback> cb_;
    uint32_t buffer_size_;
    uint32_t capture_every_nth_;
    uint32_t trigger_count_;
    bool do_capture_;
};

}

#endif // SCREEN_CAPTURE_PRODUCER_H
