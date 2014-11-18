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

        stats_ = std::shared_ptr<StatsCollector>(new StatsCollector("CPUPhotometricEqualisation_Shader"));
    }

    virtual ~CPULocalPhotometricEqualisation_Shader()
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
            const unsigned long widthTimesNumComponents=width*numComponents;

            unsigned char * const data=(unsigned char *)Image_->data();


            const uint32_t numElements=numPixels*numComponents;

            uint64_t sum=0;

            //=== Calc integral image ===
            int64_t * const integralImageData=new int64_t[numElements];
            {
                unsigned long integralImageOffset=0;
                unsigned long imageOffset=0;

                integralImageData[0]=data[0];

                imageOffset=numComponents;
                integralImageOffset=1;

                for (unsigned long x=1; x<width; x++)
                {
                    integralImageData[integralImageOffset]=((int64_t)data[imageOffset])
                            + integralImageData[integralImageOffset-1];

                    integralImageOffset++;
                    imageOffset+=numComponents;
                }

                imageOffset=widthTimesNumComponents;
                integralImageOffset=width;
                for (unsigned long y=1; y<height; y++)
                {
                    integralImageData[integralImageOffset]=((int64_t)data[imageOffset])
                            + integralImageData[integralImageOffset-width];

                    integralImageOffset+=width;
                    imageOffset+=widthTimesNumComponents;
                }

                imageOffset=widthTimesNumComponents;
                integralImageOffset=width;
                for (unsigned long y=1; y<height; y++)
                {
                    integralImageOffset++;
                    unsigned long integralImageOffsetMinusOne=integralImageOffset-1;
                    unsigned long integralImageOffsetMinusWidth=integralImageOffset-width;
                    unsigned long integralImageOffsetMinusWidthMinusOne=integralImageOffset-width-1;

                    imageOffset+=numComponents;

                    for (unsigned long x=1; x<width; x++)
                    {
                        integralImageData[integralImageOffset]=((int64_t)data[imageOffset])
                                + integralImageData[integralImageOffsetMinusOne]
                                + integralImageData[integralImageOffsetMinusWidth]
                                - integralImageData[integralImageOffsetMinusWidthMinusOne];

                        integralImageOffset++;
                        integralImageOffsetMinusOne++;
                        integralImageOffsetMinusWidth++;
                        integralImageOffsetMinusWidthMinusOne++;
                        imageOffset+=numComponents;
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

                long integralImageOffset=startOffset;
                for (long y=border; y<heightMinusBorder; y++)
                {
                    integralImageOffset+=border;
                    unsigned long integralImageOffsetMinusBorderMinusStartOffset=integralImageOffset-border-startOffset;
                    unsigned long integralImageOffsetPlusBorderPlusStartOffset=integralImageOffset+border+startOffset;
                    unsigned long integralImageOffsetPlusBorderMinusStartOffset=integralImageOffset+border-startOffset;
                    unsigned long integralImageOffsetMinusBorderPlusStartOffset=integralImageOffset-border+startOffset;

                    long imageOffset=integralImageOffset*numComponents;

                    for (long x=border; x<widthMinusBorder; x++)
                    {
                        float average=(integralImageData[integralImageOffsetPlusBorderPlusStartOffset]
                                       + integralImageData[integralImageOffsetMinusBorderMinusStartOffset]
                                       - integralImageData[integralImageOffsetPlusBorderMinusStartOffset]
                                       - integralImageData[integralImageOffsetMinusBorderPlusStartOffset]
                                       );

                        for (long i=0; i<numComponents; ++i)
                        {
                            data[imageOffset+i]=data[imageOffset+i] * (floatTargetAverage / average) + 0.5f;
                        }

                        integralImageOffset++;
                        integralImageOffsetMinusBorderMinusStartOffset++;
                        integralImageOffsetPlusBorderPlusStartOffset++;
                        integralImageOffsetPlusBorderMinusStartOffset++;
                        integralImageOffsetMinusBorderPlusStartOffset++;

                        imageOffset+=numComponents;
                    }
                    integralImageOffset+=border;
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

    std::shared_ptr<StatsCollector> stats_;

    bool enabled_;
};
}
#endif //CPU_LOCAL_PHOTOMETRIC_EQUALISATION_SHADER
