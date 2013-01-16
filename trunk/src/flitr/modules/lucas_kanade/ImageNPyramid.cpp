#include <cstdio>

#include <flitr/modules/lucas_kanade/ImageNPyramid.h>

double flitr::ImageNPyramid::logbase(double a, double base)
{
    return log(a) / log(base);
}


flitr::ImageNPyramid::ImageNPyramid(const osg::TextureRectangle *i_pInputTexture,
                                    std::vector< std::pair<int,int> > &i_ROIVec,
                                    unsigned long i_ulROIWidth, unsigned long i_ulROIHeight,
                                    bool i_bIndicateROI, bool i_bUseGPU, bool i_bReadOutputBackToCPU) :
    m_pInputTexture(i_pInputTexture),
    m_ulNumLevels(0),
    m_imageGausPyramid(0), m_textureGausPyramid(0),
    m_imageGausXPyramid(0), m_textureGausXPyramid(0),
    m_derivImagePyramid(0), m_derivTexturePyramid(0),
    m_gausDistSqImage(0), m_gausDistSqTexture(0),
    m_gausDistSqH2LImage(0), m_gausDistSqH2LTexture(0),
    m_ulROIWidth(i_ulROIWidth), m_ulROIHeight(i_ulROIHeight),
    m_bIndicateROI(i_bIndicateROI),
    m_bUseGPU(i_bUseGPU),
    m_bReadOutputBackToCPU(i_bReadOutputBackToCPU),
    m_rpRebuildSwitch(0)
{
    N_=i_ROIVec.size();
    m_ROIVec=i_ROIVec;

    unsigned long numLevelsX=logbase(m_ulROIWidth, 2.0);
    unsigned long numLevelsY=logbase(m_ulROIHeight, 2.0);
    unsigned long numResolutionLevels=std::min<unsigned long>(numLevelsX, numLevelsY);

    //Note: The lowest level's resolution depends on largest motion detectable on the spatial frequencies in the image.
    //      Maybe the gaussian sd can be dynamic OR the lowest res is automatically chosen based on the spatial frequencies present.
    //Note!!: Don't make the subtracted levels smaller than 3. A LUCAS_KANADE_BORDER pixel border is used to improve the robustness of LK.
    m_ulNumLevels=numResolutionLevels-3;//Lowest resolution is 16x16. Smaller low resolution seems to be less stable.


    printf("ImagePyramid::() : xDim=%d(%d) yDim=%d(%d) numLevels=%d\n",
           (int)(pow(2.0, (double)numLevelsX)+0.5),
           (int)m_ulROIWidth,
           (int)(pow(2.0, (double)numLevelsY)+0.5), (int)
           m_ulROIHeight, (int)m_ulNumLevels);

    m_imageGausPyramid=new osg::ref_ptr<osg::Image>*[N_];
    m_textureGausPyramid=new osg::ref_ptr<osg::TextureRectangle>*[N_];
    m_imageGausXPyramid=new osg::ref_ptr<osg::Image>*[N_];
    m_textureGausXPyramid=new osg::ref_ptr<osg::TextureRectangle>*[N_];
    m_derivImagePyramid=new osg::ref_ptr<osg::Image>*[N_];
    m_derivTexturePyramid=new osg::ref_ptr<osg::TextureRectangle>*[N_];

    for (int i=0; i<N_; i++)
    {
        m_imageGausPyramid[i]=new osg::ref_ptr<osg::Image>[m_ulNumLevels];
        m_textureGausPyramid[i]=new osg::ref_ptr<osg::TextureRectangle>[m_ulNumLevels];

        m_imageGausXPyramid[i]=new osg::ref_ptr<osg::Image>[m_ulNumLevels];
        m_textureGausXPyramid[i]=new osg::ref_ptr<osg::TextureRectangle>[m_ulNumLevels];

        m_derivImagePyramid[i]=new osg::ref_ptr<osg::Image>[m_ulNumLevels];
        m_derivTexturePyramid[i]=new osg::ref_ptr<osg::TextureRectangle>[m_ulNumLevels];
    }

    m_imagePyramidWidth_=new int[m_ulNumLevels];
    m_imagePyramidHeight_=new int[m_ulNumLevels];

    for (int i=0; i<N_; i++)
    {
        //xPow2 and yPow2 are modified within the level for-loop!
        unsigned long xPow2=pow(2.0, (double)numLevelsX)+0.5;
        unsigned long yPow2=pow(2.0, (double)numLevelsY)+0.5;

        unsigned long level=0;
        for (level=0; level<m_ulNumLevels; level++)
        {
            //=== Contains gaussian filtered images ===
            m_imageGausPyramid[i][level]=new osg::Image;
            m_imageGausPyramid[i][level]->allocateImage(xPow2, yPow2, 1, GL_RED, GL_FLOAT);
            m_imageGausPyramid[i][level]->setInternalTextureFormat(GL_FLOAT_R_NV);
            m_imageGausPyramid[i][level]->setPixelFormat(GL_RED);
            m_imageGausPyramid[i][level]->setDataType(GL_FLOAT);
            //===
            m_textureGausPyramid[i][level]=new osg::TextureRectangle();
            m_textureGausPyramid[i][level]->setTextureSize(xPow2, yPow2);
            m_textureGausPyramid[i][level]->setFilter(osg::TextureRectangle::MIN_FILTER,osg::TextureRectangle::NEAREST);
            m_textureGausPyramid[i][level]->setFilter(osg::TextureRectangle::MAG_FILTER,osg::TextureRectangle::NEAREST);
            m_textureGausPyramid[i][level]->setInternalFormat(GL_FLOAT_R_NV);
            m_textureGausPyramid[i][level]->setSourceFormat(GL_RED);
            m_textureGausPyramid[i][level]->setSourceType(GL_FLOAT);
            m_textureGausPyramid[i][level]->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
            m_textureGausPyramid[i][level]->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
            if (!m_bUseGPU)
            {
                m_textureGausPyramid[i][level]->setImage(m_imageGausPyramid[i][level].get());//No need to attach the image to the texture if the results are not modified by the CPU and then updated to the GPU!!!
            }
            //=========================================



            //=== Contains gaussian filtered-in-x images. First step of seperable gaussian. ===
            m_imageGausXPyramid[i][level]=new osg::Image;
            m_imageGausXPyramid[i][level]->allocateImage(xPow2, (level==0) ? yPow2 : (yPow2*2), 1, GL_RED, GL_FLOAT);
            m_imageGausXPyramid[i][level]->setInternalTextureFormat(GL_FLOAT_R_NV);
            m_imageGausXPyramid[i][level]->setPixelFormat(GL_RED);
            m_imageGausXPyramid[i][level]->setDataType(GL_FLOAT);
            //===
            m_textureGausXPyramid[i][level]=new osg::TextureRectangle();
            m_textureGausXPyramid[i][level]->setTextureSize(xPow2, (level==0) ? yPow2 : (yPow2*2));
            m_textureGausXPyramid[i][level]->setFilter(osg::TextureRectangle::MIN_FILTER,osg::TextureRectangle::NEAREST);
            m_textureGausXPyramid[i][level]->setFilter(osg::TextureRectangle::MAG_FILTER,osg::TextureRectangle::NEAREST);
            m_textureGausXPyramid[i][level]->setInternalFormat(GL_FLOAT_R_NV);
            m_textureGausXPyramid[i][level]->setSourceFormat(GL_RED);
            m_textureGausXPyramid[i][level]->setSourceType(GL_FLOAT);
            m_textureGausXPyramid[i][level]->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
            m_textureGausXPyramid[i][level]->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
            if (!m_bUseGPU)
            {
                m_textureGausXPyramid[i][level]->setImage(m_imageGausXPyramid[i][level].get());//No need to attach the image to the texture if the results are not modified by the CPU and then updated to the GPU!!!
            }
            //=================================================================================



            //===
            m_derivImagePyramid[i][level]=new osg::Image;
            m_derivImagePyramid[i][level]->allocateImage(xPow2, yPow2, 1, GL_RGBA, GL_FLOAT);
            m_derivImagePyramid[i][level]->setInternalTextureFormat(GL_RGBA32F_ARB);
            m_derivImagePyramid[i][level]->setPixelFormat(GL_RGBA);
            m_derivImagePyramid[i][level]->setDataType(GL_FLOAT);
            //===
            m_derivTexturePyramid[i][level]=new osg::TextureRectangle();
            m_derivTexturePyramid[i][level]->setTextureSize(xPow2, yPow2);
            m_derivTexturePyramid[i][level]->setFilter(osg::TextureRectangle::MIN_FILTER,osg::TextureRectangle::NEAREST);
            m_derivTexturePyramid[i][level]->setFilter(osg::TextureRectangle::MAG_FILTER,osg::TextureRectangle::NEAREST);
            m_derivTexturePyramid[i][level]->setInternalFormat(GL_RGBA32F_ARB);
            m_derivTexturePyramid[i][level]->setSourceFormat(GL_RGBA);
            m_derivTexturePyramid[i][level]->setSourceType(GL_FLOAT);
            m_derivTexturePyramid[i][level]->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
            m_derivTexturePyramid[i][level]->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
            if (!m_bUseGPU)
            {
                m_derivTexturePyramid[i][level]->setImage(m_derivImagePyramid[i][level].get());//No need to attach the image to the texture if the results are not modified by the CPU and then updated to the GPU!!!
            }
            //===



            m_imagePyramidWidth_[level]=xPow2;
            m_imagePyramidHeight_[level]=yPow2;

            xPow2/=2;
            yPow2/=2;
        }
    }


    //===== Gaus Dist Squared ====================
    {
        m_gausDistSqImage=new osg::Image;
        m_gausDistSqImage->allocateImage(6, 1, 1, GL_RGBA, GL_FLOAT);//{0.22, 0.20, 0.10, 0.07, 0.04, 0.0}
        m_gausDistSqImage->setInternalTextureFormat(GL_RGBA32F_ARB);
        m_gausDistSqImage->setPixelFormat(GL_RGBA);
        m_gausDistSqImage->setDataType(GL_FLOAT);
        float *data=((float *)(m_gausDistSqImage->data()));
        data[0]=0.22;  data[1]=0.22;  data[2]=0.22;  data[3]=0.22;
        data[4]=0.20;  data[5]=0.20;  data[6]=0.20;  data[7]=0.20;
        data[8]=0.10;  data[9]=0.10;  data[10]=0.10; data[11]=0.10;
        data[12]=0.07; data[13]=0.07; data[14]=0.07; data[15]=0.07;
        data[16]=0.04; data[17]=0.04; data[18]=0.04; data[19]=0.04;
        data[20]=0.00; data[21]=0.00; data[22]=0.00; data[23]=0.00;

        m_gausDistSqTexture=new osg::TextureRectangle();
        m_gausDistSqTexture->setTextureSize(6, 1);
        m_gausDistSqTexture->setFilter(osg::TextureRectangle::MIN_FILTER,osg::TextureRectangle::NEAREST);
        m_gausDistSqTexture->setFilter(osg::TextureRectangle::MAG_FILTER,osg::TextureRectangle::NEAREST);
        m_gausDistSqTexture->setInternalFormat(GL_RGBA32F_ARB);
        m_gausDistSqTexture->setSourceFormat(GL_RGBA);
        m_gausDistSqTexture->setSourceType(GL_FLOAT);
        m_gausDistSqTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        m_gausDistSqTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        m_gausDistSqTexture->setImage(m_gausDistSqImage.get());
        m_gausDistSqImage->dirty();
    }
    //============================================
    //===== Gaus Dist Squared H2L ================
    {
        m_gausDistSqH2LImage=new osg::Image;
        m_gausDistSqH2LImage->allocateImage(8, 1, 1, GL_RGBA, GL_FLOAT);//{0.22, 0.20, 0.10, 0.07, 0.04, 0.02, 0.01, 0.00}
        m_gausDistSqH2LImage->setInternalTextureFormat(GL_RGBA32F_ARB);
        m_gausDistSqH2LImage->setPixelFormat(GL_RGBA);
        m_gausDistSqH2LImage->setDataType(GL_FLOAT);
        float *data=((float *)(m_gausDistSqH2LImage->data()));
        data[0]=0.22;  data[1]=0.22;  data[2]=0.22;  data[3]=0.22;
        data[4]=0.20;  data[5]=0.20;  data[6]=0.20;  data[7]=0.20;
        data[8]=0.10;  data[9]=0.10;  data[10]=0.10; data[11]=0.10;
        data[12]=0.07; data[13]=0.07; data[14]=0.07; data[15]=0.07;
        data[16]=0.04; data[17]=0.04; data[18]=0.04; data[19]=0.04;
        data[20]=0.02; data[21]=0.02; data[22]=0.02; data[23]=0.02;
        data[24]=0.01; data[25]=0.01; data[26]=0.01; data[27]=0.01;
        data[28]=0.00; data[29]=0.00; data[30]=0.00; data[31]=0.00;

        m_gausDistSqH2LTexture=new osg::TextureRectangle();
        m_gausDistSqH2LTexture->setTextureSize(8, 1);
        m_gausDistSqH2LTexture->setFilter(osg::TextureRectangle::MIN_FILTER,osg::TextureRectangle::NEAREST);
        m_gausDistSqH2LTexture->setFilter(osg::TextureRectangle::MAG_FILTER,osg::TextureRectangle::NEAREST);
        m_gausDistSqH2LTexture->setInternalFormat(GL_RGBA32F_ARB);
        m_gausDistSqH2LTexture->setSourceFormat(GL_RGBA);
        m_gausDistSqH2LTexture->setSourceType(GL_FLOAT);
        m_gausDistSqH2LTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        m_gausDistSqH2LTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        m_gausDistSqH2LTexture->setImage(m_gausDistSqH2LImage.get());
        m_gausDistSqH2LImage->dirty();
    }
    //============================================
}



