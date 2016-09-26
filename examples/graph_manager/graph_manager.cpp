#include <flitr/graph_manager.h>
#include <flitr/ffmpeg_producer.h>
#include <flitr/multi_osg_consumer.h>
#include <flitr/textured_quad.h>
#include <flitr/ortho_texture_manipulator.h>
#include <flitr/modules/flitr_image_processors/flip/fip_flip.h>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

using std::shared_ptr;
using namespace flitr;

class ElementCreator {
public:
    ElementCreator() {}

    bool createFFmpegElement(std::string category, AttributeVector attributes, std::shared_ptr<flitr::ImageProducer>& producer, std::shared_ptr<flitr::ImageConsumer>& consumer);
    bool createDisplayElement(std::string category, AttributeVector attributes, std::shared_ptr<flitr::ImageProducer>& producer, std::shared_ptr<flitr::ImageConsumer>& consumer);
    bool createFlipElement(std::string category, AttributeVector attributes, std::shared_ptr<flitr::ImageProducer>& producer, std::shared_ptr<flitr::ImageConsumer>& consumer);

    /* List of producers. Normally member variables must be protected
     * or private, but this is just an example of the manager. */
    std::weak_ptr<FFmpegProducer> d_ffmpegProducers;
    std::weak_ptr<MultiOSGConsumer> d_osgConsumers;

    osg::ref_ptr<osgViewer::Viewer> d_viewer;
};
typedef std::shared_ptr<ElementCreator> ElementCreatorPtr;


