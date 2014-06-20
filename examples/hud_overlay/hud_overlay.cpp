#include <flitr/ffmpeg_producer.h>
#include <flitr/multi_osg_consumer.h>
#include <flitr/textured_quad.h>
#include <flitr/manipulator_utils.h>
#include <flitr/ortho_texture_manipulator.h>
#include <flitr/modules/geometry_overlays/quad_overlay.h>
#include <flitr/modules/geometry_overlays/crosshair_overlay.h>
#include <flitr/hud.h>

#include <osg/Vec3>
#include <osg/Vec4>
#include <osg/Quat>
#include <osg/Matrix>

#include <osgDB/FileUtils>
#include <osgDB/ReadFile>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <osgGA/TrackballManipulator>
#include <osg/io_utils>

#include <iostream>
#include <sstream>

using std::shared_ptr;
using namespace flitr;

// Adapted from osgpick example
class PickHandler : public osgGA::GUIEventHandler {
  public: 
    PickHandler() 
    { 
      _pick_location_available = false;
      _pick_size_available = false;
    }

    ~PickHandler() {}

    enum PickType
    {
      LOCATION,
      SIZE
    };
    
    bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa);
    virtual void pick(osgViewer::View* view, const osgGA::GUIEventAdapter& ea, PickType pt);

    bool getPickLocation(osg::Vec3d& location)
    {
        if (_pick_location_available) {
            location = _pick_location;
            _pick_location_available = false;
            return true;
        }
        return false;
    }

    bool getPickSize(osg::Vec3d& size)
    {
        if (_pick_size_available) {
            size = _pick_size;
            _pick_size_available = false;
            return true;
        }
        return false;
    }

  private:
    bool _pick_location_available;
    bool _pick_size_available;
    osg::Vec3d _pick_location;
    osg::Vec3d _pick_size;
};

bool PickHandler::handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
{
    unsigned int button_mask=ea.getButtonMask();//GUIEventAdapter::MIDDLE_MOUSE_BUTTON
    unsigned int mod_key_mask=ea.getModKeyMask();
    unsigned int event_type=ea.getEventType();

    osgViewer::View* view = dynamic_cast<osgViewer::View*>(&aa);

    if (view)
    {
	if (mod_key_mask & osgGA::GUIEventAdapter::MODKEY_SHIFT)
	{
	  if (button_mask & osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON)
	  {
	    if (event_type==osgGA::GUIEventAdapter::PUSH)
	    {
	      pick(view,ea, LOCATION);
	      
	    } else
	    if (event_type==osgGA::GUIEventAdapter::DRAG)
	    {
	      pick(view,ea, SIZE);
	      
	    }
	  } else
	  if (button_mask & osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON)
	  {
	    if (event_type==osgGA::GUIEventAdapter::PUSH)
	    {
	      pick(view,ea, LOCATION);
	      
	    } else
	    if (event_type==osgGA::GUIEventAdapter::DRAG)
	    {
	      pick(view,ea, SIZE);
	      
	    }
	  } 

 	  return true;//Don't send the event to the camera manipulator if shift is held down.
	}
    }

    return false;
}

void PickHandler::pick(osgViewer::View* view, const osgGA::GUIEventAdapter& ea, PickType pt)
{
    osgUtil::LineSegmentIntersector::Intersections intersections;

    float x = ea.getX();
    float y = ea.getY();

    if (view->computeIntersections(x,y,intersections))
    {
        for(osgUtil::LineSegmentIntersector::Intersections::iterator hitr = intersections.begin();
            hitr != intersections.end();
            ++hitr)
        {
            std::ostringstream os;
            if (!hitr->nodePath.empty() && !(hitr->nodePath.back()->getName().empty()))
            {
                // the geodes are identified by name.
                os<<"Object \""<<hitr->nodePath.back()->getName()<<"\""<<std::endl;
            }
            else if (hitr->drawable.valid())
            {
                os<<"Object \""<<hitr->drawable->className()<<"\""<<std::endl;
            }

            if (pt==LOCATION)
            {
              _pick_location = hitr->getWorldIntersectPoint();
              _pick_location_available = true;
            } else
            if (pt==SIZE)
            {
              _pick_size = hitr->getWorldIntersectPoint() - _pick_location;
              _pick_size_available = true;
            }

            os<<"        local coords vertex("<< hitr->getLocalIntersectPoint()<<")"<<"  normal("<<hitr->getLocalIntersectNormal()<<")"<<std::endl;
            os<<"        world coords vertex("<< hitr->getWorldIntersectPoint()<<")"<<"  normal("<<hitr->getWorldIntersectNormal()<<")"<<std::endl;
            const osgUtil::LineSegmentIntersector::Intersection::IndexList& vil = hitr->indexList;
            for(unsigned int i=0;i<vil.size();++i)
            {
                os<<"        vertex indices ["<<i<<"] = "<<vil[i]<<std::endl;
            }

            std::cout << os.str();
        }
    }
}

