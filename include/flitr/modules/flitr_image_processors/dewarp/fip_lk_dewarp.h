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

#ifndef FIP_LK_DEWARP_H
#define FIP_LK_DEWARP_H 1

#include <flitr/image_processor_utils.h>
#include <flitr/image_processor.h>

#include <mutex>

namespace flitr {
    
    /*! Uses LK optical flow to dewarp scintillaty images. */
    class FLITR_EXPORT FIPLKDewarp : public ImageProcessor
    {
    public:
        
        /*! Constructor given the upstream producer.
         *@param upStreamProducer The upstream image producer.
         *@param images_per_slot The number of images per image slot from the upstream producer.
         *@param avrgImageLongevity Filter constant for how strong the averaging of the reference image is.
         *@param buffer_size The size of the shared image buffer of the downstream producer.*/
        FIPLKDewarp(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                    const float avrgImageLongevity,
                    uint32_t buffer_size=FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS);
        
        /*! Virtual destructor */
        virtual ~FIPLKDewarp();
        
        /*! Method to initialise the object.
         *@return Boolean result flag. True indicates successful initialisation.*/
        virtual bool init();
        
        /*! Synchronous trigger method. Called automatically by the trigger thread in ImageProcessor base class if started.
         *@sa ImageProcessor::startTriggerThread*/
        virtual bool trigger();
        
        
    private:
        inline float bilinearRead(float const * const data,
                                  const ptrdiff_t offsetLT,
                                  const ptrdiff_t width,
                                  const float fx, const float fy)
        {
            return
            data[offsetLT] * ((1.0f-fx) * (1.0f-fy)) +
            data[offsetLT+((ptrdiff_t)1)] * (fx * (1.0f-fy)) +
            data[offsetLT+width] * ((1.0f-fx) * fy) +
            data[offsetLT+(((ptrdiff_t)1)+width)] * (fx * fy);
        }
        
        inline void bilinearAdd(const float value, float * const data, const ptrdiff_t offsetLT, const ptrdiff_t width, const float fx, const float fy)
        {
            data[offsetLT]                        += value * ((1.0f-fx) * (1.0f-fy));
            data[offsetLT+((ptrdiff_t)1)]         += value * (fx * (1.0f-fy));
            data[offsetLT+width]                  += value * ((1.0f-fx) * fy);
            data[offsetLT+(((ptrdiff_t)1)+width)] += value * (fx * fy);
        }
        
        const float avrgImageLongevity_;
        const float recipGradientThreshold_;
        
        const size_t numLevels_;
        
        std::vector<float *> imgVec_;
        std::vector<float *> refImgVec_;
        
        std::vector<float *> dxVec_;
        std::vector<float *> dyVec_;
        std::vector<float *> dSqRecipVec_;
        
        std::vector<float *> hxVec_;
        std::vector<float *> hyVec_;
        
        GaussianFilter gaussianFilter_;
        GaussianDownsample gaussianDownsample_;

        GaussianFilter gaussianReguFilter_;
        
        float *scratchData_;
        
        float *inputImgData_;
        float *finalImgData_; //With lucky regions, etc.
    };
    
}

#endif //FIP_LK_DEWARP_H