int main(int argc, char *argv[])
{
    std::cout << "This example shows how the GraphManager class is used to set up processing graphs.\n"
              << "The example sets up the graph purely in code, but the setup and creation of the graph \n"
              << "can also be done using an XML file along wit the flitr::XMLConfig class.\n"
              << "Each time the manager is used, the corresponding XML element is shown in the comments \n"
              << "to show how the XML file would look for the given configuration. \n"
              << "The XML file will look as follow:                                                   \n"
              << "<config>                                                                            \n"
              << "  <!-- Definition of the Graph Elements -->                                         \n"
              << "  <graph_element name=\"file\" category=\"ffmpeg\" file_name=\"video.mp4\"/>        \n"
              << "  <graph_element name=\"view\" category=\"display\"/>                               \n"
              << "  <graph_element name=\"flip\" category=\"flip\"/>                                  \n"
              << "  <!-- Graph paths for graph_1 -->                                                  \n"
              << "  <graph_path graph_name=\"graph_1\" producer_name=\"file\" consumer_name=\"view\"/>\n"
              << "  <!-- Graph paths for graph_2 -->                                                  \n"
              << "  <graph_path graph_name=\"graph_2\" producer_name=\"file\" consumer_name=\"flip\"/>\n"
              << "  <graph_path graph_name=\"graph_2\" producer_name=\"flip\" consumer_name=\"view\"/>\n"
              << "  <!-- Create graph_1 -->                                                           \n"
              << "  <graph name=\"graph_1\"/>                                                         \n"
              << "</config>" << std::endl;

    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " video_file graph_name\n"
                  << "Where: \n"
                  << "    - video_file: Name of video file to be played\n"
                  << "    - graph_name: Name of graph to create, graph_1 or graph_2" << std::endl;
        return 1;
    }
    std::string fileName  = argv[1];
    std::string graphName = argv[2];
    /* Create an instance of the Element Creator */
    ElementCreatorPtr creator = std::make_shared<ElementCreator>();

    /* Force the creation of the Graph Manager. This is not needed in this example
     * since addition of creator callbacks will happen directly after, but in applications
     * where the addition of callbacks are not done in the main() function, it is advised
     * to do this to make sure that the manager is initialised in a thread safe way. */
    GraphManager::instance();
    GraphManager::instance()->registerGraphElementCategory("ffmpeg" , creator, &ElementCreator::createFFmpegElement);
    GraphManager::instance()->registerGraphElementCategory("display", creator, &ElementCreator::createDisplayElement);
    GraphManager::instance()->registerGraphElementCategory("flip"   , creator, &ElementCreator::createFlipElement);

    /* Add an element to the Graph Manager */
    /* The xml file will look as follow, given a file name of "video.mp4"
     *
     * <graph_element name="file" category="ffmpeg" file_name="video.mp4"/> */
    GraphManager::instance()->addGraphElementInformation("file", "ffmpeg", {{"file_name", fileName}});
    /* Add the display element: The XML fill look as follow:
     *
     * <graph_element name="view" category="display"/> */
    GraphManager::instance()->addGraphElementInformation("view", "display", {});
    /* Add the flip element: The XML fill look as follow:
     *
     * <graph_element name="flip" category="flip"/> */
    GraphManager::instance()->addGraphElementInformation("flip", "flip", {{"flip_left_right", "false"} , {"flip_top_bottom", "true"}});
    /* Add a path between the two elements added above for graph_1:
     * The XML will look as follow:
     *
     * <graph_path graph_name="graph_1" producer_name="file" consumer_name="view"/> */
    GraphManager::instance()->addGraphPathInformation("graph_1", "file", "view");
    /* Add the paths for graph_2
     * The XML will look as follow:
     *
     * <graph_path graph_name="graph_2" producer_name="file" consumer_name="flip"/>
     * <graph_path graph_name="graph_2" producer_name="flip" consumer_name="view"/> */
    GraphManager::instance()->addGraphPathInformation("graph_2", "file", "flip");
    GraphManager::instance()->addGraphPathInformation("graph_2", "flip", "view");
    /* Now create the graph. The XML will look as follow, say graph_1 was chosen
     *
     * <graph name="graph_1"/> */
    bool created = GraphManager::instance()->createGraph(graphName);
    if(created == false) {
        std::cout << "Graph could not be created, make sure to choose between \'graph_1\' and \'graph_2\'" << std::endl;
        return 1;
    }


    /* At this point the producers and consumers should be valid. The below code are
     * just for the sake of the example. It is not the best way to do the triggering
     * but should be valid for this example. */
    assert(creator->d_ffmpegProducers.lock() != nullptr);
    assert(creator->d_osgConsumers.lock()    != nullptr);
    assert(creator->d_viewer                 != nullptr);

    while(!creator->d_viewer->done())
    {
        auto ffp = creator->d_ffmpegProducers.lock();
        if(ffp != nullptr) {
            ffp->trigger();
        } else {
            std::cout << "FFmpeg producer not valid any more. Manager destroyed the graph probably." << std::endl;
        }

        auto osgc = creator->d_osgConsumers.lock();
        if(osgc != nullptr) {
            if (creator->d_osgConsumers.lock()->getNext()) {
                creator->d_viewer->frame();
            } else {
                std::this_thread::sleep_for(std::chrono::microseconds(5000));
            }
        } else {
            std::cout << "OSG Consumer not valid any more. Manager destroyed the graph probably." << std::endl;
            std::this_thread::sleep_for(std::chrono::microseconds(5000));
        }
    }

    return 0;
}
//--------------------------------------------------

bool ElementCreator::createFFmpegElement(std::string category, AttributeVector attributes, std::shared_ptr<ImageProducer>& producer, std::shared_ptr<ImageConsumer>& consumer)
{
    /* The category is sent through if multiple categories make use of the same callback.
     * Since this is not the case, it can be ignored. This call removes the "unused variable"
     * compiler warning. */
    (void)category;
    /* The element created by this category is a producer, just a check to make sure that the
     * manager is calling it as such. */
    if(producer != nullptr) {
        std::cout << "FFmpeg element must be created as a producer" << std::endl;
        return false;
    }
    /* This should also be empty */
    if(consumer != nullptr) {
        std::cout << "consumer pointer must be empty" << std::endl;
        return false;
    }

    /* Get the file that must be opened from the list of attributes */
    std::string fileName;
    for(const auto &attrib: attributes) {
        if(attrib.first == "file_name") {
            fileName = attrib.second;
        }
    }

    /* Now create the element, the element for this category is the FFmpegProducer */
    shared_ptr<FFmpegProducer> ffp = std::make_shared<FFmpegProducer>(fileName, ImageFormat::FLITR_PIX_FMT_Y_8);
    if (!ffp->init()) {
        std::cout << "Could not load input file." << std::endl;
        return false;
    }
    /* The class also wants a pointer to the producer, it is important to keep this
     * pointer as a weak pointer (see the GraphManager docs. */
    d_ffmpegProducers = ffp;
    /* If the element creation was successful, set the pointer */
    producer = ffp;
    return true;
}
//--------------------------------------------------

