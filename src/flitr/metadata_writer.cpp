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

#include <flitr/metadata_writer.h>

using namespace flitr;

MetadataWriter::MetadataWriter() :
    SaveFileName_("")
{

}

MetadataWriter::MetadataWriter(std::string filename) :
    SaveFileName_(filename)
{
    openFile();
}

MetadataWriter::~MetadataWriter()
{
    closeFile();
}

bool MetadataWriter::openFile()
{
    FileStream_.open(SaveFileName_.c_str(), std::ios::out | std::ios::binary);

    if (!FileStream_.is_open()) {
        return false;
    }
    return true;
}

bool MetadataWriter::closeFile()
{
    if (FileStream_.is_open()) {
        FileStream_.close();
    }
    return true;
}

bool MetadataWriter::writeFrame(Image& in_frame)
{
    if (FileStream_.is_open()) {
        if (in_frame.metadata()) {
            in_frame.metadata()->writeToStream(FileStream_);
//            std::cout << "MetadataWriter: " << in_frame.metadata()->getString();
//            std::cout.flush();
        }

        return true;
    }
    return false;
}
