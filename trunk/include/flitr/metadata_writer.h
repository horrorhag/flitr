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
    MetadataWriter(std::string filename);
    ~MetadataWriter();

    bool writeFrame(Image& in_frame);

  private:
    std::ofstream FileStream_;

    bool openFile();
    bool closeFile();
	
    std::string SaveFileName_;
};

}
#endif //METADATA_WRITER_H
