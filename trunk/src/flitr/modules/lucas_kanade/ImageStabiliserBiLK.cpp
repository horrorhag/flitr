#include <flitr/modules/lucas_kanade/ImageStabiliserBiLK.h>

#include <iostream>
#include <string.h>

unsigned long flitr::ImageStabiliserBiLK::m_ulLKBorder=32;//To be refined in the constructor.

double flitr::ImageStabiliserBiLK::logbase(double a, double base)
{
    return log(a) / log(base);
}

void flitr::ImageStabiliserBiLK::PostBiPyramidRebuiltCallback::callback()
{
    if ((!m_pImageStabiliserBiLK->m_bDoGPULKIterations)&&(m_pImageStabiliserBiLK->m_ulFrameNumber>=2))
    {//CPU BiLK...
        m_pImageStabiliserBiLK->updateH_BiLucasKanadeCPU();

        osg::Matrixd deltaTransform=m_pImageStabiliserBiLK->getDeltaTransformationMatrix();

        for (int i=0; i<(int)(m_pImageStabiliserBiLK->filterPairs_.size()); i++)
        {

            m_pImageStabiliserBiLK->filterPairs_[i].first=m_pImageStabiliserBiLK->filterPairs_[i].first*(1.0f-m_pImageStabiliserBiLK->filterPairs_[i].second)+osg::Vec2d(deltaTransform(3,0), deltaTransform(3,2))*(m_pImageStabiliserBiLK->filterPairs_[i].second);

            if (i==0)
            {//Update output quad's transform with the delta transform.
                osg::Matrixd quadDeltaTransform=deltaTransform;
                quadDeltaTransform(3,0)=quadDeltaTransform(3,0)-m_pImageStabiliserBiLK->filterPairs_[i].first._v[0];
                quadDeltaTransform(3,2)=quadDeltaTransform(3,2)-m_pImageStabiliserBiLK->filterPairs_[i].first._v[1];

                osg::Matrixd quadTransform=m_pImageStabiliserBiLK->m_rpQuadMatrixTransform->getMatrix();
                quadTransform=quadDeltaTransform*quadTransform;
                m_pImageStabiliserBiLK->m_rpQuadMatrixTransform->setMatrix(quadTransform);
                m_pImageStabiliserBiLK->offsetQuadPositionByMatrix(&quadTransform,
                                                                   m_pImageStabiliserBiLK->m_pInputTexture->getTextureWidth(),
                                                                   m_pImageStabiliserBiLK->m_pInputTexture->getTextureHeight());
            }
        }

    }
}



flitr::ImageStabiliserBiLK::ImageStabiliserBiLK(const osg::TextureRectangle *i_pInputTexture,
                                                unsigned long i_ulROIX_left, unsigned long i_ulROIY_left,
                                                unsigned long i_ulROIX_right, unsigned long i_ulROIY_right,
                                                unsigned long i_ulROIWidth, unsigned long i_ulROIHeight,
                                                bool i_bIndicateROI,
                                                bool i_bDoGPUPyramids,
                                                bool i_bDoGPULKIterations,
                                                unsigned long i_ulNumGPUHVectorReductionLevels,
                                                bool i_bReadOutputBackToCPU,
                                                bool i_bBiLinearOutputFilter,
                                                int i_iOutputScaleFactor, float i_fOutputCropFactor,
                                                float filterHistory,
                                                int numFilterPairs) :
    m_ulROIX_left(i_ulROIX_left), m_ulROIY_left(i_ulROIY_left),
    m_ulROIX_right(i_ulROIX_right), m_ulROIY_right(i_ulROIY_right),
    m_ulROIWidth(i_ulROIWidth), m_ulROIHeight(i_ulROIHeight),
    m_ulNumLKIterations(0),
    m_pCurrentBiPyramid(0),
    m_pPreviousBiPyramid(0),
    m_bAutoSwapCurrentPrevious(true),
    m_rpQuadMatrixTransform(new osg::MatrixTransform(osg::Matrixd::identity())),
    m_quadGeom(0),
    m_outputTexture(0),
    m_outputOSGImage(0),
    m_bReadOutputBackToCPU(i_bReadOutputBackToCPU),
    m_iOutputScaleFactor(i_iOutputScaleFactor),
    m_fOutputCropFactor(i_fOutputCropFactor),
    m_postBiPyramidRebuiltCallback(this),
    m_postBiLKIterationDrawCallback(this),
    m_lkResultImagePyramid(0),
    m_lkResultTexturePyramid(0),
    m_lkReducedResultImagePyramid(0),
    m_lkReducedResultTexturePyramid(0),
    m_imagePyramidFormat_(0),
    m_rpCurrentLKIterationRebuildSwitch(0),
    m_rpPreviousLKIterationRebuildSwitch(0),
    m_rpHEstimateUniform_left(0),
    m_rpHEstimateUniform_right(0),
    m_pInputTexture(i_pInputTexture),
    m_bIndicateROI(i_bIndicateROI),
    m_bDoGPUPyramids(i_bDoGPUPyramids),
    m_bDoGPULKIterations(i_bDoGPULKIterations),
    m_ulNumGPUHVectorReductionLevels(i_ulNumGPUHVectorReductionLevels),
    m_ulMaxGPUHVectorReductionLevels(0),
    m_bBiLinearOutputFilter(i_bBiLinearOutputFilter),
    m_h_left(osg::Vec2(0.0, 0.0)), m_h_right(osg::Vec2(0.0, 0.0)),
    m_ulFrameNumber(0)
{
    for (int i=0; i<numFilterPairs; i++)
    {
        filterPairs_.push_back(std::pair<osg::Vec2f, float>(osg::Vec2f(0.0,0.0), filterHistory));
    }

    m_ulLKBorder=16;//logbase(m_ulROIWidth,2)-1;

    if (!i_pInputTexture->getImage())
    {
        if (m_bDoGPUPyramids==false) logMessage(LOG_INFO) << "WARNING: Forcing the GPU pyramid construction to ON. Reason: The input texture is not available as an image on the CPU.\n";
        m_bDoGPUPyramids=true;
    }
}


