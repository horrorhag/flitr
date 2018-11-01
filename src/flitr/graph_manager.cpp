/* Framework for Live Image Transformation (FLITr)
 * Copyright (c) 2010 CSIR
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

#include <flitr/graph_manager.h>

#include <algorithm>

using namespace flitr;
using namespace flitr::Private;

flitr::GraphManager* flitr::GraphManager::d_instance = nullptr;

namespace flitr {
namespace Private {

/*! Struct containing the information of a Graph Element. */
struct GraphElementProperties
{
    std::string name;
    std::string category;
    flitr::AttributeVector attributes;
};

///*! Struct containing information of created Graphs. *
class CreatedGraphProperties
{
public:
    std::string name;
    std::vector<std::shared_ptr<flitr::ImageProducer>> producers;
    std::vector<std::shared_ptr<flitr::ImageConsumer>> consumers;

    CreatedGraphProperties() {}
    ~CreatedGraphProperties()
    {
        /* First clear all consumers */
        std::vector<std::shared_ptr<flitr::ImageConsumer>>::reverse_iterator consumerIter = consumers.rbegin();
        while(consumerIter != consumers.rend()) {
            (*consumerIter) = nullptr;
            ++consumerIter;
        }
        consumers.clear();
        /* Now only clear all the producers */
        std::vector<std::shared_ptr<flitr::ImageProducer>>::reverse_iterator producerIter = producers.rbegin();
        while(producerIter != producers.rend()) {
            (*producerIter) = nullptr;
            ++producerIter;
        }
        producers.clear();
    }
};
typedef std::map<std::string, CreatedGraphProperties> CreatedGraphsMap;

typedef std::pair<std::string /* Producer */, std::string /* Consumer */> GraphPath;
typedef std::vector<GraphPath> GraphPathVector;
typedef std::map<std::string, GraphPathVector> GraphPathsMap;

typedef std::map<std::string, std::weak_ptr<flitr::ImageProducer>> ProducersMap;
typedef std::map<std::string, std::weak_ptr<flitr::ImageConsumer>> ConsumersMap;

typedef std::map<std::string, GraphElementProperties> GraphElementPropertiesMap;

/*! Struct containing information of Element Creators.
 *
 * A weak void pointer is kept to the creator so that the manager does not
 * take shared ownership of the creator. It is void since the manager does
 * not care what the actual object is. */
struct CategoryCreatorInfo
{
    std::string category;
    std::weak_ptr<void> creator;
    Private::GraphElementCreationCallback member;
};
typedef std::map<std::string, CategoryCreatorInfo> CategoryCreatorsMap;
} // namespace Private
} // namespace flitr

class Private::GraphManagerPrivate {
public:
    GraphElementPropertiesMap graphElements;
    GraphPathsMap graphPaths;
    CreatedGraphsMap createdGraphs;
    ProducersMap producers;
    ConsumersMap consumers;

    CategoryCreatorsMap creators;
    GraphManagerPrivate() {}
};
//--------------------------------------------------
//--------------------------------------------------
//--------------------------------------------------

GraphManager::GraphManager()
    : d(new Private::GraphManagerPrivate())
{

}
//--------------------------------------------------

GraphManager::~GraphManager()
{
    delete d;
}
//--------------------------------------------------

GraphManager *GraphManager::instance()
{
    if(d_instance == nullptr) {
        d_instance = new GraphManager();
    }
    return d_instance;
}
//--------------------------------------------------

bool GraphManager::addGraphElementInformation(const std::string& name, const std::string& category, const AttributeVector& attributes)
{
    if((name.empty() == true)
            || (category.empty() == true)) {
        return false;
    }
    if(d->graphElements.count(name) != 0) {
        logMessage(flitr::LOG_CRITICAL) << "Duplicate \'graph_element\' name: " << name << " (" << category << ")" << std::endl;
        return false;
    }

    d->graphElements.insert({name, GraphElementProperties{name, category, attributes}});
    return true;
}
//--------------------------------------------------