int main(int argc, char *argv[])
{

    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " video_file\n";
        return 1;
    }

    shared_ptr<FFmpegProducer> ffp(new FFmpegProducer(argv[1], ImageFormat::FLITR_PIX_FMT_Y_8));
    if (!ffp->init()) {
        std::cerr << "Could not load input file.\n";
        exit(-1);
    }

    shared_ptr<MultiOSGConsumer> osgc(new MultiOSGConsumer(*ffp,1));
    if (!osgc->init()) {
        std::cerr << "Could not init OSG consumer\n";
        exit(-1);
    }

    osg::Group *root_node = new osg::Group;
    
    shared_ptr<TexturedQuad> quad(new TexturedQuad(osgc->getOutputTexture()));
    root_node->addChild(quad->getRoot().get());

    osgViewer::Viewer viewer;
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
        
    ImageFormat imf = ffp->getFormat();
    double w = imf.getWidth();
    double h = imf.getHeight();

    QuadOverlay* qov = new QuadOverlay(w/2,h/2,w/4,h/4);
    qov->setLineWidth(2);
    //qov->flipVerticalCoordinates(h);
    root_node->addChild(qov);


    viewer.setSceneData(root_node);
    viewer.realize();//Make sure that the camera, etc is set up. Otherwise the HUD overlay can't be set up!!!


    OrthoTextureManipulator* om = new OrthoTextureManipulator(w, h);
    viewer.setCameraManipulator(om);

   //NOTE!!:Make sure to call viewer.realize() such that the camera, etc is set up. Otherwise the HUD overlay can't be set up!!!
    HUD* hud=new HUD(&viewer, -1.0, 1.0, -1.0, 1.0);
    hud->setAspectMode(HUD::HUD_ASPECT_FIXED);   

    CrosshairOverlay* ch0 = new CrosshairOverlay(0.0,0.0,0.1,0.1);
      ch0->setLineWidth(2);
      ch0->setColour(osg::Vec4d(1.0, 1.0, 1.0, 1.0));
      hud->addChild(ch0);
    CrosshairOverlay* ch1 = new CrosshairOverlay(-0.975,-0.975, 0.045,0.045);
      ch1->setLineWidth(2);
      ch1->setColour(osg::Vec4d(1.0, 0.0, 0.0, 1.0));
      hud->addChild(ch1);
    CrosshairOverlay* ch2 = new CrosshairOverlay(0.975,-0.975, 0.045,0.045);
      ch2->setLineWidth(2);
      ch2->setColour(osg::Vec4d(0.0, 1.0, 0.0, 1.0));
      hud->addChild(ch2);
    CrosshairOverlay* ch3 = new CrosshairOverlay(0.975,0.975, 0.045,0.045);
      ch3->setLineWidth(2);
      ch3->setColour(osg::Vec4d(0.0, 0.0, 1.0, 1.0));
      hud->addChild(ch3);
    CrosshairOverlay* ch4 = new CrosshairOverlay(-0.975,0.975, 0.045,0.045);
      ch4->setLineWidth(2);
      ch4->setColour(osg::Vec4d(1.0, 1.0, 0.0, 1.0));
      hud->addChild(ch4);


    // disable all culling
    viewer.getCamera()->setCullingMode(0);

    // add pick handler
    PickHandler* ph = new PickHandler; 
    viewer.addEventHandler(ph);

    while(!viewer.done())
    {
        ffp->trigger();

        if (osgc->getNext()) {
            viewer.frame();

            osg::Vec3d pickLocation;
            if (ph->getPickLocation(pickLocation)) {
                //ortho_manip->setCenter(pickpoint);
                qov->setCenter(pickLocation.x(), pickLocation.y());
            }

            osg::Vec3d pickSize;
            if (ph->getPickSize(pickSize)) {
                qov->setWidth(pickSize.x()*2.0);
                qov->setHeight(pickSize.y()*2.0);
            }

        } else {
            OpenThreads::Thread::microSleep(5000);
        }
    }

    return 0;
}
