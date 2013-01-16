#include <flitr/modules/lucas_kanade/ImageStabiliserNLK.h>

#include <iostream>
#include <string.h>


unsigned long flitr::ImageStabiliserNLK::m_ulLKBorder=32;//To be refined in the constructor.

double flitr::ImageStabiliserNLK::logbase(double a, double base)
{
    return log(a) / log(base);
}

void flitr::ImageStabiliserNLK::PostNPyramidRebuiltCallback::callback()
{
    if ((!m_pImageStabiliserNLK->m_bDoGPULKIterations)&&(m_pImageStabiliserNLK->m_ulFrameNumber>=2))
    {//CPU NLK...
        m_pImageStabiliserNLK->updateH_NLucasKanadeCPU();

        osg::Matrixd deltaTransform=m_pImageStabiliserNLK->getDeltaTransformationMatrix();

        for (int filterNum=0; filterNum<(int)(m_pImageStabiliserNLK->filterPairs_.size()); filterNum++)
        {
            m_pImageStabiliserNLK->filterPairs_[filterNum].first=m_pImageStabiliserNLK->filterPairs_[filterNum].first*(1.0f-m_pImageStabiliserNLK->filterPairs_[filterNum].second)+osg::Vec2d(deltaTransform(3,0), deltaTransform(3,2))*(m_pImageStabiliserNLK->filterPairs_[filterNum].second);

            if (filterNum==0)
            {//Update output quad's transform with the delta transform.
                osg::Matrixd quadDeltaTransform=deltaTransform;
                quadDeltaTransform(3,0)=quadDeltaTransform(3,0)-m_pImageStabiliserNLK->filterPairs_[filterNum].first._v[0];
                quadDeltaTransform(3,2)=quadDeltaTransform(3,2)-m_pImageStabiliserNLK->filterPairs_[filterNum].first._v[1];

                osg::Matrixd quadTransform=m_pImageStabiliserNLK->m_rpQuadMatrixTransform->getMatrix();
                quadTransform=quadDeltaTransform*quadTransform;
                m_pImageStabiliserNLK->m_rpQuadMatrixTransform->setMatrix(quadTransform);
                m_pImageStabiliserNLK->offsetQuadPositionByMatrix(&quadTransform,
                                                                  m_pImageStabiliserNLK->m_pInputTexture->getTextureWidth(),
                                                                  m_pImageStabiliserNLK->m_pInputTexture->getTextureHeight());
            }
        }

    }
}



