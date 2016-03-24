#ifndef XML_CONFIG_H
#define XML_CONFIG_H 1

#include <flitr/flitr_export.h>
#include <flitr/modules/xml_config/tinyxml.h>

#include <flitr/log_message.h>

#include <string>
#include <sstream>
#include <vector>
#include <utility>

/**
 *  This class is a thin wrapper around the TinyXML parser. An XML
 *  configuration file is read into memory and attributes of elements
 *  can easily be accessed.
 */

namespace flitr {

class FLITR_EXPORT XMLConfig
{
  public:
    /// No-op constructor.
    XMLConfig() {};
    /// No-op destructor.
    ~XMLConfig() {};

    /**
       \brief Read the configuration file specified and create an XML structure in memory.
       \param filename The configuration XML filename to read.
       \return true on success.
    */
    bool init(std::string filename);

    /**
       \brief Save the configuration structure in memory to the specified file.
       \param filename The configuration XML filename to save to.
       \return true on success.
    */
    bool save(std::string filename);

    /// Return a default and print a message about the missing element.
    template<class T> T getDefault(std::string element, std::string attribute, T dflt);

    /**
       \brief Get a string variable from the configuration file.
       \param element The XML element to look for.
       \param attribute The element attribute to look for.
       \param dflt Default value used if attribute is not found.
       \return Configuration string value.
    */
    std::string getString(std::string element, std::string attribute, std::string dflt);
    
    /**
       \brief Get a vector of string variables from the configuration file.
       \param element The XML element to look for.
       \param attribute The element attribute to look for.
       \return Configuration string values. Returns all elements.
    */
    std::vector<std::string> getStringVector(std::string element, std::string attribute);

    
    /**
       \brief Get a vector of elements (and each's attributes) with specified name from the configuration file.
       \param element The XML element to look for.
       \return Configuration string values. Returns all attributes of elements matching element.
    */
    std::vector<std::vector<std::pair<std::string, std::string> > > getAttributeVector(const std::string &element);


    /**
       \brief Write a vector of elements (and each's attributes) to the configuration file.
       \param element The XML element to look for.
       \param Configuration string values. With all attributes of elements belonging to name element.
       \return True on success, false on failure.
    */
    bool writeAttributeVector(const std::string &element, const std::vector<std::vector<std::pair<std::string, std::string> > > &vecvec);

    
    /**
       \brief Set the value of attribute in the configuration file.
       \param element The XML element to find or create.
       \param attribute The attribute to change.
       \param newval The new string value of the attribute.
       \return True if successful, false if not.
    */
    bool setString(std::string element, std::string attribute, std::string newval);

    /**
       \brief Get an integer variable from the configuration file.
       \param element The XML element to look for.
       \param attribute The element attribute to look for.
       \param dflt Default value used if attribute is not found.
       \return Configuration integer value.
    */
    int getInt(std::string element, std::string attribute, int dflt);

    /**
       \brief Set the value of attribute in the configuration file.
       \param element The XML element to find or create.
       \param attribute The attribute to change.
       \param newval The new int value of the attribute.
       \return True if successful, false if not.
    */
    bool setInt(std::string element, std::string attribute, int newval);

    /**
       \brief Get a double floating point variable from the configuration file.
       \param element The XML element to look for.
       \param attribute The element attribute to look for.
       \param dflt Default value used if attribute is not found.
       \return Configuration double value.
    */
    double getDouble(std::string element, std::string attribute, double dflt);

    /**
       \brief Set the value of attribute in the configuration file.
       \param element The XML element to find or create.
       \param attribute The attribute to change.
       \param newval The new double value of the attribute.
       \return True if successful, false if not.
    */
    bool setDouble(std::string element, std::string attribute, double newval);

    /**
       \brief Set the value of the root element when creating a new xml file.
       \param rootElement The root element of the file.
       \return True if successful, false if not.
    */
    bool setRootElement(std::string rootElement);
    
    /**
       \brief Obtain a vector from an xml file.
       \param element The XML element to find or create.
       \param attribute The attribute to change.
       \param newval The new double value of the attribute.
       \return the vector contained in the xml file.
    */
    std::vector<double> getVector(std::string element, std::string attribute, int numElements);
    
    bool elementTextToSStream(std::string element, std::stringstream& ss);
    
  private:
    /// TinyXML document class.
    TiXmlDocument Doc_;
    /// Pointer to the root element of the XML class.
    TiXmlElement* RootElement_;
    /// Filename initialised with
    std::string XMLFile_;
};

}

#endif // XML_CONFIG_H
