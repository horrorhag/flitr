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

#ifndef TARGET_INJECTOR_H
#define TARGET_INJECTOR_H 1

#include <flitr/image_processor.h>

namespace flitr {


class FLITR_EXPORT TargetInjector : public ImageProcessor
{
public:

    class FLITR_EXPORT SyntheticTarget
    {
    public:
        SyntheticTarget(float px, float py,
                        float dx, float dy, float v,
                        float sa, float sb) :
            px_(px), py_(py),
            dx_(dx), dy_(dy), v_(v),
            sa_(sa), sb_(sb)
        {
            //Normalise the direction vector.
            dx_/=sqrt(dx_*dx_+dy_*dy_);
            dy_/=sqrt(dx_*dx_+dy_*dy_);
        }


        //Note: Makes use of default copy and assignment constructors.


        void update()
        {
            px_+=dx_ * v_;
            py_+=dy_ * v_;
        }

        inline float UNGaussian1D(const float d, const float s) const
        {
            if (fabsf(d)<(4.0f*s))
            {
                return expf(-0.5f * powf(d/s,2.0f));
            } else
            {
                return 0.0f;
            }
        }

        float getSupportDensity(const float x, const float y) const
        {
            float a=(x-px_)*dx_ + (y-py_)*dy_;
            float b=(x-px_)*dy_ - (y-py_)*dx_;

            return UNGaussian1D(a,sa_)*UNGaussian1D(b,sb_);
        }

    private:
        float px_, py_;
        float dx_, dy_, v_;

        float sa_, sb_;
    };






    /*! Constructor given the upstream producer.
             *@param producer The upstream image producer.
             *@param images_per_slot The number of images per image slot from the upstream producer.
             *@param buffer_size The size of the shared image buffer of the downstream producer.*/
    TargetInjector(ImageProducer& producer, uint32_t images_per_slot, uint32_t buffer_size=FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS);

    /*! Virtual destructor */
    virtual ~TargetInjector();

    /*! Method to initialise the object.
             *@return Boolean result flag. True indicates successful initialisation.*/
    virtual bool init();

    /*!Synchronous trigger method. Called automatically by the trigger thread if started.
             *@sa ImageProcessor::startTriggerThread*/
    virtual bool trigger();

    virtual void setTargetBrightness(const float brightness)
    {
        targetBrightness_=std::min<float>(std::max<float>(brightness, 0.0f),255.5f);
    }

    virtual void addTarget(const SyntheticTarget &target)
    {
        targetVector_.push_back(target);
    }

private:

    float targetBrightness_;
    std::vector<SyntheticTarget> targetVector_;
};

}

#endif //TARGET_INJECTOR_H