flitr::ImageStabiliserNLK::ImageStabiliserNLK(const osg::TextureRectangle *i_pInputTexture,
                                              std::vector< std::pair<int,int> > &i_ROIVec,
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

    m_ulROIWidth(i_ulROIWidth), m_ulROIHeight(i_ulROIHeight),
    m_ulNumLKIterations(0),
    m_pCurrentNPyramid(0),
    m_pPreviousNPyramid(0),
    m_bAutoSwapCurrentPrevious(true),
    m_rpQuadMatrixTransform(new osg::MatrixTransform(osg::Matrixd::identity())),
    m_quadGeom(0),
    m_outputTexture(0),
    m_outputOSGImage(0),
    m_bReadOutputBackToCPU(i_bReadOutputBackToCPU),
    m_iOutputScaleFactor(i_iOutputScaleFactor),
    m_fOutputCropFactor(i_fOutputCropFactor),
    m_postNPyramidRebuiltCallback(this),
    m_postNLKIterationDrawCallback(this),
    m_lkResultImagePyramid(0),
    m_lkResultTexturePyramid(0),
    m_lkReducedResultImagePyramid(0),
    m_lkReducedResultTexturePyramid(0),
    m_imagePyramidFormat_(0),
    m_rpCurrentLKIterationRebuildSwitch(0),
    m_rpPreviousLKIterationRebuildSwitch(0),
    m_pInputTexture(i_pInputTexture),

    m_bIndicateROI(i_bIndicateROI),
    m_bDoGPUPyramids(i_bDoGPUPyramids),
    m_bDoGPULKIterations(i_bDoGPULKIterations),
    m_ulNumGPUHVectorReductionLevels(i_ulNumGPUHVectorReductionLevels),
    m_ulMaxGPUHVectorReductionLevels(0),
    m_bBiLinearOutputFilter(i_bBiLinearOutputFilter),
    m_ulFrameNumber(0)
{
    m_ROIVec=i_ROIVec;
    N_=i_ROIVec.size();

    m_rpHEstimateUniform=new osg::ref_ptr<osg::Uniform>[N_];
    m_h=new osg::Vec2[N_];

    for (int i=0; i<(int)N_; i++)
    {
        m_rpHEstimateUniform[i]=0;
        m_h[i]=osg::Vec2(0.0, 0.0);
    }

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


bool flitr::ImageStabiliserNLK::init(osg::Group *root_group)
{
    osg::Group *stabRoot=new osg::Group();

    bool returnVal=true;

    const unsigned long numLevelsX=logbase(m_ulROIWidth, 2.0);
    const unsigned long numLevelsY=logbase(m_ulROIHeight, 2.0);

    const unsigned long width=pow(2.0, (double)numLevelsX)+0.5;
    const unsigned long height=pow(2.0, (double)numLevelsY)+0.5;

    m_ulMaxGPUHVectorReductionLevels=(unsigned long)std::min<unsigned long>(numLevelsX, numLevelsY);

    m_ulNumGPUHVectorReductionLevels=std::min<unsigned long>(logbase(std::min<unsigned long>(width, height), 2.0)-1, m_ulNumGPUHVectorReductionLevels);

    m_pCurrentNPyramid=new ImageNPyramid(m_pInputTexture,
                                         m_ROIVec,
                                         m_ulROIWidth, m_ulROIHeight,
                                         m_bIndicateROI, m_bDoGPUPyramids, !m_bDoGPULKIterations);
    returnVal&=m_pCurrentNPyramid->init(stabRoot, &m_postNPyramidRebuiltCallback);

    m_pPreviousNPyramid=new ImageNPyramid(m_pInputTexture,
                                          m_ROIVec,
                                          m_ulROIWidth, m_ulROIHeight,
                                          m_bIndicateROI, m_bDoGPUPyramids, !m_bDoGPULKIterations);
    returnVal&=m_pPreviousNPyramid->init(stabRoot, &m_postNPyramidRebuiltCallback);


    //=== Allocate lk result textures and images : Begin ===//
    m_lkResultImagePyramid=new osg::ref_ptr<osg::Image>*[N_];
    m_lkResultTexturePyramid=new osg::ref_ptr<osg::TextureRectangle>*[N_];
    m_lkReducedResultImagePyramid=new osg::ref_ptr<osg::Image>*[N_];
    m_lkReducedResultTexturePyramid=new osg::ref_ptr<osg::TextureRectangle>*[N_];

    m_imagePyramidFormat_=new ImageFormat[m_ulMaxGPUHVectorReductionLevels];

    for (int i=0; i<(int)N_; i++)
    {
        //xPow2 and yPow2 are modified within the level for-loop!
        unsigned long xPow2=pow(2.0, (double)numLevelsX)+0.5;
        unsigned long yPow2=pow(2.0, (double)numLevelsY)+0.5;


        ImageFormat imageFormat=flitr::ImageFormat(m_pInputTexture->getTextureWidth(), m_pInputTexture->getTextureWidth());

        m_lkResultImagePyramid[i]=new osg::ref_ptr<osg::Image>[m_pCurrentNPyramid->getNumLevels()];

        m_lkResultTexturePyramid[i]=new osg::ref_ptr<osg::TextureRectangle>[m_pCurrentNPyramid->getNumLevels()];

        m_lkReducedResultImagePyramid[i]=new osg::ref_ptr<osg::Image>[m_ulMaxGPUHVectorReductionLevels];

        m_lkReducedResultTexturePyramid[i]=new osg::ref_ptr<osg::TextureRectangle>[m_ulMaxGPUHVectorReductionLevels];


        //=== Initialise LK result images and textures. For GPU LK Iterations ONLY! ===//
        unsigned long level=0;
        for (level=0; level<m_pCurrentNPyramid->getNumLevels(); level++)
        {
            imageFormat.setWidth(xPow2);
            imageFormat.setHeight(yPow2);
            //====================
            m_lkResultImagePyramid[i][level]=new osg::Image;
            m_lkResultImagePyramid[i][level]->allocateImage(xPow2, yPow2, 1, GL_RGBA, GL_FLOAT);//GL_RGB not faster than GL_RGBA!!!
            m_lkResultImagePyramid[i][level]->setInternalTextureFormat(GL_RGBA32F_ARB);
            m_lkResultImagePyramid[i][level]->setPixelFormat(GL_RGBA);
            m_lkResultImagePyramid[i][level]->setDataType(GL_FLOAT);


            m_lkResultTexturePyramid[i][level]=new osg::TextureRectangle();
            m_lkResultTexturePyramid[i][level]->setTextureSize(xPow2, yPow2);
            m_lkResultTexturePyramid[i][level]->setFilter(osg::TextureRectangle::MIN_FILTER,osg::TextureRectangle::NEAREST);
            m_lkResultTexturePyramid[i][level]->setFilter(osg::TextureRectangle::MAG_FILTER,osg::TextureRectangle::NEAREST);
            m_lkResultTexturePyramid[i][level]->setInternalFormat(GL_RGBA32F_ARB);
            m_lkResultTexturePyramid[i][level]->setSourceFormat(GL_RGBA);
            m_lkResultTexturePyramid[i][level]->setSourceType(GL_FLOAT);
            m_lkResultTexturePyramid[i][level]->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
            m_lkResultTexturePyramid[i][level]->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
            //			m_lkResultTexturePyramid[i][level]->setImage(m_lkResultImagePyramid[i][level].get());//No need to attach the image to the texture if the results are not modified by the CPU and then updated to the GPU!!!

            //====================
            //====================
            m_lkReducedResultImagePyramid[i][level]=new osg::Image;
            m_lkReducedResultImagePyramid[i][level]->allocateImage(xPow2, yPow2, 1, GL_RGBA, GL_FLOAT);//GL_RGB not faster than GL_RGBA!!!
            m_lkReducedResultImagePyramid[i][level]->setInternalTextureFormat(GL_RGBA32F_ARB);
            m_lkReducedResultImagePyramid[i][level]->setPixelFormat(GL_RGBA);
            m_lkReducedResultImagePyramid[i][level]->setDataType(GL_FLOAT);


            m_lkReducedResultTexturePyramid[i][level]=new osg::TextureRectangle();
            m_lkReducedResultTexturePyramid[i][level]->setTextureSize(xPow2, yPow2);
            m_lkReducedResultTexturePyramid[i][level]->setFilter(osg::TextureRectangle::MIN_FILTER,osg::TextureRectangle::NEAREST);
            m_lkReducedResultTexturePyramid[i][level]->setFilter(osg::TextureRectangle::MAG_FILTER,osg::TextureRectangle::NEAREST);
            m_lkReducedResultTexturePyramid[i][level]->setInternalFormat(GL_RGBA32F_ARB);
            m_lkReducedResultTexturePyramid[i][level]->setSourceFormat(GL_RGBA);
            m_lkReducedResultTexturePyramid[i][level]->setSourceType(GL_FLOAT);
            m_lkReducedResultTexturePyramid[i][level]->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
            m_lkReducedResultTexturePyramid[i][level]->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
            //			m_lkReducedResultTexturePyramid[i][level]->setImage(m_lkReducedResultImagePyramid[i][level].get());//No need to attach the image to the texture if the results are not modified by the CPU and then updated to the GPU!!!

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
            m_lkReducedResultImagePyramid[i][level]=new osg::Image;
            m_lkReducedResultImagePyramid[i][level]->allocateImage(xPow2, yPow2, 1, GL_RGBA, GL_FLOAT);//GL_RGB not faster than GL_RGBA!!!
            m_lkReducedResultImagePyramid[i][level]->setInternalTextureFormat(GL_RGBA32F_ARB);
            m_lkReducedResultImagePyramid[i][level]->setPixelFormat(GL_RGBA);
            m_lkReducedResultImagePyramid[i][level]->setDataType(GL_FLOAT);


            m_lkReducedResultTexturePyramid[i][level]=new osg::TextureRectangle();
            m_lkReducedResultTexturePyramid[i][level]->setTextureSize(xPow2, yPow2);
            m_lkReducedResultTexturePyramid[i][level]->setFilter(osg::TextureRectangle::MIN_FILTER,osg::TextureRectangle::NEAREST);
            m_lkReducedResultTexturePyramid[i][level]->setFilter(osg::TextureRectangle::MAG_FILTER,osg::TextureRectangle::NEAREST);
            m_lkReducedResultTexturePyramid[i][level]->setInternalFormat(GL_RGBA32F_ARB);
            m_lkReducedResultTexturePyramid[i][level]->setSourceFormat(GL_RGBA);
            m_lkReducedResultTexturePyramid[i][level]->setSourceType(GL_FLOAT);
            m_lkReducedResultTexturePyramid[i][level]->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
            m_lkReducedResultTexturePyramid[i][level]->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
            //			m_lkReducedResultTexturePyramid[i][level]->setImage(m_lkReducedResultImagePyramid[i][level].get());//No need to attach the image to the texture if the results are not modified by the CPU and then updated to the GPU!!!

            //====================

            m_imagePyramidFormat_[level]=imageFormat;

            xPow2/=2;
            yPow2/=2;
        }
    }
    //=== Allocate lk result textures and images : End ===//

    for (int i=0; i<(int)N_; i++)
    {
        char uniformName[256];
        sprintf(uniformName, "hEstimate_%d" ,i);
        m_rpHEstimateUniform[i]=new osg::Uniform(uniformName, osg::Vec2(0.0, 0.0));
    }


    if (m_bDoGPULKIterations)
    {//Not CPU BiLK. Add GPU Bi-LK algo to stabRoot...
        m_rpCurrentLKIterationRebuildSwitch=new osg::Switch();
        m_rpPreviousLKIterationRebuildSwitch=new osg::Switch();

        for (long level=(m_pCurrentNPyramid->getNumLevels()-1); level>=0; level--)
        {
            for (long newtRaphIter=(NEWTON_RAPHSON_ITERATIONS-1); newtRaphIter>=0; newtRaphIter--)
            {
                //=== Calculate weighted per pixel h-vector ===//
                m_rpCurrentLKIterationRebuildSwitch->addChild(createNLKIteration(m_pCurrentNPyramid, m_pPreviousNPyramid, level,
                                                                                 width>>level, height>>level,
                                                                                 !(m_ulNumGPUHVectorReductionLevels>0)));

                m_rpPreviousLKIterationRebuildSwitch->addChild(createNLKIteration(m_pPreviousNPyramid, m_pCurrentNPyramid, level,
                                                                                  width>>level, height>>level,
                                                                                  !(m_ulNumGPUHVectorReductionLevels>0)));
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


osg::ref_ptr<osg::Geode> flitr::ImageStabiliserNLK::createScreenAlignedQuadLK(unsigned long i_ulWidth, unsigned long i_ulHeight, double i_dBorderPixels)
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
    tcoords->push_back(osg::Vec2(0+i_dBorderPixels,         0+i_dBorderPixels));
    tcoords->push_back(osg::Vec2(i_ulWidth-i_dBorderPixels, 0+i_dBorderPixels));
    tcoords->push_back(osg::Vec2(i_ulWidth-i_dBorderPixels, i_ulHeight-i_dBorderPixels));
    tcoords->push_back(osg::Vec2(0+i_dBorderPixels,         i_ulHeight-i_dBorderPixels));
    
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

osg::Camera *flitr::ImageStabiliserNLK::createScreenAlignedCameraLK(unsigned long i_ulWidth, unsigned long i_ulHeight, double i_dBorderPixels)
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


osg::Node* flitr::ImageStabiliserNLK::createNLKIteration(ImageNPyramid *i_pCurrentNPyramid, ImageNPyramid *i_pPreviousNPyramid, unsigned long i_ulLevel, unsigned long i_ulWidth, unsigned long i_ulHeight, bool i_bDoPostLKIterationCallback)
{
    osg::ref_ptr<osg::Geode> geode = createScreenAlignedQuadLK(i_ulWidth, i_ulHeight);
    osg::ref_ptr<osg::StateSet> geomss = geode->getOrCreateStateSet();

    char uniformName[256];

    for (int i=0; i<(int)N_; i++)
    {
        geomss->setTextureAttributeAndModes(i, i_pCurrentNPyramid->m_textureGausPyramid[i][i_ulLevel].get(), osg::StateAttribute::ON);
        //	geomss->setTextureAttribute(0, new osg::TexEnv(osg::TexEnv::DECAL));
        sprintf(uniformName, "gausFiltTexture_%d" ,i);
        geomss->addUniform(new osg::Uniform(uniformName, (int)i));

        geomss->setTextureAttributeAndModes(i+N_, i_pCurrentNPyramid->m_derivTexturePyramid[i][i_ulLevel].get(), osg::StateAttribute::ON);
        //	geomss->setTextureAttribute(i+N_, new osg::TexEnv(osg::TexEnv::DECAL));
        sprintf(uniformName, "derivTexture_%d" ,i);
        geomss->addUniform(new osg::Uniform(uniformName, (int)(i+N_)));

        geomss->setTextureAttributeAndModes(i+N_*2, i_pPreviousNPyramid->m_textureGausPyramid[i][i_ulLevel].get(), osg::StateAttribute::ON);
        //	geomss->setTextureAttribute(i+N_*2, new osg::TexEnv(osg::TexEnv::DECAL));
        sprintf(uniformName, "previousgausFiltTexture_%d" ,i);
        geomss->addUniform(new osg::Uniform(uniformName, (int)(i+N_*2)));

        geomss->addUniform(m_rpHEstimateUniform[i]);
    }
    //*******************************************
    //create and add the shaders to the state set
    //*******************************************
    osg::ref_ptr<osg::Program> textureShader = new osg::Program;
    textureShader->setName("createLKIteration");
    textureShader->addShader(new osg::Shader(osg::Shader::FRAGMENT,
                                             "uniform sampler2DRect gausFiltTexture_0;\n"

                                             "uniform sampler2DRect derivTexture_0;\n"

                                             "uniform sampler2DRect previousgausFiltTexture_0;\n"

                                             "uniform vec2 hEstimate_0;\n"

                                             "void main(void)"
                                             "{\n"
                                             "  float imagePixel_0=texture2DRect(gausFiltTexture_0, gl_TexCoord[0].xy).r;\n"
                                             "  vec3 derivPixel_0=texture2DRect(derivTexture_0, gl_TexCoord[0].xy).rgb;\n"


                                             //======================
                                             ////Bi-linear filter for RGBAF32 pixel. Would not be required on newer hardware!
                                             "  vec2 previousImagePixelPosition_0=(gl_TexCoord[0].xy-vec2(0.5, 0.5))-hEstimate_0;\n"
                                             "  vec2 previousImagePixelPositionFloor_0=floor(previousImagePixelPosition_0);\n"
                                             "  vec2 previousImagePixelPositionFrac_0=previousImagePixelPosition_0-previousImagePixelPositionFloor_0;\n"

                                             "  float previousImagePixelTopLeft_0=texture2DRect(previousgausFiltTexture_0, previousImagePixelPositionFloor_0+vec2(0.5, 0.5)).r;\n"
                                             "  float previousImagePixelTopRight_0=texture2DRect(previousgausFiltTexture_0, previousImagePixelPositionFloor_0+vec2(1.5, 0.5)).r;\n"
                                             "  float previousImagePixelBotLeft_0=texture2DRect(previousgausFiltTexture_0, previousImagePixelPositionFloor_0+vec2(0.5, 1.5)).r;\n"
                                             "  float previousImagePixelBotRight_0=texture2DRect(previousgausFiltTexture_0, previousImagePixelPositionFloor_0+vec2(1.5, 1.5)).r;\n"
                                             "  float previousImagePixelTop_0=(previousImagePixelTopRight_0-previousImagePixelTopLeft_0)*previousImagePixelPositionFrac_0.x+previousImagePixelTopLeft_0;\n"
                                             "  float previousImagePixelBot_0=(previousImagePixelBotRight_0-previousImagePixelBotLeft_0)*previousImagePixelPositionFrac_0.x+previousImagePixelBotLeft_0;\n"
                                             "  float previousImagePixel_0=(previousImagePixelBot_0-previousImagePixelTop_0)*previousImagePixelPositionFrac_0.y+previousImagePixelTop_0;\n"
                                             //======================
                                             //OR
                                             //======================
                                             //"  vec2 previousImagePixelPosition_0=gl_TexCoord[0].xy-hEstimate_0;\n"
                                             //"  float previousImagePixel_0=texture2DRect(previousgausFiltTexture_0, previousImagePixelPosition_0).r;\n"
                                             //======================

                                             "  float error_0=imagePixel_0-previousImagePixel_0;\n"
                                             "  vec2 hNumerator_0=derivPixel_0.rg * error_0;\n"
                                             "  float hDenominator_0=derivPixel_0.b;\n"

                                             "  gl_FragData[0].rg=hNumerator_0;\n"
                                             "  gl_FragData[0].b=hDenominator_0;\n"
                                             //	    "  gl_FragData[0].a=1.0;\n"

                                             "}\n"
                                             ));
    geomss->setAttributeAndModes(textureShader.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE );


    osg::Camera *theCamera=createScreenAlignedCameraLK(i_ulWidth, i_ulHeight);
    theCamera->addChild(geode.get());

    for (int i=0; i<(int)N_; i++)
    {
        theCamera->attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER0+i), this->m_lkResultTexturePyramid[i][i_ulLevel].get());
    }


    if (i_bDoPostLKIterationCallback)
    {
        for (int i=0; i<(int)N_; i++)
        {
            theCamera->attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER0+i), this->m_lkResultImagePyramid[i][i_ulLevel].get());
        }
        theCamera->setPostDrawCallback(&m_postNLKIterationDrawCallback);
    }

    return theCamera;
}   


osg::Node* flitr::ImageStabiliserNLK::createHVectorReductionLevel(unsigned long i_ulLevel, unsigned long i_ulWidth,
                                                                  unsigned long i_ulHeight, unsigned long i_ulReductionLevel,
                                                                  bool i_bDoPostLKIterationCallback)
{
    osg::ref_ptr<osg::Geode> geode = 0;
    osg::ref_ptr<osg::StateSet> geomss = 0;

    if (i_ulReductionLevel==1)
    {//Start from LK result. Also implement LUCAS_KANADE_BORDER!
        geode = createScreenAlignedQuadLK(i_ulWidth, i_ulHeight, (m_ulLKBorder>>(i_ulLevel-1+1)));//'*0.5' because we're working with a boundary at the next HIGHER resolution.
        geomss = geode->getOrCreateStateSet();

        for (int i=0; i<(int)N_; i++)
        {
            geomss->setTextureAttributeAndModes(i, this->m_lkResultTexturePyramid[i][i_ulLevel-1].get(), osg::StateAttribute::ON);
            //	geomss->setTextureAttribute(i, new osg::TexEnv(osg::TexEnv::DECAL));
        }
    } else
    {//Use already reduced results.
        geode = createScreenAlignedQuadLK(i_ulWidth, i_ulHeight, 0);
        geomss = geode->getOrCreateStateSet();

        for (int i=0; i<(int)N_; i++)
        {
            geomss->setTextureAttributeAndModes(i, this->m_lkReducedResultTexturePyramid[i][i_ulLevel-1].get(), osg::StateAttribute::ON);
            //	geomss->setTextureAttribute(i, new osg::TexEnv(osg::TexEnv::DECAL));
        }
    }

    //****************
    //add the uniforms
    //****************
    for (int i=0; i<(int)N_; i++)
    {
        char uniformName[256];
        sprintf(uniformName, "highResLKResultTexture_%d",i);
        geomss->addUniform(new osg::Uniform(uniformName, i));
    }

    //*******************************************
    //create and add the shaders to the state set
    //*******************************************
    osg::ref_ptr<osg::Program> textureShader = new osg::Program;
    textureShader->setName("createHVectorReductionLevel");
    textureShader->addShader(new osg::Shader(osg::Shader::FRAGMENT,
                                             "uniform sampler2DRect highResLKResultTexture_0;\n"

                                             "void main(void)"
                                             "{\n"
                                             "  vec4 imagePixel0_0=texture2DRect(highResLKResultTexture_0, (gl_TexCoord[0].xy-vec2(0.5, 0.5))*2.0+vec2(0.5, 0.5));\n"
                                             "  vec4 imagePixel1_0=texture2DRect(highResLKResultTexture_0, (gl_TexCoord[0].xy-vec2(0.5, 0.5))*2.0+vec2(1.5, 0.5));\n"
                                             "  vec4 imagePixel2_0=texture2DRect(highResLKResultTexture_0, (gl_TexCoord[0].xy-vec2(0.5, 0.5))*2.0+vec2(1.5, 1.5));\n"
                                             "  vec4 imagePixel3_0=texture2DRect(highResLKResultTexture_0, (gl_TexCoord[0].xy-vec2(0.5, 0.5))*2.0+vec2(0.5, 1.5));\n"

                                             "  gl_FragData[0]=(imagePixel0_0+imagePixel1_0+imagePixel2_0+imagePixel3_0);\n"
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

    for (int i=0; i<(int)N_; i++)
    {
        theCamera->attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER0+i), this->m_lkReducedResultTexturePyramid[i][i_ulLevel].get());
    }

    if (i_bDoPostLKIterationCallback)
    {
        //LK iteration result currently readback to CPU for h-vector reduction operation!
        for (int i=0; i<(int)N_; i++)
        {
            theCamera->attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER0+i), this->m_lkReducedResultImagePyramid[i][i_ulLevel].get());
        }
        theCamera->setPostDrawCallback(&m_postNLKIterationDrawCallback);
    }

    return theCamera;
}


