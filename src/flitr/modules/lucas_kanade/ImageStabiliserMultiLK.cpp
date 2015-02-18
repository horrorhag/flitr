#include <flitr/modules/lucas_kanade/ImageStabiliserMultiLK.h>

#include <iostream>
#include <string.h>


unsigned long flitr::ImageStabiliserMultiLK::m_ulLKBorder=32;//To be refined in the constructor.

double flitr::ImageStabiliserMultiLK::logbase(const double a, const double base)
{
    return log(a) / log(base);
}

void flitr::ImageStabiliserMultiLK::PostNPyramidRebuiltCallback::callback()
{//Called if Pyramid built on GPU, but rest of algorithm run on CPU.
    if ((!m_pImageStabiliserNLK->m_bDoGPULKIterations)&&(m_pImageStabiliserNLK->m_ulFrameNumber>=2))
    {//CPU NLK...
        m_pImageStabiliserNLK->updateH_NLucasKanadeCPU();

        m_pImageStabiliserNLK->updateOutputQuadTransform();
    }
}



flitr::ImageStabiliserMultiLK::ImageStabiliserMultiLK(const flitr::TextureRectangle *i_pInputTexture,
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

    m_numPyramids_(i_ROIVec.size()),
    m_ulROIWidth(i_ulROIWidth), m_ulROIHeight(i_ulROIHeight),
    m_ulNumLKIterations(0),
    m_pCurrentNPyramid(0),
    m_pPreviousNPyramid(0),
    m_bAutoSwapCurrentPrevious(true),
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
    if (m_pInputTexture->getFilter(osg::Texture::MIN_FILTER)!=osg::Texture::LINEAR)
    {
        logMessage(LOG_CRITICAL) << "WARNING: ImageStabiliserMultiLK:: The input texture does not have its filter set to bi-linear/LINEAR. The registration will not be as accurate as it could be!!!\n";
    }

    m_ROIVec=i_ROIVec;
    osg::Vec2f ROICentre(0.0, 0.0);

    for (uint32_t i=0; i<m_numPyramids_; i++)
    {
        osg::Vec2f rc=getROI_Centre(i);
        m_TransformedROICentreVec.push_back(std::pair<double, double>((double)rc._v[0], (double)rc._v[1]));
        ROICentre+=rc;
    }

    ROICentre/=m_numPyramids_;

    m_rpHEstimateUniform=new osg::ref_ptr<osg::Uniform>[m_numPyramids_];
    m_h=new osg::Vec2[m_numPyramids_];

    for (int i=0; i<(int)m_numPyramids_; i++)
    {
        m_rpHEstimateUniform[i]=0;
        m_h[i]=osg::Vec2(0.0, 0.0);
    }

    for (int i=0; i<numFilterPairs; i++)
    {
        filterPairs_.push_back(std::pair<osg::Vec2f, float>(osg::Vec2f(0.0,0.0), filterHistory));
        filteredROICentres_.push_back(ROICentre);
    }

    previousROICentre_=ROICentre;

    m_ulLKBorder=16;//logbase(m_ulROIWidth,2)-1;

    if (!i_pInputTexture->getImage())
    {
        if (m_bDoGPUPyramids==false) logMessage(LOG_INFO) << "WARNING: ImageStabiliserMultiLK:: Forcing the GPU pyramid construction to ON. Reason: The input texture is not available as an image on the CPU.\n";
        m_bDoGPUPyramids=true;
    }
}


