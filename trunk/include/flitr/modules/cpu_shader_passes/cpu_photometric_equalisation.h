#ifndef CPU_PHOTOMETRIC_EQUALISATION_SHADER
#define CPU_PHOTOMETRIC_EQUALISATION_SHADER 1

#include <flitr/flitr_export.h>
#include <flitr/modules/cpu_shader_passes/cpu_shader_pass.h>
#include <flitr/stats_collector.h>


namespace flitr {

class FLITR_EXPORT CPUPhotometricEqualisation_Shader : public flitr::CPUShaderPass::CPUShader
{
public:
    CPUPhotometricEqualisation_Shader(osg::Image* image, double initialTargetAverage, double targetAverageUpdateSpeed=0.025) :
        Image_(image),
        TargetAverage_(new double),
        TargetAverageUpdateSpeed_(targetAverageUpdateSpeed),
        enabled_(true)
    {
        *TargetAverage_=initialTargetAverage;

        stats_ = std::shared_ptr<StatsCollector>(new StatsCollector("CPUPhotometricEqualisation_Shader"));
    }

    virtual ~CPUPhotometricEqualisation_Shader()
    {
        delete TargetAverage_;
    }

    virtual void operator () (osg::RenderInfo& renderInfo) const
    {
        if (enabled_)
        {
            stats_->tick();

            const unsigned long width=Image_->s();
            const unsigned long height=Image_->t();
            const unsigned long numPixels=width*height;
            const unsigned long numComponents=osg::Image::computeNumComponents(Image_->getPixelFormat());

            unsigned char * const data=(unsigned char *)Image_->data();

            uint64_t sum0=0;
            uint64_t sum1=0;

            const uint32_t numElements=numPixels*numComponents;
            const uint32_t numElementsDiv2=numElements>>1;



            //CPU out of order optimisation to calculate image sum.
            for (uint32_t i=0; i<numElements; i+=2)
            {
                sum0+=data[i];
                sum1+=data[i+1];
            }
            sum0+=sum1;


            const double instAverage=(sum0 / (255.0*numElements));
            *TargetAverage_=(*TargetAverage_)*(1.0-TargetAverageUpdateSpeed_) + instAverage*TargetAverageUpdateSpeed_;

            const double ef=(*TargetAverage_) / instAverage;


            //=== set up lookup table for equalisation ===
            unsigned char remap[256];
            for (int i=0; i<256; i++)
            {
                const double equalisedValue=i*ef+0.5;
                remap[i]=equalisedValue<255.5 ? equalisedValue : 255;
            }
            //=== ===


#pragma omp parallel sections
            {
#pragma omp section
                {
                    for (uint32_t i=0; i<numElementsDiv2; i++)
                    {
                        data[i]=remap[data[i]];
                    }
                }

#pragma omp section
                {
                    for (uint32_t i=numElementsDiv2; i<numElements; i++)
                    {
                        data[i]=remap[data[i]];
                    }
                }
            }

            Image_->dirty();

            stats_->tock();
        }
    }


    virtual int getNumberOfParms()
    {
        return 2;
    }

    virtual EParmType getParmType(int id)
    {
        return PARM_FLOAT;
    }


    virtual std::string getParmName(int id)
    {
        if (id==0)
        {
            return std::string("Average");
        } else
        {
            return std::string("Update Speed");
        }
    }


    virtual float getFloat(int id)
    {
        if (id==0)
        {
            return *TargetAverage_;
        } else
        {
            return TargetAverageUpdateSpeed_;
        }
    }

    virtual bool getFloatRange(int id, float &low, float &high)
    {
        low=0.00;
        high=1.0;

        return true;
    }

    virtual bool setFloat(int id, float v)
    {
        if (id==0)
        {
            *TargetAverage_=v;
        } else
        {
            TargetAverageUpdateSpeed_=v;
        }
        return true;
    }

    virtual std::string getTitle()
    {
        return "Photometric Equalisation";
    }

    virtual void enable(bool state=true)
    {
        enabled_=state;
    }

    virtual bool isEnabled()
    {
        return enabled_;
    }

protected:
    osg::Image* Image_;
    double *TargetAverage_;
    double TargetAverageUpdateSpeed_;

    std::shared_ptr<StatsCollector> stats_;

    bool enabled_;
};
}
#endif //CPU_PHOTOMETRIC_EQUALISATION_SHADER
