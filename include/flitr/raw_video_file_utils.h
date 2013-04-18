/* Framework for Live Image Transformation (FLITr)
 * Copyright (c) 2013 CSIR
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

#ifndef RAW_VIDEO_FILE_UTILS_H
#define RAW_VIDEO_FILE_UTILS_H 1

#include <flitr/image_format.h>

namespace flitr {
/* This is already declared in ffmpeg_utils.h but should be moved since it is
 * not ffmpeg specific */
#define FLITR_DEFAULT_VIDEO_FRAME_RATE 20

const char FILE_HEADER_START_CHAR = 0xAA;
const char FILE_HEADER_STOP_CHAR = 0xFF;
const char HEADER_START_CHAR = 0xAB;
const char HEADER_STOP_CHAR = 0x5B;
const char FILE_END_CHAR = 0x55;
const uint32_t FRAME_START = 0xBABEFACE;
const uint32_t FRAME_END   = 0xDEADBEEF;

/* Make sure the compiler does not try to align the data to some
 * byte number. This is a problem when the structures get written/read
 * to/from the file. If the data is aligned it can cause problems with
 * direct memory writes using memcopy and fwrite. */
#pragma pack(1)

/**
 * Structure containing the the Image Format Data.
 *
 * This structure gets saved to the file and the read from the
 * file to recreate the image format. This data is essential in
 * the recreation of the original video stream from the video file
 */
struct ImageFormatData {
    uint32_t imageHeight;
    uint32_t imageWidth;
    uint32_t pixelFormat;
    uint32_t frameRate;
    uint32_t numberFrames;
};

/**
 * The InfoHeader struct
 *
 * Contains information about the consumer that created
 * the video. This gets written to the video file as part
 * of the header.
 *
 * The header contains a Major and Minor version. This is
 * used by producers that read from the file to check it
 * the version of the video file is supported by the
 * version of the producer. The idea is that producers should
 * always be able to support the current major version of the
 * producer and lower. It should be able to forward support
 * video file minor versions.
 *
 * It contains a header start ena stop marker so that
 * readers can check the validity of the info data. The header
 * stop marker and the size of the data can be used to align
 * newer readers with older recorders. If the minor version
 * of the file is not the same as the minor version of the
 * producer the producer can use the stop marker and the data
 * size to align the producer correctly to get access to the
 * images in the file.
 */
struct InfoHeader {
    InfoHeader()
        : startChar(HEADER_START_CHAR)
        , versionMajor(0)
        , versionMinor(1)
        , dataSize(sizeof(ImageFormatData))
        , stopChar(HEADER_STOP_CHAR){}

    char startChar;
    char versionMajor;
    char versionMinor;
    unsigned int dataSize; ///< Size of the data contained in the Data structure
    ImageFormatData data;
    char stopChar;
};

/**
 * The FileHeader struct
 *
 * Header written to the start of each Custom FLITr video recording
 * to indicate the format of the video and to make sure that the
 * video is supported by the FLITr Custom video readers.
 */
struct FileHeader {
    FileHeader()
        : startChar(FILE_HEADER_START_CHAR)
        , stopChar(FILE_HEADER_STOP_CHAR) {}

    char startChar;
    InfoHeader info;
    char stopChar;
};

/* Reset the packing to default */
#pragma pack()

}

#endif //RAW_VIDEO_FILE_UTILS_H