bool ElementCreator::createDisplayElement(std::string category, AttributeVector attributes, std::shared_ptr<ImageProducer>& producer, std::shared_ptr<ImageConsumer>& consumer)
{
    (void)category;
    (void)attributes;
    /* The element created by this category is a consumer, just a check to make sure that the
     * manager is calling it as such. Thus the producer should be valid */
    if(producer == nullptr) {
        std::cout << "Display element must be created as a consumer" << std::endl;
        return false;
    }
    /* This should be empty */
    if(consumer != nullptr) {
        std::cout << "consumer pointer must be empty" << std::endl;
        return false;
    }
    /* This element does not expect any attributes */
    shared_ptr<MultiOSGConsumer> osgc = std::make_shared<MultiOSGConsumer>(*producer,1);
    if (!osgc->init()) {
        std::cerr << "Could not init OSG consumer" << std::endl;
        return false;
    }

    osg::Group *root_node = new osg::Group;
    shared_ptr<TexturedQuad> quad(new TexturedQuad(osgc->getOutputTexture()));
    root_node->addChild(quad->getRoot().get());

    ImageFormat imf = producer->getFormat();
    double w = imf.getWidth();
    double h = imf.getHeight();

    d_viewer = osg::ref_ptr<osgViewer::Viewer>(new osgViewer::Viewer());
    d_viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
    d_viewer->addEventHandler(new osgViewer::StatsHandler);
    d_viewer->setSceneData(root_node);
    d_viewer->setUpViewInWindow(100, 100, w, h);
    d_viewer->realize();

    OrthoTextureManipulator* om = new OrthoTextureManipulator(w, h);
    d_viewer->setCameraManipulator(om);

    /* Keep a pointer to it. */
    d_osgConsumers = osgc;
    /* If the element creation was successful, set the pointer */
    consumer = osgc;
    return true;
}
//--------------------------------------------------

bool ElementCreator::createFlipElement(std::string category, AttributeVector attributes, std::shared_ptr<ImageProducer>& producer, std::shared_ptr<ImageConsumer>& consumer)
{
    (void)category;
    (void)attributes;

    /* The element created by this category is a pass, thus it must be
     * created as a consumer, just a check to make sure that the
     * manager is calling it as such. Thus the producer should be valid */
    if(producer == nullptr) {
        std::cout << "Flip element must be created as a consumer" << std::endl;
        return false;
    }
    /* This should be empty */
    if(consumer != nullptr) {
        std::cout << "consumer pointer must be empty" << std::endl;
        return false;
    }

    std::vector<bool> leftRight;
    std::vector<bool> topBot;

    for(const auto &attrib: attributes) {
        if(attrib.first == "flip_left_right") {
            leftRight.push_back((attrib.second == "true")? true: false);
        } else if(attrib.first == "flip_top_bottom") {
            topBot.push_back((attrib.second == "true")? true: false);
        }
    }

    std::shared_ptr<flitr::FIPFlip> flipper = std::make_shared<flitr::FIPFlip>(*producer, 1, leftRight, topBot);
    if(!flipper->init()) {
        std::cerr << "Could not init Flip pass" << std::endl;
        return false;
    }

    flipper->startTriggerThread();

    /* If the element creation was successful, set the pointers.
     * Since the element is a pass, both pointers must be set. */

    consumer = flipper;
    producer = flipper;
    return true;
}
//--------------------------------------------------