osg::ref_ptr<osg::Geode> flitr::ImageNPyramid::createScreenAlignedQuad(unsigned long i_ulWidth, unsigned long i_ulHeight, double i_dBorderPixels=0)
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

osg::Camera *flitr::ImageNPyramid::createScreenAlignedCamera(unsigned long i_ulWidth, unsigned long i_ulHeight, double i_dBorderPixels=0.0)
{
    //g_ulNumCameras++;
    osg::Camera *theCamera = new osg::Camera;
    theCamera->setClearMask((i_dBorderPixels>0.0) ? GL_COLOR_BUFFER_BIT : 0);
    theCamera->setClearColor(osg::Vec4(0.0, 0.0, 0.0, 0.0));
    //theCamera->setProjectionMatrix(osg::Matrix::ortho2D(0, i_ulWidth, 0, i_ulHeight));
    theCamera->setProjectionMatrixAsOrtho(0, i_ulWidth, 0, i_ulHeight,-100,100);
    theCamera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
    theCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    theCamera->setViewMatrix(osg::Matrix::identity());
    theCamera->setViewport(0, 0, i_ulWidth, i_ulHeight);
    theCamera->setRenderOrder(osg::Camera::PRE_RENDER);
    theCamera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);

    return theCamera;
}

//TODO!!!: Still some errors at boundaries of pyramid. Either in deriv or gaus filt calculations!!! 
//         Currently solved by ignoring border of LUCAS_KANADE_BORDER pixels when calculating weighted h-vector from per-pixel h-vectors!
osg::Node* flitr::ImageNPyramid::createDerivLevel(unsigned long i_ulLevel, unsigned long i_ulWidth, unsigned long i_ulHeight, bool i_bDoPostPyramidRebuiltCallback)
{
    osg::ref_ptr<osg::Geode> geode = createScreenAlignedQuad(i_ulWidth, i_ulHeight);
    osg::ref_ptr<osg::StateSet> geomss = geode->getOrCreateStateSet();


    for (int i=0; i<N_; i++)
    {
        //************************************
        //set up state set with input textures
        //************************************
        geomss->setTextureAttributeAndModes(i, this->m_textureGausPyramid[i][i_ulLevel].get(), osg::StateAttribute::ON);
        geomss->setTextureAttribute(i, new osg::TexEnv(osg::TexEnv::DECAL));

        //****************
        //add the uniforms
        //****************
        char uniformName[256];
        sprintf(uniformName, "imageTexture_%d" ,i);
        geomss->addUniform(new osg::Uniform(uniformName, i));
    }


    //*******************************************
    //create and add the shaders to the state set
    //*******************************************
    osg::ref_ptr<osg::Program> textureShader = new osg::Program;
    textureShader->setName("createDerivLevel");
    textureShader->addShader(new osg::Shader(osg::Shader::FRAGMENT,
                                             "uniform sampler2DRect imageTexture_0;\n"

                                             "void main(void)"
                                             "{\n"
                                             "  float centrePixel_0=texture2DRect(imageTexture_0, gl_TexCoord[0].xy).r;\n"
                                             "  float leftPixel_0=texture2DRect(imageTexture_0, gl_TexCoord[0].xy-vec2(1.0, 0.0)).r;\n"
                                             "  float leftDeriv_0=(centrePixel_0-leftPixel_0);\n"

                                             "  float rightPixel_0=texture2DRect(imageTexture_0, gl_TexCoord[0].xy+vec2(1.0, 0.0)).r;\n"
                                             "  float rightDeriv_0=(rightPixel_0-centrePixel_0);\n"
                                             "  float dX_0=(leftDeriv_0+rightDeriv_0)*0.5;\n"

                                             "  float topPixel_0=texture2DRect(imageTexture_0, gl_TexCoord[0].xy-vec2(0.0, 1.0)).r;\n"
                                             "  float topDeriv_0=(centrePixel_0-topPixel_0);\n"

                                             "  float bottomPixel_0=texture2DRect(imageTexture_0, gl_TexCoord[0].xy+vec2(0.0, 1.0)).r;\n"
                                             "  float bottomDeriv_0=(bottomPixel_0-centrePixel_0);\n"
                                             "  float dY_0=(topDeriv_0+bottomDeriv_0)*0.5;\n"


                                             "  gl_FragData[0].r=dX_0;\n"
                                             "  gl_FragData[0].g=dY_0;\n"
                                             "  gl_FragData[0].b=dX_0*dX_0+dY_0*dY_0;\n"
                                             "  gl_FragData[0].a=1.0;\n"

                                             "}\n"
                                             ));
    geomss->setAttributeAndModes(textureShader.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE );

    osg::Camera *theCamera=createScreenAlignedCamera(i_ulWidth, i_ulHeight);
    theCamera->addChild(geode.get());


    for (int i=0; i<N_; i++)
    {
        theCamera->attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER0+i), this->m_derivTexturePyramid[i][i_ulLevel].get());

        if (m_bReadOutputBackToCPU)
            theCamera->attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER0+i), this->m_derivImagePyramid[i][i_ulLevel].get());
    }


    if (i_bDoPostPyramidRebuiltCallback)
    {
        theCamera->setPostDrawCallback(&m_pyramidRebuiltCallback);
    }

    return theCamera;
}   