bool flitr::ImageStabiliserBiLK::init(osg::Group *root_group)
{
    osg::Group *stabRoot=new osg::Group();

    bool returnVal=true;

    const unsigned long numLevelsX=logbase(m_ulROIWidth, 2.0);
    const unsigned long numLevelsY=logbase(m_ulROIHeight, 2.0);

    const unsigned long width=pow(2.0, (double)numLevelsX)+0.5;
    const unsigned long height=pow(2.0, (double)numLevelsY)+0.5;

    m_ulMaxGPUHVectorReductionLevels=(unsigned long)std::min<unsigned long>(numLevelsX, numLevelsY);

    m_ulNumGPUHVectorReductionLevels=std::min<unsigned long>(logbase(std::min<unsigned long>(width, height), 2.0)-1, m_ulNumGPUHVectorReductionLevels);

    m_pCurrentBiPyramid=new ImageBiPyramid(m_pInputTexture,
                                           m_ulROIX_left, m_ulROIY_left,
                                           m_ulROIX_right, m_ulROIY_right,
                                           m_ulROIWidth, m_ulROIHeight,
                                           m_bIndicateROI, m_bDoGPUPyramids, !m_bDoGPULKIterations);
    returnVal&=m_pCurrentBiPyramid->init(stabRoot, &m_postBiPyramidRebuiltCallback);

    m_pPreviousBiPyramid=new ImageBiPyramid(m_pInputTexture,
                                            m_ulROIX_left, m_ulROIY_left,
                                            m_ulROIX_right, m_ulROIY_right,
                                            m_ulROIWidth, m_ulROIHeight,
                                            m_bIndicateROI, m_bDoGPUPyramids, !m_bDoGPULKIterations);
    returnVal&=m_pPreviousBiPyramid->init(stabRoot, &m_postBiPyramidRebuiltCallback);


    //=== Allocate lk result textures and images : Begin ===//
    {
        unsigned long xPow2=pow(2.0, (double)numLevelsX)+0.5;
        unsigned long yPow2=pow(2.0, (double)numLevelsY)+0.5;

        m_lkResultImagePyramid=new osg::ref_ptr<osg::Image>*[2];
        m_lkResultImagePyramid[0]=new osg::ref_ptr<osg::Image>[m_pCurrentBiPyramid->getNumLevels()];
        m_lkResultImagePyramid[1]=new osg::ref_ptr<osg::Image>[m_pCurrentBiPyramid->getNumLevels()];
        m_lkResultTexturePyramid=new osg::ref_ptr<osg::TextureRectangle>*[2];
        m_lkResultTexturePyramid[0]=new osg::ref_ptr<osg::TextureRectangle>[m_pCurrentBiPyramid->getNumLevels()];
        m_lkResultTexturePyramid[1]=new osg::ref_ptr<osg::TextureRectangle>[m_pCurrentBiPyramid->getNumLevels()];

        m_lkReducedResultImagePyramid=new osg::ref_ptr<osg::Image>*[2];
        m_lkReducedResultImagePyramid[0]=new osg::ref_ptr<osg::Image>[m_ulMaxGPUHVectorReductionLevels];
        m_lkReducedResultImagePyramid[1]=new osg::ref_ptr<osg::Image>[m_ulMaxGPUHVectorReductionLevels];
        m_lkReducedResultTexturePyramid=new osg::ref_ptr<osg::TextureRectangle>*[2];
        m_lkReducedResultTexturePyramid[0]=new osg::ref_ptr<osg::TextureRectangle>[m_ulMaxGPUHVectorReductionLevels];
        m_lkReducedResultTexturePyramid[1]=new osg::ref_ptr<osg::TextureRectangle>[m_ulMaxGPUHVectorReductionLevels];

        //=== Initialise LK result images and textures. For GPU LK Iterations ONLY! ===//
        ImageFormat imageFormat=flitr::ImageFormat(m_pInputTexture->getTextureWidth(), m_pInputTexture->getTextureWidth());
        m_imagePyramidFormat_=new ImageFormat[m_ulMaxGPUHVectorReductionLevels];
        unsigned long level=0;
        for (level=0; level<m_pCurrentBiPyramid->getNumLevels(); level++)
        {
            imageFormat.setWidth(xPow2);
            imageFormat.setHeight(yPow2);
            //====================
            m_lkResultImagePyramid[0][level]=new osg::Image;
            m_lkResultImagePyramid[0][level]->allocateImage(xPow2, yPow2, 1, GL_RGBA, GL_FLOAT);//GL_RGB not faster than GL_RGBA!!!
            m_lkResultImagePyramid[0][level]->setInternalTextureFormat(GL_RGBA32F_ARB);
            m_lkResultImagePyramid[0][level]->setPixelFormat(GL_RGBA);
            m_lkResultImagePyramid[0][level]->setDataType(GL_FLOAT);

            m_lkResultImagePyramid[1][level]=new osg::Image;
            m_lkResultImagePyramid[1][level]->allocateImage(xPow2, yPow2, 1, GL_RGBA, GL_FLOAT);//GL_RGB not faster than GL_RGBA!!!
            m_lkResultImagePyramid[1][level]->setInternalTextureFormat(GL_RGBA32F_ARB);
            m_lkResultImagePyramid[1][level]->setPixelFormat(GL_RGBA);
            m_lkResultImagePyramid[1][level]->setDataType(GL_FLOAT);


            m_lkResultTexturePyramid[0][level]=new osg::TextureRectangle();
            m_lkResultTexturePyramid[0][level]->setTextureSize(xPow2, yPow2);
            m_lkResultTexturePyramid[0][level]->setFilter(osg::TextureRectangle::MIN_FILTER,osg::TextureRectangle::NEAREST);
            m_lkResultTexturePyramid[0][level]->setFilter(osg::TextureRectangle::MAG_FILTER,osg::TextureRectangle::NEAREST);
            m_lkResultTexturePyramid[0][level]->setInternalFormat(GL_RGBA32F_ARB);
            m_lkResultTexturePyramid[0][level]->setSourceFormat(GL_RGBA);
            m_lkResultTexturePyramid[0][level]->setSourceType(GL_FLOAT);
            m_lkResultTexturePyramid[0][level]->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
            m_lkResultTexturePyramid[0][level]->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
            //			m_lkResultTexturePyramid[0][level]->setImage(m_lkResultImagePyramid[0][level].get());//No need to attach the image to the texture if the results are not modified by the CPU and then updated to the GPU!!!

            m_lkResultTexturePyramid[1][level]=new osg::TextureRectangle();
            m_lkResultTexturePyramid[1][level]->setTextureSize(xPow2, yPow2);
            m_lkResultTexturePyramid[1][level]->setFilter(osg::TextureRectangle::MIN_FILTER,osg::TextureRectangle::NEAREST);
            m_lkResultTexturePyramid[1][level]->setFilter(osg::TextureRectangle::MAG_FILTER,osg::TextureRectangle::NEAREST);
            m_lkResultTexturePyramid[1][level]->setInternalFormat(GL_RGBA32F_ARB);

            m_lkResultTexturePyramid[1][level]->setSourceFormat(GL_RGBA);
            m_lkResultTexturePyramid[1][level]->setSourceType(GL_FLOAT);
            m_lkResultTexturePyramid[1][level]->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
            m_lkResultTexturePyramid[1][level]->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
            //			m_lkResultTexturePyramid[1][level]->setImage(m_lkResultImagePyramid[1][level].get());//No need to attach the image to the texture if the results are not modified by the CPU and then updated to the GPU!!!
            //====================
            //====================
            m_lkReducedResultImagePyramid[0][level]=new osg::Image;
            m_lkReducedResultImagePyramid[0][level]->allocateImage(xPow2, yPow2, 1, GL_RGBA, GL_FLOAT);//GL_RGB not faster than GL_RGBA!!!
            m_lkReducedResultImagePyramid[0][level]->setInternalTextureFormat(GL_RGBA32F_ARB);
            m_lkReducedResultImagePyramid[0][level]->setPixelFormat(GL_RGBA);
            m_lkReducedResultImagePyramid[0][level]->setDataType(GL_FLOAT);

            m_lkReducedResultImagePyramid[1][level]=new osg::Image;
            m_lkReducedResultImagePyramid[1][level]->allocateImage(xPow2, yPow2, 1, GL_RGBA, GL_FLOAT);//GL_RGB not faster than GL_RGBA!!!
            m_lkReducedResultImagePyramid[1][level]->setInternalTextureFormat(GL_RGBA32F_ARB);
            m_lkReducedResultImagePyramid[1][level]->setPixelFormat(GL_RGBA);
            m_lkReducedResultImagePyramid[1][level]->setDataType(GL_FLOAT);


            m_lkReducedResultTexturePyramid[0][level]=new osg::TextureRectangle();
            m_lkReducedResultTexturePyramid[0][level]->setTextureSize(xPow2, yPow2);
            m_lkReducedResultTexturePyramid[0][level]->setFilter(osg::TextureRectangle::MIN_FILTER,osg::TextureRectangle::NEAREST);
            m_lkReducedResultTexturePyramid[0][level]->setFilter(osg::TextureRectangle::MAG_FILTER,osg::TextureRectangle::NEAREST);
            m_lkReducedResultTexturePyramid[0][level]->setInternalFormat(GL_RGBA32F_ARB);
            m_lkReducedResultTexturePyramid[0][level]->setSourceFormat(GL_RGBA);
            m_lkReducedResultTexturePyramid[0][level]->setSourceType(GL_FLOAT);
            m_lkReducedResultTexturePyramid[0][level]->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
            m_lkReducedResultTexturePyramid[0][level]->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
            //			m_lkReducedResultTexturePyramid[0][level]->setImage(m_lkReducedResultImagePyramid[0][level].get());//No need to attach the image to the texture if the results are not modified by the CPU and then updated to the GPU!!!

            m_lkReducedResultTexturePyramid[1][level]=new osg::TextureRectangle();
            m_lkReducedResultTexturePyramid[1][level]->setTextureSize(xPow2, yPow2);
            m_lkReducedResultTexturePyramid[1][level]->setFilter(osg::TextureRectangle::MIN_FILTER,osg::TextureRectangle::NEAREST);
            m_lkReducedResultTexturePyramid[1][level]->setFilter(osg::TextureRectangle::MAG_FILTER,osg::TextureRectangle::NEAREST);
            m_lkReducedResultTexturePyramid[1][level]->setInternalFormat(GL_RGBA32F_ARB);
            m_lkReducedResultTexturePyramid[1][level]->setSourceFormat(GL_RGBA);
            m_lkReducedResultTexturePyramid[1][level]->setSourceType(GL_FLOAT);
            m_lkReducedResultTexturePyramid[1][level]->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
            m_lkReducedResultTexturePyramid[1][level]->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
            //			m_lkReducedResultTexturePyramid[1][level]->setImage(m_lkReducedResultImagePyramid[1][level].get());//No need to attach the image to the texture if the results are not modified by the CPU and then updated to the GPU!!!
            //====================

            m_imagePyramidFormat_[level]=imageFormat;

            xPow2/=2;
            yPow2/=2;
        }
        for (; level<m_ulMaxGPUHVectorReductionLevels; level++)
        {
            imageFormat.setWidth(xPow2);
            imageFormat.setHeight(yPow2);
            //====================
            m_lkReducedResultImagePyramid[0][level]=new osg::Image;
            m_lkReducedResultImagePyramid[0][level]->allocateImage(xPow2, yPow2, 1, GL_RGBA, GL_FLOAT);//GL_RGB not faster than GL_RGBA!!!
            m_lkReducedResultImagePyramid[0][level]->setInternalTextureFormat(GL_RGBA32F_ARB);
            m_lkReducedResultImagePyramid[0][level]->setPixelFormat(GL_RGBA);
            m_lkReducedResultImagePyramid[0][level]->setDataType(GL_FLOAT);

            m_lkReducedResultImagePyramid[1][level]=new osg::Image;
            m_lkReducedResultImagePyramid[1][level]->allocateImage(xPow2, yPow2, 1, GL_RGBA, GL_FLOAT);//GL_RGB not faster than GL_RGBA!!!
            m_lkReducedResultImagePyramid[1][level]->setInternalTextureFormat(GL_RGBA32F_ARB);
            m_lkReducedResultImagePyramid[1][level]->setPixelFormat(GL_RGBA);
            m_lkReducedResultImagePyramid[1][level]->setDataType(GL_FLOAT);


            m_lkReducedResultTexturePyramid[0][level]=new osg::TextureRectangle();
            m_lkReducedResultTexturePyramid[0][level]->setTextureSize(xPow2, yPow2);
            m_lkReducedResultTexturePyramid[0][level]->setFilter(osg::TextureRectangle::MIN_FILTER,osg::TextureRectangle::NEAREST);
            m_lkReducedResultTexturePyramid[0][level]->setFilter(osg::TextureRectangle::MAG_FILTER,osg::TextureRectangle::NEAREST);
            m_lkReducedResultTexturePyramid[0][level]->setInternalFormat(GL_RGBA32F_ARB);
            m_lkReducedResultTexturePyramid[0][level]->setSourceFormat(GL_RGBA);
            m_lkReducedResultTexturePyramid[0][level]->setSourceType(GL_FLOAT);
            m_lkReducedResultTexturePyramid[0][level]->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
            m_lkReducedResultTexturePyramid[0][level]->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
            //			m_lkReducedResultTexturePyramid[0][level]->setImage(m_lkReducedResultImagePyramid[0][level].get());//No need to attach the image to the texture if the results are not modified by the CPU and then updated to the GPU!!!

            m_lkReducedResultTexturePyramid[1][level]=new osg::TextureRectangle();
            m_lkReducedResultTexturePyramid[1][level]->setTextureSize(xPow2, yPow2);
            m_lkReducedResultTexturePyramid[1][level]->setFilter(osg::TextureRectangle::MIN_FILTER,osg::TextureRectangle::NEAREST);
            m_lkReducedResultTexturePyramid[1][level]->setFilter(osg::TextureRectangle::MAG_FILTER,osg::TextureRectangle::NEAREST);
            m_lkReducedResultTexturePyramid[1][level]->setInternalFormat(GL_RGBA32F_ARB);
            m_lkReducedResultTexturePyramid[1][level]->setSourceFormat(GL_RGBA);
            m_lkReducedResultTexturePyramid[1][level]->setSourceType(GL_FLOAT);
            m_lkReducedResultTexturePyramid[1][level]->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
            m_lkReducedResultTexturePyramid[1][level]->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
            //			m_lkReducedResultTexturePyramid[1][level]->setImage(m_lkReducedResultImagePyramid[1][level].get());//No need to attach the image to the texture if the results are not modified by the CPU and then updated to the GPU!!!
            //====================

            m_imagePyramidFormat_[level]=imageFormat;

            xPow2/=2;
            yPow2/=2;
        }
    }
    //=== Allocate lk result textures and images : End ===//

    m_rpHEstimateUniform_left=new osg::Uniform("hEstimate_left", osg::Vec2(0.0, 0.0));
    m_rpHEstimateUniform_right=new osg::Uniform("hEstimate_right", osg::Vec2(0.0, 0.0));

    if (m_bDoGPULKIterations)
    {//Not CPU BiLK. Add GPU Bi-LK algo to stabRoot...
        m_rpCurrentLKIterationRebuildSwitch=new osg::Switch();
        m_rpPreviousLKIterationRebuildSwitch=new osg::Switch();

        for (long level=(m_pCurrentBiPyramid->getNumLevels()-1); level>=0; level--)
        {
            for (long newtRaphIter=(NEWTON_RAPHSON_ITERATIONS-1); newtRaphIter>=0; newtRaphIter--)
            {
                //=== Calculate weighted per pixel h-vector ===//
                m_rpCurrentLKIterationRebuildSwitch->addChild(createBiLKIteration(m_pCurrentBiPyramid, m_pPreviousBiPyramid, level,
                                                                                  width>>level, height>>level,
                                                                                  m_rpHEstimateUniform_left.get(), m_rpHEstimateUniform_right.get(), !(m_ulNumGPUHVectorReductionLevels>0)));

                m_rpPreviousLKIterationRebuildSwitch->addChild(createBiLKIteration(m_pPreviousBiPyramid, m_pCurrentBiPyramid, level,
                                                                                   width>>level, height>>level,
                                                                                   m_rpHEstimateUniform_left.get(), m_rpHEstimateUniform_right.get(), !(m_ulNumGPUHVectorReductionLevels>0)));
                //=============================================//


                //=============================================//
                for (long reducLevel=1; reducLevel<=(long)m_ulNumGPUHVectorReductionLevels; reducLevel++)
                {
                    if ((level+reducLevel)>=(long)m_ulMaxGPUHVectorReductionLevels) break;//Lowest res GPU reduction of 4x4 to 2x2 already reached.

                    osg::Node *hVectorReduction=createHVectorReductionLevel((level+reducLevel),
                                                                            width>>(level+reducLevel), height>>(level+reducLevel),
                                                                            reducLevel, (reducLevel==(long)m_ulNumGPUHVectorReductionLevels));

                    m_rpCurrentLKIterationRebuildSwitch->addChild(hVectorReduction);
                    m_rpPreviousLKIterationRebuildSwitch->addChild(hVectorReduction);
                }
                //=============================================//
            }
        }

        m_rpCurrentLKIterationRebuildSwitch->setAllChildrenOff();
        m_rpPreviousLKIterationRebuildSwitch->setAllChildrenOff();
        stabRoot->addChild(m_rpCurrentLKIterationRebuildSwitch.get());
        stabRoot->addChild(m_rpPreviousLKIterationRebuildSwitch.get());
    }
    


    m_outputTexture=new osg::TextureRectangle();
    m_outputTexture->setTextureSize(((int)(m_pInputTexture->getTextureWidth()*m_iOutputScaleFactor/m_fOutputCropFactor+0.5f)), ((int)(m_pInputTexture->getTextureHeight()*m_iOutputScaleFactor/m_fOutputCropFactor+0.5f)));
    if (m_bBiLinearOutputFilter)
    {
        m_outputTexture->setFilter(osg::TextureRectangle::MIN_FILTER,osg::TextureRectangle::LINEAR);
        m_outputTexture->setFilter(osg::TextureRectangle::MAG_FILTER,osg::TextureRectangle::LINEAR);
    } else
    {
        m_outputTexture->setFilter(osg::TextureRectangle::MIN_FILTER,osg::TextureRectangle::NEAREST);
        m_outputTexture->setFilter(osg::TextureRectangle::MAG_FILTER,osg::TextureRectangle::NEAREST);
    }
    m_outputTexture->setSourceFormat(m_pInputTexture->getSourceFormat());
    m_outputTexture->setInternalFormat(m_pInputTexture->getInternalFormat());
    m_outputTexture->setSourceType(m_pInputTexture->getSourceType());
    m_outputTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    m_outputTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);


    if (m_bReadOutputBackToCPU)
    {
        m_outputOSGImage=new osg::Image;
        m_outputOSGImage->allocateImage((int)(m_pInputTexture->getTextureWidth()*m_iOutputScaleFactor/m_fOutputCropFactor+0.5f),
                                        (int)(m_pInputTexture->getTextureWidth()*m_iOutputScaleFactor/m_fOutputCropFactor+0.5f), 1,
                                   m_pInputTexture->getSourceFormat(), m_pInputTexture->getSourceType());
    }

    stabRoot->addChild(createOutputPass());

    root_group->addChild(stabRoot);

    return returnVal;
}


