#ifndef CPU_PHOTOMETRIC_EQUALISATION_SHADER
#define CPU_PHOTOMETRIC_EQUALISATION_SHADER 1

#include <flitr/modules/cpu_shader_passes/cpu_shader_pass.h>

namespace flitr {

class FLITR_EXPORT CPUPhotometricCalibration_Shader : public flitr::CPUShaderPass::CPUShader
{
public:
    CPUPhotometricCalibration_Shader(osg::Image* image, double initialTargetAverage, double targetAverageUpdateSpeed=0.025) :
        Image_(image),
        TargetAverage_(new double),
        TargetAverageUpdateSpeed_(targetAverageUpdateSpeed)
    {
        *TargetAverage_=initialTargetAverage;
    }

    ~CPUPhotometricCalibration_Shader()
    {
        delete TargetAverage_;
    }

    virtual void operator () (osg::RenderInfo& renderInfo) const
    {
        const unsigned long width=Image_->s();
        const unsigned long height=Image_->t();
        const unsigned long numPixels=width*height;
        const unsigned long numComponents=osg::Image::computeNumComponents(Image_->getPixelFormat());

        unsigned char * const data=(unsigned char *)Image_->data();

        uint64_t sum=0;
        uint32_t numElementsSummed=0;

        const uint32_t numElements=numPixels*numComponents;
        for (uint32_t i=0; i<numElements; i++)
        {
            if ((data[i]<224)&&(data[i]>32))//Only use mid-tones for measurement.
            {
                sum+=data[i];
                numElementsSummed++;
            }
        }

        if (numElementsSummed)
        {
            const double instAverage=(sum / (255.0*numElementsSummed));
            *TargetAverage_=(*TargetAverage_)*(1.0-TargetAverageUpdateSpeed_) + instAverage*TargetAverageUpdateSpeed_;

            const double ef=(*TargetAverage_) / instAverage;

            for (uint32_t i=0; i<numElements; i++)
            {
                double equalisedValue=data[i]*ef;
                data[i]=equalisedValue<255 ? equalisedValue : 255;
            }

            Image_->dirty();
        }
    }

    osg::Image* Image_;
    double *TargetAverage_;
    double TargetAverageUpdateSpeed_;
};
}
#endif //CPU_PHOTOMETRIC_EQUALISATION_SHADER
