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

#ifndef FIP_MSR_H
#define FIP_MSR_H 1

#include <flitr/image_processor.h>
#include <flitr/image_processor_utils.h>

namespace flitr {
    
    /*! Multi-Scale Retinex Implementation.*/
    class FLITR_EXPORT FIPMSR : public ImageProcessor
    {
    public:
        enum class FilterType : uint8_t { GausXY = 1, BoxII = 2, BoxRS = 3};
        
        /*! Constructor given the upstream producer.
         *@param upStreamProducer The upstream image producer.
         *@param images_per_slot The number of images per image slot from the upstream producer.
         *@param buffer_size The size of the shared image buffer of the downstream producer.*/
        FIPMSR(ImageProducer& upStreamProducer, uint32_t images_per_slot,
               const FilterType filterType,
               uint32_t buffer_size=FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS);
        
        /*! Virtual destructor */
        virtual ~FIPMSR();
        
        /*! Method to initialise the object.
         *@return Boolean result flag. True indicates successful initialisation.*/
        virtual bool init();
        
        /*!Synchronous trigger method. Called automatically by the trigger thread in ImageProcessor base class if started.
         *@sa ImageProcessor::startTriggerThread*/
        virtual bool trigger();
        
        void setGFScale(size_t scale)
        {
            if (scale <2) scale=2;
            _GFScale=scale;
        }
        
        size_t getGFScale() const
        {
            return _GFScale;
        }

        void setNumGaussianScales(const size_t numScales)
        {
            _numScales=numScales;
        }

        size_t getNumGaussianScales() const
        {
            return _numScales;
        }

        virtual void enable(bool state=true)
        {
            _enabled = state;
        }

        virtual bool isEnabled()
        {
            return _enabled;
        }
        
        /*!
        * Following functions overwrite the flitr::Parameters virtual functions
        */
        virtual std::string getTitle()
        {
            return _Title;
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
		        case 0 :return std::string("GF Scale");
				case 1 :return std::string("Number of Scales");
            }
            return std::string("???");
        }

        virtual float getFloat(int id)
        {
            switch (id)
            {
            	case 0 : return (float)getGFScale();
				case 1 : return (float)getNumGaussianScales();
            }

            return 0.0f;
        }

        virtual bool getFloatRange(int id, float &low, float &high)
        {
			switch (id)
            {
            	case 0 : low=1.0; high=100.0; return true;
				case 1 : low=1.0; high=5.0; return true;
            }

            return false;
        }

        virtual bool setFloat(int id, float v)
        {
            switch (id)
            {
                case 0 : setGFScale((size_t) v); return true;
				case 1 : setNumGaussianScales((size_t) v); return true;
            }

            return false;
        }
        
    private:
        bool _enabled;
        const FilterType _filterType;
		std::string _Title;
        
        //!Box filter helper. No significant state.
        GaussianFilter _GFXY;
        
        //!Box filter helper. No significant state.
        BoxFilterII _GFII;
        
        //!Box filter helper. No significant state.
        BoxFilterRS _GFRS;
        
        
        size_t _GFScale;
        size_t _numScales;
        
        /*! The grayscale image per slot. */
        float *_intensityScratchData;
        
        float *_GFScratchData;
        float *_MSRScratchData;
        float *_floatScratchData;
        
        //!Integral image of input image.
        double *_doubleScratchData1;
        
        //!Integral image used for approximate gaussian filter.
        double *_doubleScratchData2;
        
#define _histoBinArrSize 10000
        
        size_t *_histoBins;
        
        size_t _triggerCount;
    };
    
}

#endif //FIP_MSR_H