osg::ref_ptr<osg::Geode> flitr::ImageStabiliserBiLK::createScreenAlignedQuadLK(unsigned long i_ulWidth, unsigned long i_ulHeight, double i_dTextXOffset, double i_dTextYOffset, double i_dBorderPixels)
{
    osg::ref_ptr<osg::Geode>       geode;
    osg::ref_ptr<osg::Geometry>    geom;
    osg::ref_ptr<osg::Vec2Array>   tcoords;
    osg::ref_ptr<osg::Vec3Array>   vcoords;
    osg::ref_ptr<osg::Vec4Array>   colors;
    osg::ref_ptr<osg::DrawArrays>  da;
    
    //*************************************
    //create the group and its single geode
    //*************************************
    //    sceneGroup = new osg::Group();
    geode   = new osg::Geode();
    
    //********************************************
    //create quad to stick the stitched texture on
    //********************************************
    da = new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, 4);
    
    //******************************
    //set quad colour to white
    //******************************
    colors = new osg::Vec4Array;
    colors->push_back(osg::Vec4(1.0f,1.0f,1.0f,1.0f));

    //**************************************************************
    //set the texture coords to coincide with the size of the stitch
    //**************************************************************
    tcoords = new osg::Vec2Array;
    tcoords->push_back(osg::Vec2(0+i_dTextXOffset+i_dBorderPixels,         0+i_dTextYOffset+i_dBorderPixels));
    tcoords->push_back(osg::Vec2(i_ulWidth+i_dTextXOffset-i_dBorderPixels, 0+i_dTextYOffset+i_dBorderPixels));
    tcoords->push_back(osg::Vec2(i_ulWidth+i_dTextXOffset-i_dBorderPixels, i_ulHeight+i_dTextYOffset-i_dBorderPixels));
    tcoords->push_back(osg::Vec2(0+i_dTextXOffset+i_dBorderPixels,         i_ulHeight+i_dTextYOffset-i_dBorderPixels));
    
    //********************************************
    //set up the quad in free space recalling that
    //the camera is going to look along the Z axis
    //********************************************
    vcoords = new osg::Vec3Array;
    vcoords->push_back(osg::Vec3d(0+i_dBorderPixels,     0+i_dBorderPixels,      -1));
    vcoords->push_back(osg::Vec3d(i_ulWidth-i_dBorderPixels, 0+i_dBorderPixels,      -1));
    vcoords->push_back(osg::Vec3d(i_ulWidth-i_dBorderPixels, i_ulHeight-i_dBorderPixels, -1));
    vcoords->push_back(osg::Vec3d(0+i_dBorderPixels,     i_ulHeight-i_dBorderPixels, -1));
    
    //*************************************
    //tie all the above properties together
    //*************************************
    geom = new osg::Geometry;
    geom->setVertexArray(vcoords.get());
    geom->setTexCoordArray(0, tcoords.get());
    geom->addPrimitiveSet(da.get());
    geom->setColorArray(colors.get());
    geom->setColorBinding(osg::Geometry::BIND_OVERALL);

    geode->addDrawable(geom.get());

    osg::ref_ptr<osg::StateSet> geomss = geode->getOrCreateStateSet();
    geomss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    geomss->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    geomss->setMode(GL_BLEND,osg::StateAttribute::OFF);
    //	geomss->setAttributeAndModes(new osg::AlphaFunc, osg::StateAttribute::OFF);

    return geode;
}

