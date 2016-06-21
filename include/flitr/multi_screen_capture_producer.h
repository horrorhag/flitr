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

#ifndef MULTI_SCREEN_CAPTURE_PRODUCER_H
#define MULTI_SCREEN_CAPTURE_PRODUCER_H 1

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
class FLITR_EXPORT MultiScreenCaptureProducer : public ImageProducer {
  public:
    class ScreenCaptureCallback : public osg::Camera::DrawCallback
    {
      public:
        ScreenCaptureCallback(MultiScreenCaptureProducer* p, int w, int h, int index);
        virtual void operator()(osg::RenderInfo& ri) const;


      public:
        bool flipV_;
        bool flipH_;

      private:
        void readPixels() const;

        MultiScreenCaptureProducer* producer_;
        osg::ref_ptr<osg::Image> osg_image_;
        int32_t width_orig_;
        int32_t height_orig_;
        int32_t index_;
    };

    MultiScreenCaptureProducer(std::vector<osgViewer::View*> views, uint32_t buffer_size, uint32_t capture_every_nth);
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
        for (size_t i=0;i<views_.size();i++)
            if (cbs_[i])
            {
                cbs_[i]->flipV_ = flip;
            }
    }

    void FlipHorizontal(bool flip)
    {
        for (size_t i=0;i<views_.size();i++)
            if (cbs_[i])
            {
                cbs_[i]->flipH_ = flip;
            }
    }

    std::vector<Image**> & getSlot(int index);

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

    std::vector<osgViewer::View*> views_;
    std::vector<osg::ref_ptr<ScreenCaptureCallback> > cbs_;
    uint32_t buffer_size_;
    uint32_t capture_every_nth_;
    uint32_t trigger_count_;
    bool do_capture_;
    std::string nextSnapFileName_;
    uint32_t req_bits_;
    std::vector<Image**> imv_;

};

}

#endif // SCREEN_CAPTURE_PRODUCER_H
