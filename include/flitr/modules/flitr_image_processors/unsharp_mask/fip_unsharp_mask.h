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

#ifndef FIP_UNSHARP_MASK_H
#define FIP_UNSHARP_MASK_H 1

#include <flitr/image_processor_utils.h>
#include <flitr/image_processor.h>

namespace flitr {
    
    /*! Applies an unsharp mask to the image. */
    class FLITR_EXPORT FIPUnsharpMask : public ImageProcessor
    {
    public:
        
        /*! Constructor given the upstream producer.
         *@param upStreamProducer The upstream image producer.
         *@param images_per_slot The number of images per image slot from the upstream producer.
         *@param buffer_size The size of the shared image buffer of the downstream producer.*/
        FIPUnsharpMask(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                       const float gain,
                       const float filterRadius,
                       uint32_t buffer_size=FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS);
        
        /*! Virtual destructor */
        virtual ~FIPUnsharpMask();
        
        /*! Method to initialise the object.
         *@return Boolean result flag. True indicates successful initialisation.*/
        virtual bool init();
        
        /*!Synchronous trigger method. Called automatically by the trigger thread in ImageProcessor base class if started.
         *@sa ImageProcessor::startTriggerThread*/
        virtual bool trigger();
        
        //!Sets the filter gain. This method is thread safe.
        virtual void setGain(const float gain);
        //!Gets the filter gain. This method is thread safe.
        float getGain() const;
        
        //!Sets the filter radius. This method is thread safe.
        virtual void setFilterRadius(const float filterRadius);
        //!Gets the filter radius. This method is thread safe.
        float getFilterRadius() const;

        /*!
        * Following functions overwrite the flitr::Parameters virtual functions
        */
        virtual std::string getTitle(){return _title;   }
        virtual void setTitle(const std::string &title){_title=title;}

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
            case 0 :return std::string("Filter Radius");
            case 1 :return std::string("Gain");
            }
            return std::string("???");
        }

        virtual float getFloat(int id)
        {
            switch (id)
            {
            case 0 : return getFilterRadius();
            case 1 : return getGain();
            }

            return 0.0f;
        }

        virtual bool getFloatRange(int id, float &low, float &high)
        {
            if (id==0)
            {
                low=1.0; high=100.0;
                return true;
            }
            if (id==1)
            {
                low=0.0; high=32.0;
                return true;
            }

            return false;
        }

        virtual bool setFloat(int id, float v)
        {
            switch (id)
            {
                case 0 : setFilterRadius(v); return true;
                case 1 : setGain(v); return true;
            }

            return false;
        }

    private:
        float gain_;
        
        float *xFiltData_;
        float *filtData_;

        GaussianFilter gaussianFilter_;

        std::string _title = "Unsharp Mask";
    };
    
}

#endif //FIP_UNSHARP_MASK_H