osg::Camera *flitr::ImageStabiliserBiLK::createScreenAlignedCameraLK(unsigned long i_ulWidth, unsigned long i_ulHeight, double i_dBorderPixels)
{
    //g_ulNumCameras++;
    osg::Camera *theCamera = new osg::Camera;
    theCamera->setClearMask((i_dBorderPixels>0.0) ? GL_COLOR_BUFFER_BIT : 0);
    theCamera->setClearColor(osg::Vec4(0.0, 0.0, 0.0, 0.0));
    theCamera->setProjectionMatrixAsOrtho(0, i_ulWidth, 0, i_ulHeight,-100,100);
    theCamera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
    theCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    theCamera->setViewMatrix(osg::Matrix::identity());
    theCamera->setViewport(0, 0, i_ulWidth, i_ulHeight);
    theCamera->setRenderOrder(osg::Camera::PRE_RENDER);
    theCamera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);

    return theCamera;
}


osg::Node* flitr::ImageStabiliserBiLK::createBiLKIteration(ImageBiPyramid *i_pCurrentBiPyramid, ImageBiPyramid *i_pPreviousBiPyramid, unsigned long i_ulLevel, unsigned long i_ulWidth, unsigned long i_ulHeight, osg::Uniform *i_pHEstimateUniform_left, osg::Uniform *i_pHEstimateUniform_right, bool i_bDoPostLKIterationCallback)
{
    osg::ref_ptr<osg::Geode> geode = createScreenAlignedQuadLK(i_ulWidth, i_ulHeight, 0, 0);
    osg::ref_ptr<osg::StateSet> geomss = geode->getOrCreateStateSet();

    geomss->setTextureAttributeAndModes(0, i_pCurrentBiPyramid->m_textureGausPyramid[0][i_ulLevel].get(), osg::StateAttribute::ON);
    //	geomss->setTextureAttribute(0, new osg::TexEnv(osg::TexEnv::DECAL));
    geomss->setTextureAttributeAndModes(1, i_pCurrentBiPyramid->m_textureGausPyramid[1][i_ulLevel].get(), osg::StateAttribute::ON);
    //	geomss->setTextureAttribute(1, new osg::TexEnv(osg::TexEnv::DECAL));

    geomss->setTextureAttributeAndModes(2, i_pCurrentBiPyramid->m_derivTexturePyramid[0][i_ulLevel].get(), osg::StateAttribute::ON);

    //	geomss->setTextureAttribute(2, new osg::TexEnv(osg::TexEnv::DECAL));
    geomss->setTextureAttributeAndModes(3, i_pCurrentBiPyramid->m_derivTexturePyramid[1][i_ulLevel].get(), osg::StateAttribute::ON);
    //	geomss->setTextureAttribute(3, new osg::TexEnv(osg::TexEnv::DECAL));

    geomss->setTextureAttributeAndModes(4, i_pPreviousBiPyramid->m_textureGausPyramid[0][i_ulLevel].get(), osg::StateAttribute::ON);
    //	geomss->setTextureAttribute(4, new osg::TexEnv(osg::TexEnv::DECAL));
    geomss->setTextureAttributeAndModes(5, i_pPreviousBiPyramid->m_textureGausPyramid[1][i_ulLevel].get(), osg::StateAttribute::ON);
    //	geomss->setTextureAttribute(5, new osg::TexEnv(osg::TexEnv::DECAL));


    //****************
    //add the uniforms
    //****************
    geomss->addUniform(i_pHEstimateUniform_left);
    geomss->addUniform(i_pHEstimateUniform_right);
    geomss->addUniform(new osg::Uniform("gausFiltTexture_left", 0));
    geomss->addUniform(new osg::Uniform("gausFiltTexture_right", 1));
    geomss->addUniform(new osg::Uniform("derivTexture_left", 2));
    geomss->addUniform(new osg::Uniform("derivTexture_right", 3));
    geomss->addUniform(new osg::Uniform("previousgausFiltTexture_left", 4));
    geomss->addUniform(new osg::Uniform("previousgausFiltTexture_right", 5));

    //*******************************************
    //create and add the shaders to the state set
    //*******************************************
    osg::ref_ptr<osg::Program> textureShader = new osg::Program;
    textureShader->setName("createLKIteration");
    textureShader->addShader(new osg::Shader(osg::Shader::FRAGMENT,
                                             "uniform sampler2DRect gausFiltTexture_left;\n"
                                             "uniform sampler2DRect gausFiltTexture_right;\n"

                                             "uniform sampler2DRect derivTexture_left;\n"
                                             "uniform sampler2DRect derivTexture_right;\n"

                                             "uniform sampler2DRect previousgausFiltTexture_left;\n"
                                             "uniform sampler2DRect previousgausFiltTexture_right;\n"

                                             "uniform vec2 hEstimate_left;\n"
                                             "uniform vec2 hEstimate_right;\n"

                                             "void main(void)"
                                             "{\n"
                                             "  float imagePixel_left=texture2DRect(gausFiltTexture_left, gl_TexCoord[0].xy).r;\n"
                                             "  vec3 derivPixel_left=texture2DRect(derivTexture_left, gl_TexCoord[0].xy).rgb;\n"

                                             "  float imagePixel_right=texture2DRect(gausFiltTexture_right, gl_TexCoord[0].xy).r;\n"
                                             "  vec3 derivPixel_right=texture2DRect(derivTexture_right, gl_TexCoord[0].xy).rgb;\n"

                                             //======================
                                             ////Bi-linear filter for RGBAF32 pixel. Would not be required on newer hardware!
                                             "  vec2 previousImagePixelPosition_left=(gl_TexCoord[0].xy-vec2(0.5, 0.5))-hEstimate_left;\n"
                                             "  vec2 previousImagePixelPositionFloor_left=floor(previousImagePixelPosition_left);\n"
                                             "  vec2 previousImagePixelPositionFrac_left=previousImagePixelPosition_left-previousImagePixelPositionFloor_left;\n"

                                             "  float previousImagePixelTopLeft_left=texture2DRect(previousgausFiltTexture_left, previousImagePixelPositionFloor_left+vec2(0.5, 0.5)).r;\n"
                                             "  float previousImagePixelTopRight_left=texture2DRect(previousgausFiltTexture_left, previousImagePixelPositionFloor_left+vec2(1.5, 0.5)).r;\n"
                                             "  float previousImagePixelBotLeft_left=texture2DRect(previousgausFiltTexture_left, previousImagePixelPositionFloor_left+vec2(0.5, 1.5)).r;\n"
                                             "  float previousImagePixelBotRight_left=texture2DRect(previousgausFiltTexture_left, previousImagePixelPositionFloor_left+vec2(1.5, 1.5)).r;\n"
                                             "  float previousImagePixelTop_left=(previousImagePixelTopRight_left-previousImagePixelTopLeft_left)*previousImagePixelPositionFrac_left.x+previousImagePixelTopLeft_left;\n"
                                             "  float previousImagePixelBot_left=(previousImagePixelBotRight_left-previousImagePixelBotLeft_left)*previousImagePixelPositionFrac_left.x+previousImagePixelBotLeft_left;\n"
                                             "  float previousImagePixel_left=(previousImagePixelBot_left-previousImagePixelTop_left)*previousImagePixelPositionFrac_left.y+previousImagePixelTop_left;\n"
                                             //======================
                                             //======================
                                             ////Bi-linear filter for RGBAF32 pixel. Would not be required on newer hardware!
                                             "  vec2 previousImagePixelPosition_right=(gl_TexCoord[0].xy-vec2(0.5, 0.5))-hEstimate_right;\n"
                                             "  vec2 previousImagePixelPositionFloor_right=floor(previousImagePixelPosition_right);\n"
                                             "  vec2 previousImagePixelPositionFrac_right=previousImagePixelPosition_right-previousImagePixelPositionFloor_right;\n"

                                             "  float previousImagePixelTopLeft_right=texture2DRect(previousgausFiltTexture_right, previousImagePixelPositionFloor_right+vec2(0.5, 0.5)).r;\n"
                                             "  float previousImagePixelTopRight_right=texture2DRect(previousgausFiltTexture_right, previousImagePixelPositionFloor_right+vec2(1.5, 0.5)).r;\n"
                                             "  float previousImagePixelBotLeft_right=texture2DRect(previousgausFiltTexture_right, previousImagePixelPositionFloor_right+vec2(0.5, 1.5)).r;\n"
                                             "  float previousImagePixelBotRight_right=texture2DRect(previousgausFiltTexture_right, previousImagePixelPositionFloor_right+vec2(1.5, 1.5)).r;\n"
                                             "  float previousImagePixelTop_right=(previousImagePixelTopRight_right-previousImagePixelTopLeft_right)*previousImagePixelPositionFrac_right.x+previousImagePixelTopLeft_right;\n"
                                             "  float previousImagePixelBot_right=(previousImagePixelBotRight_right-previousImagePixelBotLeft_right)*previousImagePixelPositionFrac_right.x+previousImagePixelBotLeft_right;\n"
                                             "  float previousImagePixel_right=(previousImagePixelBot_right-previousImagePixelTop_right)*previousImagePixelPositionFrac_right.y+previousImagePixelTop_right;\n"
                                             //======================
                                             //OR
                                             //======================
                                             //"  vec2 previousImagePixelPosition_left=gl_TexCoord[0].xy-hEstimate_left;\n"
                                             //"  float previousImagePixel_left=texture2DRect(previousgausFiltTexture_left, previousImagePixelPosition_left).r;\n"
                                             //======================
                                             //======================
                                             //"  vec2 previousImagePixelPosition_right=gl_TexCoord[0].xy-hEstimate_right;\n"
                                             //"  float previousImagePixel_right=texture2DRect(previousgausFiltTexture_right, previousImagePixelPosition_right).r;\n"
                                             //======================

                                             "  float error_left=imagePixel_left-previousImagePixel_left;\n"
                                             "  vec2 hNumerator_left=derivPixel_left.rg * error_left;\n"
                                             "  float hDenominator_left=derivPixel_left.b;\n"

                                             "  float error_right=imagePixel_right-previousImagePixel_right;\n"
                                             "  vec2 hNumerator_right=derivPixel_right.rg * error_right;\n"
                                             "  float hDenominator_right=derivPixel_right.b;\n"

                                             "  gl_FragData[0].rg=hNumerator_left;\n"
                                             "  gl_FragData[0].b=hDenominator_left;\n"
                                             //	    "  gl_FragData[0].a=1.0;\n"

                                             "  gl_FragData[1].rg=hNumerator_right;\n"
                                             "  gl_FragData[1].b=hDenominator_right;\n"
                                             //	    "  gl_FragData[1].a=1.0;\n"

                                             "}\n"
                                             ));
    geomss->setAttributeAndModes(textureShader.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE );


    osg::Camera *theCamera=createScreenAlignedCameraLK(i_ulWidth, i_ulHeight);
    theCamera->addChild(geode.get());

    theCamera->attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER0+0), this->m_lkResultTexturePyramid[0][i_ulLevel].get());
    theCamera->attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER0+1), this->m_lkResultTexturePyramid[1][i_ulLevel].get());


    if (i_bDoPostLKIterationCallback)
    {
        theCamera->attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER0+0), this->m_lkResultImagePyramid[0][i_ulLevel].get());
        theCamera->attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER0+1), this->m_lkResultImagePyramid[1][i_ulLevel].get());

        theCamera->setPostDrawCallback(&m_postBiLKIterationDrawCallback);
    }

    return theCamera;
}   


