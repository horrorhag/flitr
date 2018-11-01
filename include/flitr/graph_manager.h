/* Framework for Live Image Transformation (FLITr)
 * Copyright (c) 2016 CSIR
 *
 * This file is part of FLITr.
 *
 * FLITr is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * FLITr is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FLITr. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef GRAPH_MANAGER_H
#define GRAPH_MANAGER_H 1

#include <flitr/modules/xml_config/xml_config.h>
#include <flitr/image_producer.h>
#include <flitr/image_consumer.h>

#include <memory>
#include <vector>
#include <string>

namespace flitr {

// Private namespace for internal usage in FLITr.
namespace Private {
class GraphManagerPrivate;
typedef std::function<bool(std::string category, AttributeVector attributes, std::shared_ptr<flitr::ImageProducer>& producer, std::shared_ptr<flitr::ImageConsumer>& consumer)> GraphElementCreationCallback;
} // namespace Private

/**
 * The Graph Manager
 *
 * \section sec_graph_manager_intro Introduction
 * The Graph Manager is a class that manages image processing elements and the paths
 * between them in a running application.
 *
 * An graph element can either be a producer or consumer. Or it can be a image processing
 * pass. The way that the elements are connected are specified using paths between
 * the elements. Consumers can only consume images from producers or passes. Producers
 * can not consume images from producers unless the producer pass.
 *
 * The manager loads information from and flitr::XMLConfig object for elements.
 * From this, paths are specified between elements to create processing chains or a
 * processing graph. The manager can then based on the name of graphs create or destroy
 * graphs using the added information about elements and the paths that the graph consists
 * of.
 *
 * This manager uses registered callbacks to create different elements, depending on the
 * registered categories and the category of the element. This allows the manager to be
 * generic enough so that it can be reused in many applications. To register a callback for
 * a specific category, look at the registerGraphElementCategory() function.
 *
 * Elements are only created on demand of creation of a specific graph. Thus one can specify
 * more elements in the configuration file than the total amount that will be used in the
 * application.
 *
 * If the runtime destruction of graphs are needed, it is vital that only the manager keeps
 * strong references to the elements that gets created by the callback functions. The
 * functions or classes that creates and uses these elements must use weak pointers to track
 * the lifetime, and only promote them to strong pointers if and when needed, to check if
 * they are still available. This will also ensure that they stay valid until the user is
 * finished with the element object.
 *
 * Creation of graphs can also happen during run time, given the manager is aware of the
 * elements and the paths that form part of the required graph.
 *
 * The manager is a singleton object so that it can be accessed from anywhere in the
 * application.
 * This class is not thread safe.
 *
 * \section sec_graph_manager_notation XML File notation
 * The following nodes are needed along with their attributes to be able to work with the
 * manager.
 * \subsection subsec_graph_manager_notation_element Graph Element
 * Elements are specified as follow:
 *     - <b>%Element name:</b> graph_element
 *     - <b>Attributes:</b>
 *         - \e name - Name of the element. This name must be unique between all elements
 *                      that will be added to the manager.
 *         - \e category - Category that the element is in. This will cause the registered
 *                      function associated with the category to be called to create and set
 *                      up the element.
 *          - ... - Any other attributes needed to create the element, used by the registered
 *                      callback function and ignored by the manager.
 *
 * \subsection subsec_graph_manager_notation_path Graph Path
 * Graph paths are specified as follow:
 *     - <b>%Node name:</b> graph_path
 *     - <b>Attributes:</b>
 *         - \e graph_name - Name of the graph that this link belongs to. Multiple nodes with
 *                  the same graph name can be specified to define a graph with multiple paths.
 *                  The paths does not have to be connected to each other but they will be
 *                  created and destroyed together.
 *         - \e producer_name - Name of the producer that will produce images in the path.
 *                  This must be the name of a producer defined as a Element node, and it must
 *                  be a producer. If the producer does not exist already, it will be created
 *                  using the registered callback. If it does exist it will be reused.
 *         - \e consumer_name - Name of the consumer that will consume the images from the
 *                  producer specified. Creation will follow the same logic as discussed above
 *                  for the producer.
 *
 * \subsection subsec_graph_manager_notation_graph Graph Creation
 * Graphs are created as follow:
 *     - <b>%Node name:</b> graph
 *     - <b>Attributes:</b>
 *         - \e name - Name of the graph that must be created. If the graph consists of more
 *                  than one path, each path will be created in the order that the paths are
 *                  specified in the file.
 *
 * \subsection subsec_graph_manager_notation_example XML Example
 * The following example can be used:
 * \code
 * <!-- Elements -->
 * <graph_element name="Camera1"      category="camera_video" Type="FFmpeg" address="MyVid.mp4"/>
 * <graph_element name="DisplayLeft"  category="display" location="left"/>
 * <graph_element name="DisplayRight" category="display" location="right"/>
 * <graph_element name="Recorder"     category="Recorder" root="/path/to/Recordings/">
 *
 * <!-- graph 1 -->
 * <graph_path graph_name="Graph_1" producer_name="Camera1" consumer_name="DisplayLeft"/>
 * <graph_path graph_name="Graph_1" producer_name="Camera1" consumer_name="Recorder"/>
 * <!-- graph 2 -->
 * <graph_path graph_name="Graph_2" producer_name="Camera1" consumer_name="DisplayRight"/>
 *
 * <graph name="Graph_1"/>
 * \endcode
 * In the above example, given the correct creators are defined, a graph is created that
 * will display <tt>MyVid.mp4</tt> on the left side of the display and create a recorder
 * connected to the camera.
 *
 * If both graphs are specified to be created, the display should be duplicated on left and
 * right, based on the implementation of the creation callbacks.
 * */