bool GraphManager::loadGraphElementsInformation(const std::shared_ptr<flitr::XMLConfig> &cfg)
{
    flitr::logMessage(flitr::LOG_INFO) << "Starting to load graph elements or vertices." << std::endl;

    const AttributeVectorVector graphElementsAttributesVector = cfg->getAttributeVector("graph_element");

    for(const AttributeVector& elementAttributes: graphElementsAttributesVector) {
        std::string elementName;
        std::string elementCategory;

        for(const Attribute &attribute: elementAttributes) {
            if(attribute.first == "name") {
                elementName = attribute.second;
            } else if(attribute.first == "category") {
                elementCategory = attribute.second;
            }
        }

        /* Add the element information. */
        bool added = addGraphElementInformation(elementName, elementCategory, elementAttributes);
        if(added == true) {
            logMessage(flitr::LOG_DEBUG) << "-- Added graph element: " << elementName << " (" << elementCategory << ")" << std::endl;
        } else {
            logMessage(flitr::LOG_DEBUG) << "-- Failed to add graph element: " << elementName << " (" << elementCategory << ")\n"
                                         << "    - Check that the \'name\' and \'category\' is valid for the \'graph_element\' and that the name is unique." << std::endl;
        }
    }
    return loadGraphPathsInformation(cfg);
}

bool GraphManager::replaceGraphElementsInformation(const std::shared_ptr<flitr::XMLConfig> &cfg, const std::string &replaceElementName)
{
    flitr::logMessage(flitr::LOG_INFO) << "Starting to replace graph elements or vertices." << std::endl;

    const AttributeVectorVector graphElementsAttributesVector = cfg->getAttributeVector("graph_element");

    for(const AttributeVector& elementAttributes: graphElementsAttributesVector) {
        std::string elementName;
        std::string elementCategory;

        for(const Attribute &attribute: elementAttributes) {
            if(attribute.first == "name") {
                elementName = attribute.second;
            } else if(attribute.first == "category") {
                elementCategory = attribute.second;
            }
        }

        /* Replace the element information. */
        if (replaceElementName == elementName)
        {
            GraphElementPropertiesMap::iterator elementMapPropertiesIter = d->graphElements.find(elementName);
            if (elementMapPropertiesIter != d->graphElements.end())  
            {
                elementMapPropertiesIter->second = GraphElementProperties{elementName, elementCategory, elementAttributes};
            }  

        }

    }
    return loadGraphPathsInformation(cfg);
}
//--------------------------------------------------

bool GraphManager::addGraphPathInformation(const std::string& name, const std::string& producerName, const std::string& consumerName)
{
    /* Check the values */
    if((name.empty() == true)
            || (producerName.empty() == true)
            || (consumerName.empty() == true)) {
        return false;
    }

    d->graphPaths[name].push_back(GraphPath{producerName, consumerName});
    return true;
}
//--------------------------------------------------

bool GraphManager::loadGraphPathsInformation(const std::shared_ptr<flitr::XMLConfig> &cfg)
{
    logMessage(flitr::LOG_INFO) << "Starting to load graph paths information." << std::endl;

    const AttributeVectorVector pathsAttributesVector = cfg->getAttributeVector("graph_path");

    for(const AttributeVector& pathAttributes: pathsAttributesVector) {
        std::string graphName;
        std::string producerName;
        std::string consumerName;

        for(const Attribute &attribute: pathAttributes) {
            if(attribute.first == "graph_name") {
                graphName = attribute.second;
            } else if(attribute.first == "producer_name") {
                producerName = attribute.second;
            } else if(attribute.first == "consumer_name") {
                consumerName = attribute.second;
            }
        }

        bool added = addGraphPathInformation(graphName, producerName, consumerName);
        if(added == true) {
            logMessage(flitr::LOG_DEBUG) << "-- Added path: " << graphName << " (" << producerName << " -> " << consumerName << ")" << std::endl;
        } else {
            logMessage(flitr::LOG_DEBUG) << "-- Failed to add graph path: " << graphName << " (" << producerName << " -> " << consumerName << ")\n"
                                         << "    - Check that the \'graph_name\', \'producer_name\' and \'consumer_name\' is valid for the \'graph_path\'." << std::endl;
        }
    }

    return loadGraphsInformation(cfg);
}
//--------------------------------------------------

