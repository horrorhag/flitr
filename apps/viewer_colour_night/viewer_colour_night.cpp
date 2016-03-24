
#include <flitr/ffmpeg_producer.h>
#include <flitr/libtiff_producer.h>
#include <flitr/multi_osg_consumer.h>
#include <flitr/textured_quad.h>
#include <flitr/manipulator_utils.h>
#include <flitr/ortho_texture_manipulator.h>
#include <flitr/modules/geometry_overlays/crosshair_overlay.h>
#include <flitr/modules/xml_config/xml_config.h>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/TrackballManipulator>
#include <osg/io_utils>

#include <iostream>
#include <string>
#include <atomic>


class KeyPressedHandler : public osgGA::GUIEventHandler
{
public:
    KeyPressedHandler() :
    textureID_(0),
    leftRightEvents_(0),
    upDownEvents_(0),
    rotateEvents_(0),
    scaleEvents_(0)
    {}
    
    ~KeyPressedHandler()
    {
    }
    
    bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        osgViewer::Viewer* viewer = dynamic_cast<osgViewer::Viewer*>(&aa);
        if (!viewer) return false;
        
        switch(ea.getEventType())
        {
            case(osgGA::GUIEventAdapter::KEYDOWN):
            {
                if (ea.getKey()=='1')
                {
                    textureID_.store(0);
                } else
                    if (ea.getKey()=='2')
                    {
                        textureID_.store(1);
                    } else
                        if (ea.getKey()=='3')
                        {
                            textureID_.store(2);
                        } else
                            if (ea.getKey()==osgGA::GUIEventAdapter::KEY_Left)
                            {
                                leftRightEvents_--;
                            } else
                                if (ea.getKey()==osgGA::GUIEventAdapter::KEY_Right)
                                {
                                    leftRightEvents_++;
                                } else
                                    if (ea.getKey()==osgGA::GUIEventAdapter::KEY_Up)
                                    {
                                        upDownEvents_--;
                                    } else
                                        if (ea.getKey()==osgGA::GUIEventAdapter::KEY_Down)
                                        {
                                            upDownEvents_++;
                                        } else
                                            if (ea.getKey()=='a')
                                            {
                                                rotateEvents_--;
                                            } else
                                                if (ea.getKey()=='d')
                                                {
                                                    rotateEvents_++;
                                                } else
                                                    if (ea.getKey()=='w')
                                                    {
                                                        scaleEvents_++;
                                                    } else
                                                        if (ea.getKey()=='x')
                                                        {
                                                            scaleEvents_--;
                                                        }
                
                break;
            }
            default:
                break;
        }
        
        return false;
    }
    
    //! Get the currently selected texture channel.
    int getTextureID() const
    {
        return textureID_.load();
    }
    
    //! Get the left (negative) vs. right (positive) change.
    int getLeftRightEvents()
    {
        return leftRightEvents_.exchange(0);
    }
    
    //! Get the up (negative) vs. down (positive) change.
    int getUpDownEvents()
    {
        return upDownEvents_.exchange(0);
    }
    
    //! Get the clockwise (positive) vs. anti-clock (negative) change.
    int getRotateEvents()
    {
        return rotateEvents_.exchange(0);
    }
    
    //! Get the upscale (positive) vs. downscale (negative) change AROUND ZERO!
    int getScaleEvents()
    {
        return scaleEvents_.exchange(0);
    }
    
private:
    std::atomic_int textureID_;
    std::atomic_int leftRightEvents_;
    std::atomic_int upDownEvents_;
    std::atomic_int rotateEvents_;
    std::atomic_int scaleEvents_;
};