osg::Node* flitr::ImageStabiliserBiLK::createHVectorReductionLevel(unsigned long i_ulLevel, unsigned long i_ulWidth,
                                                                   unsigned long i_ulHeight, unsigned long i_ulReductionLevel,
                                                                   bool i_bDoPostLKIterationCallback)
{
    osg::ref_ptr<osg::Geode> geode = 0;
    osg::ref_ptr<osg::StateSet> geomss = 0;

    if (i_ulReductionLevel==1)
    {//Start from LK result. Also implement LUCAS_KANADE_BORDER!
        geode = createScreenAlignedQuadLK(i_ulWidth, i_ulHeight, 0, 0, (m_ulLKBorder>>(i_ulLevel-1+1)));//'*0.5' because we're working with a boundary at the next HIGHER resolution.
        geomss = geode->getOrCreateStateSet();

        geomss->setTextureAttributeAndModes(0, this->m_lkResultTexturePyramid[0][i_ulLevel-1].get(), osg::StateAttribute::ON);
        //	geomss->setTextureAttribute(0, new osg::TexEnv(osg::TexEnv::DECAL));

        geomss->setTextureAttributeAndModes(1, this->m_lkResultTexturePyramid[1][i_ulLevel-1].get(), osg::StateAttribute::ON);
        //	geomss->setTextureAttribute(1, new osg::TexEnv(osg::TexEnv::DECAL));
    } else
    {//Use already reduced results.
        geode = createScreenAlignedQuadLK(i_ulWidth, i_ulHeight, 0, 0, 0);
        geomss = geode->getOrCreateStateSet();

        geomss->setTextureAttributeAndModes(0, this->m_lkReducedResultTexturePyramid[0][i_ulLevel-1].get(), osg::StateAttribute::ON);
        //	geomss->setTextureAttribute(0, new osg::TexEnv(osg::TexEnv::DECAL));

        geomss->setTextureAttributeAndModes(1, this->m_lkReducedResultTexturePyramid[1][i_ulLevel-1].get(), osg::StateAttribute::ON);
        //	geomss->setTextureAttribute(1, new osg::TexEnv(osg::TexEnv::DECAL));
    }

    //****************
    //add the uniforms
    //****************
    geomss->addUniform(new osg::Uniform("highResLKResultTexture_left", 0));
    geomss->addUniform(new osg::Uniform("highResLKResultTexture_right", 1));

    //*******************************************
    //create and add the shaders to the state set
    //*******************************************
    osg::ref_ptr<osg::Program> textureShader = new osg::Program;
    textureShader->setName("createHVectorReductionLevel");
    textureShader->addShader(new osg::Shader(osg::Shader::FRAGMENT,
                                             "uniform sampler2DRect highResLKResultTexture_left;\n"
                                             "uniform sampler2DRect highResLKResultTexture_right;\n"
                                             "void main(void)"
                                             "{\n"
                                             "  vec4 imagePixel0_left=texture2DRect(highResLKResultTexture_left, (gl_TexCoord[0].xy-vec2(0.5, 0.5))*2.0+vec2(0.5, 0.5));\n"
                                             "  vec4 imagePixel1_left=texture2DRect(highResLKResultTexture_left, (gl_TexCoord[0].xy-vec2(0.5, 0.5))*2.0+vec2(1.5, 0.5));\n"
                                             "  vec4 imagePixel2_left=texture2DRect(highResLKResultTexture_left, (gl_TexCoord[0].xy-vec2(0.5, 0.5))*2.0+vec2(1.5, 1.5));\n"
                                             "  vec4 imagePixel3_left=texture2DRect(highResLKResultTexture_left, (gl_TexCoord[0].xy-vec2(0.5, 0.5))*2.0+vec2(0.5, 1.5));\n"

                                             "  vec4 imagePixel0_right=texture2DRect(highResLKResultTexture_right, (gl_TexCoord[0].xy-vec2(0.5, 0.5))*2.0+vec2(0.5, 0.5));\n"
                                             "  vec4 imagePixel1_right=texture2DRect(highResLKResultTexture_right, (gl_TexCoord[0].xy-vec2(0.5, 0.5))*2.0+vec2(1.5, 0.5));\n"
                                             "  vec4 imagePixel2_right=texture2DRect(highResLKResultTexture_right, (gl_TexCoord[0].xy-vec2(0.5, 0.5))*2.0+vec2(1.5, 1.5));\n"
                                             "  vec4 imagePixel3_right=texture2DRect(highResLKResultTexture_right, (gl_TexCoord[0].xy-vec2(0.5, 0.5))*2.0+vec2(0.5, 1.5));\n"

                                             "  gl_FragData[0]=(imagePixel0_left+imagePixel1_left+imagePixel2_left+imagePixel3_left);\n"
                                             "  gl_FragData[1]=(imagePixel0_right+imagePixel1_right+imagePixel2_right+imagePixel3_right);\n"
                                             "}\n"
                                             ));
    geomss->setAttributeAndModes(textureShader.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE );


    osg::Camera *theCamera=0;
    if (i_ulReductionLevel==1)
    {
        theCamera=createScreenAlignedCameraLK(i_ulWidth, i_ulHeight, (m_ulLKBorder>>(i_ulLevel-1+1)));
    } else
    {
        theCamera=createScreenAlignedCameraLK(i_ulWidth, i_ulHeight);
    }
    theCamera->addChild(geode.get());

    theCamera->attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER0+0), this->m_lkReducedResultTexturePyramid[0][i_ulLevel].get());
    theCamera->attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER0+1), this->m_lkReducedResultTexturePyramid[1][i_ulLevel].get());

    if (i_bDoPostLKIterationCallback)
    {
        //LK iteration result currently readback to CPU for h-vector reduction operation!
        theCamera->attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER0+0), this->m_lkReducedResultImagePyramid[0][i_ulLevel].get());
        theCamera->attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER0+1), this->m_lkReducedResultImagePyramid[1][i_ulLevel].get());

        theCamera->setPostDrawCallback(&m_postBiLKIterationDrawCallback);
    }

    return theCamera;
}


