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

#include <flitr/raw_video_file_reader.h>
#include <flitr/raw_video_file_utils.h>

#include <sstream>

using namespace flitr;
using std::tr1::shared_ptr;

RawVideoFileReader::RawVideoFileReader(std::string filename) :
    FrameRate_(FLITR_DEFAULT_VIDEO_FRAME_RATE),
    FileName_(filename),
    File_(NULL)
{
    std::stringstream getimage_stats_name;
    getimage_stats_name << filename << " RawVideoFileReader::getImage";
    GetImageStats_ = shared_ptr<StatsCollector>(new StatsCollector(getimage_stats_name.str()));

    SingleImage_ = shared_ptr<Image>(new Image(ImageFormat_));
    CurrentImage_ = -1;

    bool opened = openVideoFile();
    if(opened == false)
    {
        throw RawVideoFileReaderException();
    }
}

RawVideoFileReader::~RawVideoFileReader()
{
    fclose(File_);
}

bool RawVideoFileReader::openVideoFile()
{
    File_ = fopen(FileName_.c_str(), "rb");
    if(File_ == NULL)
    {
        logMessage(LOG_CRITICAL) << "Cannot open the raw video file for playback: " <<  FileName_<< std::endl;
        logMessage(LOG_CRITICAL).flush();
        return false;
    }

    FileHeader fileHeader;
    fread(&fileHeader, 1, sizeof(fileHeader), File_);
    /* Check to make sure that this is a correct video file */
    if(fileHeader.startChar != FILE_HEADER_START_CHAR)
    {
        logMessage(LOG_CRITICAL) << "Video File not a valid FLITr Video File: " <<  FileName_<< std::endl;
        logMessage(LOG_CRITICAL).flush();
        return false;
    }

    /* Check if this reader supports the current version of the video file */
    if(fileHeader.info.versionMajor > 0)
    {
        logMessage(LOG_CRITICAL) << "File Major version is too high for this version of FLITr: " <<  FileName_<< std::endl;
        logMessage(LOG_CRITICAL).flush();
        return false;
    }

    /* Check that the data size read from the file is at least the
     * size of this versions data size. If it is smaller it means that
     * the data does not contain all information needed by this version. */
    if(fileHeader.info.dataSize < sizeof(ImageFormatData))
    {
        logMessage(LOG_CRITICAL) << "File header data is too small to contain all needed information: " <<  FileName_<< std::endl;
        logMessage(LOG_CRITICAL).flush();
        return false;
    }

    /* Create the ImageFormat structure from the data read from the header. This
     * information is required for consumers consuming this video. */
    ImageFormat_ = ImageFormat(fileHeader.info.data.imageWidth
                               , fileHeader.info.data.imageHeight
                               , static_cast<ImageFormat::PixelFormat>(fileHeader.info.data.pixelFormat));

    BytesPerImage_ = ImageFormat_.getBytesPerImage(); /* Store the value so that it does not need to be calculated for each frame */
    FrameRate_ = fileHeader.info.data.frameRate;
    NumImages_ = fileHeader.info.data.numberFrames;

    /* Set the index of the file to the start of the first frame. This should
     * be to make sure that if the header size changed between versions that it
     * starts at the beginning of the video frames. */
    if(fileHeader.info.dataSize > sizeof(ImageFormatData))
    {
        int32_t seekOffset = fileHeader.info.dataSize - sizeof(ImageFormatData);
        fseek(File_, seekOffset, SEEK_CUR);

        fread(&fileHeader.info.stopChar, 1, sizeof(fileHeader.info.stopChar), File_);
        fread(&fileHeader.stopChar, 1, sizeof(fileHeader.stopChar), File_);
    }

    /* Make sure the info header end chars are correct */
    if(fileHeader.info.stopChar != HEADER_STOP_CHAR)
    {
        logMessage(LOG_CRITICAL) << "Info Header not terminated correctly: " <<  FileName_<< std::endl;
        logMessage(LOG_CRITICAL).flush();
        return false;
    }

    /* Make sure the file header end chars are correct */
    if(fileHeader.stopChar != FILE_HEADER_STOP_CHAR)
    {
        logMessage(LOG_CRITICAL) << "File Header not terminated correctly: " <<  FileName_<< std::endl;
        logMessage(LOG_CRITICAL).flush();
    }
    FirstFramePos_ = ftell(File_);
    return true;
}

bool RawVideoFileReader::getImage(Image &out_image, int im_number)
{
    GetImageStats_->tick();

    /* Check to see if we need to get the next image from the last
     * location that we were at. If this is the case then we do
     * not need to search for a frame, just continue where we were
     * last. */
    if((CurrentImage_ + 1) != im_number)
    {
        /* First seek to the first frame position in the file. Then calculate
         * the offset to the requesed frame in the file and seek to it. */
        fseek(File_, FirstFramePos_, SEEK_SET);
        uint32_t frameSize = BytesPerImage_ + sizeof(FRAME_START) + sizeof(FRAME_END);
        uint32_t seekPos = frameSize * im_number;
        fseek(File_, seekPos, SEEK_CUR);
    }

    uint32_t startFrameMarker = 0x00;
    uint32_t stopFrameMarker = 0x00;
    /* Check to make sure that the start frame marker is correct. This is needed to
     * make sure that the video is correct and the image offsets are not corrupted. */
    fread(&startFrameMarker, 1, sizeof(startFrameMarker), File_);
    if(startFrameMarker != FRAME_START)
    {
        std::cout << "Start Frame Marker is not correct for current frame!!!" << std::endl;
        std::cout.flush();
        return false;
    }

    /* Read the image from the file */
    fread(&out_image.data()[0], 1, BytesPerImage_, File_);
    /* Check to make sure that the end frame marker is correct. */
    fread(&stopFrameMarker, 1, sizeof(stopFrameMarker), File_);
    if(stopFrameMarker != FRAME_END)
    {
        std::cout << "Stop Frame Marker is not correct for current frame!!!" << std::endl;
        std::cout.flush();
        return false;
    }

    CurrentImage_ = im_number;
    GetImageStats_->tock();
    return true;
}
