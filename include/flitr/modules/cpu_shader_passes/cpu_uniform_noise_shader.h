#ifndef CPU_UNIFORM_NOISE_SHADER
#define CPU_UNIFORM_NOISE_SHADER 1

#include <flitr/modules/cpu_shader_passes/cpu_shader_pass.h>

namespace flitr {

class FLITR_EXPORT CPUUniformNoise_Shader : public flitr::CPUShaderPass::CPUShader
{
public:
    CPUUniformNoise_Shader(osg::Image* image, float noiseLevel) :
        Image_(image),
        NoiseLevel_(noiseLevel)
    {
    }

    ~CPUUniformNoise_Shader()
    {
    }

    virtual void operator () (osg::RenderInfo& renderInfo) const
    {
        const unsigned long width=Image_->s();
        const unsigned long height=Image_->t();
        const unsigned long numPixels=width*height;
        const unsigned long numComponents=osg::Image::computeNumComponents(Image_->getPixelFormat());

        unsigned char * const data=(unsigned char *)Image_->data();


        const float randAtt=NoiseLevel_ * 255.0f / RAND_MAX;
        const uint32_t numElements=numPixels*numComponents;

        for (uint32_t i=0; i<numElements; i++)
        {
            int pvalue=data[i]+randAtt*rand() + 0.5f;

            data[i]=(pvalue<255) ? pvalue : 255;
        }

            Image_->dirty();
    }

    osg::Image* Image_;
    float NoiseLevel_;
};
}
#endif //CPU_UNIFORM_NOISE_SHADER