osg::Node* flitr::ImageNPyramid::createGausFiltLevel0(unsigned long i_ulWidth, unsigned long i_ulHeight, bool i_bDoPostPyramidRebuiltCallback)
{
    osg::ref_ptr<osg::Geode> geode = createScreenAlignedQuad(i_ulWidth, i_ulHeight);
    osg::ref_ptr<osg::StateSet> geomss = geode->getOrCreateStateSet();

    geomss->setTextureAttributeAndModes(0, (osg::TextureRectangle *)m_pInputTexture, osg::StateAttribute::ON);
    geomss->setTextureAttribute(0, new osg::TexEnv(osg::TexEnv::DECAL));

    geomss->setTextureAttributeAndModes(1, m_gausDistSqTexture.get(), osg::StateAttribute::ON);
    geomss->setTextureAttribute(1, new osg::TexEnv(osg::TexEnv::DECAL));

    //****************
    //add the uniforms
    //****************
    geomss->addUniform(new osg::Uniform("inputTexture", 0));
    geomss->addUniform(new osg::Uniform("gausDistSqTexture", 1));

    for (int i=0; i<N_; i++)
    {
        char uniformName[256];
        sprintf(uniformName, "ROI_%d" ,i);
        geomss->addUniform(new osg::Uniform(uniformName, osg::Vec2f(this->m_ROIVec[i].first, this->m_ROIVec[i].second) ));
    }

    //*******************************************
    //create and add the shaders to the state set
    //*******************************************
    osg::ref_ptr<osg::Program> textureShader = new osg::Program;
    textureShader->setName("createGausFiltLevel0");
    textureShader->addShader(new osg::Shader(osg::Shader::FRAGMENT,
                                             "uniform sampler2DRect inputTexture;\n"
                                             "uniform sampler2DRect gausDistSqTexture;\n"

                                             "uniform vec2 ROI_0;\n"

                                             "void main(void)"
                                             "{\n"
                                             "  float gSum_0=0.0;\n"

                                             "  float gSumReference=0.0;\n"

                                             "  for (float y=-2.0; y<=2.0; y+=1.0)\n"
                                             "  {\n"
                                             "    float ySq=y*y;\n"
                                             "    for (float x=-2.0; x<=2.0; x+=1.0)\n"
                                             "    {\n"
                                             "      float radiusSq=x*x+ySq;\n"
                                             "      float gausDistSqIndex=radiusSq;\n"
                                             "      float gausDistSqVal=texture2DRect(gausDistSqTexture, vec2(gausDistSqIndex, 0.5)).r;\n"

                                             "      gSum_0+=texture2DRect(inputTexture, gl_TexCoord[0].xy+vec2(x,y)+ROI_0).r*255.0*gausDistSqVal;\n"

                                             "      gSumReference+=gausDistSqVal;\n"
                                             "    }\n"
                                             "  }\n"

                                             "  gl_FragData[0].r=gSum_0/gSumReference;\n"
                                             "}\n"
                                             ));
    geomss->setAttributeAndModes(textureShader.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE );

    osg::Camera *theCamera=createScreenAlignedCamera(i_ulWidth, i_ulHeight);
    theCamera->addChild(geode.get());


    for (int i=0; i<N_; i++)
    {
        theCamera->attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER0+i), this->m_textureGausPyramid[i][0].get());

        if (m_bReadOutputBackToCPU)
            theCamera->attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER0+i), this->m_imageGausPyramid[i][0].get());
    }


    if (i_bDoPostPyramidRebuiltCallback)
    {
        theCamera->setPostDrawCallback(&m_pyramidRebuiltCallback);
    }

    return theCamera;
}   

