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

#ifndef FIP_AVERAGE_IMAGE_H
#define FIP_AVERAGE_IMAGE_H 1

#include <flitr/image_processor.h>

namespace flitr {
    
    /*! Calculates the average image on the CPU. The performance is independent of the number of frames. */
    class FLITR_EXPORT FIPAverageImage : public ImageProcessor
    {
    public:
        
        /*! Constructor given the upstream producer.
         *@param upStreamProducer The upstream image producer.
         *@param images_per_slot The number of images per image slot from the upstream producer.
         *@param buffer_size The size of the shared image buffer of the downstream producer.*/
        FIPAverageImage(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                        uint8_t base2WindowLength,
                        uint32_t buffer_size=FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS);
        
        /*! Virtual destructor */
        virtual ~FIPAverageImage();
        
        /*! Method to initialise the object.
         *@return Boolean result flag. True indicates successful initialisation.*/
        virtual bool init();
        
        /*!Synchronous trigger method. Called automatically by the trigger thread in ImageProcessor base class if started.
         *@sa ImageProcessor::startTriggerThread*/
        virtual bool trigger();

        virtual std::string getTitle()
        {
            return Title_;
        }
        
    private:
        const uint8_t base2WindowLength_;
        const size_t windowLength_;
        const float recipWindowLength_;
        std::string Title_;

        /*! The sum images per slot. */
        std::vector<float *> sumImageVec_;
        
        /*! The history ring buffer/vector for each image in the slot. */
        std::vector<std::vector<float * > > historyImageVecVec_;
        
        size_t oldestHistorySlot_;
    };
    
}

#endif //FIP_AVERAGE_IMAGE_H
