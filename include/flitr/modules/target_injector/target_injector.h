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

#include <algorithm>

//#include <boost/timer.hpp>

namespace flitr {


class FLITR_EXPORT TargetInjector : public ImageProcessor
{
public:

    class FLITR_EXPORT SyntheticTarget
    {
    public:
        SyntheticTarget(float px, float py,
                        float dx, float dy, float v,
                        float sa, float sb,
                        float dsa=0.0f, float dsb=0.0f) :
            px_(px), py_(py),
            dx_(dx), dy_(dy), v_(v),
            sa_(sa), sb_(sb),
            dsa_(dsa), dsb_(dsb),
			interval_(0),
			elapsedTime_(0)
        {
            //Normalise the direction vector.
            dx_/=sqrt(dx_*dx_+dy_*dy_);
            dy_/=sqrt(dx_*dx_+dy_*dy_);
        }

		SyntheticTarget(float px, float py, 
						const std::vector< std::pair<float, float> >& samples, 
						float interval,
						float sa, float sb,
						float dsa=0.0f, float dsb=0.0f) :
			px_(px), py_(py),
            dx_(0), dy_(0), v_(0),
            sa_(sa), sb_(sb),
            dsa_(dsa), dsb_(dsb),
			samples_(samples),
			interval_(interval),
			elapsedTime_(0)
		{
		}


        //Note: Makes use of default copy and assignment constructors.


        void update(float dT)
        {
			if (interval_ == 0.0f)
			{
				px_+=dx_ * v_;
				py_+=dy_ * v_;
			}
			else
			{
				elapsedTime_ += dT;
				int index = (int)(elapsedTime_ / interval_);
				
				if (index >= (int)samples_.size() - 1) 
				{
					px_ = samples_.back().first;
					py_ = samples_.back().second;
				}
				else
				{
					std::pair<float, float> curSample = samples_[index];
					std::pair<float, float> nextSample = samples_[index+1];
					float weight = (elapsedTime_ - (index * interval_)) / interval_;

					// lerp
					float x = curSample.first + (nextSample.first - curSample.first) * weight;
					float y = curSample.second + (nextSample.second - curSample.second) * weight;

					dx_ = (x - px_);
					dy_ = (y - py_);
					//Normalise the direction vector.
					dx_/=sqrt(dx_*dx_+dy_*dy_);
					dy_/=sqrt(dx_*dx_+dy_*dy_);

					px_ = x;
					py_ = y;
				}
				
			}

            sa_+=dsa_;
            sb_+=dsb_;
        }

        inline float UNGaussian1D(const float d, const float s) const
        {
            return expf(-0.5f * powf(d/s,2.0f));
        }

        float getSupportDensity(const float x, const float y) const
        {
            float supportDensity=0.0f;
            unsigned long numSubpixelSamples=0;

            const float subPixelSize=0.1f;//0.1f, 0.2f, 0.4f

            const float rx=x-px_;
            const float ry=y-py_;

            if ((fabsf(rx)<sa_*4.0f)&&(fabsf(ry)<sa_*4.0f)) //Assumes that sa_>sb_
            {

                for (float subPixelY=-(0.5f-subPixelSize*0.5f); subPixelY<0.5f; subPixelY+=subPixelSize)
                {
                    const float aY=(ry+subPixelY)*dy_;
                    const float bY=-(ry+subPixelY)*dx_;

                    for (float subPixelX=-(0.5f-subPixelSize*0.5f); subPixelX<0.5f; subPixelX+=subPixelSize)
                    {
                        float a=(rx+subPixelX)*dx_ + aY;
                        float b=(rx+subPixelX)*dy_ + bY;

                        supportDensity+=UNGaussian1D(a,sa_)*UNGaussian1D(b,sb_);
                        numSubpixelSamples++;
                    }
                }


                return supportDensity/numSubpixelSamples;
            }

            return 0.0f;
        }

    private:
        float px_, py_;
        float dx_, dy_, v_;

        float sa_, sb_;
        float dsa_, dsb_;

		std::vector<std::pair<float, float> > samples_;	//!< position samples for a given interval [pixels]
		float interval_;								//!< interval [s]
		float elapsedTime_;									//!< elapsed time since start [s]
    };






    /*! Constructor given the upstream producer.
             *@param producer The upstream image producer.
             *@param images_per_slot The number of images per image slot from the upstream producer.
             *@param buffer_size The size of the shared image buffer of the downstream producer.*/
    TargetInjector(ImageProducer& upStreamProducer, uint32_t images_per_slot, uint32_t buffer_size=FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS);

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
    double startTimeSec_;//boost::timer timer_;
};

}

#endif //TARGET_INJECTOR_H