osg::Node* flitr::ImageNPyramid::createGausXFiltLevel0(unsigned long i_ulWidth, unsigned long i_ulHeight, bool i_bDoPostPyramidRebuiltCallback)
{
    osg::ref_ptr<osg::Geode> geode = createScreenAlignedQuad(i_ulWidth, i_ulHeight);
    osg::ref_ptr<osg::StateSet> geomss = geode->getOrCreateStateSet();

    geomss->setTextureAttributeAndModes(0, (osg::TextureRectangle *)m_pInputTexture, osg::StateAttribute::ON);
    geomss->setTextureAttribute(0, new osg::TexEnv(osg::TexEnv::DECAL));

    geomss->setTextureAttributeAndModes(1, m_gausDistSqTexture.get(), osg::StateAttribute::ON);
    geomss->setTextureAttribute(1, new osg::TexEnv(osg::TexEnv::DECAL));

    //****************
    //add the uniforms
    //****************
    geomss->addUniform(new osg::Uniform("inputTexture", 0));
    geomss->addUniform(new osg::Uniform("gausDistSqTexture", 1));

    for (int i=0; i<N_; i++)
    {
        char uniformName[256];
        sprintf(uniformName, "ROI_%d" ,i);
        geomss->addUniform(new osg::Uniform(uniformName, osg::Vec2f(this->m_ROIVec[i].first, this->m_ROIVec[i].second) ));
    }

    //*******************************************
    //create and add the shaders to the state set
    //*******************************************
    osg::ref_ptr<osg::Program> textureShader = new osg::Program;
    textureShader->setName("createGausXFiltLevel0");
    textureShader->addShader(new osg::Shader(osg::Shader::FRAGMENT,
                                             "uniform sampler2DRect inputTexture;\n"
                                             "uniform sampler2DRect gausDistSqTexture;\n"

                                             "uniform vec2 ROI_0;\n"

                                             "void main(void)"
                                             "{\n"
                                             "  float gSum_0=0.0;\n"
                                             "  float gSumReference=0.0;\n"

                                             "    for (float x=-2.0; x<=2.0; x+=1.0)\n"
                                             "    {\n"
                                             "      float radiusSq=x*x;\n"
                                             "      float gausDistSqIndex=radiusSq;\n"
                                             "      float gausDistSqVal=texture2DRect(gausDistSqTexture, vec2(gausDistSqIndex, 0.5)).r;\n"

                                             "      gSum_0+=texture2DRect(inputTexture, gl_TexCoord[0].xy+vec2(x,0)+ROI_0).r*255.0*gausDistSqVal;\n"

                                             "      gSumReference+=gausDistSqVal;\n"
                                             "    }\n"

                                             "  gl_FragData[0].r=gSum_0/gSumReference;\n"
                                             "}\n"
                                             ));
    geomss->setAttributeAndModes(textureShader.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE );

    osg::Camera *theCamera=createScreenAlignedCamera(i_ulWidth, i_ulHeight);
    theCamera->addChild(geode.get());


    for (int i=0; i<N_; i++)
    {
        theCamera->attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER0+i), this->m_textureGausXPyramid[i][0].get());

        if (m_bReadOutputBackToCPU)
            theCamera->attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER0+i), this->m_imageGausXPyramid[i][0].get());
    }


    if (i_bDoPostPyramidRebuiltCallback)
    {
        theCamera->setPostDrawCallback(&m_pyramidRebuiltCallback);
    }

    return theCamera;
}   

