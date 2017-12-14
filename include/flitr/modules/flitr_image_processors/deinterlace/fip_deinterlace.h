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

#ifndef FIP_DEINTERLACE_H
#define FIP_DEINTERLACE_H

#include <flitr/image_processor.h>




//# Removed this namespace entending for IPF deinterlace to work
namespace flitr{
    enum FLITR_DEINTERLACE_METHODS {smoothFilter, linear, interframe};

    /*! De-interlaces a video scream. The performance is independent of the number of frames. */
    class FLITR_EXPORT FIPDeinterlace : public ImageProcessor{
    private:
        //Count all the images
        unsigned int imageCount = 0;

        flitr::Image * imReadPrevious; // = NULL;
        float  * dataDeinterlacedIm;
        flitr::Image * deinterlacedIm;


    public:

        /*#include "fip_deinterlace.h"! Constructor given the upstream producer.
         *@param upStreamProducer The upstream image producer.
         *@param images_per_slot The number of images per image slot from the upstream producer.
         *@param buffer_size The size of the shared image buffer of the downstream producer.*/
        FIPDeinterlace(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                        uint32_t buffer_size=FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS, FLITR_DEINTERLACE_METHODS method = FLITR_DEINTERLACE_METHODS::interframe);

        /*! Virtual destructor */
        virtual ~FIPDeinterlace();

        /*! Method to initialise the object.
         *@return Boolean result flag. True indicates successful initialisation.*/
        virtual bool init();

        /*!Synchronous trigger method. Called automatically by the trigger thread if started.
         *@sa ImageProcessor::startTriggerThread*/
        virtual bool trigger();



    private:
        flitr::FLITR_DEINTERLACE_METHODS _method;

        /*! The sum images per slot. */
        std::vector<float *> resultImageVec_;

        /*! Three Consecutive images for Deinterlace filter */
        std::vector<float *> currentImageVec_;




        /*! Pixel on the same position of the 2 consecutive image */
        float * PixelsOnSamePosition = NULL;

        /*! The history ring buffer/vector for each image in the slot. */
        //std::vector<std::vector<float * > > historyImageVecVec_;
        //size_t oldestHistorySlot_;
    };

}

#endif //FIP_DEINTERLACE_H