osg::Matrixd flitr::ImageStabiliserBiLK::getDeltaTransformationMatrix() const
{			

    //	osg::Vec2d lNorm=osg::Vec2d(1.0, 0.0);
    //	osg::Vec2d mNorm=osg::Vec2d(0.0, 1.0);
    //	osg::Vec2d translation=m_h_right;

    osg::Vec2d translation=(m_h_right+m_h_left)*0.5;

    osg::Vec2d lNorm;
    osg::Vec2d mNorm;

    if (getROI_rightCentre()!=getROI_leftCentre())
    {
        lNorm=(m_h_right+getROI_rightCentre())-(m_h_left+getROI_leftCentre());
        lNorm.normalize();
        mNorm=osg::Vec2d(-lNorm.y(), lNorm.x());

        osg::Vec2d roiReference=(getROI_rightCentre()+getROI_leftCentre())*0.5-osg::Vec2d(m_pInputTexture->getTextureWidth()*0.5, m_pInputTexture->getTextureHeight()*0.5);
        osg::Vec2d transformedROIReference=osg::Vec2d(roiReference.x()*lNorm.x()+roiReference.y()*mNorm.x(),
                                                      roiReference.x()*lNorm.y()+roiReference.y()*mNorm.y());

        //Compensate for movement of rotation reference.
        translation-=(transformedROIReference-roiReference);
    } else
    {
        lNorm=osg::Vec2d(1.0, 0.0);
        mNorm=osg::Vec2d(0.0, 1.0);
    }

    osg::Matrixd transformation(lNorm.x(),        0.0,  -lNorm.y(),         0.0,
                                0.0,              1.0,  0.0,               0.0,
                                -mNorm.x(),        0.0,  mNorm.y(),         0.0,
                                -translation.x(), 0.0,  -translation.y(),  1.0);

    return transformation;
}

void flitr::ImageStabiliserBiLK::getHomographyMatrix(double &a00, double &a01, double &a02,
                                                     double &a10, double &a11, double &a12,
                                                     double &a20, double &a21, double &a22) const
{			
    osg::Vec2d translation=(m_h_right+m_h_left)*0.5;

    osg::Vec2d lNorm;
    osg::Vec2d mNorm;


    if (getROI_rightCentre()!=getROI_leftCentre())
    {
        lNorm=(m_h_right+getROI_rightCentre())-(m_h_left+getROI_leftCentre());
        lNorm.normalize();
        mNorm=osg::Vec2d(-lNorm.y(), lNorm.x());

        osg::Vec2d roiReference=(getROI_rightCentre()+getROI_leftCentre())*0.5-osg::Vec2d(m_pInputTexture->getTextureWidth()*0.5, m_pInputTexture->getTextureHeight()*0.5);
        osg::Vec2d transformedROIReference=osg::Vec2d(roiReference.x()*lNorm.x()+roiReference.y()*mNorm.x(),
                                                      roiReference.x()*lNorm.y()+roiReference.y()*mNorm.y());

        //Compensate for movement of rotation reference.
        translation-=(transformedROIReference-roiReference);
    } else
    {
        lNorm=osg::Vec2d(1.0, 0.0);
        mNorm=osg::Vec2d(0.0, 1.0);
    }

    a00=lNorm.x(); 		a01=-mNorm.x();		a02=-translation.x();
    a10=-lNorm.y(); 		a11=mNorm.y();		a12=-translation.y();
    a20=0.0; 		a21=0.0; 		a22=1.0;
}

