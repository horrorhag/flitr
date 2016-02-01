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

#ifndef FIP_TRANSFORM_2D_H
#define FIP_TRANSFORM_2D_H 1

#include <flitr/image_processor.h>

namespace flitr {
    
    /*! Applies a 2D matrix transform to the image. */
    class FLITR_EXPORT FIPTransform2D : public ImageProcessor
    {
    public:
        
        struct M2D
        {
            float a_, c_;
            float b_, d_;
            
            M2D(const float a, const float b, const float c, const float d) :
            a_(a), c_(c), b_(b), d_(d)
            {}
        };
        
        /*! Constructor given the upstream producer.
         *@param upStreamProducer The upstream image producer.
         *@param images_per_slot The number of images per image slot from the upstream producer.
         *@param buffer_size The size of the shared image buffer of the downstream producer.*/
        FIPTransform2D(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                       const std::vector<M2D> transformVect,
                       uint32_t buffer_size=FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS);
        
        /*! Virtual destructor */
        virtual ~FIPTransform2D();
        
        /*! Method to initialise the object.
         *@return Boolean result flag. True indicates successful initialisation.*/
        virtual bool init();
        
        /*!Synchronous trigger method. Called automatically by the trigger thread in ImageProcessor base class if started.
         *@sa ImageProcessor::startTriggerThread*/
        virtual bool trigger();
        
    private:
        std::vector<M2D> transformVect_;
    };
}

#endif //FIP_TRANSFORM_2D_H
