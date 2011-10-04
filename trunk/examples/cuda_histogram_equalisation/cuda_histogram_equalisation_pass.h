#ifndef CUDA_HISTOGRAM_EQUALISATION_PASS_H
#define CUDA_HISTOGRAM_EQUALISATION_PASS_H 1

#include <flitr/image_format.h>

#include <osg/ref_ptr>
#include <osg/Group>

#include <vector_types.h> // cuda

#include <osgCompute/Module>
#include <osgCompute/Memory>
#include <osgCuda/Texture>

namespace flitr {

class CModuleHistEq;

class CUDAHistogramEqualisationPass 
{
  public:
    CUDAHistogramEqualisationPass(osgCuda::TextureRectangle* pInputTexture,
                                  ImageFormat& imageFormat);

    osg::ref_ptr<osg::Group> getRoot() { return m_rpRootGroup; }
    osg::ref_ptr<osg::Texture2D> getOutputTexture() { return m_rpOutputTexture; }

  private:
    friend class CModuleHistEq;
    void setupComputation();
        
    osg::ref_ptr<osgCuda::TextureRectangle> m_rpInputTexture;
    osg::ref_ptr<osgCuda::Texture2D> m_rpOutputTexture;
    osg::ref_ptr<osg::Group> m_rpRootGroup;

    ImageFormat m_ImageFormat;
    uint32_t m_uTextureWidth;
    uint32_t m_uTextureHeight;

    ImageFormat m_OutputFormat;
};

class CModuleHistEq : public osgCompute::Module 
{
  public:
    CModuleHistEq() : osgCompute::Module(), m_pOwner(0) { clearLocal(); }

    META_Object( flitr, CModuleHistEq )

    // Modules methods
    virtual bool init();
    virtual void launch();
    virtual void acceptResource( osgCompute::Resource& resource );

    virtual void clear() { clearLocal(); osgCompute::Module::clear(); }
      
    void setOwner(CUDAHistogramEqualisationPass *pOwner) { m_pOwner = pOwner; }
    void setThreadCount(uint32_t uThreadX, uint32_t uThreadY);
      
  protected:
    virtual ~CModuleHistEq() { clearLocal(); }
    void clearLocal();

    dim3 m_vBlocks;
    dim3 m_vThreads;
    osg::ref_ptr<osgCompute::Memory> m_rpSrcMem;
    osg::ref_ptr<osgCompute::Memory> m_rpTargetMem;
      
  private:
    CUDAHistogramEqualisationPass *m_pOwner;
    CModuleHistEq(const CModuleHistEq&, const osg::CopyOp& ) {} 
    inline CModuleHistEq &operator=(const CModuleHistEq &) { return *this; }
};

}

#endif //CUDA_HISTOGRAM_EQUALISATION_PASS_H