osg::Node* flitr::ImageNPyramid::createGausYFiltLevel0(unsigned long i_ulWidth, unsigned long i_ulHeight, bool i_bDoPostPyramidRebuiltCallback)
{
    osg::ref_ptr<osg::Geode> geode = createScreenAlignedQuad(i_ulWidth, i_ulHeight);
    osg::ref_ptr<osg::StateSet> geomss = geode->getOrCreateStateSet();

    for (int i=0; i<N_; i++)
    {
        geomss->setTextureAttributeAndModes(i, (osg::TextureRectangle *)m_textureGausXPyramid[i][0].get(), osg::StateAttribute::ON);
        geomss->setTextureAttribute(i, new osg::TexEnv(osg::TexEnv::DECAL));

        char uniformName[256];
        sprintf(uniformName, "inputTexture_%d" ,i);
        geomss->addUniform(new osg::Uniform(uniformName, i));
    }


    geomss->setTextureAttributeAndModes(N_, m_gausDistSqTexture.get(), osg::StateAttribute::ON);
    geomss->setTextureAttribute(N_, new osg::TexEnv(osg::TexEnv::DECAL));
    geomss->addUniform(new osg::Uniform("gausDistSqTexture", (int)N_));


    //*******************************************
    //create and add the shaders to the state set
    //*******************************************
    osg::ref_ptr<osg::Program> textureShader = new osg::Program;
    textureShader->setName("createGausYFiltLevel0");
    textureShader->addShader(new osg::Shader(osg::Shader::FRAGMENT,
                                             "uniform sampler2DRect inputTexture_0;\n"

                                             "uniform sampler2DRect gausDistSqTexture;\n"

                                             "void main(void)"
                                             "{\n"
                                             "  float gSum_0=0.0;\n"
                                             "  float gSumReference=0.0;\n"

                                             "    for (float y=-2.0; y<=2.0; y+=1.0)\n"
                                             "    {\n"
                                             "      float radiusSq=y*y;\n"
                                             "      float gausDistSqIndex=radiusSq;\n"
                                             "      float gausDistSqVal=texture2DRect(gausDistSqTexture, vec2(gausDistSqIndex, 0.5)).r;\n"

                                             "      gSum_0+=texture2DRect(inputTexture_0, gl_TexCoord[0].xy+vec2(0,y)).r*gausDistSqVal;\n"

                                             "      gSumReference+=gausDistSqVal;\n"
                                             "    }\n"

                                             "  gl_FragData[0].r=gSum_0/gSumReference;\n"
                                             "}\n"
                                             ));
    geomss->setAttributeAndModes(textureShader.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE );

    osg::Camera *theCamera=createScreenAlignedCamera(i_ulWidth, i_ulHeight);
    theCamera->addChild(geode.get());


    for (int i=0; i<N_; i++)
    {
        theCamera->attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER0+i), this->m_textureGausPyramid[i][0].get());

        if (m_bReadOutputBackToCPU)
            theCamera->attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER0+i), this->m_imageGausPyramid[i][0].get());
    }



    if (i_bDoPostPyramidRebuiltCallback)
    {
        theCamera->setPostDrawCallback(&m_pyramidRebuiltCallback);
    }

    return theCamera;
}   


