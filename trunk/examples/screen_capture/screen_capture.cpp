#include <iostream>
#include <string>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/TrackballManipulator>
#include <osg/io_utils>
#include <osgDB/ReadFile>

#include <flitr/screen_capture_producer.h>
#include <flitr/multi_ffmpeg_consumer.h>

using std::shared_ptr;
using namespace flitr;

int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " osg_file\n";
        return 1;
    }
    
    osg::ArgumentParser arguments(&argc, argv);

    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgViewer::StatsHandler);
    
    // load the data
    osg::ref_ptr<osg::Node> loadedModel = osgDB::readNodeFiles(arguments);
    if (!loadedModel) 
    {
        std::cout << arguments.getApplicationName() <<": No data loaded" << std::endl;
        return 1;
    }

    viewer.setSceneData(loadedModel.get());
    
    viewer.setUpViewInWindow(40, 40, 800, 600);
    viewer.realize();
    std::vector<osgViewer::View*> views;
    viewer.getViews(views);
    if (views.size() < 1) return 1;

    shared_ptr<ScreenCaptureProducer> scp(new ScreenCaptureProducer(*(views[0]), 60, 3));
    scp->init();
    scp->startCapture();

    shared_ptr<MultiFFmpegConsumer> mffc(new MultiFFmpegConsumer(*scp, 1));
    mffc->setCodec(flitr::FLITR_MJPEG_CODEC, 4000000);
    mffc->setContainer(flitr::FLITR_AVI_CONTAINER);
    mffc->init();
    mffc->openFiles("screen_cap",20);
    mffc->startWriting();

    viewer.run();

    mffc->stopWriting();
    mffc->closeFiles();

    return 0;
}
