#include <flitr/modules/xml_config/xml_config.h>
#include <string>
#include <iomanip>
#include <limits>

using namespace flitr;

bool XMLConfig::init(std::string filename)
{
    TiXmlBase::SetCondenseWhiteSpace(false);

    XMLFile_ = filename;
    // try to load the file
    if (!Doc_.LoadFile(XMLFile_)) {
        logMessage(flitr::LOG_CRITICAL) << "Cannot load " << XMLFile_ << std::endl;
        logMessage(flitr::LOG_CRITICAL).flush();
        return false;
    }
	
    RootElement_ = Doc_.RootElement();
    if (!RootElement_) {
        logMessage(LOG_CRITICAL) << "Cannot find root node in " << XMLFile_ << std::endl;
        logMessage(flitr::LOG_CRITICAL).flush();
        return false;
    }
    return true;
}

bool XMLConfig::save(std::string filename)
{
    if (!Doc_.SaveFile(filename)) {
        logMessage(LOG_CRITICAL) << "Cannot save " << filename << std::endl;
        logMessage(flitr::LOG_CRITICAL).flush();
        return false;
    }
    return true;
}

template<class T> T XMLConfig::getDefault(std::string element, std::string attribute, T dflt) 
{
    logMessage(LOG_DEBUG) << "Cannot find or read attribute (" << attribute << ") of element (" << element << ") in " << XMLFile_ << ". Using default value (" << dflt << ").\n";
    logMessage(flitr::LOG_DEBUG).flush();
    return dflt;
}

std::string XMLConfig::getString(std::string element, std::string attribute, std::string dflt)
{
    if (!RootElement_) return getDefault<std::string>(element, attribute, dflt);
    TiXmlElement* dataElement = RootElement_->FirstChildElement(element);
    if (!dataElement) return getDefault<std::string>(element, attribute, dflt);
    if (dataElement->Attribute(attribute) == 0) return getDefault<std::string>(element, attribute, dflt);
 
    std::string val = *dataElement->Attribute(attribute);
    return val;
}

std::vector<std::string> XMLConfig::getStringVector(std::string element, std::string attribute)
{
    std::vector<std::string> vec;
    if (!RootElement_) return vec;//getDefault<std::string>(element, attribute, dflt);
    TiXmlElement* dataElement = RootElement_->FirstChildElement(element);
    if (!dataElement) return vec;//getDefault<std::string>(element, attribute, dflt);
    if (dataElement->Attribute(attribute) == 0) return vec;//getDefault<std::string>(element, attribute, dflt);

    do
    {
        std::string val = *dataElement->Attribute(attribute);
        vec.push_back(val);
        dataElement = dataElement->NextSiblingElement(element);
    } while (dataElement && dataElement->Attribute(attribute));

    return vec;
}


std::vector<std::vector<std::pair<std::string, std::string> > > XMLConfig::getAttributeVector(const std::string &element)
{
    std::vector<std::vector<std::pair<std::string, std::string> > > vecvec;
    if (!RootElement_)
    {
        return vecvec;
    }

    TiXmlElement const *dataElement = RootElement_->FirstChildElement(element);

    if (!dataElement)
    {
        return vecvec;
    }

    do
    {
        std::vector<std::pair<std::string, std::string> > vec;

        TiXmlAttribute const *att = dataElement->FirstAttribute();
        while (att != nullptr)
        {
            const std::string strname = att->NameTStr(); // TiXMLString could be redefined
            const std::string strval  = att->ValueStr();
            vec.push_back(std::make_pair(strname, strval));

            att = att->Next();
        }

        dataElement = dataElement->NextSiblingElement(element);

        vecvec.push_back(vec);
    } while (dataElement);

    return vecvec;
}

bool XMLConfig::writeAttributeVector(const std::string &element, const std::vector<std::vector<std::pair<std::string, std::string> > > &vecvec)
{
    if (!RootElement_)
    {
        return false;
    }

    for (auto eventList:vecvec)
    {
        TiXmlElement *dataElement = dynamic_cast<TiXmlElement *>(RootElement_->InsertEndChild(TiXmlElement(element)));
        if (!dataElement)
        {
            return false;
        }

        for (auto attributeList:eventList)
        {
            dataElement->SetAttribute(attributeList.first, attributeList.second);
        }

        dataElement = dataElement->NextSiblingElement(element);
        
    }

    return true;
}

