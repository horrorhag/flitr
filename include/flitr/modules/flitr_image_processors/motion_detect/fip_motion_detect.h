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

#ifndef FIP_MOTION_DETECT_H
#define FIP_MOTION_DETECT_H 1

#include <flitr/image_processor.h>
#include <flitr/image_processor_utils.h>
#include <math.h>

namespace flitr {
    
    /*! Applies Gaussian filter. */
    class FLITR_EXPORT FIPMotionDetect : public ImageProcessor
    {
    public:
        
        /*! Constructor given the upstream producer.
         *@param upStreamProducer The upstream image producer.
         *@param images_per_slot The number of images per image slot from the upstream producer.
         *@param buffer_size The size of the shared image buffer of the downstream producer.
         *@param motionThreshold Motion above this threshold would result in a plot.
         *@param detectionThreshold A stable plot count above this threshold would result in a detection.
         */
        FIPMotionDetect(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                        const bool showOverlays, const bool produceOnlyMotionImages, const bool forceRGBOutput,
                        const float motionThreshold, const int detectionThreshold,
                        uint32_t buffer_size=FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS);
        
        /*! Virtual destructor */
        virtual ~FIPMotionDetect();
        
        /*! Method to initialise the object.
         @todo  Does doxygen take the comment from base class? Look at inherit docs config setting. Does IDE bring up base comments?
         *@return Boolean result flag. True indicates successful initialisation.*/
        virtual bool init();
        
        
        /*!Synchronous trigger method. Called automatically by the trigger thread in ImageProcessor base class if started.
         *@sa ImageProcessor::startTriggerThread*/
        virtual bool trigger();

        //!Update the motion threshold. Motion above this threshold would result in a plot.
        void setMotionThreshold(const float motionThreshold)
        {
            _motionThreshold=motionThreshold;
        }
        //!Get the motion threshold. Motion above this threshold would result in a plot.
        float getMotionThreshold() const
        {
            return _motionThreshold;
        }

        //!Update the detection threshold. A stable plot count above this threshold would result in a detection.
        void setDetectionThreshold(const int detectionThreshold)
        {
            _detectionThreshold=detectionThreshold;
        }
        //!Get the detection threshold. A stable plot count above this threshold would result in a detection.
        int getDetectionThreshold() const
        {
            return _detectionThreshold;
        }

        //!Set if the pass should add the detection overlays to the output image.
        void setShowOverlays(const bool showOverlays)
        {
            _showOverlays=showOverlays;
        }
        //!Get if the pass should add the detection overlays to the output image.
        bool getShowOverlays() const
        {
            return _showOverlays;
        }
    
        /*!
        * Following functions overwrite the flitr::Parameters virtual functions
        */
        virtual std::string getTitle()
        {
            return _title;
        }

        /*!
        * Following functions overwrite the flitr::Parameters virtual functions
        */
        virtual int getNumberOfParms()
        {
            return 3;
        }

        virtual flitr::Parameters::EParmType getParmType(int id)
        {
			switch (id)
			{
				case 0: return flitr::Parameters::PARM_FLOAT;
				case 1: return flitr::Parameters::PARM_INT;
				case 2: return flitr::Parameters::PARM_BOOL;
			}
        }

        virtual std::string getParmName(int id)
        {
            switch (id)
            {
            	case 0 :return std::string("Motion Threshold");
				case 1 :return std::string("Detection Threshold");
				case 2 :return std::string("ShowOverlays");
            }
            return std::string("???");
        }

        virtual float getFloat(int id)
        {
            switch (id)
            {
            case 0 : return getMotionThreshold();
            }

            return 0.0f;
        }

		virtual int getInt(int id)
        {
            switch (id)
            {
            case 1 : return getDetectionThreshold();
            }

            return 0;
        }
		
		virtual bool getBool(int id)
        {
            switch (id)
            {
            case 2 : return getShowOverlays();
            }

            return false;
        }

        virtual bool getFloatRange(int id, float &low, float &high)
        {
            if (id==0)
            {
                low=1.0; high=10.0;
                return true;
            }

            return false;
        }

		virtual bool getIntRange(int id, int &low, int &high)
        {
            if (id==1)
            {
                low=1; high=10;
                return true;
            }

            return false;
        }

        virtual bool setFloat(int id, float v)
        {
            switch (id)
            {
                case 0 : setMotionThreshold(v); return true;
            }

            return false;
        }
	
		virtual bool setInt(int id, int v)
        {
            switch (id)
            {
                case 1 : setDetectionThreshold(v); return true;
            }

            return false;
        }
		
		virtual bool setBool(int id, bool v)
        {
            switch (id)
            {
                case 2 : setShowOverlays(v); return true;
            }

            return false;
        }
    
    private:
		std::string _title;

        uint64_t _frameCounter;

        float *_avrgImg;
        float *_varImg;
        
        uint8_t *_detectionImg;
        int *_detectionCountImg;
        
        bool _showOverlays;
        bool _produceOnlyMotionImages;
        bool _forceRGBOutput;
        
        float _motionThreshold;
        int _detectionThreshold;
    };
    
}

#endif //FIP_MOTION_DETECT_H
