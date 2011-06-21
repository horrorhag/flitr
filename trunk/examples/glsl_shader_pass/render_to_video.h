#ifndef RENDER_TO_VIDEO_H
#define RENDER_TO_VIDEO_H

#include <osg/Camera>
#include <flitr/ffmpeg_writer.h>

class RenderToVideo {
    
    struct SaveCameraPostDrawCallback : public osg::Camera::DrawCallback
    {
        SaveCameraPostDrawCallback(osg::Image* image, flitr::FFmpegWriter* ffw) :
            Image_(image),
            FFW_(ffw)
        {
        }
        
        virtual void operator () (const osg::Camera& /*camera*/) const
        {
            FFW_->writeVideoFrame(Image_->data());
        }
    
        osg::Image* Image_;
        flitr::FFmpegWriter* FFW_;
    };

  public:
    RenderToVideo(osg::TextureRectangle* tex, std::string filename)
    {
        init(tex, false, filename);
    }

    RenderToVideo(osg::Texture2D* tex, std::string filename)
    {
        init(tex, true, filename);
    }

    ~RenderToVideo() {}

    void init(osg::Texture* tex, bool is2D, std::string filename)
    {
        TotalWidth_ = tex->getTextureWidth();
        TotalHeight_ = tex->getTextureHeight();
        OutputFormat_ = flitr::ImageFormat(TotalWidth_, TotalHeight_, flitr::ImageFormat::FLITR_PIX_FMT_RGB_8);

        FFW_ = std::tr1::shared_ptr<flitr::FFmpegWriter>(new flitr::FFmpegWriter(filename, OutputFormat_));

        RootGroup_ = new osg::Group;
        osg::ref_ptr<osg::Group> geode_group = new osg::Group;
        
        // create quad to display image on
        osg::ref_ptr<osg::Geode> geode = new osg::Geode();
        
        // each geom will contain a quad
        osg::ref_ptr<osg::DrawArrays> da = new osg::DrawArrays(osg::PrimitiveSet::QUADS,0,4);
        
        osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
        colors->push_back(osg::Vec4(1.0f,1.0f,1.0f,1.0f));
        
        osg::ref_ptr<osg::Vec2Array> tcoords = new osg::Vec2Array; // texture coords
        if (is2D) {
            tcoords->push_back(osg::Vec2(0, 1));
            tcoords->push_back(osg::Vec2(1, 1));
            tcoords->push_back(osg::Vec2(1, 0));
            tcoords->push_back(osg::Vec2(0, 0));
        } else {
            tcoords->push_back(osg::Vec2(0, TotalHeight_));
            tcoords->push_back(osg::Vec2(TotalWidth_, TotalHeight_));
            tcoords->push_back(osg::Vec2(TotalWidth_, 0));
            tcoords->push_back(osg::Vec2(0, 0));
        }

        osg::ref_ptr<osg::Vec3Array> vcoords = new osg::Vec3Array; // vertex coords
        osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
        
        vcoords->push_back(osg::Vec3d(0, 0, -1));
        vcoords->push_back(osg::Vec3d(TotalWidth_, 0, -1));
        vcoords->push_back(osg::Vec3d(TotalWidth_, TotalHeight_, -1));
        vcoords->push_back(osg::Vec3d(0, TotalHeight_, -1));
    
        geom->setVertexArray(vcoords.get());
        geom->setTexCoordArray(0, tcoords.get());
        geom->addPrimitiveSet(da.get());
        geom->setColorArray(colors.get());
        geom->setColorBinding(osg::Geometry::BIND_OVERALL);
        osg::ref_ptr<osg::StateSet> geomss = geom->getOrCreateStateSet();
        geomss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
        geomss->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
        geomss->setTextureAttributeAndModes(0, 
                                            tex,
                                            osg::StateAttribute::ON);
        geode->addDrawable(geom.get());
        geode_group->addChild(geode.get());

        OutputImage_ = new osg::Image;
        OutputImage_->allocateImage(TotalWidth_, TotalHeight_, 1, GL_RGB, GL_UNSIGNED_BYTE);
        OutputImage_->setPixelFormat(GL_RGB);
        OutputImage_->setDataType(GL_UNSIGNED_BYTE);

        OutputTexture_ = new osg::TextureRectangle;   
        OutputTexture_->setTextureSize(TotalWidth_, TotalHeight_);
        OutputTexture_->setInternalFormat(GL_RGB);
        //OutputTexture_->setSourceFormat(GL_LUMINANCE);
        OutputTexture_->setSourceType(GL_UNSIGNED_BYTE);
        OutputTexture_->setFilter(osg::Texture2D::MIN_FILTER,osg::Texture2D::LINEAR);
        OutputTexture_->setFilter(osg::Texture2D::MAG_FILTER,osg::Texture2D::LINEAR);
        OutputTexture_->setImage(0,OutputImage_.get());

        Camera_ = new osg::Camera;
        Camera_->setClearColor(osg::Vec4(0.1f,0.1f,0.3f,1.0f));
        Camera_->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Camera_->setProjectionMatrixAsOrtho(0,TotalWidth_,0,TotalHeight_,-100,100);
        Camera_->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
        Camera_->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
        Camera_->setViewMatrix(osg::Matrix::identity());
        Camera_->setViewport(0, 0, TotalWidth_, TotalHeight_);

        Camera_->setRenderOrder(osg::Camera::PRE_RENDER);
        Camera_->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);

        Camera_->attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER),
                        OutputImage_);

        Camera_->addChild(geode_group.get());

        Camera_->setPostDrawCallback(new SaveCameraPostDrawCallback(OutputImage_.get(), FFW_.get()));

        RootGroup_->addChild(Camera_.get());
    }

    osg::ref_ptr<osg::Group> getRoot() { return RootGroup_; }
    osg::ref_ptr<osg::TextureRectangle> getOutputTexture() { return OutputTexture_; }
    uint8_t* getDataPtr() { return OutputImage_->data(); }
    flitr::ImageFormat getOutputFormat() { return OutputFormat_; }
	
  private:
    osg::ref_ptr<osg::Group> RootGroup_;
    osg::ref_ptr<osg::Camera> Camera_;
    osg::ref_ptr<osg::TextureRectangle> OutputTexture_;
    osg::ref_ptr<osg::Image> OutputImage_;
	int TotalWidth_;
	int TotalHeight_;
    flitr::ImageFormat OutputFormat_;
    std::tr1::shared_ptr<flitr::FFmpegWriter> FFW_;
};

#endif // RENDER_TO_VIDEO_H