osg::Node* flitr::ImageNPyramid::createGausFiltLevel1toN(unsigned long i_ulLevel, unsigned long i_ulWidth, unsigned long i_ulHeight, bool i_bDoPostPyramidRebuiltCallback)
{
    osg::ref_ptr<osg::Geode> geode = createScreenAlignedQuad(i_ulWidth, i_ulHeight);
    osg::ref_ptr<osg::StateSet> geomss = geode->getOrCreateStateSet();

    for (int i=0; i<N_; i++)
    {
        geomss->setTextureAttributeAndModes(i, this->m_textureGausPyramid[i][i_ulLevel-1].get(), osg::StateAttribute::ON);
        //	geomss->setTextureAttribute(i, new osg::TexEnv(osg::TexEnv::DECAL));

        char uniformName[256];
        sprintf(uniformName, "inputTexture_%d" ,i);

        geomss->addUniform(new osg::Uniform(uniformName, i));
    }

    geomss->setTextureAttributeAndModes(N_, m_gausDistSqH2LTexture.get(), osg::StateAttribute::ON);
    //	geomss->setTextureAttribute(N_, new osg::TexEnv(osg::TexEnv::DECAL));

    geomss->addUniform(new osg::Uniform("gausDistSqH2LTexture", (int)N_));

    //*******************************************
    //create and add the shaders to the state set
    //*******************************************
    osg::ref_ptr<osg::Program> textureShader = new osg::Program;
    textureShader->setName("createGausFiltLevel1toN");
    textureShader->addShader(new osg::Shader(osg::Shader::FRAGMENT,
                                             "uniform sampler2DRect inputTexture_0;\n"

                                             "uniform sampler2DRect gausDistSqH2LTexture;\n"
                                             "void main(void)"
                                             "{\n"
                                             "  float gSum_0=0.0;\n"
                                             "  float gSumReference=0.0;\n"

                                             "  for (float y=-2.0; y<=3.0; y+=1.0)\n"
                                             "  {\n"
                                             "    float ySq=(y-0.5)*(y-0.5);\n"
                                             "    for (float x=-2.0; x<=3.0; x+=1.0)\n"
                                             "    {\n"
                                             "      float radiusSq=(x-0.5)*(x-0.5)+ySq;\n"
                                             "      float gausDistSqH2LIndex=radiusSq;\n"
                                             "      float gausDistSqH2LVal=texture2DRect(gausDistSqH2LTexture, vec2(gausDistSqH2LIndex, 0.5)).r;\n"

                                             "      gSum_0+=texture2DRect(inputTexture_0, (gl_TexCoord[0].xy-vec2(0.25, 0.25))*2.0+vec2(x,y)).r*gausDistSqH2LVal;\n"//Texture coords are: 0.5, 1.5, 2.5, etc. -0.25 is to assure that texCoord*2.0 is within correct pixel.
                                             "      gSumReference+=gausDistSqH2LVal;\n"
                                             "    }\n"
                                             "  }\n"

                                             "  gl_FragData[0].r=gSum_0/gSumReference;\n"
                                             "}\n"
                                             ));

    geomss->setAttributeAndModes(textureShader.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE );

    osg::Camera *theCamera=createScreenAlignedCamera(i_ulWidth, i_ulHeight);
    theCamera->addChild(geode.get());



    for (int i=0; i<N_; i++)
    {
        theCamera->attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER0+i), this->m_textureGausPyramid[i][i_ulLevel].get());

        if (m_bReadOutputBackToCPU)
            theCamera->attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER0+i), this->m_imageGausPyramid[i][i_ulLevel].get());
    }


    if (i_bDoPostPyramidRebuiltCallback)
    {
        theCamera->setPostDrawCallback(&m_pyramidRebuiltCallback);
    }

    return theCamera;
}   

osg::Node* flitr::ImageNPyramid::createGausXFiltLevel1toN(unsigned long i_ulLevel, unsigned long i_ulWidth, unsigned long i_ulHeight, bool i_bDoPostPyramidRebuiltCallback)
{
    osg::ref_ptr<osg::Geode> geode = createScreenAlignedQuad(i_ulWidth, i_ulHeight);
    osg::ref_ptr<osg::StateSet> geomss = geode->getOrCreateStateSet();

    for (int i=0; i<N_; i++)
    {
        geomss->setTextureAttributeAndModes(i, this->m_textureGausPyramid[i][i_ulLevel-1].get(), osg::StateAttribute::ON);
        //	geomss->setTextureAttribute(i, new osg::TexEnv(osg::TexEnv::DECAL));

        char uniformName[256];
        sprintf(uniformName, "inputTexture_%d" ,i);

        geomss->addUniform(new osg::Uniform(uniformName, i));
    }


    geomss->setTextureAttributeAndModes(N_, m_gausDistSqH2LTexture.get(), osg::StateAttribute::ON);
    //	geomss->setTextureAttribute(N_, new osg::TexEnv(osg::TexEnv::DECAL));

    geomss->addUniform(new osg::Uniform("gausDistSqH2LTexture", (int)N_));

    //*******************************************
    //create and add the shaders to the state set
    //*******************************************
    osg::ref_ptr<osg::Program> textureShader = new osg::Program;
    textureShader->setName("createGausXFiltLevel1toN");
    textureShader->addShader(new osg::Shader(osg::Shader::FRAGMENT,
                                             "uniform sampler2DRect inputTexture_0;\n"

                                             "uniform sampler2DRect gausDistSqH2LTexture;\n"
                                             "void main(void)"
                                             "{\n"
                                             "  float gSum_0=0.0;\n"
                                             "  float gSumReference=0.0;\n"

                                             "    for (float x=-2.0; x<=3.0; x+=1.0)\n"
                                             "    {\n"
                                             "      float radiusSq=(x-0.5)*(x-0.5);\n"
                                             "      float gausDistSqH2LIndex=radiusSq;\n"
                                             "      float gausDistSqH2LVal=texture2DRect(gausDistSqH2LTexture, vec2(gausDistSqH2LIndex, 0.5)).r;\n"

                                             "      gSum_0+=texture2DRect(inputTexture_0, (gl_TexCoord[0].xy-vec2(0.25, 0.00))*vec2(2.0, 1.0)+vec2(x,0)).r*gausDistSqH2LVal;\n"//Texture coords are: 0.5, 1.5, 2.5, etc. -0.25 is to assure that texCoord*2.0 is within correct pixel.
                                             "      gSumReference+=gausDistSqH2LVal;\n"
                                             "    }\n"

                                             "  gl_FragData[0].r=gSum_0/gSumReference;\n"
                                             "}\n"
                                             ));
    geomss->setAttributeAndModes(textureShader.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE );

    osg::Camera *theCamera=createScreenAlignedCamera(i_ulWidth, i_ulHeight);
    theCamera->addChild(geode.get());



    for (int i=0; i<N_; i++)
    {
        theCamera->attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER0+i), this->m_textureGausXPyramid[i][i_ulLevel].get());

        if (m_bReadOutputBackToCPU)
            theCamera->attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER0+1), this->m_imageGausXPyramid[i][i_ulLevel].get());
    }
    \

    if (i_bDoPostPyramidRebuiltCallback)
    {
        theCamera->setPostDrawCallback(&m_pyramidRebuiltCallback);
    }

    return theCamera;
}   

