#ifndef CPU_PALETTE_REMAP_SHADER
#define CPU_PALETTE_REMAP_SHADER 1

#include <flitr/flitr_export.h>
#include <flitr/modules/cpu_shader_passes/cpu_shader_pass.h>


namespace flitr {

class FLITR_EXPORT CPUPaletteRemap_Shader : public flitr::CPUShaderPass::CPUShader
{
public:
    CPUPaletteRemap_Shader(osg::Image* image) :
        Image_(image), enabled_(true)
    {
        paletteMap_=new uint32_t[256];
        for (uint32_t i=0; i<256; i++)
        {
            paletteMap_[i]=i;
        }

        parameterTitle_=std::string("Palette Remap");
    }
    ~CPUPaletteRemap_Shader()
    {
        delete [] paletteMap_;
    }

    virtual void setParameterTitle(const std::string &rValue)
    {
        parameterTitle_=rValue;
    }

    virtual void operator () (osg::RenderInfo& renderInfo) const
    {
        if (enabled_)
        {
            const unsigned long width=Image_->s();
            const unsigned long height=Image_->t();
            const unsigned long numPixels=width*height;
            const unsigned long numComponents=osg::Image::computeNumComponents(Image_->getPixelFormat());

            unsigned char * const data=(unsigned char *)Image_->data();

            // Apply palette transformation.
            const uint32_t numElements=numPixels*numComponents;
            for (uint32_t i=0; i<numElements; i++)
            {
                data[i]=paletteMap_[data[i]];
            }

            Image_->dirty();
        }
    }

    osg::Image* Image_;

    void setPaletteMap(const std::vector<uint8_t> &map)
    {
        uint32_t size=std::min<int>(map.size(),256);

        for (uint32_t i=0; i<size; i++)
        {
            paletteMap_[i]=map[i];
        }
    }

    virtual int getNumberOfParms()
    {
        return 0;
    }

    virtual std::string getTitle()
    {
        return parameterTitle_;
    }

    virtual void enable(bool state=true)
    {
        enabled_=state;
    }

    virtual bool isEnabled()
    {
        return enabled_;
    }

private:
    uint32_t *paletteMap_;

    std::string parameterTitle_;

    bool enabled_;
};
}
#endif //CPU_PALETTE_REMAP_SHADER
