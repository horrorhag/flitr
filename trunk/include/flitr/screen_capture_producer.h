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
#include <osgViewer/View>
#include <osgDB/FileUtils>
#include <osgDB/Registry>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>

#include <flitr/image_producer.h>
#include <flitr/image_format.h>

namespace flitr {

/**
 * Simple producer that reads pixels after every rendered OSG frame.
 * 
 */
class FLITR_EXPORT ScreenCaptureProducer : public ImageProducer {
  public:
    class ScreenCaptureCallback : public osg::Camera::DrawCallback
    {
      public:
        ScreenCaptureCallback(ScreenCaptureProducer* p);
        virtual void operator()(osg::RenderInfo& ri) const;


      public:
        bool flipV_;
        bool flipH_;

      private:
        void readPixels() const;

        ScreenCaptureProducer* producer_;
        osg::ref_ptr<osg::Image> osg_image_;
        int32_t width_orig_;
        int32_t height_orig_;

    };

    ScreenCaptureProducer(osgViewer::View& view, uint32_t buffer_size, uint32_t capture_every_nth);
    /** 
     * The init method is used after construction to be able to return
     * success or failure of initialisation.
     * 
     * \return True if successful.
     */
    bool init();
    void startCapture();
    void stopCapture();

    void snapNextImage(std::string nextSnapFileName)
    {
        nextSnapFileName_=nextSnapFileName;
    }

    void FlipVertical(bool flip)
    {
        if (cb_)
        {
            cb_->flipV_ = flip;
        }
    }

    void FlipHorizontal(bool flip)
    {
        if (cb_)
        {
            cb_->flipH_ = flip;
        }
    }

  private:
    bool shouldCaptureNow();

    const std::string getNextSnapFileName() const
    {
        return nextSnapFileName_;
    }

    void resetSnapFileName()
    {
        nextSnapFileName_="";
    }

    osgViewer::View& view_;
    osg::ref_ptr<ScreenCaptureCallback> cb_;
    uint32_t buffer_size_;
    uint32_t capture_every_nth_;
    uint32_t trigger_count_;
    bool do_capture_;
    std::string nextSnapFileName_;
};

}

#endif // SCREEN_CAPTURE_PRODUCER_H
