#include <iostream>
#include <string>
#include <sstream>
#include <fstream>

#include <boost/lexical_cast.hpp>

#include <osgDB/WriteFile>
#include <osgDB/ReadFile>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/TrackballManipulator>
#include <osg/io_utils>

#include <flitr/video_producer.h>
#include <flitr/image_producer.h>
#include <flitr/multi_ffmpeg_consumer.h>
#include <flitr/multi_osg_consumer.h>
#include <flitr/textured_quad.h>
#include <flitr/manipulator_utils.h>
#include <flitr/ortho_texture_manipulator.h>

using std::tr1::shared_ptr;
using namespace flitr;
using namespace std;

class KeyPressedHandler : public osgGA::GUIEventHandler 
{
	public: 

    KeyPressedHandler(shared_ptr<MultiOSGConsumer> osgc) : osgc_(osgc), count_(0) {}
    ~KeyPressedHandler() {}

    bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
		osgViewer::Viewer* viewer = dynamic_cast<osgViewer::Viewer*>(&aa);
        if (!viewer) return false;

        switch(ea.getEventType())
        {
            case(osgGA::GUIEventAdapter::KEYUP):
            {
                if (ea.getKey()=='c')
                {
					osg::Image *theImage;
					theImage = osgc_->getOutputTexture()->getImage();
					theImage->flipHorizontal();
					theImage->flipVertical();
					
					std::string s = "frame_cap_" + boost::lexical_cast<std::string>(count_) + ".rgb";
					osgDB::writeImageFile(*theImage, s);

					count_++;
                }
			}
            default:
                return false;
        }

		return true;
	}

	shared_ptr<MultiOSGConsumer> osgc_;
	int count_;
};

int main(int argc, char *argv[])
{
    shared_ptr<VideoProducer> producer(new VideoProducer());
    if (!producer->init())
    {
        std::cerr << "Failed to init producer\n";
        exit(-1);
    }

    shared_ptr<MultiOSGConsumer> consumer(new MultiOSGConsumer(*producer,1));
    if (!consumer->init()) {
        std::cerr << "Could not init OSG consumer\n";
        exit(-1);
    }

    osg::Group *root_node = new osg::Group;

    shared_ptr<TexturedQuad> quad(new TexturedQuad(consumer->getOutputTexture()));
    root_node->addChild(quad->getRoot().get());

    osgViewer::Viewer viewer;
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.addEventHandler(new osgViewer::StatsHandler);
	viewer.addEventHandler(new KeyPressedHandler(consumer));
    viewer.setSceneData(root_node);

    viewer.setUpViewInWindow(50, 50, 1024, 768);
    viewer.realize();
  
    viewer.setCameraManipulator(new OrthoTextureManipulator(640, 480));
    
    while(!viewer.done())
    {
        if (consumer->getNext())
        {
            viewer.frame();
        }
        else
        {
            OpenThreads::Thread::microSleep(5000);
        }
    }

    return 0;
}