bool GraphManager::loadGraphsInformation(const std::shared_ptr<flitr::XMLConfig> &cfg)
{
    logMessage(flitr::LOG_INFO) << "Starting to load graphs." << std::endl;

    const AttributeVectorVector graphsAttributesVector = cfg->getAttributeVector("graph");

    for(const AttributeVector& graphAttributes: graphsAttributesVector) {
        std::string graphName;

        for(const Attribute &attribute: graphAttributes) {
            if(attribute.first == "name") {
                graphName = attribute.second;
            }
        }

        /* Check the values. All of these are checked in createGraph() but also checked
         * here for custom logging messages. */
        if(graphName == "") {
            logMessage(flitr::LOG_CRITICAL) << "\'name\' not set or invalid for the \'graph\'" << std::endl;
            continue;
        }

        logMessage(flitr::LOG_DEBUG) << "-- Creating graph: " << graphName << std::endl;
        if(d->graphPaths.count(graphName) == 0) {
            logMessage(flitr::LOG_CRITICAL) << "    - Invalid graph" << graphName << "\n"
                                            << "    - Check that there are valid \'graph_path\' and \'graph_element\' for the given graph." << std::endl;
            continue;
        }
        createGraph(graphName);
    }

    /* TODO: Perhaps check that all names in the links are valid prosumers */

    return true;
}
//--------------------------------------------------

/* Helper function to get a Producer or Consumer from the member map.
 * A template function is used to minimise code duplication for better
 * maintainability. */
template <class ElementType>
std::shared_ptr<ElementType> getElement(std::map<std::string, std::weak_ptr<ElementType>>& map, const std::string& type, const std::string& name)
{
    typedef std::map<std::string, std::weak_ptr<ElementType>> ElementMap;
    typename std::shared_ptr<ElementType> element;
    typename ElementMap::iterator iter = map.find(name);
    if(iter != map.end()) {
        /* Get the element from the list. Since it is a weak pointer
         * there might be a need to remove it from the list if the
         * object was destroyed. This should not happen however. */
        element = iter->second.lock();
        logMessage(flitr::LOG_DEBUG) << type << " in list: " << name << std::endl;
        if(element == nullptr) {
            logMessage(flitr::LOG_DEBUG) << type << " was in list, but not valid any more: " << name << std::endl;
            /* remove it from the list since it is invalid */
            map.erase(iter);
        }
    } else {
        /* Not in list, must be created*/
        logMessage(flitr::LOG_DEBUG) << type << " not in list: " + name << std::endl;
    }
    return element;
}
//--------------------------------------------------
#define getProducer(map, name) getElement(map, "Producer", name)
#define getConsumer(map, name) getElement(map, "Consumer", name)

