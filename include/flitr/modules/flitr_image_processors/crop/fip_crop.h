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

#ifndef FIP_CROP_H
#define FIP_CROP_H 1

#include <flitr/image_processor.h>

#include <algorithm>

namespace flitr {
    
    /*! Crops the image. */
    class FLITR_EXPORT FIPCrop : public ImageProcessor
    {
    public:
        
        /*! Constructor given the upstream producer.
         *@param upStreamProducer The upstream image producer.
         *@param images_per_slot The number of images per image slot from the upstream producer.
         *@param buffer_size The size of the shared image buffer of the downstream producer.*/
        FIPCrop(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                size_t startX,
                size_t startY,
                size_t width,
                size_t height,
                uint32_t buffer_size=FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS);
        
        /*! Virtual destructor */
        virtual ~FIPCrop();
        
        /*! Method to initialise the object.
         *@return Boolean result flag. True indicates successful initialisation.*/
        virtual bool init();
        
        /*!Synchronous trigger method. Called automatically by the trigger thread in ImageProcessor base class if started.
         *@sa ImageProcessor::startTriggerThread*/
        virtual bool trigger();
        
        virtual void setStartX(const float f)
        {
            startX_ = std::min<float>(f,maxWidth_);
        }

        virtual float getStartX() {
            return startX_;
        }

        virtual void setStartY(const float f)
        {
            startY_ = std::min<float>(f,maxHeight_);
        }

        virtual float getStartY() {
            return startY_;
        }


        virtual void setWidth(const float f)
        {
            width_ = std::min<float>(f,maxWidth_);

            //Setup image format being produced to downstream.
            for (uint32_t i=0; i<ImagesPerSlot_; i++)
            {
                ImageFormat_[i].setWidth(width_);
            }

        }

        virtual float getWidth() {
            return width_;
        }


        virtual void setHeight(const float f)
        {
            height_ = std::min<float>(f,maxHeight_);

            //Setup image format being produced to downstream.
            for (uint32_t i=0; i<ImagesPerSlot_; i++)
            {
                ImageFormat_[i].setHeight(height_);
            }

        }

        virtual float getHeight() {
            return height_;
        }


        virtual int getNumberOfParms()
        {
            return 2;
        }

        virtual flitr::Parameters::EParmType getParmType(int id)
        {
            return flitr::Parameters::PARM_FLOAT;
        }

        virtual std::string getParmName(int id)
        {
            switch (id)
            {
            case 0 :return std::string("Start X");
            case 1 :return std::string("Start Y");
            //case 2 :return std::string("Width");
            //case 3 :return std::string("Height");

            }
            return std::string("???");
        }

        virtual float getFloat(int id)
        {
            switch (id)
            {
            case 0 : return getStartX();
            case 1 : return getStartY();
            case 2 : return getWidth();
            case 3 : return getHeight();
            }

            return 0.0f;
        }

        virtual bool getFloatRange(int id, float &low, float &high)
        {
	    switch (id)
            {
            case 0 : low=0.0; high=maxWidth_ - width_; return true;
            case 1 : low=0.0; high=maxHeight_ - height_; return true;
            case 2 : low=0.0; high=maxWidth_; return true;
            case 3 : low=0.0; high=maxHeight_; return true;

            }

            return false;
        }

        virtual bool setFloat(int id, float v)
        {
            switch (id)
            {
            case 0 : setStartX(v); return true;
            case 1 : setStartY(v); return true;
            //case 2 : setWidth(v); return true;
            //case 3 : setHeight(v); return true;

            }

            return false;
        }

        virtual std::string getTitle() {
            return Title_;
        }


    private:
        std::string Title_;

        size_t startX_;
        size_t startY_;
        
        size_t width_;
        size_t height_;

        int maxWidth_;
        int maxHeight_;
    };
}

#endif //FIP_CROP_H