osg::Node* flitr::ImageNPyramid::createGausYFiltLevel1toN(unsigned long i_ulLevel, unsigned long i_ulWidth, unsigned long i_ulHeight, bool i_bDoPostPyramidRebuiltCallback)
{
    osg::ref_ptr<osg::Geode> geode = createScreenAlignedQuad(i_ulWidth, i_ulHeight);
    osg::ref_ptr<osg::StateSet> geomss = geode->getOrCreateStateSet();

    for (int i=0; i<N_; i++)
    {
        geomss->setTextureAttributeAndModes(i, this->m_textureGausXPyramid[i][i_ulLevel].get(), osg::StateAttribute::ON);
        //	geomss->setTextureAttribute(i, new osg::TexEnv(osg::TexEnv::DECAL));

        char uniformName[256];
        sprintf(uniformName, "inputTexture_%d" ,i);

        geomss->addUniform(new osg::Uniform(uniformName, i));
    }

    geomss->setTextureAttributeAndModes(N_, m_gausDistSqH2LTexture.get(), osg::StateAttribute::ON);
    //	geomss->setTextureAttribute(N_, new osg::TexEnv(osg::TexEnv::DECAL));

    geomss->addUniform(new osg::Uniform("gausDistSqH2LTexture", (int)N_));

    //*******************************************
    //create and add the shaders to the state set
    //*******************************************
    osg::ref_ptr<osg::Program> textureShader = new osg::Program;
    textureShader->setName("createGausYFiltLevel1toN");
    textureShader->addShader(new osg::Shader(osg::Shader::FRAGMENT,
                                             "uniform sampler2DRect inputTexture_0;\n"

                                             "uniform sampler2DRect gausDistSqH2LTexture;\n"
                                             "void main(void)"
                                             "{\n"
                                             "  float gSum_0=0.0;\n"
                                             "  float gSumReference=0.0;\n"

                                             "    for (float y=-2.0; y<=3.0; y+=1.0)\n"
                                             "    {\n"
                                             "      float radiusSq=(y-0.5)*(y-0.5);\n"
                                             "      float gausDistSqH2LIndex=radiusSq;\n"
                                             "      float gausDistSqH2LVal=texture2DRect(gausDistSqH2LTexture, vec2(gausDistSqH2LIndex, 0.5)).r;\n"

                                             "      gSum_0+=texture2DRect(inputTexture_0, (gl_TexCoord[0].xy-vec2(0.00, 0.25))*vec2(1.0, 2.0)+vec2(0,y)).r*gausDistSqH2LVal;\n"//Texture coords are: 0.5, 1.5, 2.5, etc. -0.25 is to assure that texCoord*2.0 is within correct pixel.
                                             "      gSumReference+=gausDistSqH2LVal;\n"
                                             "    }\n"

                                             "  gl_FragData[0].r=gSum_0/gSumReference;\n"
                                             "}\n"
                                             ));
    geomss->setAttributeAndModes(textureShader.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE );

    osg::Camera *theCamera=createScreenAlignedCamera(i_ulWidth, i_ulHeight);
    theCamera->addChild(geode.get());



    for (int i=0; i<N_; i++)
    {
        theCamera->attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER0+i), this->m_textureGausPyramid[i][i_ulLevel].get());

        if (m_bReadOutputBackToCPU)
            theCamera->attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER0+i), this->m_imageGausPyramid[i][i_ulLevel].get());
    }


    if (i_bDoPostPyramidRebuiltCallback)
    {
        theCamera->setPostDrawCallback(&m_pyramidRebuiltCallback);
    }

    return theCamera;
}   



bool flitr::ImageNPyramid::init(osg::Group *root_node, PostRenderCallback *i_pPostPyramidRebuiltCallback)//Allocate pyramid levels
{
    m_pyramidRebuiltCallback.m_pPostPyramidRebuiltCallback=i_pPostPyramidRebuiltCallback;

    unsigned long numLevelsX=logbase(m_ulROIWidth, 2.0);
    unsigned long numLevelsY=logbase(m_ulROIHeight, 2.0);

    if ((m_bUseGPU)&&(root_node!=0))
    {
        unsigned long width=pow(2.0, (double)numLevelsX)+0.5;
        unsigned long height=pow(2.0, (double)numLevelsY)+0.5;

        m_rpRebuildSwitch=new osg::Switch();

        //			unsigned long level=0;
        for (unsigned long level=0; level<m_ulNumLevels; level++)//NB!!!! - The level/pass that does the postPyramidRebuilt callback must be rendered LAST!!!
        {
#define SEPERATED_XY_GAUSSIAN_CONVOLUTIONS
            //Note:Seperated gaussian results in a 5% performance increase on laptop.
            if (level==0)
            {
#ifndef SEPERATED_XY_GAUSSIAN_CONVOLUTIONS
                m_rpRebuildSwitch->addChild(createGausFiltLevel0(width, height, false));
#else
                m_rpRebuildSwitch->addChild(createGausXFiltLevel0(width, height, false));
                m_rpRebuildSwitch->addChild(createGausYFiltLevel0(width, height, false));
#endif
            } else
            {
#ifndef SEPERATED_XY_GAUSSIAN_CONVOLUTIONS
                m_rpRebuildSwitch->addChild(createGausFiltLevel1toN(level, width>>level, height>>level, false));
#else
                m_rpRebuildSwitch->addChild(createGausXFiltLevel1toN(level, width>>level, height>>(level-1), false));
                m_rpRebuildSwitch->addChild(createGausYFiltLevel1toN(level, width>>level, height>>level, false));
#endif
            }

            //Note: See comment on callback at end of below line!
            m_rpRebuildSwitch->addChild(createDerivLevel(level, width>>level, height>>level, level==(m_ulNumLevels-1) ));//Use this callback to insert CPU step directly after GPU pyramid rebuild.
        }

        m_rpRebuildSwitch->setAllChildrenOff();
        root_node->addChild(m_rpRebuildSwitch.get());
    }

    return true;
}





