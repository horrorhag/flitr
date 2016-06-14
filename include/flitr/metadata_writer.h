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

#ifndef METADATA_WRITER_H
#define METADATA_WRITER_H 1

#include <flitr/image.h>
#include <flitr/log_message.h>

#include <iostream>
#include <fstream>
#include <sstream>

namespace flitr {

class FLITR_EXPORT MetadataWriter {
  public:
    /**
     * Create the MetadataWriter with an empty file name.
     *
     * This constructor will not attempt to set a file name or open the file.
     * This constructor is useful when this class is extended by deriving from
     * it.
     */
    MetadataWriter();
    /**
     * Create the MetadataWriter and open the file specified with \a filename.
     *
     * When this constructor is used, the openFile() function will be
     * called from the constructor, as if it is not virtual due to the
     * way C++ handles virtual function calls from the constructor of a class.
     * Thus when deriving from this class and the openFile() function gets
     * re-implemented, use the default constructor instead and then call the
     * openFile() so that the overloaded implementation can be called.
     */
    MetadataWriter(std::string filename);
    virtual ~MetadataWriter();

    virtual bool writeFrame(Image& in_frame);

  protected:
    std::ofstream FileStream_;

    virtual bool openFile();
    virtual bool closeFile();

    std::string SaveFileName_;
};

}
#endif //METADATA_WRITER_H