bool GraphManager::createGraph(const std::string &graphName)
{
    if(graphName.empty() == true) {
        return false;
    }
    logMessage(flitr::LOG_INFO) << "Starting to create graph: " << graphName << std::endl;
    /* First check that the graph is not already created */
    if(d->createdGraphs.count(graphName) != 0) {
        logMessage(flitr::LOG_CRITICAL) << "Requesting to create a graph that is already created: " << graphName << std::endl;
        return false;
    }
    GraphPathsMap::iterator graphIter = d->graphPaths.find(graphName);
    if(graphIter == d->graphPaths.end()) {
        logMessage(flitr::LOG_CRITICAL) << "No graph information for the requested graph : " << graphName << std::endl;
        return false;
    }
    const GraphPathVector links = graphIter->second;
    if(links.size() == 0) {
        logMessage(flitr::LOG_CRITICAL) << "The graph to be created has no valid paths: " << graphName << std::endl;
        return false;
    }
    logMessage(flitr::LOG_INFO) << "Number of paths in the graph: " << links.size() << std::endl;

    CreatedGraphProperties createdGraph;
    /* Create the links for the graph.
    * Failing to create a consumer will continue to the next path. Failure to
    * create a producer will fail creating the path, since a producer is
    * needed for a consumer. */
    for(const GraphPath& link: links) {
        const std::string &producerName = link.first;
        const std::string &consumerName = link.second;
        std::shared_ptr<flitr::ImageProducer> producer;
        std::shared_ptr<flitr::ImageConsumer> consumer;
        producer = getProducer(d->producers, producerName);

        if(producer == nullptr) {
            /* No producer was obtained from the previously created producers. Try to
             * create the producer using the added information for the element. */
            logMessage(flitr::LOG_DEBUG) << "Attempting to create producer: " << producerName << std::endl;
            bool created = createElement(producerName, producer, consumer);
            if(created == false) {
                logMessage(flitr::LOG_CRITICAL) << "Failed to create the producer: " << producerName << std::endl;
                return false;
            }
            logMessage(flitr::LOG_DEBUG) << "Producer successfully created: " << producerName << std::endl;
        }
        /* Check to see if the consumer is now valid. */
        if(producer == nullptr) {
            logMessage(flitr::LOG_CRITICAL) << "Could not create or get a valid instance for the specified producer: " << producerName << std::endl;
            return false;
        }
        /* Add it to the local structure */
        createdGraph.producers.push_back(producer);
        /* Add a global weak pointer */
        d->producers.insert({producerName, producer});

        /* Reset the consumer for in case. */
        consumer = getConsumer(d->consumers, consumerName);
        std::shared_ptr<flitr::ImageProducer> upstreamProducer = producer;
        if(consumer == nullptr) {
            /* No consumer was obtained from the previously created consumers. Try to
             * create the consumer using the added information for the element. */
            logMessage(flitr::LOG_DEBUG) << "Attempting to create consumer: " << consumerName << std::endl;
            bool created = createElement(consumerName, upstreamProducer, consumer);
            if(created == false) {
                logMessage(flitr::LOG_CRITICAL) << "Failed to create the consumer: " << consumerName << std::endl;
                continue;
            }
            logMessage(flitr::LOG_DEBUG) << "Consumer successfully created: " << consumerName << std::endl;
        }
        /* Check to see if the consumer is now valid. */
        if(consumer == nullptr) {
            logMessage(flitr::LOG_CRITICAL) << "Could not create or get a valid instance for the specified consumer: " << consumerName << std::endl;
            continue;
        }
        /* Add it to the local structure */
        createdGraph.consumers.push_back(consumer);
        /* Add a global weak pointer */
        d->consumers.insert({consumerName, consumer});

        if((upstreamProducer != nullptr)
                && (upstreamProducer != producer)) {
            logMessage(flitr::LOG_DEBUG) << "Created an Image Processor: " << consumerName << std::endl;
            /* The last consumer created is also a producer. Thus it has to be added to the
             * list of producers. */
            createdGraph.producers.push_back(upstreamProducer);
            /* Add a global weak pointer */
            d->producers.insert({consumerName, upstreamProducer});
        }
    }
    /* At this point the graph should be valid, add it to the member variable */
    d->createdGraphs[graphName] = createdGraph;
    return true;
}
//--------------------------------------------------

/* Helper function to erase elements from a map since std::remove_if is not
 * valid for maps. */
template <class ContainerT, typename PredicateT>
void eraseElementsIf(ContainerT& container, const PredicateT& predicate)
{
    auto iter = container.begin();
    while(iter != container.end()) {
        if(predicate(*iter) == true)  {
            iter = container.erase(iter);
        } else {
            ++iter;
        }
    }
}
//--------------------------------------------------