class FLITR_EXPORT GraphManager
{
    GraphManager();
    GraphManager(const GraphManager& ) = delete;
public:
    ~GraphManager();

    /**
     * Create the singleton instance.
     *
     * Since creation normally happens during startup, the creation
     * of the instance is not thread safe. */
    static GraphManager* instance();

    /** Load the information of graph elements from the configuration file.
     *
     * This function can be called multiple times, given the configuration information
     * is different compared to the previous call. No previous information will
     * be overwritten or lost.
     *
     * This function will call addGraphElementInformation() for each graph element
     * found in the configuration file.
     *
     * This function will call loadGraphPathsInformation() after completion. Any errors
     * during parsing will be printed out to the log and will be ignored (loading
     * will not stop on errors).
     * \param[in] cfg Pointer to XML Configuration file.
     * \return True if the information of the element was successfully added,
     * otherwise false.
     */
    bool loadGraphElementsInformation(const std::shared_ptr<flitr::XMLConfig>& cfg);

    /**
     * Load the information of the graph paths from the configuration file.
     *
     * This function can be called multiple times, given the configuration information
     * is different compared to the previous call. No previous information will
     * be overwritten or lost.
     *
     * This function will call addGraphPathInformation() for each graph path
     * found in the configuration file.
     *
     * This function will call loadGraphsInformation() after completion. Any errors
     * during parsing will be printed out to the log and will be ignored (loading
     * will not stop on errors).
     * \param[in] cfg Pointer to XML Configuration file.
     * \return True if the information of the paths was successfully added,
     * otherwise false. */
    bool loadGraphPathsInformation(const std::shared_ptr<flitr::XMLConfig>& cfg);
    bool replaceGraphElementsInformation(const std::shared_ptr<flitr::XMLConfig> &cfg, const std::string &replaceElementName);

    /**
     * Load the information of the graphs from the configuration file.
     *
     * This function will call createGraph() for each successfully loaded graph.
     *
     * Any errors during parsing will be printed out to the log and will be ignored
     * (loading will not stop on errors).
     * \param[in] cfg Pointer to XML Configuration file.
     * \return True. There is currently no scenario that will cause this function
     * to fail.
     */
    bool loadGraphsInformation(const std::shared_ptr<flitr::XMLConfig> &cfg);

    /**
     * Create a graph given the name.
     *
     * This function is used to create a graph based on the added path and element
     * information added with either the add* or load* functions.
     * If the graph does not exist, the function will fail.
     *
     * Even though this function is called by the loadGraphsInformation() function,
     * it can be called manually during runtime to create a graph as needed.
     * If the graph is already created it will not attempt to create the graph again.
     *
     * A created graph can be destroyed using the destroyGraph() function.
     */
    bool createGraph(const std::string& graphName);

    /**
     * Destroy a graph given the name.
     *
     * The name of a created graph must be valid otherwise the function will do nothing.
     */
    bool destroyGraph(const std::string& graphName);

    /**
     * Add information of a graph element.
     *
     * This function is used to manually add information about graph elements to the
     * manager. This information will be used to create the element if a graph is
     * created that contains the element.
     *
     * The element must have a valid name and category for it to be added. Each element
     * added must also have a unique \a name. If a previous element was added
     * with the same name, this function will fail.
     * \param[in] name Name of the element to add.
     * \param[in] category Category that the element belongs to.
     * \param[in] attributes Vector containing the attributes of the element.
     * \return True if the element was added, otherwise false.
     */
    bool addGraphElementInformation(const std::string& name, const std::string& category, const AttributeVector& attributes);

