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

#ifndef FLITR_IMAGE_METADATA_H
#define FLITR_IMAGE_METADATA_H 1

#include <flitr/flitr_export.h>
#include <functional>
#include <memory>

#include <ostream>

namespace flitr {

class FLITR_EXPORT ImageMetadata {
  public:
    ImageMetadata() {}
    virtual ~ImageMetadata() {}
    virtual bool writeToStream(std::ostream& s) const = 0;
    virtual bool readFromStream(std::istream& s) const = 0;

    virtual ImageMetadata* clone() const = 0;

    virtual uint32_t getSizeInBytes() const = 0; // size when packed in stream.

    virtual std::string getString() const = 0; // used for printing when debugging, etc.
};

typedef std::function < std::shared_ptr<ImageMetadata> () > CreateMetadataFunction;
/*! Typedef for the PassMetadata function used by the ImageProcessor base class.
 *
 * The input is a shared pointer to image metadata and returns a pointer
 * to image metadata. The signature looks as follow:
 * @code
 * std::shared_ptr<ImageMetadata> someFunction(std::shared_ptr<ImageMetadata> input_metadata);
 * @endcode */
typedef std::function < std::shared_ptr<ImageMetadata> (std::shared_ptr<ImageMetadata>) > PassMetadataFunction;

}

#endif //FLITR_IMAGE_METADATA_H