bool XMLConfig::setString(std::string element, std::string attribute, std::string newval)
{
    if (!RootElement_) return false;
    TiXmlElement* dataElement = RootElement_->FirstChildElement(element);
    if (!dataElement) {
        TiXmlElement newElement(element);
        dataElement = dynamic_cast<TiXmlElement*>(RootElement_->InsertEndChild(newElement));
        if (!dataElement) return false;
    }
    dataElement->SetAttribute(attribute, newval);
    return true;
}

int XMLConfig::getInt(std::string element, std::string attribute, int dflt)
{
    int val;
    if (!RootElement_) return getDefault<int>(element, attribute, dflt);
    TiXmlElement* dataElement = RootElement_->FirstChildElement(element);
    if (!dataElement) return getDefault<int>(element, attribute, dflt);
    if (dataElement->Attribute(attribute) == 0) return getDefault<int>(element, attribute, dflt);
    
    std::stringstream i(*dataElement->Attribute(attribute));
    if (!(i >> val)) return getDefault<int>(element, attribute, dflt);

    return val;
}

bool XMLConfig::setInt(std::string element, std::string attribute, int newval)
{
    if (!RootElement_)
    {
        return false;
    }

    TiXmlElement* dataElement = RootElement_->FirstChildElement(element);

    if (!dataElement)
    {
        TiXmlElement newElement(element);
        dataElement = dynamic_cast<TiXmlElement*>(RootElement_->InsertEndChild(newElement));
        if (!dataElement) return false;
    }

    dataElement->SetAttribute(attribute, newval);
    return true;
}

double XMLConfig::getDouble(std::string element, std::string attribute, double dflt)
{
    double val;
    if (!RootElement_)
    {
        return getDefault<double>(element, attribute, dflt);
    }

    TiXmlElement* dataElement = RootElement_->FirstChildElement(element);

    if (!dataElement)
    {
        return getDefault<double>(element, attribute, dflt);
    }

    if (dataElement->Attribute(attribute) == 0)
    {
        return getDefault<double>(element, attribute, dflt);
    }
    
    std::stringstream i(*dataElement->Attribute(attribute));

    if (!(i >> val))
    {
        return getDefault<double>(element, attribute, dflt);
    }

    return val;
}

bool XMLConfig::setDouble(std::string element, std::string attribute, double newval)
{
    std::ostringstream ss;
    ss << std::setprecision(std::numeric_limits<double>::digits10) << newval;
    
    if (!RootElement_) return false;
    TiXmlElement* dataElement = RootElement_->FirstChildElement(element);
    if (!dataElement) {
        TiXmlElement newElement(element);
        dataElement = dynamic_cast<TiXmlElement*>(RootElement_->InsertEndChild(newElement));
        if (!dataElement) return false;
    }
    dataElement->SetAttribute(attribute, ss.str());
    return true;
}

bool XMLConfig::setRootElement(std::string rootElement)
{
    TiXmlElement* dataElement(new TiXmlElement(rootElement));
    RootElement_ = dataElement;
    Doc_.LinkEndChild(RootElement_);
    return true;
}

std::vector<double> XMLConfig::getVector(std::string element, std::string attribute, int numElements)
{
	std::vector<double> val;
	if (!RootElement_) {
		printf("Root Element not found\n");
		val.assign(numElements, 0.0);
	   	return val;
	}
	TiXmlElement* dataElement = RootElement_->FirstChildElement(element);
	if (!dataElement) {
		printf("Data Element not found\n");
		val.assign(numElements, 0.0);
	   	return val;
	}
	if (dataElement->Attribute(attribute) == 0) {
		printf("Attribute not found\n");
		val.assign(numElements, 0.0);
	   	return val;
	}

	std::stringstream inval(*dataElement->Attribute(attribute));

	for (int i = 0; i < numElements; i++) {
	  double x;
	  inval >> x;
	  val.push_back(x);
	}
	return val;
}
  

bool XMLConfig::elementTextToSStream(std::string element, std::stringstream& ss)
{
    if (!RootElement_) return false;
    TiXmlElement* dataElement = RootElement_->FirstChildElement(element);
    if (!dataElement) return false;
    ss << dataElement->GetText();
    return true;
}
    
