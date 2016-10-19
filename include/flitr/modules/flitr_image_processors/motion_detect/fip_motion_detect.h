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

        //!Update the detection threshold. A stable plot count above this threshold would result in a detection.
        void setDetectionThreshold(const int detectionThreshold)
        {
            _detectionThreshold=detectionThreshold;
        }

    private:
        uint64_t _frameCounter;
        uint8_t *_scratchData;
        
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