void flitr::ImageStabiliserBiLK::offsetQuadPositionByMatrix(const osg::Matrixd *i_pTransformation, unsigned long i_ulWidth, unsigned long i_ulHeight)
{

    osg::Vec4d vert=osg::Vec4d(0.0, 0.0, 0.0, -1.0);
    osg::ref_ptr<osg::Vec3Array> vcoords = new osg::Vec3Array; // vertex coords

    vert=osg::Vec4d(-0.5*(i_ulWidth-2.0), 0, -0.5*(i_ulHeight-2.0), 1.0)*(*i_pTransformation);
    vcoords->push_back(osg::Vec3d(vert.x(), vert.z(), -1));

    vert=osg::Vec4d(0.5*(i_ulWidth-2.0), 0, -0.5*(i_ulHeight-2.0), 1.0)*(*i_pTransformation);
    vcoords->push_back(osg::Vec3d(vert.x(), vert.z(), -1));

    vert=osg::Vec4d(0.5*(i_ulWidth-2.0), 0, 0.5*(i_ulHeight-2.0), 1.0)*(*i_pTransformation);
    vcoords->push_back(osg::Vec3d(vert.x(), vert.z(), -1));

    vert=osg::Vec4d(-0.5*(i_ulWidth-2.0), 0, 0.5*(i_ulHeight-2.0), 1.0)*(*i_pTransformation);
    vcoords->push_back(osg::Vec3d(vert.x(), vert.z(), -1));

    m_quadGeom->setVertexArray(vcoords.get());

}


osg::ref_ptr<osg::Geode> flitr::ImageStabiliserBiLK::createScreenAlignedQuad(unsigned long i_ulWidth, unsigned long i_ulHeight, double i_dTextXOffset, double i_dTextYOffset)
{
    osg::ref_ptr<osg::Geode>       geode;
    
    osg::ref_ptr<osg::Vec2Array>   tcoords;
    osg::ref_ptr<osg::Vec3Array>   vcoords;
    osg::ref_ptr<osg::Vec4Array>   colors;
    osg::ref_ptr<osg::DrawArrays>  da;
    
    //*************************************
    //create the group and its single geode
    //*************************************
    //    sceneGroup = new osg::Group();
    geode   = new osg::Geode();
    
    //********************************************
    //create quad to stick the stitched texture on
    //********************************************
    da = new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, 4);
    
    //******************************
    //set quad colour to white
    //******************************
    colors = new osg::Vec4Array;
    colors->push_back(osg::Vec4(1.0f,1.0f,1.0f,1.0f));

    //**************************************************************
    //set the texture coords to coincide with the size of the stitch
    //**************************************************************
    tcoords = new osg::Vec2Array;
    tcoords->push_back(osg::Vec2(1+i_dTextXOffset,         1+i_dTextYOffset));
    tcoords->push_back(osg::Vec2(i_ulWidth-1+i_dTextXOffset, 1+i_dTextYOffset));
    tcoords->push_back(osg::Vec2(i_ulWidth-1+i_dTextXOffset, i_ulHeight-1+i_dTextYOffset));
    tcoords->push_back(osg::Vec2(1+i_dTextXOffset,         i_ulHeight-1+i_dTextYOffset));
    
    //********************************************
    //set up the quad in free space recalling that
    //the camera is going to look along the Z axis
    //********************************************
    vcoords = new osg::Vec3Array;
    vcoords->push_back(osg::Vec3d(-(i_ulWidth-2.0)*0.5,     -(i_ulHeight-2.0)*0.5,      -1));
    vcoords->push_back(osg::Vec3d((i_ulWidth-2.0)*0.5, -(i_ulHeight-2.0)*0.5,      -1));
    vcoords->push_back(osg::Vec3d((i_ulWidth-2.0)*0.5, (i_ulHeight-2.0)*0.5, -1));
    vcoords->push_back(osg::Vec3d(-(i_ulWidth-2.0)*0.5,     (i_ulHeight-2.0)*0.5, -1));
    
    //*************************************
    //tie all the above properties together
    //*************************************
    m_quadGeom = new osg::Geometry;
    m_quadGeom->setVertexArray(vcoords.get());
    m_quadGeom->setTexCoordArray(0, tcoords.get());
    m_quadGeom->addPrimitiveSet(da.get());
    m_quadGeom->setColorArray(colors.get());
    m_quadGeom->setColorBinding(osg::Geometry::BIND_OVERALL);

    geode->addDrawable(m_quadGeom.get());

    osg::ref_ptr<osg::StateSet> geomss = geode->getOrCreateStateSet();
    geomss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    geomss->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    geomss->setMode(GL_BLEND,osg::StateAttribute::OFF);
    //	geomss->setAttributeAndModes(new osg::AlphaFunc, osg::StateAttribute::OFF);

    return geode;
}

osg::Camera *flitr::ImageStabiliserBiLK::createScreenAlignedCamera(unsigned long i_ulWidth, unsigned long i_ulHeight)
{
    //g_ulNumCameras++;
    osg::Camera *theCamera = new osg::Camera;
    theCamera->setClearMask(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
    theCamera->setClearColor(osg::Vec4(0.5, 0.0, 0.0, 0.0));
        theCamera->setProjectionMatrixAsOrtho(-0.5/m_fOutputCropFactor*i_ulWidth, 0.5/m_fOutputCropFactor*i_ulWidth, -0.5/m_fOutputCropFactor*i_ulHeight, 0.5/m_fOutputCropFactor*i_ulHeight,-100,100);
    theCamera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
    theCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    theCamera->setViewMatrix(osg::Matrix::identity());
    theCamera->setViewport(0, 0, i_ulWidth, i_ulHeight);
    theCamera->setRenderOrder(osg::Camera::PRE_RENDER);
    theCamera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);

    return theCamera;
}


osg::Node* flitr::ImageStabiliserBiLK::createOutputPass()
{
    osg::ref_ptr<osg::Geode> geode = createScreenAlignedQuad(m_pInputTexture->getTextureWidth(), m_pInputTexture->getTextureHeight(), 0, 0);
    osg::ref_ptr<osg::StateSet> geomss = geode->getOrCreateStateSet();

    geomss->setTextureAttributeAndModes(0,(osg::TextureRectangle *)m_pInputTexture, osg::StateAttribute::ON);
    //	geomss->setTextureAttribute(0, new osg::TexEnv(osg::TexEnv::DECAL));


    //****************
    //add the uniforms
    //****************
    geomss->addUniform(new osg::Uniform("inputTexture", 0));

    geomss->addUniform(new osg::Uniform("ROIX_left", (float)m_ulROIX_left));
    geomss->addUniform(new osg::Uniform("ROIY_left", (float)m_ulROIY_left));

    geomss->addUniform(new osg::Uniform("ROIX_right", (float)m_ulROIX_right));
    geomss->addUniform(new osg::Uniform("ROIY_right", (float)m_ulROIY_right));

    geomss->addUniform(new osg::Uniform("ROIWidth", (float)m_ulROIWidth));
    geomss->addUniform(new osg::Uniform("ROIHeight", (float)m_ulROIHeight));

    geomss->addUniform(new osg::Uniform("ROIFade", m_bIndicateROI ? 0.5f : 1.0f));

    //*******************************************
    //create and add the shaders to the state set
    //*******************************************
    osg::ref_ptr<osg::Program> textureShader = new osg::Program;
    textureShader->addShader(new osg::Shader(osg::Shader::FRAGMENT,
                                             "uniform sampler2DRect inputTexture;\n"

                                             "  uniform float ROIX_left;\n"
                                             "  uniform float ROIY_left;\n"
                                             "  uniform float ROIX_right;\n"
                                             "  uniform float ROIY_right;\n"

                                             "  uniform float ROIWidth;\n"
                                             "  uniform float ROIHeight;\n"
                                             "  uniform float ROIFade;\n"

                                             "void main(void)"
                                             "{\n"
                                             "  vec2 inputTexCoord=gl_TexCoord[0].xy;\n"
                                             "  vec3 imagePixel=texture2DRect(inputTexture, inputTexCoord.xy).rgb;\n"

                                             "  if ((inputTexCoord.x>ROIX_left)&&(inputTexCoord.x<(ROIWidth+ROIX_left)) &&\n"
                                             "      (inputTexCoord.y>ROIY_left)&&(inputTexCoord.y<(ROIHeight+ROIY_left)) )\n"
                                             "  {\n"
                                             "    imagePixel*=ROIFade;\n"
                                             "  }\n"

                                             "  if ((inputTexCoord.x>ROIX_right)&&(inputTexCoord.x<(ROIWidth+ROIX_right)) &&\n"
                                             "      (inputTexCoord.y>ROIY_right)&&(inputTexCoord.y<(ROIHeight+ROIY_right)) )\n"
                                             "  {\n"
                                             "    imagePixel*=ROIFade;\n"
                                             "  }\n"

                                             "  gl_FragColor.rgb=imagePixel;\n"
                                             "  gl_FragColor.a=1.0;\n"
                                             "}\n"
                                             ));
    geomss->setAttributeAndModes(textureShader.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE );


    osg::Camera *theCamera=createScreenAlignedCamera(m_pInputTexture->getTextureWidth(), m_pInputTexture->getTextureHeight());

    theCamera->setViewport(0, 0, ((int)(m_pInputTexture->getTextureWidth()*m_iOutputScaleFactor/m_fOutputCropFactor+0.5f)), ((int)(m_pInputTexture->getTextureHeight()*m_iOutputScaleFactor/m_fOutputCropFactor+0.5f)));
    theCamera->addChild(geode.get());

    theCamera->attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER), m_outputTexture.get());

    if (m_bReadOutputBackToCPU)
    {
        theCamera->attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER), m_outputOSGImage.get());
    }

    return theCamera;
}   

