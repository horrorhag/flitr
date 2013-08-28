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

#include <flitr/video_producer.h>
#include <flitr/image.h>
#include <stdlib.h>
#include "comutil.h"
#include "DSVL.h"

using namespace flitr;

namespace flitr
{
struct VideoParam
{
	DSVL_VideoSource* graphManager;
	MemoryBufferHandle g_Handle;
	bool bufferCheckedOut;
	__int64 g_Timestamp;
};
}

namespace
{
	static flitr::VideoParam* gVid = 0;
	const std::string config_default = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><dsvl_input><camera show_format_dialog=\"true\" friendly_name=\"PGR\"><pixel_format><RGB24 flip_h=\"true\" flip_v=\"false\"/></pixel_format></camera></dsvl_input>";
}

/**************************************************************
* Thread
***************************************************************/
void VideoProducer::VideoProducerThread::run()
{
    std::vector<Image**> aImages;
    //std::vector<uint8_t> rgb(producer->ImageFormat_.back().getBytesPerImage());

    while (!shouldExit)
    {
        // wait for video frame
        bool ready = true;

		//
		std::vector<uint8_t> rgb(producer->ImageFormat_.back().getBytesPerImage());
        do
        {
			if (gVid->bufferCheckedOut) 
			{
				if (FAILED(gVid->graphManager->CheckinMemoryBuffer(gVid->g_Handle)))
				{
					ready = false;
				}
				else
				{
					gVid->bufferCheckedOut = false;
				}
			}
			ready = false;
			if (gVid->graphManager->WaitForNextSample(0L) == WAIT_OBJECT_0)
			{
				unsigned char* pixelBuffer;
				if (!FAILED(gVid->graphManager->CheckoutMemoryBuffer(&(gVid->g_Handle), &pixelBuffer, NULL, NULL, NULL, &(gVid->g_Timestamp))))
				{
					gVid->bufferCheckedOut = true;
					ready = true;
					uint8_t* buffer_vid = static_cast<uint8_t*>(pixelBuffer);
					std::copy(buffer_vid, buffer_vid + producer->ImageFormat_.back().getBytesPerImage(), rgb.begin());
				}
			}
			
            if (!ready)
            {
                Thread::microSleep(1000);
            }

        } while (!ready && !shouldExit);

		if (gVid->bufferCheckedOut) 
		{
			if (!FAILED(gVid->graphManager->CheckinMemoryBuffer(gVid->g_Handle, true)))
			{
				gVid->bufferCheckedOut = false;
			}
		}

        // wait until there is space in the write buffer so that we can get a valid image pointer
        do
        {
            aImages = producer->reserveWriteSlot();
            if (aImages.size() != 1)
            {
                Thread::microSleep(1000);
            }
        } while ((aImages.size() != 1) && !shouldExit);

		if (!shouldExit)
		{
			Image* img = *(aImages[0]);
			uint8_t* buffer = img->data();
			std::copy(rgb.begin(), rgb.end(), buffer);
		}

		producer->releaseWriteSlot();

		Thread::microSleep(1000);
    }
}

/**************************************************************
* Producer
***************************************************************/

VideoProducer::VideoProducer()
	: thread(0)
	, imageSlots(10)
{
}

VideoProducer::~VideoProducer()
{
	if (!gVid && !gVid->graphManager) return;
	
	if (gVid->bufferCheckedOut) 
	{
		gVid->graphManager->CheckinMemoryBuffer(gVid->g_Handle, true);
		gVid->bufferCheckedOut = false;
	}

	// PRL 2005-09-21: Commented out due to issue where stopping the
	// media stream cuts off glut's periodic tasks, including functions
	// registered with glutIdleFunc() and glutDisplayFunc();
	//if(FAILED(vid->graphManager->Stop())) return (-1);

	if (gVid->bufferCheckedOut) 
		gVid->graphManager->CheckinMemoryBuffer(gVid->g_Handle, true);

	thread->setExit();
    thread->join();

	gVid->graphManager->Stop();
	delete gVid->graphManager;
	gVid->graphManager = NULL;
	free(gVid);
	gVid = 0;

	// COM should be closed down in the same context
	CoUninitialize();
}

bool VideoProducer::init()
{
	gVid = new flitr::VideoParam;
	gVid->bufferCheckedOut = false;

	CoInitialize(NULL);

	std::string _config;
	gVid->graphManager = new DSVL_VideoSource();
	if (_config.empty() || _config.find("<?xml") != 0) 
	{
		_config = config_default;
	} 
	
	char* c = new char[_config.size() + 1];
	std::copy(_config.begin(), _config.end(), c);
	c[_config.size()] = '\0';
	if (FAILED(gVid->graphManager->BuildGraphFromXMLString(c))) return false;
	if (FAILED(gVid->graphManager->EnableMemoryBuffer())) return false;

	
	if (gVid == 0) return false;
	if (gVid->graphManager == 0) return false;

	long frame_width;
	long frame_height;
	gVid->graphManager->GetCurrentMediaFormat(&frame_width, &frame_height, 0, 0);

	
	
	ImageFormat_.push_back(ImageFormat(frame_width, frame_height, ImageFormat::FLITR_PIX_FMT_BGR));

    SharedImageBuffer_ = std::tr1::shared_ptr<SharedImageBuffer>(new SharedImageBuffer(*this, imageSlots, 1));
    SharedImageBuffer_->initWithStorage();



	if (FAILED(gVid->graphManager->Run())) false;
		


	thread = new VideoProducerThread(this);
    thread->startThread();

	return true;
}