    /**
     * Add information of a graph path.
     *
     * This function is used to manually add information about graph paths to the
     * manager. This information will be used to create the path if a graph is
     * created that contains the path.
     *
     * The path must have a valid name, producer and consumer for it to be added. Paths that
     * shared the same name, will be added to the same graph once the graph is created.
     * \param[in] name Name graph that this path is part of.
     * \param[in] producerName Name of the producer that will produce images.
     * \param[in] consumerName Name of the consumer that will consume the images produced by
     *      the producer.
     * \return True if the path was added, otherwise false.
     */
    bool addGraphPathInformation(const std::string& name, const std::string& producerName, const std::string& consumerName);

    /**
     * Register a graph element category and a callback function that will be
     *          responsible for creating elements in that category.
     *
     * A category can only be registered once, but the same callback can be used to
     * register for multiple categories.
     *
     * A strong pointer to the creation object must be passed to the manager. The manager does
     * not take ownership of this object, it uses it only to monitor the lifetime of the object.
     *
     * The signature of the callback member must look as follow:
     * \code
     * bool memberFunction(std::string category, AttributeVector attributes, std::shared_ptr<flitr::ImageProducer>& producer, std::shared_ptr<flitr::ImageConsumer>& consumer);
     * \endcode
     *
     * When the callback is called, the creator must attempt to create the consumer or producer
     * based on the \e attributes passed to the callback. If the creation fails the function
     * must return with false. If the element was created successfully, the correct pointer
     * must be set based on the type of element. If the created object is a producer, the
     * \e producer pointer must be set to point to the newly created producer.
     *
     * If a consumer is created, the \e producer pointer will be a smart pointer to the
     * producer that must be consumed by the new consumer. After successful creation of the
     * consumer the \e consumer pointer must be set to the new consumer.
     *
     * If the consumer is an image processor or pass, it must also set the \e producer pointer
     * to point to itself to indicate to the manager that it is a pass.
     *
     * To register a callback the following must be done:
     * \code
     * // Define a class that will create the element
     * class CameraCreator
     * {
     * public:
     *     CameraCreator();
     *     // Implement the callback using the discussed signature
     *     bool callback(std::string category, AttributeVector attributes, std::shared_ptr<flitr::ImageProducer>& producer, std::shared_ptr<flitr::ImageConsumer>& consumer);
     * };
     *
     * // Register the callback on the manager
     * int main()
     * {
     *     // Create an object of our creator class (it must be a shared_ptr)
     *     std::shared_ptr<CameraCreator> creator = std::make_shared<CameraCreator>();
     *     // Register the creator on the manager
     *     flitr::GraphManager::instance()->registerGraphElementCategory("camera_video", creator, &CameraCreator::callback);
     *     // Use the manager to load parse the XML file and create links
     *     // ...
     * }
     * \endcode
     *
     * \note Although the function is implemented using templates, the template arguments must not be specified since
     * template argument deduction should be used.
     */
    template<class Base, class Derived, class FuncCallback>
    bool registerGraphElementCategory(const std::string& category, const std::shared_ptr<Base>& receiver, FuncCallback Derived::* creationCallbackMember)
    {
        if(receiver == nullptr) {
            return false;
        }
        Private::GraphElementCreationCallback callbackMember;
        /* Bind the member function to the creator object. The raw pointer is used in the
         * bind to make sure that the callback does not take shared ownership of the
         * shared pointer. */
        callbackMember = std::bind(creationCallbackMember, receiver.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
        return registerGraphElementCategoryImp(category, receiver, callbackMember);
    }

    /**
     * Get the name of a given producer.
     *
     * This is a convenience function.
     */
    std::string producerName(const std::shared_ptr<flitr::ImageProducer>& producer) const;

    /**
     * Get the name of a given consumer.
     *
     * This is a convenience function.
     */
    std::string consumerName(const std::shared_ptr<flitr::ImageConsumer>& consumer) const;
private:
    /**
     * %Private function used by the manager to create en element.
     *
     * The element can either be a producer, consumer or a pass.
     */
    bool createElement(const std::string& elementName, std::shared_ptr<flitr::ImageProducer>& producer, std::shared_ptr<flitr::ImageConsumer>& consumer);

    /**
     * %Private function used by the manager to register a callback.
     *
     * This function is used to hide the specifics of the registerGraphElementCategory()
     * function as well as to get access to the member variables in the D-pointer.
     */
    bool registerGraphElementCategoryImp(const std::string& category, std::shared_ptr<void> receiver, Private::GraphElementCreationCallback member);
private:
    /** Static instance to the manager. */
    static GraphManager* d_instance;
    /** D or Opaque pointer to the private member data. */
    Private::GraphManagerPrivate* const d;
};

} // namespace flitr


#endif // GRAPH_MANAGER_H