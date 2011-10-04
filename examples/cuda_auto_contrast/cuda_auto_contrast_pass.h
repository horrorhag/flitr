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

#ifndef CUDA_AUTO_CONTRAST_PASS_H
#define CUDA_AUTO_CONTRAST_PASS_H 1

#include <flitr/image_format.h>

#include <osg/ref_ptr>
#include <osg/Group>

#include <vector_types.h> // cuda

#include <osgCompute/Module>
#include <osgCompute/Memory>
#include <osgCuda/Texture>

namespace flitr {

class CModuleAutoContrast;

class CUDAAutoContrastPass 
{
  public:
    enum AutoContrastMethod {
        CONTRAST_STRETCH = 0,
        HISTOGRAM_EQUALISATION
    };
    CUDAAutoContrastPass(osgCuda::TextureRectangle* pInputTexture,
                         ImageFormat& imageFormat,
                         AutoContrastMethod method);

    osg::ref_ptr<osg::Group> getRoot() { return m_rpRootGroup; }
    osg::ref_ptr<osg::Texture2D> getOutputTexture() { return m_rpOutputTexture; }

  private:
    friend class CModuleAutoContrast;
    void setupComputation();
        
    osg::ref_ptr<osgCuda::TextureRectangle> m_rpInputTexture;
    osg::ref_ptr<osgCuda::Texture2D> m_rpOutputTexture;
    osg::ref_ptr<osg::Group> m_rpRootGroup;

    ImageFormat m_ImageFormat;
    uint32_t m_uTextureWidth;
    uint32_t m_uTextureHeight;

    ImageFormat m_OutputFormat;

    AutoContrastMethod m_Method;
};

class CModuleAutoContrast : public osgCompute::Module 
{
  public:
    CModuleAutoContrast() : osgCompute::Module(), m_pOwner(0) { clearLocal(); }

    META_Object( flitr, CModuleAutoContrast )

    // Modules methods
    virtual bool init();
    virtual void launch();
    virtual void acceptResource( osgCompute::Resource& resource );

    virtual void clear() { clearLocal(); osgCompute::Module::clear(); }
      
    void setOwner(CUDAAutoContrastPass *pOwner) { m_pOwner = pOwner; }
    void setThreadCount(uint32_t uThreadX, uint32_t uThreadY);
      
  protected:
    virtual ~CModuleAutoContrast() { clearLocal(); }
    void clearLocal();

    dim3 m_vBlocks;
    dim3 m_vThreads;
    osg::ref_ptr<osgCompute::Memory> m_rpSrcMem;
    osg::ref_ptr<osgCompute::Memory> m_rpTargetMem;
      
  private:
    CUDAAutoContrastPass *m_pOwner;
    CModuleAutoContrast(const CModuleAutoContrast&, const osg::CopyOp& ) {} 
    inline CModuleAutoContrast &operator=(const CModuleAutoContrast &) { return *this; }
};

}

#endif //CUDA_AUTO_CONTRAST_PASS_H