bool GraphManager::destroyGraph(const std::string &graphName)
{
    if(graphName.empty() == true) {
        return false;
    }
    logMessage(flitr::LOG_INFO) << "Starting to destroy created graph: " << graphName << std::endl;
    /* Check the list of created graphs */
    CreatedGraphsMap::iterator graphIter = d->createdGraphs.find(graphName);
    if(graphIter == d->createdGraphs.end()) {
        logMessage(flitr::LOG_CRITICAL) << "Requesting to destroy a graph that was not created: " << graphName << std::endl;
        return false;
    }

    /* Erase the graph from the list of created ones. This will cause all
     * strong pointers to decrement their counters and destroy them if the
     * last reference was removed. */
    d->createdGraphs.erase(graphIter);
    /* Now remove all empty weak pointers from the member vectors */
    eraseElementsIf(d->producers, [](const std::pair<std::string, std::weak_ptr<flitr::ImageProducer>>& item){ return (item.second.lock() == nullptr); });
    eraseElementsIf(d->consumers, [](const std::pair<std::string, std::weak_ptr<flitr::ImageConsumer>>& item){ return (item.second.lock() == nullptr); });

    return true;
}
//--------------------------------------------------

std::string GraphManager::producerName(const std::shared_ptr<ImageProducer>& producer) const
{
    for(ProducersMap::value_type value: d->producers) {
        std::shared_ptr<ImageProducer> pStrong = value.second.lock();
        if(pStrong == nullptr) {
            continue;
        }
        if(pStrong == producer) {
            return value.first;
        }
    };
    return "";
}
//--------------------------------------------------

std::string GraphManager::consumerName(const std::shared_ptr<ImageConsumer>& consumer) const
{
    for(ConsumersMap::value_type value: d->consumers) {
        std::shared_ptr<ImageConsumer> cStrong = value.second.lock();
        if(cStrong == nullptr) {
            continue;
        }
        if(cStrong == consumer) {
            return value.first;
        }
    };
    return "";
}
//--------------------------------------------------

bool GraphManager::createElement(const std::string &elementName, std::shared_ptr<flitr::ImageProducer> &producer, std::shared_ptr<flitr::ImageConsumer> &consumer)
{
    GraphElementPropertiesMap::iterator elementInfo = d->graphElements.find(elementName);
    if(elementInfo == d->graphElements.end()) {
        logMessage(flitr::LOG_CRITICAL) << "The Graph Manager does not have any information for the specified element to be created: " << elementName << std::endl;
        return false;
    }

    const GraphElementProperties& properties = elementInfo->second;
    logMessage(flitr::LOG_DEBUG) << "-- Element to create: " << properties.name << std::endl;
    logMessage(flitr::LOG_DEBUG) << "     category: " << properties.category << std::endl;

    CategoryCreatorsMap::iterator creatorIter = d->creators.find(properties.category);
    if(creatorIter == d->creators.end()) {
        logMessage(flitr::LOG_CRITICAL) << "No creator function registered for element category: " << properties.category << std::endl;
        return false;
    }

    /* Create a copy of the creator info. */
    const CategoryCreatorInfo& creatorInfo = creatorIter->second;
    /* First lock the weak ptr to see if the object still exists and to keep it alive if it does */
    std::shared_ptr<void> strong = creatorInfo.creator.lock();
    if(strong == nullptr) {
        /* The object got destroyed at some point */
        logMessage(flitr::LOG_CRITICAL) << "The registered object to create the category is not valid any more: " << properties.category << std::endl;
        return false;
    }
    /* Now that it is locked, the object should stay alive while the element is created. */
    bool created = creatorInfo.member(properties.category, properties.attributes, producer, consumer);
    if(created == false) {
        logMessage(flitr::LOG_CRITICAL) << "The creator could not create the element: " << properties.category << std::endl;
        return false;
    }

    return true;
}
//--------------------------------------------------

bool GraphManager::registerGraphElementCategoryImp(const std::string &category, std::shared_ptr<void> receiver, Private::GraphElementCreationCallback member)
{
    if(d->creators.count(category) != 0) {
        return false;
    }

    d->creators.insert({category, CategoryCreatorInfo{category, receiver, member}});
    return true;
}
//--------------------------------------------------
