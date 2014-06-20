#include <flitr/ffmpeg_producer.h>
#include <flitr/multi_osg_consumer.h>
#include <flitr/textured_quad.h>
#include <flitr/manipulator_utils.h>
#include <flitr/ortho_texture_manipulator.h>
#include <flitr/modules/geometry_overlays/quad_overlay.h>
#include <flitr/modules/geometry_overlays/crosshair_overlay.h>

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
int32_t startup_viewport_width=0;
int32_t startup_viewport_height=0;

// Adapted from osgpick example
class PickHandler : public osgGA::GUIEventHandler {
  public: 
    PickHandler() { _pick_available = false; }
    virtual ~PickHandler() {}
    
    bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa);
    virtual void pick(osgViewer::View* view, const osgGA::GUIEventAdapter& ea);
    bool getPick(osg::Vec3d& location)
    {
        if (_pick_available) {
            location = _pick_location;
            _pick_available = false;
            return true;
        }
        return false;
    }
  private:
    bool _pick_available;
    osg::Vec3d _pick_location;
};

bool PickHandler::handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
{
    switch(ea.getEventType())
    {
        case(osgGA::GUIEventAdapter::PUSH):
        {
            if (ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_SHIFT) {
                osgViewer::View* view = dynamic_cast<osgViewer::View*>(&aa);
                if (view) pick(view,ea);
            }
            return false;
        }    
        default:
            return false;
    }
}

void PickHandler::pick(osgViewer::View* view, const osgGA::GUIEventAdapter& ea)
{
    osgUtil::LineSegmentIntersector::Intersections intersections;

    float x = ea.getX();
    float y = ea.getY();

    if ((startup_viewport_width!=0)&&(startup_viewport_height!=0))
    {
        osg::Viewport *viewport=view->getCamera()->getViewport();
        x*=startup_viewport_width/viewport->width();
        y*=startup_viewport_height/viewport->height();
    }

    if (view->computeIntersections(x,y,intersections))
    {
        for(osgUtil::LineSegmentIntersector::Intersections::iterator hitr = intersections.begin();
            hitr != intersections.end();
            ++hitr)
        {
            //std::stringstream os;

            if (!hitr->nodePath.empty() && !(hitr->nodePath.back()->getName().empty()))
            {
                // the geodes are identified by name.
                std::cout<<"Object \""<<hitr->nodePath.back()->getName()<<"\""<<std::endl;
            }
            else if (hitr->drawable.valid())
            {
                std::cout<<"Object \""<<hitr->drawable->className()<<"\""<<std::endl;
            }

            _pick_location = hitr->getWorldIntersectPoint();
            _pick_available = true;
            std::cout<<"        local coords vertex("<< hitr->getLocalIntersectPoint()<<")"<<"  normal("<<hitr->getLocalIntersectNormal()<<")"<<std::endl;
            std::cout<<"        world coords vertex("<< hitr->getWorldIntersectPoint()<<")"<<"  normal("<<hitr->getWorldIntersectNormal()<<")"<<std::endl;
            const osgUtil::LineSegmentIntersector::Intersection::IndexList& vil = hitr->indexList;
            for(unsigned int i=0;i<vil.size();++i)
            {
                std::cout<<"        vertex indices ["<<i<<"] = "<<vil[i]<<std::endl;
            }

            //std::cout << os.str();
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " video_file\n";
        return 1;
    }

    std::cout << "Shift + left_click sets the quad center.\n";

    shared_ptr<FFmpegProducer> ffp(new FFmpegProducer(argv[1], ImageFormat::FLITR_PIX_FMT_Y_8));
    if (!ffp->init()) {
        std::cerr << "Could not load " << argv[1] << "\n";
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

    //viewer.setUpViewInWindow(0,0, 640, 480);

    osg::Viewport *viewport=viewer.getCamera()->getViewport();
    if (viewport)
    {
      startup_viewport_width=viewport->width();
      startup_viewport_height=viewport->height();
      std::cout << "Viewport: " << startup_viewport_width << " " << startup_viewport_height << "\n";
      std::cout.flush();
    }


    // add pick handler
    PickHandler* ph = new PickHandler; 
    viewer.addEventHandler(ph);
    
    ImageFormat imf = ffp->getFormat();
    double w = imf.getWidth();
    double h = imf.getHeight();
    QuadOverlay* qov = new QuadOverlay(w/2,h/2,w/4,h/4, true);
    qov->setLineWidth(2);
    //qov->flipVerticalCoordinates(h);
    qov->setName("qov");
    root_node->addChild(qov);

    CrosshairOverlay* ch = new CrosshairOverlay(w/2,h/2,20,30);
    ch->setLineWidth(2);
    root_node->addChild(ch);
    
    viewer.setSceneData(root_node);

    OrthoTextureManipulator* om = new OrthoTextureManipulator(imf.getWidth(), imf.getHeight());
    viewer.setCameraManipulator(om);

    // disable all culling
    //viewer.getCamera()->setCullingMode(0);

    while(!viewer.done())
    {
        ffp->trigger();
        if (osgc->getNext()) {
            viewer.frame();
            osg::Vec3d pickpoint;
            if (ph->getPick(pickpoint)) {
                //ortho_manip->setCenter(pickpoint);
                //qov->setCenter(pickpoint.x(), pickpoint.y());
                ch->setCenter(pickpoint.x(), pickpoint.y());
            }
        } else {
            OpenThreads::Thread::microSleep(5000);
        }
    }

    return 0;
}