osg::Matrixd flitr::ImageStabiliserNLK::getDeltaTransformationMatrix() const
{			
    osg::Vec2d translation=m_h[0];

    osg::Vec2d lNorm;
    osg::Vec2d mNorm;

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

void flitr::ImageStabiliserNLK::getHomographyMatrix(double &a00, double &a01, double &a02,
                                                    double &a10, double &a11, double &a12,
                                                    double &a20, double &a21, double &a22) const
{			
    osg::Vec2d translation=m_h[0];

    osg::Vec2d lNorm;
    osg::Vec2d mNorm;

    {
        lNorm=osg::Vec2d(1.0, 0.0);
        mNorm=osg::Vec2d(0.0, 1.0);
    }

    a00=lNorm.x(); 		a01=-mNorm.x();		a02=-translation.x();
    a10=-lNorm.y(); 		a11=mNorm.y();		a12=-translation.y();
    a20=0.0; 		a21=0.0; 		a22=1.0;
}

void flitr::ImageStabiliserNLK::offsetQuadPositionByMatrix(const osg::Matrixd *i_pTransformation, unsigned long i_ulWidth, unsigned long i_ulHeight)
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


osg::ref_ptr<osg::Geode> flitr::ImageStabiliserNLK::createScreenAlignedQuad(unsigned long i_ulWidth, unsigned long i_ulHeight)
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
    tcoords->push_back(osg::Vec2(1,         1));
    tcoords->push_back(osg::Vec2(i_ulWidth-1, 1));
    tcoords->push_back(osg::Vec2(i_ulWidth-1, i_ulHeight-1));
    tcoords->push_back(osg::Vec2(1,         i_ulHeight-1));
    
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

osg::Camera *flitr::ImageStabiliserNLK::createScreenAlignedCamera(unsigned long i_ulWidth, unsigned long i_ulHeight)
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


osg::Node* flitr::ImageStabiliserNLK::createOutputPass()
{
    osg::ref_ptr<osg::Geode> geode = createScreenAlignedQuad(m_pInputTexture->getTextureWidth(), m_pInputTexture->getTextureHeight());
    osg::ref_ptr<osg::StateSet> geomss = geode->getOrCreateStateSet();

    geomss->setTextureAttributeAndModes(0,(osg::TextureRectangle *)m_pInputTexture, osg::StateAttribute::ON);
    //	geomss->setTextureAttribute(0, new osg::TexEnv(osg::TexEnv::DECAL));


    //****************
    //add the uniforms
    //****************
    geomss->addUniform(new osg::Uniform("inputTexture", 0));

    char uniformName[256];
    for (int i=0; i<(int)N_; i++)
    {
        sprintf(uniformName,"ROIX_%d",i);
        geomss->addUniform(new osg::Uniform(uniformName, (float)(m_ROIVec[i].first)));
        sprintf(uniformName,"ROIY_%d",i);
        geomss->addUniform(new osg::Uniform(uniformName, (float)(m_ROIVec[i].second)));
    }

    geomss->addUniform(new osg::Uniform("ROIWidth", (float)m_ulROIWidth));
    geomss->addUniform(new osg::Uniform("ROIHeight", (float)m_ulROIHeight));

    geomss->addUniform(new osg::Uniform("ROIFade", m_bIndicateROI ? 0.5f : 1.0f));

    //*******************************************
    //create and add the shaders to the state set
    //*******************************************

    std::stringstream ss;

    ss << "uniform sampler2DRect inputTexture;\n";

    for (int i=0; i<N_; i++)
    {
        ss << "uniform float ROIX_"<<i<<";\n"
              "uniform float ROIY_"<<i<<";\n";
    }

    ss << "uniform float ROIWidth;\n"
          "uniform float ROIHeight;\n"
          "uniform float ROIFade;\n";

    ss << "void main(void)"
          "{\n"
          "  vec2 inputTexCoord=gl_TexCoord[0].xy;\n"
          "  vec3 imagePixel=texture2DRect(inputTexture, inputTexCoord.xy).rgb;\n";

    for (int i=0; i<N_; i++)
    {
        ss << "  if ((inputTexCoord.x>ROIX_"<<i<<")&&(inputTexCoord.x<(ROIWidth+ROIX_"<<i<<")) &&\n"
              "      (inputTexCoord.y>ROIY_"<<i<<")&&(inputTexCoord.y<(ROIHeight+ROIY_"<<i<<")) )\n"
              "  {\n"
              "    imagePixel*=ROIFade;\n"
              "  }\n";
    }

    ss << "  gl_FragColor.rgb=imagePixel;\n"
          "  gl_FragColor.a=1.0;\n"
          "}\n";

    osg::ref_ptr<osg::Program> textureShader = new osg::Program;
    textureShader->addShader(new osg::Shader(osg::Shader::FRAGMENT, ss.str()));
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
void flitr::ImageStabiliserNLK::updateH_NLucasKanadeCPU()
{
    //	CViewScopeTimer scopeTimer(&timer, &timingMap, "findH_lucasKanade");
    long numLevels=m_pCurrentNPyramid->getNumLevels();
    osg::Vec2d hEstimate[N_];

    //	float *tempVizBuf=new float[m_pCurrentBiPyramid->getLevelFormat(0).getWidth() * m_pCurrentBiPyramid->getLevelFormat(0).getHeight()];
    float *resampPreviousImg=new float[m_pCurrentNPyramid->getLevelWidth(0) * m_pCurrentNPyramid->getLevelHeight(0)];

    for (unsigned long pyramidNum=0; pyramidNum<N_; pyramidNum++)
    {
        hEstimate[pyramidNum]=osg::Vec2d(0.0, 0.0);

        for (long levelNum=numLevels-1; levelNum>=0; levelNum--)
        {
            long width=m_pCurrentNPyramid->getLevelWidth(levelNum);
            long height=m_pCurrentNPyramid->getLevelHeight(levelNum);


            for (long i=0; i<NEWTON_RAPHSON_ITERATIONS; i++)//Newton-Raphson...
            {
                //Generate bi-linearly interpolated previous image!
                m_pPreviousNPyramid->bilinearResample(pyramidNum, levelNum, -hEstimate[pyramidNum], resampPreviousImg);

                float *iData=(float *)m_pCurrentNPyramid->getLevel(pyramidNum, levelNum)->data();
                float *dData=(float *)m_pCurrentNPyramid->getLevelDeriv(pyramidNum, levelNum)->data();
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

    for (int i=0; i<(int)N_; i++)
    {
        this->m_h[i]=hEstimate[i]*0.5;
    }
}
