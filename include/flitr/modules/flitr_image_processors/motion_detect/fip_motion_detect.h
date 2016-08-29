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
         *@param buffer_size The size of the shared image buffer of the downstream producer.*/
        FIPMotionDetect(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                        const bool showOverlays, const bool produceOnlyMotionImages, const int motionThreshold,
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



    private:
        uint64_t upStreamFrameCount_;

        uint8_t *scratchData_;
        double *integralImageScratchData_;
        BoxFilterII boxFilter_; //No significant state associated with this.
        
        uint8_t *currentFrame_;
        uint8_t *previousFrame_;
        
        bool showOverlays_;
        bool produceOnlyMotionImages_;
        int motionThreshold_;
    };
    
}

#endif //FIP_MOTION_DETECT_H
