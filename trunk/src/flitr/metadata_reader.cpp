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

#include <flitr/metadata_reader.h>

using namespace flitr;

MetadataReader::MetadataReader(std::string filename) :
    LoadFileName_(filename)
{
	openFile();
}

MetadataReader::~MetadataReader()
{
	closeFile();
}

bool MetadataReader::openFile()
{
    FileStream_.open(LoadFileName_.c_str(), std::ios::in | std::ios::binary);

    if (!FileStream_.is_open()) {
		return false;
    }
	return true;
}

bool MetadataReader::closeFile()
{
    if (FileStream_.is_open()) {
        FileStream_.close();
    }
    return true;
}

bool MetadataReader::readFrame(Image& out_frame, uint32_t seek_to)
{
    if (FileStream_.is_open()) {
        if (out_frame.metadata()) {

            // Start at the beginning of the meta data stream once the end has been reached.
            if (FileStream_.eof())
            {
                FileStream_.seekg(0, std::ios::beg);
            }

            if (seek_to!=std::numeric_limits<uint32_t>::max())
            {
                FileStream_.seekg(seek_to * out_frame.metadata()->getSizeInBytes() , std::ios::beg);
            }

            out_frame.metadata()->readFromStream(FileStream_);
        }
        return true;
    }
    return false;
}
