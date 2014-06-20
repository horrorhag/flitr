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

#include <flitr/raw_video_file_writer.h>

using namespace flitr;
using std::shared_ptr;

RawVideoFileWriter::RawVideoFileWriter(std::string filename, const ImageFormat& image_format, const uint32_t frame_rate) :
    ImageFormat_(image_format),
    SaveFileName_(filename),
    FrameRate_(frame_rate),
    WrittenFrameCount_(0),
    File_(NULL)
{
    std::stringstream writeframe_stats_name;
    writeframe_stats_name << filename << " RawVideoFileWriter::writeFrame";
    WriteFrameStats_ = shared_ptr<StatsCollector>(new StatsCollector(writeframe_stats_name.str()));
    VideoFrameSize_ = ImageFormat_.getBytesPerImage();

    openVideoFile();
}

RawVideoFileWriter::~RawVideoFileWriter()
{
    closeVideoFile();
}

bool RawVideoFileWriter::openVideoFile()
{
    /* Create the file and write the header data to the file */
    File_ = fopen(SaveFileName_.c_str(), "wb");
    if(File_ == NULL) 
    {
        logMessage(LOG_CRITICAL) << "Cannot open the raw video file: " <<  SaveFileName_<< std::endl;
        logMessage(LOG_CRITICAL).flush();
        throw RawVideoFileWriterException();
    }

    /* Write the header data to the file. The header data contains all of
     * the information needed by the RawVideoFileReader to recreate the
     * image stream from the file. */
    FileHeader_.info.data.imageHeight = ImageFormat_.getHeight();
    FileHeader_.info.data.imageWidth = ImageFormat_.getWidth();
    FileHeader_.info.data.pixelFormat = ImageFormat_.getPixelFormat();
    FileHeader_.info.data.frameRate = FrameRate_;
    FileHeader_.info.data.numberFrames = 100;

    std::cout.flush();

    writeFileHeader();

    return true;
}

bool RawVideoFileWriter::closeVideoFile()
{
    std::cout << "RawVideoFileWriter: Frames written to file: " << WrittenFrameCount_ << "\n";
    std::cout.flush();

    /* Write the file end char to the file. */
    fwrite(&FILE_END_CHAR, 1, sizeof(char), File_);
    fflush(File_);

    /* Write the number of frames to the header. Just rewrite the
     * old file header at the start of the file. It should work since
     * the header size should not change during execution */
    FileHeader_.info.data.numberFrames = WrittenFrameCount_;
    writeFileHeader();
    fclose(File_);

    return true;
}

void RawVideoFileWriter::writeFileHeader()
{
    rewind(File_);
    fwrite(&FileHeader_, 1, sizeof(FileHeader_), File_);
}

bool RawVideoFileWriter::writeVideoFrame(uint8_t *in_buf)
{
    WriteFrameStats_->tick();

    /* Frame Start */
    fwrite(&FRAME_START, 1, sizeof(FRAME_START), File_);
    /* Image buffer */
    fwrite(in_buf, 1, VideoFrameSize_, File_);
    /* Frame End */
    fwrite(&FRAME_END, 1, sizeof(FRAME_END), File_);

    WrittenFrameCount_++;
    WriteFrameStats_->tock();
    return true;
}