bool flitr::ImageStabiliserMultiLK::init(osg::Group *root_group)
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
    m_lkResultImagePyramid=new osg::ref_ptr<osg::Image>*[m_numPyramids_];
    m_lkResultTexturePyramid=new osg::ref_ptr<flitr::TextureRectangle>*[m_numPyramids_];
    m_lkReducedResultImagePyramid=new osg::ref_ptr<osg::Image>*[m_numPyramids_];
    m_lkReducedResultTexturePyramid=new osg::ref_ptr<flitr::TextureRectangle>*[m_numPyramids_];

    m_imagePyramidFormat_=new ImageFormat[m_ulMaxGPUHVectorReductionLevels];

    for (int i=0; i<(int)m_numPyramids_; i++)
    {
        //xPow2 and yPow2 are modified within the level for-loop!
        unsigned long xPow2=pow(2.0, (double)numLevelsX)+0.5;
        unsigned long yPow2=pow(2.0, (double)numLevelsY)+0.5;


        ImageFormat imageFormat=flitr::ImageFormat(m_pInputTexture->getTextureWidth(), m_pInputTexture->getTextureWidth());

        m_lkResultImagePyramid[i]=new osg::ref_ptr<osg::Image>[m_pCurrentNPyramid->getNumLevels()];

        m_lkResultTexturePyramid[i]=new osg::ref_ptr<flitr::TextureRectangle>[m_pCurrentNPyramid->getNumLevels()];

        m_lkReducedResultImagePyramid[i]=new osg::ref_ptr<osg::Image>[m_ulMaxGPUHVectorReductionLevels];

        m_lkReducedResultTexturePyramid[i]=new osg::ref_ptr<flitr::TextureRectangle>[m_ulMaxGPUHVectorReductionLevels];


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


            m_lkResultTexturePyramid[i][level]=new flitr::TextureRectangle();
            m_lkResultTexturePyramid[i][level]->setTextureSize(xPow2, yPow2);
            m_lkResultTexturePyramid[i][level]->setFilter(flitr::TextureRectangle::MIN_FILTER,flitr::TextureRectangle::NEAREST);
            m_lkResultTexturePyramid[i][level]->setFilter(flitr::TextureRectangle::MAG_FILTER,flitr::TextureRectangle::NEAREST);
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


            m_lkReducedResultTexturePyramid[i][level]=new flitr::TextureRectangle();
            m_lkReducedResultTexturePyramid[i][level]->setTextureSize(xPow2, yPow2);
            m_lkReducedResultTexturePyramid[i][level]->setFilter(flitr::TextureRectangle::MIN_FILTER,flitr::TextureRectangle::NEAREST);
            m_lkReducedResultTexturePyramid[i][level]->setFilter(flitr::TextureRectangle::MAG_FILTER,flitr::TextureRectangle::NEAREST);
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


            m_lkReducedResultTexturePyramid[i][level]=new flitr::TextureRectangle();
            m_lkReducedResultTexturePyramid[i][level]->setTextureSize(xPow2, yPow2);
            m_lkReducedResultTexturePyramid[i][level]->setFilter(flitr::TextureRectangle::MIN_FILTER,flitr::TextureRectangle::NEAREST);
            m_lkReducedResultTexturePyramid[i][level]->setFilter(flitr::TextureRectangle::MAG_FILTER,flitr::TextureRectangle::NEAREST);
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

    for (int i=0; i<(int)m_numPyramids_; i++)
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
                                                                                 (m_ulNumGPUHVectorReductionLevels==0)));

                m_rpPreviousLKIterationRebuildSwitch->addChild(createNLKIteration(m_pPreviousNPyramid, m_pCurrentNPyramid, level,
                                                                                  width>>level, height>>level,
                                                                                  (m_ulNumGPUHVectorReductionLevels==0)));
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
    


    m_outputTexture=new flitr::TextureRectangle();
    m_outputTexture->setTextureSize(((int)(m_pInputTexture->getTextureWidth()*m_iOutputScaleFactor/m_fOutputCropFactor+0.5f)), ((int)(m_pInputTexture->getTextureHeight()*m_iOutputScaleFactor/m_fOutputCropFactor+0.5f)));
    if (m_bBiLinearOutputFilter)
    {
        m_outputTexture->setFilter(flitr::TextureRectangle::MIN_FILTER,flitr::TextureRectangle::LINEAR);
        m_outputTexture->setFilter(flitr::TextureRectangle::MAG_FILTER,flitr::TextureRectangle::LINEAR);
    } else
    {
        m_outputTexture->setFilter(flitr::TextureRectangle::MIN_FILTER,flitr::TextureRectangle::NEAREST);
        m_outputTexture->setFilter(flitr::TextureRectangle::MAG_FILTER,flitr::TextureRectangle::NEAREST);
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


osg::ref_ptr<osg::Geode> flitr::ImageStabiliserMultiLK::createScreenAlignedQuadLK(unsigned long i_ulWidth, unsigned long i_ulHeight, double i_dBorderPixels)
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

osg::Camera *flitr::ImageStabiliserMultiLK::createScreenAlignedCameraLK(unsigned long i_ulWidth, unsigned long i_ulHeight, double i_dBorderPixels)
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


osg::Node* flitr::ImageStabiliserMultiLK::createNLKIteration(ImageNPyramid *i_pCurrentNPyramid, ImageNPyramid *i_pPreviousNPyramid, unsigned long i_ulLevel, unsigned long i_ulWidth, unsigned long i_ulHeight, bool i_bDoPostLKIterationCallback)
{
    osg::ref_ptr<osg::Geode> geode = createScreenAlignedQuadLK(i_ulWidth, i_ulHeight);
    osg::ref_ptr<osg::StateSet> geomss = geode->getOrCreateStateSet();

    char uniformName[256];

    for (int i=0; i<(int)m_numPyramids_; i++)
    {
        geomss->setTextureAttributeAndModes(i, i_pCurrentNPyramid->m_textureGausPyramid[i][i_ulLevel].get(), osg::StateAttribute::ON);
        //	geomss->setTextureAttribute(0, new osg::TexEnv(osg::TexEnv::DECAL));
        sprintf(uniformName, "gausFiltTexture_%d" ,i);
        geomss->addUniform(new osg::Uniform(uniformName, (int)i));

        geomss->setTextureAttributeAndModes(i+m_numPyramids_, i_pCurrentNPyramid->m_derivTexturePyramid[i][i_ulLevel].get(), osg::StateAttribute::ON);
        //	geomss->setTextureAttribute(i+N_, new osg::TexEnv(osg::TexEnv::DECAL));
        sprintf(uniformName, "derivTexture_%d" ,i);
        geomss->addUniform(new osg::Uniform(uniformName, (int)(i+m_numPyramids_)));

        geomss->setTextureAttributeAndModes(i+m_numPyramids_*2, i_pPreviousNPyramid->m_textureGausPyramid[i][i_ulLevel].get(), osg::StateAttribute::ON);
        //	geomss->setTextureAttribute(i+N_*2, new osg::TexEnv(osg::TexEnv::DECAL));
        sprintf(uniformName, "previousgausFiltTexture_%d" ,i);
        geomss->addUniform(new osg::Uniform(uniformName, (int)(i+m_numPyramids_*2)));

        geomss->addUniform(m_rpHEstimateUniform[i]);
    }
    //*******************************************
    //create and add the shaders to the state set
    //*******************************************
    std::stringstream ss;

    for (int i=0; i<(int)m_numPyramids_; i++)
    {
        ss << "uniform sampler2DRect gausFiltTexture_"<<i<<";\n"

              "uniform sampler2DRect derivTexture_"<<i<<";\n"

              "uniform sampler2DRect previousgausFiltTexture_"<<i<<";\n"

              "uniform vec2 hEstimate_"<<i<<";\n";
    }

    ss << "void main(void)"
          "{\n";

    for (int i=0; i<(int)m_numPyramids_; i++)
    {
        ss << "  float imagePixel_"<<i<<"=texture2DRect(gausFiltTexture_"<<i<<", gl_TexCoord[0].xy).r;\n"
              "  vec3 derivPixel_"<<i<<"=texture2DRect(derivTexture_"<<i<<", gl_TexCoord[0].xy).rgb;\n"


              //======================
              ////Bi-linear filter for RGBAF32 pixel. Would not be required on newer hardware!
              "  vec2 previousImagePixelPosition_"<<i<<"=(gl_TexCoord[0].xy-vec2(0.5, 0.5))-hEstimate_"<<i<<";\n"
              "  vec2 previousImagePixelPositionFloor_"<<i<<"=floor(previousImagePixelPosition_"<<i<<");\n"
              "  vec2 previousImagePixelPositionFrac_"<<i<<"=previousImagePixelPosition_"<<i<<"-previousImagePixelPositionFloor_"<<i<<";\n"

              "  float previousImagePixelTopLeft_"<<i<<"=texture2DRect(previousgausFiltTexture_"<<i<<", previousImagePixelPositionFloor_"<<i<<"+vec2(0.5, 0.5)).r;\n"
              "  float previousImagePixelTopRight_"<<i<<"=texture2DRect(previousgausFiltTexture_"<<i<<", previousImagePixelPositionFloor_"<<i<<"+vec2(1.5, 0.5)).r;\n"
              "  float previousImagePixelBotLeft_"<<i<<"=texture2DRect(previousgausFiltTexture_"<<i<<", previousImagePixelPositionFloor_"<<i<<"+vec2(0.5, 1.5)).r;\n"
              "  float previousImagePixelBotRight_"<<i<<"=texture2DRect(previousgausFiltTexture_"<<i<<", previousImagePixelPositionFloor_"<<i<<"+vec2(1.5, 1.5)).r;\n"
              "  float previousImagePixelTop_"<<i<<"=(previousImagePixelTopRight_"<<i<<"-previousImagePixelTopLeft_"<<i<<")*previousImagePixelPositionFrac_"<<i<<".x+previousImagePixelTopLeft_"<<i<<";\n"
              "  float previousImagePixelBot_"<<i<<"=(previousImagePixelBotRight_"<<i<<"-previousImagePixelBotLeft_"<<i<<")*previousImagePixelPositionFrac_"<<i<<".x+previousImagePixelBotLeft_"<<i<<";\n"
              "  float previousImagePixel_"<<i<<"=(previousImagePixelBot_"<<i<<"-previousImagePixelTop_"<<i<<")*previousImagePixelPositionFrac_"<<i<<".y+previousImagePixelTop_"<<i<<";\n"
              //======================
              //OR
              //======================
              //"  vec2 previousImagePixelPosition_"<<i<<"=gl_TexCoord[0].xy-hEstimate_"<<i<<";\n"
              //"  float previousImagePixel_"<<i<<"=texture2DRect(previousgausFiltTexture_"<<i<<", previousImagePixelPosition_"<<i<<").r;\n"
              //======================

              "  float error_"<<i<<"=imagePixel_"<<i<<"-previousImagePixel_"<<i<<";\n"
              "  vec2 hNumerator_"<<i<<"=derivPixel_"<<i<<".rg * error_"<<i<<";\n"
              "  float hDenominator_"<<i<<"=derivPixel_"<<i<<".b;\n"

              "  gl_FragData["<<i<<"].rg=hNumerator_"<<i<<";\n"
              "  gl_FragData["<<i<<"].b=hDenominator_"<<i<<";\n";
    }

    ss << "}\n";


    osg::ref_ptr<osg::Program> textureShader = new osg::Program;
    textureShader->setName("ImageStabiliserMultiLK::createLKIteration");
    textureShader->addShader(new osg::Shader(osg::Shader::FRAGMENT, ss.str()));
    geomss->setAttributeAndModes(textureShader.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE );


    osg::Camera *theCamera=createScreenAlignedCameraLK(i_ulWidth, i_ulHeight);
    theCamera->addChild(geode.get());

    for (int i=0; i<(int)m_numPyramids_; i++)
    {
        theCamera->attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER0+i), this->m_lkResultTexturePyramid[i][i_ulLevel].get());
    }


    if (i_bDoPostLKIterationCallback)
    {
        for (int i=0; i<(int)m_numPyramids_; i++)
        {
            theCamera->attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER0+i), this->m_lkResultImagePyramid[i][i_ulLevel].get());
        }
        theCamera->setPostDrawCallback(&m_postNLKIterationDrawCallback);
    }

    return theCamera;
}   


