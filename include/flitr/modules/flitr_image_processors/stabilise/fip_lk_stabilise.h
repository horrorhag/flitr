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

#ifndef FIP_LK_STABILISE_H
#define FIP_LK_STABILISE_H 1

#include <flitr/image_processor.h>
#include <mutex>

namespace flitr {

/*! Uses LK optical flow to dewarp scintillaty images. */
class FLITR_EXPORT FIPLKStabilise : public ImageProcessor
{
public:
    
    enum class Mode : uint8_t { NOTRANSFORM = 1, CROP_FILTER_SUBPIXELSTAB = 2, SUBPIXELSTAB = 3, INTSTAB = 4 };

    /*! Constructor given the upstream producer.
         *@param upStreamProducer The upstream image producer.
         *@param images_per_slot The number of images per image slot from the upstream producer.
         *@param Mode Mode of transform applied to the output image.
         *@param buffer_size The size of the shared image buffer of the downstream producer.*/
    FIPLKStabilise(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                   Mode outputMode,
                   uint32_t buffer_size=FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS);

    /*! Virtual destructor */
    virtual ~FIPLKStabilise();

    /*! Method to initialise the object.
         *@return Boolean result flag. True indicates successful initialisation.*/
    virtual bool init();

    /*! Synchronous trigger method. Called automatically by the trigger thread in ImageProcessor base class if started.
         *@sa ImageProcessor::startTriggerThread*/
    virtual bool trigger();

    /*! Get the latest h-vector calculated between the input and reference frame. Should be called after trigger().
         */
    virtual void getLatestHVect(float &hx, float &hy, size_t &frameNumber) const
    {
        std::lock_guard<std::mutex> scopedLock(latestHMutex_);

        hx=latestHx_;//Hx_;
        hy=latestHy_;//Hy_;
        frameNumber=latestHFrameNumber_;//frameNumber_;
    }

    /*! Burns/filters the output image transform.
         *@param fx Factor [0..1] by which to reduce the output image transform in x.
         *@param fy Factor [0..1] by which to reduce the output image transform in y.
     */
    virtual void setupOutputTransformBurn(const float fx, const float fy)
    {
        burnFx_=fx;
        burnFy_=fy;
    }

    virtual void getOutputTransformBurn(float &fx, float &fy)
    {
        fx=burnFx_;
        fy=burnFy_;
    }

    virtual std::string getTitle() {
        return Title_;
    }

    virtual int getNumberOfParms() override
    {
        return 3;
    }

    virtual flitr::Parameters::EParmType getParmType(int id) override
    {
        switch (id)
        {
        case 0 :return flitr::Parameters::PARM_INT;
        case 1 :return flitr::Parameters::PARM_FLOAT;
        case 2 :return flitr::Parameters::PARM_FLOAT;
        }
        return flitr::Parameters::PARM_UNDF;
    }

    virtual std::string getParmName(int id) override
    {
        switch (id)
        {
        case 0 :return std::string("Output Mode");
        case 1 :return std::string("Output Transform Burn X");
        case 2 :return std::string("Output Transform Burn Y");
        }
        return std::string("???");
    }

    virtual float getFloat(int id) override
    {
        switch (id)
        {
        case 1 : return burnFx_;
        case 2 : return burnFy_;
        }

        return 0.0f;
    }

    virtual int getInt(int id) override
    {
        switch (id)
        {
        case 0 : return static_cast<int>(outputMode_);
        }
        return 0;
    }

    virtual bool setFloat(int id, float v) override
    {
        switch (id)
        {
            case 1 : burnFx_=v; return true;
            case 2 : burnFy_=v; return true;
        }

        return false;
    }

private:
    inline float bilinear(float const * const data, const ptrdiff_t offsetLT, const ptrdiff_t width, const float fx, const float fy) const
    {
        return
                data[offsetLT] * ((1.0f-fx) * (1.0f-fy)) + data[offsetLT+((ptrdiff_t)1)] * (fx * (1.0f-fy)) +
                data[offsetLT+width] * ((1.0f-fx) * fy) + data[offsetLT+(((ptrdiff_t)1)+width)] * (fx * fy);
    }

    std::string Title_;

    size_t numLevels_;

    std::vector<float *> imgVec_;
    std::vector<float *> refImgVec_;

    std::vector<float *> dxVec_;
    std::vector<float *> dyVec_;
    std::vector<float *> dSqRecipVec_;

    float *scratchData_;

    Mode outputMode_;

    mutable std::mutex latestHMutex_;
    float latestHx_;
    float latestHy_;
    size_t latestHFrameNumber_;

    float sumHx_;
    float sumHy_;
    float burnFx_;
    float burnFy_;
};

}

#endif //FIP_LK_STABILISE_H
