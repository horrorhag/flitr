#ifndef CPU_LOCAL_PHOTOMETRIC_EQUALISATION_SHADER
#define CPU_LOCAL_PHOTOMETRIC_EQUALISATION_SHADER 1

#include <flitr/flitr_export.h>
#include <flitr/modules/cpu_shader_passes/cpu_shader_pass.h>
#include <flitr/stats_collector.h>


namespace flitr {

class FLITR_EXPORT CPULocalPhotometricEqualisation_Shader : public flitr::CPUShaderPass::CPUShader
{
public:
    CPULocalPhotometricEqualisation_Shader(osg::Image* image, float localRegionSize, double initialTargetAverage, double targetAverageUpdateSpeed=0.025) :
        localRegionSize_(localRegionSize),
        Image_(image),
        TargetAverage_(new double),
        TargetAverageUpdateSpeed_(targetAverageUpdateSpeed),
        enabled_(true)
    {
        *TargetAverage_=initialTargetAverage;

        stats_ = std::tr1::shared_ptr<StatsCollector>(new StatsCollector("CPUPhotometricEqualisation_Shader"));
    }

    ~CPULocalPhotometricEqualisation_Shader()
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
            const unsigned long numComponents=1;//osg::Image::computeNumComponents(Image_->getPixelFormat());

            unsigned char * const data=(unsigned char *)Image_->data();


            const uint32_t numElements=numPixels*numComponents;
            const uint32_t numElementsDiv2=numElements>>1;

            uint64_t sum=0;
            /*
        uint64_t sum1=0;
        //CPU out of order optimisation to calculate image sum.
        for (uint32_t i=0; i<numElements; i+=2)
        {
            sum+=data[i];
            sum1+=data[i+1];
        }
        sum+=sum1;
        */

            //=== Calc integral image ===
            int64_t * const integralImageData=new int64_t[numElements];
            {
                unsigned long offset=0;

                integralImageData[0]=data[0];

                offset=1;
                for (unsigned long x=1; x<width; x++)
                {
                    integralImageData[offset]=((int64_t)data[offset])
                            + integralImageData[offset-1];

                    offset++;
                }

                offset=width;
                for (unsigned long y=1; y<height; y++)
                {
                    integralImageData[offset]=((int64_t)data[offset])
                            + integralImageData[offset-width];

                    offset+=width;
                }

                offset=width;
                for (unsigned long y=1; y<height; y++)
                {
                    offset++;
                    unsigned long offsetMinusOne=offset-1;
                    unsigned long offsetMinusWidth=offset-width;
                    unsigned long offsetMinusWidthMinusOne=offset-width-1;

                    for (unsigned long x=1; x<width; x++)
                    {
                        integralImageData[offset]=((int64_t)data[offset])
                                + integralImageData[offsetMinusOne]
                                + integralImageData[offsetMinusWidth]
                                - integralImageData[offsetMinusWidthMinusOne];

                        offset++;
                        offsetMinusOne++;
                        offsetMinusWidth++;
                        offsetMinusWidthMinusOne++;
                    }
                }
            }
            //=== ===


            sum=integralImageData[numElements-1];
            const double instAverage=(sum / (255.0*numElements));
            *TargetAverage_=(*TargetAverage_)*(1.0-TargetAverageUpdateSpeed_) + instAverage*TargetAverageUpdateSpeed_;


            //=== Local photometric equalisation ===
            {
                const long localRegionSizeInPixels=width * localRegionSize_;
                const long border=(localRegionSizeInPixels>>1)+1;
                const float floatTargetAverage=(*TargetAverage_)*((border+border)*(border+border) * 255.0f);

                const long startOffset=border*width;
                const long widthMinusBorder=width - border;
                const long heightMinusBorder=height - border;

                long offset=startOffset;
                for (long y=border; y<heightMinusBorder; y++)
                {
                    offset+=border;
                    unsigned long offsetMinusBorderMinusStartOffset=offset-border-startOffset;
                    unsigned long offsetPlusBorderPlusStartOffset=offset+border+startOffset;
                    unsigned long offsetPlusBorderMinusStartOffset=offset+border-startOffset;
                    unsigned long offsetMinusBorderPlusStartOffset=offset-border+startOffset;

                    for (long x=border; x<widthMinusBorder; x++)
                    {
                        float average=(integralImageData[offsetPlusBorderPlusStartOffset]
                                       + integralImageData[offsetMinusBorderMinusStartOffset]
                                       - integralImageData[offsetPlusBorderMinusStartOffset]
                                       - integralImageData[offsetMinusBorderPlusStartOffset]
                                       );

                        data[offset]=data[offset] * (floatTargetAverage / average) + 0.5f;

                        offset++;
                        offsetMinusBorderMinusStartOffset++;
                        offsetPlusBorderPlusStartOffset++;
                        offsetPlusBorderMinusStartOffset++;
                        offsetMinusBorderPlusStartOffset++;
                    }
                    offset+=border;
                }
            }
            //=== ===


            /*
=================================
OLD PHOTOMETRIC EQUALISATION CODE
        const double ef=(*TargetAverage_) / instAverage;


        //=== set up lookup table for equalisation ===
        unsigned char remap[256];
        for (int i=0; i<256; i++)
        {
            const double equalisedValue=i*ef+0.5;
            remap[i]=equalisedValue<255.5 ? equalisedValue : 255.5;
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
=================================
*/
            Image_->dirty();

            delete [] integralImageData;

            stats_->tock();
        }
    }

    virtual int getNumberOfParms()
    {
        return 3;
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
        } else if (id==1)
        {
            return std::string("Update Speed");
        } else
        {
            return std::string("Local Region Size");
        }
    }


    virtual float getFloat(int id)
    {
        if (id==0)
        {
            return *TargetAverage_;
        } else if (id==1)
        {
            return TargetAverageUpdateSpeed_;
        } else
        {
            return localRegionSize_;
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
        } else if (id==1)
        {
            TargetAverageUpdateSpeed_=v;
        } else
        {
            localRegionSize_=v;
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
    float localRegionSize_;

    osg::Image* Image_;

    double *TargetAverage_;
    double TargetAverageUpdateSpeed_;

    std::tr1::shared_ptr<StatsCollector> stats_;

    bool enabled_;
};
}
#endif //CPU_LOCAL_PHOTOMETRIC_EQUALISATION_SHADER