osg::Node* flitr::ImageStabiliserMultiLK::createHVectorReductionLevel(unsigned long i_ulLevel, unsigned long i_ulWidth,
                                                                      unsigned long i_ulHeight, unsigned long i_ulReductionLevel,
                                                                      bool i_bDoPostLKIterationCallback)
{
    osg::ref_ptr<osg::Geode> geode = 0;
    osg::ref_ptr<osg::StateSet> geomss = 0;

    if (i_ulReductionLevel==1)
    {//Start from LK result. Also implement LUCAS_KANADE_BORDER!
        geode = createScreenAlignedQuadLK(i_ulWidth, i_ulHeight, (m_ulLKBorder>>(i_ulLevel-1+1)));//'*0.5' because we're working with a boundary at the next HIGHER resolution.
        geomss = geode->getOrCreateStateSet();

        for (int i=0; i<(int)m_numPyramids_; i++)
        {
            geomss->setTextureAttributeAndModes(i, this->m_lkResultTexturePyramid[i][i_ulLevel-1].get(), osg::StateAttribute::ON);
            //	geomss->setTextureAttribute(i, new osg::TexEnv(osg::TexEnv::DECAL));
        }
    } else
    {//Use already reduced results.
        geode = createScreenAlignedQuadLK(i_ulWidth, i_ulHeight, 0);
        geomss = geode->getOrCreateStateSet();

        for (int i=0; i<(int)m_numPyramids_; i++)
        {
            geomss->setTextureAttributeAndModes(i, this->m_lkReducedResultTexturePyramid[i][i_ulLevel-1].get(), osg::StateAttribute::ON);
            //	geomss->setTextureAttribute(i, new osg::TexEnv(osg::TexEnv::DECAL));
        }
    }

    //****************
    //add the uniforms
    //****************
    for (int i=0; i<(int)m_numPyramids_; i++)
    {
        char uniformName[256];
        sprintf(uniformName, "highResLKResultTexture_%d",i);
        geomss->addUniform(new osg::Uniform(uniformName, i));
    }

    //*******************************************
    //create and add the shaders to the state set
    //*******************************************
    std::stringstream ss;
    
    for (int i=0; i<(int)m_numPyramids_; i++)
    {
        ss << "uniform sampler2DRect highResLKResultTexture_"<<i<<";\n";
    }

    ss << "void main(void)"
          "{\n";

    for (int i=0; i<(int)m_numPyramids_; i++)
    {
        ss << "  vec4 imagePixel0_"<<i<<"=texture2DRect(highResLKResultTexture_"<<i<<", (gl_TexCoord[0].xy-vec2(0.5, 0.5))*2.0+vec2(0.5, 0.5));\n"
              "  vec4 imagePixel1_"<<i<<"=texture2DRect(highResLKResultTexture_"<<i<<", (gl_TexCoord[0].xy-vec2(0.5, 0.5))*2.0+vec2(1.5, 0.5));\n"
              "  vec4 imagePixel2_"<<i<<"=texture2DRect(highResLKResultTexture_"<<i<<", (gl_TexCoord[0].xy-vec2(0.5, 0.5))*2.0+vec2(1.5, 1.5));\n"
              "  vec4 imagePixel3_"<<i<<"=texture2DRect(highResLKResultTexture_"<<i<<", (gl_TexCoord[0].xy-vec2(0.5, 0.5))*2.0+vec2(0.5, 1.5));\n"

              "  gl_FragData["<<i<<"]=(imagePixel0_"<<i<<"+imagePixel1_"<<i<<"+imagePixel2_"<<i<<"+imagePixel3_"<<i<<");\n";
    }
    ss << "}\n";

    osg::ref_ptr<osg::Program> textureShader = new osg::Program;
    textureShader->setName("ImageStabiliserMultiLK::createHVectorReductionLevel");
    textureShader->addShader(new osg::Shader(osg::Shader::FRAGMENT, ss.str()));
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

    for (int i=0; i<(int)m_numPyramids_; i++)
    {
        theCamera->attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER0+i), this->m_lkReducedResultTexturePyramid[i][i_ulLevel].get());
    }

    if (i_bDoPostLKIterationCallback)
    {
        //LK iteration result currently readback to CPU for h-vector reduction operation!
        for (int i=0; i<(int)m_numPyramids_; i++)
        {
            theCamera->attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER0+i), this->m_lkReducedResultImagePyramid[i][i_ulLevel].get());
        }
        theCamera->setPostDrawCallback(&m_postNLKIterationDrawCallback);
    }

    return theCamera;
}