//Lucas-Kanade algorithm...
void flitr::ImageStabiliserBiLK::updateH_BiLucasKanadeCPU()
{
    //	CViewScopeTimer scopeTimer(&timer, &timingMap, "findH_lucasKanade");

    long numLevels=m_pCurrentBiPyramid->getNumLevels();
    osg::Vec2d hEstimate[2];
    hEstimate[0]=osg::Vec2d(0.0, 0.0);
    hEstimate[1]=osg::Vec2d(0.0, 0.0);

    //	float *tempVizBuf=new float[m_pCurrentBiPyramid->getLevelFormat(0).getWidth() * m_pCurrentBiPyramid->getLevelFormat(0).getHeight()];
    float *resampPreviousImg=new float[m_pCurrentBiPyramid->getLevelWidth(0) * m_pCurrentBiPyramid->getLevelHeight(0)];

    for (unsigned long pyramidNum=0; pyramidNum<2; pyramidNum++)
    {
        //		memset((unsigned char *)tempVizBuf, 0, m_pCurrentBiPyramid->getLevelFormat(0).getWidth() * m_pCurrentBiPyramid->getLevelFormat(0).getHeight()*4);
        //		unsigned long vLevel=1;//numLevels-1 - 3;

        for (long levelNum=numLevels-1; levelNum>=0; levelNum--)
        {
            long width=m_pCurrentBiPyramid->getLevelWidth(levelNum);
            long height=m_pCurrentBiPyramid->getLevelHeight(levelNum);


            for (long i=0; i<NEWTON_RAPHSON_ITERATIONS; i++)//Newton-Raphson...
            {
                //Generate bi-linearly interpolated previous image!
                m_pPreviousBiPyramid->bilinearResample(pyramidNum, levelNum, -hEstimate[pyramidNum], resampPreviousImg);

                float *iData=(float *)m_pCurrentBiPyramid->getLevel(pyramidNum, levelNum)->data();
                float *dData=(float *)m_pCurrentBiPyramid->getLevelDeriv(pyramidNum, levelNum)->data();
                float *prevIData=resampPreviousImg;

                double hDenominator=0.0;
                osg::Vec2d hNumerator(0.0, 0.0);

                //Note: Ignore border of LUCAS_KANADE_BORDER pixels to improve stability.
                iData+=width * (m_ulLKBorder>>levelNum);
                dData+=4*width * (m_ulLKBorder>>levelNum);
                prevIData+=width * (m_ulLKBorder>>levelNum);

                for (long y=(m_ulLKBorder>>levelNum); y<(long)(height-(m_ulLKBorder>>levelNum)); y++)
                {
                    iData+=(m_ulLKBorder>>levelNum);
                    dData+=(m_ulLKBorder>>levelNum)<<2;
                    prevIData+=(m_ulLKBorder>>levelNum);

                    for (long x=(m_ulLKBorder>>levelNum); x<(long)(width-(m_ulLKBorder>>levelNum)); x++)
                    {
                        //osg::Vec2d previousImagePixelPosition(x, y);
                        //previousImagePixelPosition-=hEstimate[pyramidNum];
                        const double error=*iData - *prevIData;//m_pPreviousBiPyramid->bilinearLookupCPU(pyramidNum, levelNum, previousImagePixelPosition);

                        double hDeltaDenominator=*(dData+2);
                        hDenominator+=hDeltaDenominator;

                        if (hDeltaDenominator!=0)
                        {
                            osg::Vec2d hDeltaNumerator=osg::Vec2d(*(dData+0), *(dData+1)) * error;
                            hNumerator+=hDeltaNumerator;

                            /*						  if ((i==(NEWTON_RAPHSON_ITERATIONS-1))&&(levelNum==vLevel))
                          {
                              //tempVizBuf[x+y*width]=*prevIData;;
                              //tempVizBuf[x+y*width]=-1.0 * hDeltaNumerator.x();///hDeltaDenominator;
                              tempVizBuf[x+y*width]=1 * hDeltaDenominator;
                          }
*/
                        }

                        iData++;
                        dData+=4;
                        prevIData+=1;
                    }

                    iData+=(m_ulLKBorder>>levelNum);
                    dData+=(m_ulLKBorder>>levelNum)<<2;
                    prevIData+=(m_ulLKBorder>>levelNum);
                }

                osg::Vec2d deltaHEstimate=(hDenominator!=0) ? (-hNumerator/hDenominator) : osg::Vec2d(0.0, 0.0);
                hEstimate[pyramidNum]+=deltaHEstimate;
            }

            hEstimate[pyramidNum]*=2;//Next level is twice the resolution.
            //============================================================
        }

        /*
//				if (m_bIndicateROI)
            {
                long width=m_pCurrentBiPyramid->getLevelFormat(0).getWidth();
                long height=m_pCurrentBiPyramid->getLevelFormat(0).getHeight();
                for (long y=0; y<height; y++)
                for (long x=0; x<width; x++)
                {
                    *((unsigned char *)(m_pInputTexture->getImage()->data()+x+m_pCurrentBiPyramid->m_ulpROIX[pyramidNum]+m_imageFormat.getWidth()*(y+m_pCurrentBiPyramid->m_ulpROIY[pyramidNum])))
                        // *=0.8;
                        // =( (float *)(this->getLevel(vLevel)->data()) )[((x>>vLevel)+(y>>vLevel)*(width>>vLevel))*m_ucNumImageChannels]*1.0;
                        // =maxLK(minLK(((( (float *)(this->getLevelDeriv(1)->data()) )[((x>>(1))+(y>>(1))*(width>>(1)))*4])+128.0),255.0), 0);
                        // =bilinearLookup(4, osg::Vec2d(x/pow(2.0, 4.0)-0.5, y/pow(2.0, 4.0)-0.5));
                        =maxBIP(minBIP(tempVizBuf[(x>>vLevel)+(y>>vLevel)*(width>>vLevel)]*1, 127.0), -128)+128;
                }

                ((osg::TextureRectangle *)m_pInputTexture)->getImage()->dirty();
            }
*/
    }
    //	delete [] tempVizBuf;
    delete [] resampPreviousImg;

    this->m_h_left=hEstimate[0]*0.5;
    this->m_h_right=hEstimate[1]*0.5;

    //	printf("_left: %1.4f %1.4f (%1.0f)\n", m_h_left.x(), m_h_left.y(), pow(2.0, (double)(numLevels-1.0)));
    //	printf("_right: %1.4f %1.4f (%1.0f)\n", m_h_right.x(), m_h_right.y(), pow(2.0, (double)(numLevels-1.0)));
}