int main(int argc, char *argv[])
{
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " config_file\n";
        return 1;
    }
    
    std::string configFileName(argv[1]);
    
    std::shared_ptr<flitr::XMLConfig> cfg(new flitr::XMLConfig());
    
    if (!cfg->init(configFileName))
    {
        std::cerr << "Could not open configuration file." << std::endl;
        return 1;
    }
    
    const auto channelElementVec=cfg->getAttributeVector("channel_setup");
    const size_t numChannels=channelElementVec.size();
    
    std::vector<std::string> fileVec(numChannels, "");
    std::vector<float> xVec(numChannels, 0.0);
    std::vector<float> yVec(numChannels, 0.0);
    std::vector<float> scaleVec(numChannels, 1.0);
    std::vector<float> angleVec(numChannels, 0.0);
    std::vector<float> rVec(numChannels, 0.0);
    std::vector<float> gVec(numChannels, 0.0);
    std::vector<float> bVec(numChannels, 0.0);
    
    
    for (size_t channelNum=0; channelNum<numChannels; ++channelNum)
    {
        const auto &channelElement=channelElementVec[channelNum];
        
        for (const auto channelAttribute : channelElement)
        {
            if (channelAttribute.first == "filename")
            {
                fileVec[channelNum]=channelAttribute.second;
            } else
                if (channelAttribute.first == "x")
                {
                    xVec[channelNum]=std::stof(channelAttribute.second);
                } else
                    if (channelAttribute.first == "y")
                    {
                        yVec[channelNum]=std::stof(channelAttribute.second);
                    } else
                        if (channelAttribute.first == "scale")
                        {
                            scaleVec[channelNum]=std::stof(channelAttribute.second);
                        } else
                            if (channelAttribute.first == "angle")
                            {
                                angleVec[channelNum]=std::stof(channelAttribute.second);
                            } else
                                if (channelAttribute.first == "r")
                                {
                                    rVec[channelNum]=std::stof(channelAttribute.second);
                                } else
                                    if (channelAttribute.first == "g")
                                    {
                                        gVec[channelNum]=std::stof(channelAttribute.second);
                                    } else
                                        if (channelAttribute.first == "b")
                                        {
                                            bVec[channelNum]=std::stof(channelAttribute.second);
                                        }
        }
    }
    
    
    
    std::shared_ptr<flitr::MultiLibTiffProducer> ip(new flitr::MultiLibTiffProducer(fileVec));
    
    if (!ip->init()) {
        std::cerr << "Could not load tif files." << "\n";
        exit(-1);
    }
    
    
    std::shared_ptr<flitr::MultiOSGConsumer> osgc(new flitr::MultiOSGConsumer(*ip, fileVec.size()));
    
    if (!osgc->init()) {
        std::cerr << "Could not init OSG consumer\n";
        exit(-1);
    }
    
    
    osg::Group *root_node = new osg::Group;
    
    std::vector<osg::TextureRectangle *> inTexVec;
    for (size_t channelNum=0; channelNum<numChannels; ++channelNum)
    {
        inTexVec.push_back(osgc->getOutputTexture(channelNum));
    }
    
    std::shared_ptr<flitr::MultiTexturedQuad> multiTexQuad(new flitr::MultiTexturedQuad(inTexVec));
    for (size_t channelNum=0; channelNum<numChannels; ++channelNum)
    {
        multiTexQuad->setColourMask(osg::Vec4f(rVec[channelNum], gVec[channelNum], gVec[channelNum], 1.0), channelNum);
    }
    
    for (size_t channelNum=0; channelNum<numChannels; ++channelNum)
    {
        multiTexQuad->translateTexture(xVec[channelNum], yVec[channelNum], channelNum);
        multiTexQuad->rotateTexture(osgc->getOutputTexture(0)->getTextureWidth()*0.5,
                                    osgc->getOutputTexture(0)->getTextureWidth()*0.5,
                                    angleVec[channelNum], channelNum);
        multiTexQuad->scaleTexture(osgc->getOutputTexture(0)->getTextureWidth()*0.5,
                                    osgc->getOutputTexture(0)->getTextureWidth()*0.5,
                                    scaleVec[channelNum], scaleVec[channelNum], channelNum);
    }

    multiTexQuad->setShader("fuse.frag");
    
    root_node->addChild(multiTexQuad->getRoot().get());
    
    //=== Cross-hair ===//
    flitr::CrosshairOverlay* cr0 = new flitr::CrosshairOverlay(osgc->getOutputTexture(0)->getTextureWidth()*0.5,
                                                               osgc->getOutputTexture(0)->getTextureHeight()*0.5,
                                                               49,49);
    cr0->setLineWidth(1);
    cr0->setColour(osg::Vec4d(1.0, 1.0, 1.0, 0.5));
    root_node->addChild(cr0);
    
    flitr::CrosshairOverlay* cr1 = new flitr::CrosshairOverlay(osgc->getOutputTexture(0)->getTextureWidth()*0.5,
                                                               osgc->getOutputTexture(0)->getTextureHeight()*0.5,
                                                               50,50);
    cr1->setLineWidth(3);
    cr1->setColour(osg::Vec4d(1.0, 0.0, 0.0, 0.5));
    root_node->addChild(cr1);
    //=== ===//
    
    
    osgViewer::Viewer viewer;
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    
    viewer.addEventHandler(new osgViewer::StatsHandler);
    
    KeyPressedHandler *kbHandler=new KeyPressedHandler();
    viewer.addEventHandler(kbHandler);
    
    osgViewer::ScreenCaptureHandler *scHandler=new osgViewer::ScreenCaptureHandler();
    scHandler->setKeyEventTakeScreenShot('s');
    viewer.addEventHandler(scHandler);

    viewer.setSceneData(root_node);
    
    viewer.setUpViewInWindow(480+40, 40, 640, 480);
    viewer.realize();
    
    
    const int use_trackball = 0;
    
    if (use_trackball) {
        osgGA::TrackballManipulator* tb = new osgGA::TrackballManipulator;
        viewer.setCameraManipulator(tb);
        flitr::adjustCameraManipulatorHomeForYUp(tb);
    } else {
        
        flitr::ImageFormat imf = ip->getFormat(0);
        flitr::OrthoTextureManipulator* om = new flitr::OrthoTextureManipulator(imf.getWidth(), imf.getHeight());
        viewer.setCameraManipulator(om);
    }
    
    uint32_t frameRate=10;
    double framePeriodNS=1.0e9 / frameRate;
    double nextFrameTimeNS=currentTimeNanoSec()+framePeriodNS;
    
    //int numPages=ip->getNumImages(0);
    
    while(!viewer.done())
    {
        const int textureID=kbHandler->getTextureID();
        const int leftRightEvents=kbHandler->getLeftRightEvents();
        const int upDownEvents=kbHandler->getUpDownEvents();
        const int rotateEvents=kbHandler->getRotateEvents();
        const int scaleEvents=kbHandler->getScaleEvents();
        
        if (leftRightEvents)
        {
            multiTexQuad->translateTexture(leftRightEvents*0.5, 0, textureID);
        }
        if (upDownEvents)
        {
            multiTexQuad->translateTexture(0, upDownEvents*0.5, textureID);
        }
        if (rotateEvents)
        {
            multiTexQuad->rotateTexture(osgc->getOutputTexture(0)->getTextureWidth()*0.5,
                                        osgc->getOutputTexture(0)->getTextureWidth()*0.5,
                                        rotateEvents*0.1,
                                        textureID);
        }
        if (scaleEvents)
        {
            multiTexQuad->scaleTexture(osgc->getOutputTexture(0)->getTextureWidth()*0.5,
                                       osgc->getOutputTexture(0)->getTextureWidth()*0.5,
                                       1.0+scaleEvents*0.005,
                                       1.0+scaleEvents*0.005,
                                       textureID);
        }
        
        
        if (currentTimeNanoSec()>=nextFrameTimeNS)
        {
            ip->seek(0xFFFFFFFF);//Seek to end of file for live multipage view...
            //OR
            //ip->trigger();//Read every slot...
            
            if (osgc->getNext())
            {
                viewer.frame();
                nextFrameTimeNS+=framePeriodNS;
            }
        }
        else
        {
            OpenThreads::Thread::microSleep(1000);
        }
    }
    
    return 0;
}
