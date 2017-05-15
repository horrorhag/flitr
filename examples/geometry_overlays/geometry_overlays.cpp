#include <flitr/ffmpeg_producer.h>
#include <flitr/multi_osg_consumer.h>
#include <flitr/textured_quad.h>
#include <flitr/manipulator_utils.h>
#include <flitr/ortho_texture_manipulator.h>
#include <flitr/modules/geometry_overlays/quad_overlay.h>
#include <flitr/modules/geometry_overlays/crosshair_overlay.h>
#include <flitr/modules/geometry_overlays/triangle_overlay.h>
#include <flitr/modules/geometry_overlays/circle_overlay.h>
#include <flitr/modules/geometry_overlays/arrow_overlay.h>
#include <flitr/modules/geometry_overlays/tape_overlay.h>

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
using std::make_shared;
using namespace flitr;
int32_t startup_viewport_width=0;
int32_t startup_viewport_height=0;

// Adapted from osgpick example
class PickHandler : public osgGA::GUIEventHandler {
  public:
    PickHandler() { _pick_available = false; }
    ~PickHandler() {}

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
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " video_file circle_points\n";
        return 1;
    }

    std::cout << "Shift + left_click sets the crosshair center.\n";

    shared_ptr<FFmpegProducer> ffp = make_shared<FFmpegProducer>(argv[1], ImageFormat::FLITR_PIX_FMT_Y_8);
    if (!ffp->init()) {
        std::cerr << "Could not load " << argv[1] << "\n";
        exit(-1);
    }

    unsigned int circlePoints = atoi(argv[2]);

    shared_ptr<MultiOSGConsumer> osgc = make_shared<MultiOSGConsumer>(*ffp,1);
    if (!osgc->init()) {
        std::cerr << "Could not init OSG consumer\n";
        exit(-1);
    }

    osg::Group *root_node = new osg::Group;

    shared_ptr<TexturedQuad> quad = make_shared<TexturedQuad>(osgc->getOutputTexture());
    root_node->addChild(quad->getRoot().get());

    osgViewer::Viewer viewer;
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);

    viewer.setUpViewInWindow(0,0, 640, 480);

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
    double quadCol = w * 0.25;
    double triangleCol = w * 0.5;
    double circleCol = w * 0.75;
    double baseRow = h * 0.2;
    double filledRow = h * 0.4;

    double width = w * 0.2;
    double height = h * 0.15;

    CrosshairOverlay* ch = new CrosshairOverlay(w/2,h/2,20,30);
    ch->setLineWidth(2);
    root_node->addChild(ch);

    /* Quads */
    QuadOverlay* qovBase = new QuadOverlay(quadCol, baseRow, width, height, false);
    qovBase->setLineWidth(3);
    root_node->addChild(qovBase);
    qovBase->setColour(osg::Vec4d(1,0,0,0.25));

    QuadOverlay* qovfilled = new QuadOverlay(quadCol, filledRow, width, height, true);
    qovfilled->setLineWidth(2);
    root_node->addChild(qovfilled);
    qovfilled->setColour(osg::Vec4d(1,1,0,1));

    /* Triangles */
    TriangleOverlay* triangleBase = new TriangleOverlay(triangleCol, baseRow, width, height, false);
    triangleBase->setLineWidth(2);
    root_node->addChild(triangleBase);
    triangleBase->setColour(osg::Vec4d(0,1,0,1));

    TriangleOverlay* triangleFilled = new TriangleOverlay(triangleCol, filledRow, width, height, true);
    triangleFilled->setLineWidth(2);
    root_node->addChild(triangleFilled);
    triangleFilled->setColour(osg::Vec4d(1,1,0,0.5));

    TriangleOverlay* triangleUp = new TriangleOverlay(triangleCol, baseRow, width, height, true);
    triangleUp->setLineWidth(2);
    root_node->addChild(triangleUp);
    triangleUp->setColour(osg::Vec4d(0,1,1,1));
    triangleUp->flipVerticalCoordinates(h);

    /* Circles */
    CircleOverlay* circleBase = new CircleOverlay(circleCol, baseRow, height / 2, circlePoints, false);
    circleBase->setLineWidth(2);
    root_node->addChild(circleBase);
    circleBase->setColour(osg::Vec4d(0,0,1,1));

    CircleOverlay* circleFilled = new CircleOverlay(circleCol, filledRow, height / 2, circlePoints, true);
    circleFilled->setLineWidth(2);
    root_node->addChild(circleFilled);
    circleFilled->setColour(osg::Vec4d(1,1,0,1));

    /* Arrow */
    ArrowOverlay* arrowBase = new ArrowOverlay(w*0.5, h*0.95, w*0.02, w*0.02, w*0.01, w*0.02, false, 180);
    arrowBase->setLineWidth(2);
    root_node->addChild(arrowBase);
    arrowBase->setColour(osg::Vec4d(0,1,1,0.5));

    /* Tape */
    TapeOverlay* tapeBase = new TapeOverlay(w*0.5, h*0.95, w*0.8, 9, 16);
    tapeBase->setLineWidth(2);
    root_node->addChild(tapeBase);
    tapeBase->setColour(osg::Vec4d(0,1,1,1));
    tapeBase->setMajorTickHeight(20);

    /* Vertical Tape */
    TapeOverlay* vTapeBase = new TapeOverlay(w*0.97, h*0.5, h*0.8, 9, 8, false, TapeOverlay::Orientation::VERTICAL);
    vTapeBase->setLineWidth(1);
    root_node->addChild(vTapeBase);
    vTapeBase->setColour(osg::Vec4d(0,1,1,1));
    vTapeBase->setMajorTickHeight(10);
    vTapeBase->setMinorTickHeight(6);

    viewer.setSceneData(root_node);

    OrthoTextureManipulator* om = new OrthoTextureManipulator(imf.getWidth(), imf.getHeight());
    viewer.setCameraManipulator(om);

    while(!viewer.done())
    {
        ffp->trigger();
        if (osgc->getNext()) {
            viewer.frame();
            osg::Vec3d pickpoint;
            if (ph->getPick(pickpoint)) {
                ch->setCenter(pickpoint.x(), pickpoint.y());
            }
            tapeBase->setOffset(tapeBase->getOffset()+0.01);
            vTapeBase->setOffset(vTapeBase->getOffset()-0.01);
        } else {
            OpenThreads::Thread::microSleep(5000);
        }
    }

    return 0;
}
