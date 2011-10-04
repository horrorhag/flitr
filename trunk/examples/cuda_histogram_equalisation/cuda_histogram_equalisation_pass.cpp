#include "cuda_histogram_equalisation_pass.h"

#include <flitr/log_message.h>

#include <osgCuda/Computation>

#include <boost/tr1/memory.hpp>

using namespace flitr;

extern "C"
void cu_histeq(const dim3& blocks, const dim3& threads, 
               void* trgBuffer, void* srcArray,
               unsigned int imageWidth, unsigned int imageHeight);

CUDAHistogramEqualisationPass::CUDAHistogramEqualisationPass(
    osgCuda::TextureRectangle* pInputTexture,
    ImageFormat& imageFormat) :
    m_rpInputTexture(pInputTexture),
    m_rpOutputTexture(0),
    m_ImageFormat(imageFormat)
{
    m_rpInputTexture->setUsage(osgCompute::GL_TARGET_COMPUTE_SOURCE);

    m_uTextureWidth = m_ImageFormat.getWidth();
    m_uTextureHeight = m_ImageFormat.getHeight();
    m_rpRootGroup = new osg::Group;

    setupComputation();
}


void CUDAHistogramEqualisationPass::setupComputation()
{
    osg::ref_ptr<osgCompute::Computation> spCudaNode = new osgCuda::Computation;
  
    m_rpInputTexture->addIdentifier( "HISTEQ_INPUT_BUFFER" );

    m_rpOutputTexture = new osgCuda::Texture2D;  
    m_rpOutputTexture->setInternalFormat( GL_RGBA );
    m_rpOutputTexture->setSourceFormat( GL_RGBA );
    m_rpOutputTexture->setSourceType( GL_UNSIGNED_BYTE );
    m_rpOutputTexture->setTextureWidth( m_uTextureWidth );
    m_rpOutputTexture->setTextureHeight( m_uTextureHeight );
    m_rpOutputTexture->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::NEAREST);
    m_rpOutputTexture->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::NEAREST);
    m_rpOutputTexture->addIdentifier( "HISTEQ_OUTPUT_BUFFER" );
  
    // Module Setup
    osg::ref_ptr<CModuleHistEq> rpHistEqModule = new CModuleHistEq();
    rpHistEqModule->addIdentifier( "osgcuda_histeq" );
    rpHistEqModule->setLibraryName( "osgcuda_histeq" );
    rpHistEqModule->setOwner(this);

    // Execute the computation during the rendering, but before
    // the subgraph is rendered. Default is the execution during
    // the update traversal.
    spCudaNode->setComputeOrder(  osgCompute::Computation::PRERENDER_BEFORECHILDREN );
    spCudaNode->addModule( *rpHistEqModule );
    spCudaNode->addResource( *m_rpInputTexture->getMemory() );
    spCudaNode->addResource( *m_rpOutputTexture->getMemory() );
  
    m_rpRootGroup->addChild( spCudaNode );
}

void CModuleHistEq::setThreadCount(uint32_t uThreadX, uint32_t uThreadY)
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

bool CModuleHistEq::init() 
{ 
    if( !m_rpTargetMem || !m_rpSrcMem )
    {
        logMessage(LOG_CRITICAL) << "CModuleHistEq::init(): buffers are missing."
                                 << std::endl;
        return false;
    }

    // Optimal values depends on the number of CUDA cores of the GPU
    setThreadCount(16, 16); 

    return osgCompute::Module::init();
}

void CModuleHistEq::launch()
{
    if( isClear() )
        return;

    // Execute kernels
    cu_histeq(
	    m_vBlocks, m_vThreads,
        m_rpTargetMem->map(),
        m_rpSrcMem->map(osgCompute::MAP_DEVICE_SOURCE),
        m_rpTargetMem->getDimension(0),
        m_rpTargetMem->getDimension(1)
	    );
}

void CModuleHistEq::acceptResource( osgCompute::Resource& resource )
{
    // Search for your handles. This Method is called for each resource
    // located in the subgraph of this module.
    if( resource.isIdentifiedBy( "HISTEQ_OUTPUT_BUFFER" ) )
        m_rpTargetMem = dynamic_cast<osgCompute::Memory*>( &resource );
    if( resource.isIdentifiedBy( "HISTEQ_INPUT_BUFFER" ) )
        m_rpSrcMem = dynamic_cast<osgCompute::Memory*>( &resource );
}

void CModuleHistEq::clearLocal() 
{ 
    m_vThreads = dim3(0,0,0);
    m_vBlocks = dim3(0,0,0);
    m_rpTargetMem = NULL;
    m_rpSrcMem = NULL;
}