//Rebuild the image pyramid using the gaussian lookup above.
void flitr::ImageNPyramid::rebuildBiPyramidCPU()
{
    unsigned long numLevels=getNumLevels();

    for (int i=0; i<N_; i++)
    {
        for (unsigned long level=0; level<numLevels; level++)
        {
            unsigned long xDim=m_imagePyramidWidth_[level];
            unsigned long yDim=m_imagePyramidHeight_[level];

            float *iData=(float *)(m_imageGausPyramid[i][level]->data());

            if (level==0)
            {//Special case for generating filtered image from unsigned-char input texture.
                float *tempImage=new float[xDim*yDim];

                {//Convert input image to float image.
                    float *iTempData=tempImage;
                    const unsigned char *textureData=m_pInputTexture->getImage()->data();

                    textureData+=m_ROIVec[i].first+m_pInputTexture->getTextureWidth()*m_ROIVec[i].second;

                    for (unsigned long y=0; y<yDim; y++)
                    {
                        for (unsigned long x=0; x<xDim; x++)
                        {
                            //'unsigned char' to 'float' conversion!
                            *(iTempData)=(float)(*(textureData++));
                            iTempData++;
                        }
                        textureData+=m_pInputTexture->getTextureWidth() - xDim;
                    }
                }

                {//Calculate filtered image from temp float image.
                    unsigned long offset=0;
                    for (unsigned long y=0; y<yDim; y++)
                    {
                        for (unsigned long x=0; x<xDim; x++)
                        {
                            iData[offset]=gaussianLookupSymCPU(tempImage, x, y, xDim, yDim);//The gaussian filter kernel is centred on the 4 pixels that has ((x<<1),(y<<1)) as the top-left centre.

                            offset++;
                        }
                    }
                }

                delete [] tempImage;
            }
            else
            {//More general case of calculating the filtered image from a higher resolution filtered image.
                float *iHighResData=(float *)(m_imageGausPyramid[i][level-1]->data());

                unsigned long offset=0;
                for (unsigned long y=0; y<yDim; y++)
                {
                    for (unsigned long x=0; x<xDim; x++)
                    {
                        iData[offset]=gaussianLookupCPU(iHighResData, (x<<1), (y<<1), (xDim<<1), (yDim<<1));//The gaussian filter kernel is centred on the 4 pixels that has ((x<<1),(y<<1)) as the top-left centre.

                        offset++;
                    }
                }
            }

            m_imageGausPyramid[i][level]->dirty();


            {//Calculate filtered image's derivitives.
                float *dData=(float *)(m_derivImagePyramid[i][level]->data());

                unsigned long offset=0;
                for (unsigned long y=0; y<yDim; y++)
                {
                    for (unsigned long x=0; x<xDim; x++)
                    {
                        //ToDo: Move below if conditions out of for loop for efficiency!!!
                        //=== Intermediate results for for Lucas-Kanade ===
                        //Make sure dX and dY is balanced by taking average around pixel.
                        double dX = ((x!=(xDim-1))&&(x!=0)) ? ((iData[(offset+1)]-iData[offset])+(iData[offset]-iData[(offset-1)]))*0.5 : 0.0;
                        double dY = ((y!=(yDim-1))&&(y!=0)) ? ((iData[(offset+xDim)]-iData[offset])+(iData[offset]-iData[(offset-xDim)]))*0.5 : 0.0;

                        dData[(offset<<2)+0]=dX;
                        dData[(offset<<2)+1]=dY;
                        dData[(offset<<2)+2]=dX*dX+dY*dY;//+2*dX*dY;
                        //=================================================

                        offset++;
                    }
                }
                m_derivImagePyramid[i][level]->dirty();
            }

        }


        /*
    if (m_bIndicateROI)
    {
        unsigned long xDim=m_imagePyramidWidth_[0];
        unsigned long yDim=m_imagePyramidHeight_[0];

        for (unsigned long y=0; y<yDim; y++)
        for (unsigned long x=0; x<xDim; x++)
        {
//			*((unsigned char *)(m_pInputTexture->getImage()->data()+x+m_ulpROIX[i]+m_pInputTexture->getTextureWidth()*(y+m_ulpROIY[i])))*=0.8;
            *((unsigned char *)(m_pInputTexture->getImage()->data()+x+m_ulpROIX[i]+m_pInputTexture->getTextureWidth()*(y+m_ulpROIY[i])))=
//				(*( ( (float *)m_derivImagePyramid[i][0]->data())+(x+xDim*y)*4) )*2+128;
//				(*( ( (float *)m_derivImagePyramid[i][0]->data())+(x+xDim*y)*4+1) )*2+128;
//			    (*( ( (float *)this->m_imageGausPyramid[i][0]->data())+(x+xDim*y)) );
                (*( ( (float *)this->m_imageGausPyramid[i][4]->data())+((x>>4)+(xDim>>4)*(y>>4))) );
        }
    }
*/
    }
}

void flitr::ImageNPyramid::bilinearResample(unsigned long i_ulPyramidNum, unsigned long i_ulLevelNum, osg::Vec2d i_pixelOffset, float *i_dpResampPreviousImg)
{
    const osg::Image *image=m_imageGausPyramid[i_ulPyramidNum][i_ulLevelNum].get();
    float *iData=(float *)image->data();
    float *resampIData=i_dpResampPreviousImg;
    const unsigned long width=image->s();
    const unsigned long height=image->t();

    //=== Calculate offsetXFrac and offsetYFrac for bi-linear filtering ===//
    long offsetX=(long)i_pixelOffset.x();
    long offsetY=(long)i_pixelOffset.y();

    double offsetXFrac=i_pixelOffset.x()-offsetX;
    if (offsetXFrac<0.0)
    {//This ensures that offsetXFrac for bilinear filtering is always positive!
        offsetXFrac+=1.0;
        offsetX--;
    }
    double offsetYFrac=i_pixelOffset.y()-offsetY;
    if (offsetYFrac<0.0)
    {//This ensures that offsetYFrac for bilinear filtering is always positive!
        offsetYFrac+=1.0;
        offsetY--;
    }
    //=====================================================================//

    iData+=width;
    resampIData+=width;
    for (unsigned long y=1; y<(height-1); y++)
    {
        iData++;
        resampIData++;
        for (unsigned long x=1; x<(width-1); x++)
        {
            if ( ((( ((long)y)+offsetY)>=0)&&(( ((long)y)+offsetY)<((long)height-1))) && (((((long)x)+offsetX)>=0)&&((((long)x)+offsetX)<((long)width-1))) )
            {
                float dA=iData[((0+offsetX)+(0+offsetY)*width)];
                float dB=iData[((1+offsetX)+(0+offsetY)*width)];
                float dC=iData[((0+offsetX)+(1+offsetY)*width)];
                float dD=iData[((1+offsetX)+(1+offsetY)*width)];

                *resampIData=(dA*(1.0-offsetXFrac)+dB*offsetXFrac)*(1.0-offsetYFrac)+
                        (dC*(1.0-offsetXFrac)+dD*offsetXFrac)*offsetYFrac;
            } else
            {
                *resampIData=0;
            }

            iData++;
            resampIData++;
        }
        iData++;
        resampIData++;
    }
}