osg::Matrixd flitr::ImageStabiliserMultiLK::getDeltaTransformationMatrix() const
{			
    osg::Vec2d translation=osg::Vec2d(0.0, 0.0);
    for (int i=0; i<(int)m_numPyramids_; i++)
    {
        translation+=m_h[i];
    }
    translation/=m_numPyramids_;

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


void flitr::ImageStabiliserMultiLK::offsetQuadPositionByROICentres(uint32_t i_ulWidth, uint32_t i_ulHeight, double xOff, double yOff)
{
    osg::ref_ptr<osg::Vec3Array> vcoords = new osg::Vec3Array; // vertex coords

    for (double y=1.0; y<i_ulHeight-1.0f-0.5; y+=((double)(i_ulHeight-2)/NUM_OUTPUT_QUAD_STEPS))
    {
        for (double x=1.0; x<i_ulWidth-1.0f-0.5; x+=((double)(i_ulWidth-2))/NUM_OUTPUT_QUAD_STEPS)
        {

            osg::Vec2 v1=transformLRCoordByROICentres(osg::Vec2f(x,                                               y));
            osg::Vec2 v2=transformLRCoordByROICentres(osg::Vec2f(x+((double)(i_ulWidth-2))/NUM_OUTPUT_QUAD_STEPS, y));
            osg::Vec2 v3=transformLRCoordByROICentres(osg::Vec2f(x+((double)(i_ulWidth-2))/NUM_OUTPUT_QUAD_STEPS, y+((double)(i_ulHeight-2)/NUM_OUTPUT_QUAD_STEPS)));
            osg::Vec2 v4=transformLRCoordByROICentres(osg::Vec2f(x,                                               y+((double)(i_ulHeight-2)/NUM_OUTPUT_QUAD_STEPS)));

            vcoords->push_back(osg::Vec3(v1._v[0]-((double)i_ulWidth)*0.5-xOff, v1._v[1]-((double)i_ulHeight)*0.5-yOff, -1.0));
            vcoords->push_back(osg::Vec3(v2._v[0]-((double)i_ulWidth)*0.5-xOff, v2._v[1]-((double)i_ulHeight)*0.5-yOff, -1.0));
            vcoords->push_back(osg::Vec3(v3._v[0]-((double)i_ulWidth)*0.5-xOff, v3._v[1]-((double)i_ulHeight)*0.5-yOff, -1.0));
            vcoords->push_back(osg::Vec3(v4._v[0]-((double)i_ulWidth)*0.5-xOff, v4._v[1]-((double)i_ulHeight)*0.5-yOff, -1.0));
        }
    }

    m_quadGeom->setVertexArray(vcoords.get());

}


osg::ref_ptr<osg::Geode> flitr::ImageStabiliserMultiLK::createScreenAlignedQuad(unsigned long i_ulWidth, unsigned long i_ulHeight)
{
    osg::ref_ptr<osg::Geode>       geode;
    
    osg::ref_ptr<osg::Vec2Array>   tcoords;
    osg::ref_ptr<osg::Vec3Array>   vcoords;
    osg::ref_ptr<osg::Vec4Array>   colors;
    osg::ref_ptr<osg::DrawArrays>  da;
    
    geode   = new osg::Geode();

    //******************************
    //set quad colour to white
    //******************************
    colors = new osg::Vec4Array;
    colors->push_back(osg::Vec4(1.0f,1.0f,1.0f,1.0f));


    tcoords = new osg::Vec2Array;
    vcoords = new osg::Vec3Array;

    for (double y=1.0; y<i_ulHeight-1.0f-0.5; y+=((double)(i_ulHeight-2)/NUM_OUTPUT_QUAD_STEPS))
    {
        for (double x=1.0; x<i_ulWidth-1.0f-0.5; x+=((double)(i_ulWidth-2))/NUM_OUTPUT_QUAD_STEPS)
        {
         tcoords->push_back(osg::Vec2(x,                                  y));
         tcoords->push_back(osg::Vec2(x+((double)(i_ulWidth-2))/NUM_OUTPUT_QUAD_STEPS, y));
         tcoords->push_back(osg::Vec2(x+((double)(i_ulWidth-2))/NUM_OUTPUT_QUAD_STEPS, y+((double)(i_ulHeight-2)/NUM_OUTPUT_QUAD_STEPS)));
         tcoords->push_back(osg::Vec2(x,                                  y+((double)(i_ulHeight-2)/NUM_OUTPUT_QUAD_STEPS)));

         osg::Vec2 v1=(osg::Vec2f(x,                                  y));
         osg::Vec2 v2=(osg::Vec2f(x+((double)(i_ulWidth-2))/NUM_OUTPUT_QUAD_STEPS, y));
         osg::Vec2 v3=(osg::Vec2f(x+((double)(i_ulWidth-2))/NUM_OUTPUT_QUAD_STEPS, y+((double)(i_ulHeight-2)/NUM_OUTPUT_QUAD_STEPS)));
         osg::Vec2 v4=(osg::Vec2f(x,                                  y+((double)(i_ulHeight-2)/NUM_OUTPUT_QUAD_STEPS)));

         vcoords->push_back(osg::Vec3(v1._v[0]-((double)i_ulWidth)*0.5, v1._v[1]-((double)i_ulHeight)*0.5, -1.0));
         vcoords->push_back(osg::Vec3(v2._v[0]-((double)i_ulWidth)*0.5, v2._v[1]-((double)i_ulHeight)*0.5, -1.0));
         vcoords->push_back(osg::Vec3(v3._v[0]-((double)i_ulWidth)*0.5, v3._v[1]-((double)i_ulHeight)*0.5, -1.0));
         vcoords->push_back(osg::Vec3(v4._v[0]-((double)i_ulWidth)*0.5, v4._v[1]-((double)i_ulHeight)*0.5, -1.0));
        }
    }


    da = new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, vcoords->size());


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

osg::Camera *flitr::ImageStabiliserMultiLK::createScreenAlignedCamera(unsigned long i_ulWidth, unsigned long i_ulHeight)
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


osg::Node* flitr::ImageStabiliserMultiLK::createOutputPass()
{
    osg::ref_ptr<osg::Geode> geode = createScreenAlignedQuad(m_pInputTexture->getTextureWidth(), m_pInputTexture->getTextureHeight());
    osg::ref_ptr<osg::StateSet> geomss = geode->getOrCreateStateSet();

    geomss->setTextureAttributeAndModes(0,(flitr::TextureRectangle *)m_pInputTexture, osg::StateAttribute::ON);
    //	geomss->setTextureAttribute(0, new osg::TexEnv(osg::TexEnv::DECAL));


    //****************
    //add the uniforms
    //****************
    geomss->addUniform(new osg::Uniform("inputTexture", 0));

    char uniformName[256];
    for (int i=0; i<(int)m_numPyramids_; i++)
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

    for (int i=0; i<(int)m_numPyramids_; i++)
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

    for (int i=0; i<(int)m_numPyramids_; i++)
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
    textureShader->setName("ImageStabiliserMultiLK::createOutputPass");
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
void flitr::ImageStabiliserMultiLK::updateH_NLucasKanadeCPU()
{
    //	CViewScopeTimer scopeTimer(&timer, &timingMap, "findH_lucasKanade");
    long numLevels=m_pCurrentNPyramid->getNumLevels();
    osg::Vec2d hEstimate[/*m_numPyramids_*/256];

    //	float *tempVizBuf=new float[m_pCurrentBiPyramid->getLevelFormat(0).getWidth() * m_pCurrentBiPyramid->getLevelFormat(0).getHeight()];
    float *resampPreviousImg=new float[m_pCurrentNPyramid->getLevelWidth(0) * m_pCurrentNPyramid->getLevelHeight(0)];

    for (unsigned long pyramidNum=0; pyramidNum<m_numPyramids_; pyramidNum++)
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

                ((flitr::TextureRectangle *)m_pInputTexture)->getImage()->dirty();
            }
*/
    }
    //	delete [] tempVizBuf;
    delete [] resampPreviousImg;

    for (int i=0; i<(int)m_numPyramids_; i++)
    {
        this->m_h[i]=hEstimate[i]*0.5;
    }
}
