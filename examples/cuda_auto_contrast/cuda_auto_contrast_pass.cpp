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

#include "cuda_auto_contrast_pass.h"

#include <flitr/log_message.h>

#include <osgCuda/Computation>

#include <boost/tr1/memory.hpp>

using namespace flitr;

extern "C"
void cu_histeq(const dim3& blocks, const dim3& threads, 
               void* trgBuffer, int trgPitch, 
               void* srcBuffer, int srcPitch,
               unsigned int imageWidth, unsigned int imageHeight);

extern "C"
void cu_contrast_stretch(const dim3& blocks, const dim3& threads, 
                         void* trgBuffer, int trgPitch, 
                         void* srcBuffer, int srcPitch,
                         unsigned int imageWidth, unsigned int imageHeight);

CUDAAutoContrastPass::CUDAAutoContrastPass(
    osgCuda::TextureRectangle* pInputTexture,
    ImageFormat& imageFormat,
    AutoContrastMethod method) :
    m_rpInputTexture(pInputTexture),
    m_rpOutputTexture(0),
    m_ImageFormat(imageFormat),
    m_Method(method)
{
    m_rpInputTexture->setUsage(osgCompute::GL_TARGET_COMPUTE_SOURCE);

    m_uTextureWidth = m_ImageFormat.getWidth();
    m_uTextureHeight = m_ImageFormat.getHeight();
    m_rpRootGroup = new osg::Group;

    setupComputation();
}


void CUDAAutoContrastPass::setupComputation()
{
    osg::ref_ptr<osgCompute::Computation> spCudaNode = new osgCuda::Computation;
  
    m_rpInputTexture->addIdentifier( "AUTO_CONTRAST_INPUT_BUFFER" );

    m_rpOutputTexture = new osgCuda::Texture2D;  
    m_rpOutputTexture->setInternalFormat( GL_RGBA );
    m_rpOutputTexture->setSourceFormat( GL_RGBA );
    m_rpOutputTexture->setSourceType( GL_UNSIGNED_BYTE );
    m_rpOutputTexture->setTextureWidth( m_uTextureWidth );
    m_rpOutputTexture->setTextureHeight( m_uTextureHeight );
    m_rpOutputTexture->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::NEAREST);
    m_rpOutputTexture->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::NEAREST);
    m_rpOutputTexture->addIdentifier( "AUTO_CONTRAST_OUTPUT_BUFFER" );
  
    // Module Setup
    osg::ref_ptr<CModuleAutoContrast> rpAutoContrastModule = new CModuleAutoContrast();
    rpAutoContrastModule->addIdentifier( "osgcuda_auto_contrast" );
    rpAutoContrastModule->setLibraryName( "osgcuda_auto_contrast" );
    rpAutoContrastModule->setOwner(this);

    // Execute the computation during the rendering, but before
    // the subgraph is rendered. Default is the execution during
    // the update traversal.
    spCudaNode->setComputeOrder(  osgCompute::Computation::PRERENDER_BEFORECHILDREN );
    spCudaNode->addModule( *rpAutoContrastModule );
    spCudaNode->addResource( *m_rpInputTexture->getMemory() );
    spCudaNode->addResource( *m_rpOutputTexture->getMemory() );
  
    m_rpRootGroup->addChild( spCudaNode );
}

void CModuleAutoContrast::setThreadCount(uint32_t uThreadX, uint32_t uThreadY)
{
    m_vThreads = dim3( uThreadX, uThreadY, 1 );

    unsigned int numReqBlocksWidth;
    if( m_rpTargetMem->getDimension(0) % m_vThreads.x == 0) 
        numReqBlocksWidth = m_rpTargetMem->getDimension(0) / m_vThreads.x;
    else
        numReqBlocksWidth = m_rpTargetMem->getDimension(0) / m_vThreads.x + 1;

    unsigned int numReqBlocksHeight;
    if( m_rpTargetMem->getDimension(1) % m_vThreads.y == 0) 
        numReqBlocksHeight = m_rpTargetMem->getDimension(1) / m_vThreads.y;
    else
        numReqBlocksHeight = m_rpTargetMem->getDimension(1) / m_vThreads.y + 1;

    m_vBlocks = dim3( numReqBlocksWidth, numReqBlocksHeight, 1 );
}

bool CModuleAutoContrast::init() 
{ 
    if( !m_rpTargetMem || !m_rpSrcMem )
    {
        logMessage(LOG_CRITICAL) << "CModuleAutoContrast::init(): buffers are missing."
                                 << std::endl;
        return false;
    }

    // Optimal values depends on the number of CUDA cores of the GPU
    setThreadCount(16, 16); 

    return osgCompute::Module::init();
}

void CModuleAutoContrast::launch()
{
    if( isClear() )
        return;

    void* trg = m_rpTargetMem->map();
    int trgPitch = m_rpTargetMem->getPitch();
    void* src = m_rpSrcMem->map(osgCompute::MAP_DEVICE_SOURCE);
    int srcPitch = m_rpSrcMem->getPitch();

    // Execute kernels
    if (m_pOwner->m_Method == CUDAAutoContrastPass::HISTOGRAM_EQUALISATION) {
        cu_histeq(
            m_vBlocks, m_vThreads,
            trg, trgPitch,
            src, srcPitch,
            m_rpTargetMem->getDimension(0),
            m_rpTargetMem->getDimension(1)
            );
    } else {
        cu_contrast_stretch(
            m_vBlocks, m_vThreads,
            trg, trgPitch,
            src, srcPitch,
            m_rpTargetMem->getDimension(0),
            m_rpTargetMem->getDimension(1)
            );
    }
}

void CModuleAutoContrast::acceptResource( osgCompute::Resource& resource )
{
    // Search for your handles. This Method is called for each resource
    // located in the subgraph of this module.
    if( resource.isIdentifiedBy( "AUTO_CONTRAST_OUTPUT_BUFFER" ) )
        m_rpTargetMem = dynamic_cast<osgCompute::Memory*>( &resource );
    if( resource.isIdentifiedBy( "AUTO_CONTRAST_INPUT_BUFFER" ) )
        m_rpSrcMem = dynamic_cast<osgCompute::Memory*>( &resource );
}

void CModuleAutoContrast::clearLocal() 
{ 
    m_vThreads = dim3(0,0,0);
    m_vBlocks = dim3(0,0,0);
    m_rpTargetMem = NULL;
    m_rpSrcMem = NULL;
}
