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

#ifndef FIP_CAMERA_SHAKE_H
#define FIP_CAMERA_SHAKE_H 1

#include <flitr/image_processor.h>
#include <random>
#include <mutex>

namespace flitr {
    
    /*! Applies simulated camera shake to the image. */
    class FLITR_EXPORT FIPCameraShake : public ImageProcessor
    {
    public:
        
        /*! Constructor given the upstream producer.
         *@param upStreamProducer The upstream image producer.
         *@param images_per_slot The number of images per image slot from the upstream producer.
         *@param kernelWidth Width of kernel in pixels; recommended to be at least 2x the standard deviation!
         *@param sd standard deviation of Gaussian random shake in pixels.
         *@param buffer_size The size of the shared image buffer of the downstream producer.*/
        FIPCameraShake(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                       const float sd,
                       const size_t kernelWidth,
                       uint32_t buffer_size=FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS);
        
        /*! Virtual destructor */
        virtual ~FIPCameraShake();

        /*! Sets the amplitude of the shake in pixels.
         *@param sd standard deviation of Gaussian random shake in pixels.*/
        virtual void setShakeSD(const float sd);
        
        /*! Method to initialise the object.
         *@return Boolean result flag. True indicates successful initialisation.*/
        virtual bool init();
        
        /*!Synchronous trigger method. Called automatically by the trigger thread if started.
         *@sa ImageProcessor::startTriggerThread*/
        virtual bool trigger();
        
        /*! Get the latest h-vector calculated between the input and reference frame. Should be called after trigger().
         */
        virtual void getLatestHVect(float &hx, float &hy, size_t &frameNumber) const
        {
            std::lock_guard<std::mutex> scopedLock(latestHMutex_);
            
            hx=latestHx_;//currentX_ - oldX_;
            hy=latestHy_;//currentY_ - oldY_;
            frameNumber=latestHFrameNumber_;//frameNumber_;
        }

    private:
        /*! Updates/generates the kernel with a random mean taken from a Gaussian distribution with mean zero and standard deviation sd_. */
        virtual void updateKernel();
        
        std::random_device randDev_;
        std::mt19937 randGen_;
        std::normal_distribution<float> randNormDist_;
        
        float *kernel2D_;
        float sd_;
        const size_t kernelWidth_;
        
        float currentX_;
        float currentY_;
        
        float oldX_;
        float oldY_;
        
        mutable std::mutex latestHMutex_;
        float latestHx_;
        float latestHy_;
        size_t latestHFrameNumber_;
    };
}

#endif //FIP_CAMERA_SHAKE_H
