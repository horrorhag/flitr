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

//#Temp includes
#include <stdio.h>
#include <iostream>
#include <unistd.h>

#include <flitr/modules/flitr_image_processors/openCV_processor/fip_opencv_processor.h>


/////////////////////////////////////////
/*OpenCV*/
#ifdef FLITR_USE_OPENCV
    #include <opencv2/core/core.hpp>
    #include <opencv2/imgproc/imgproc.hpp>
    #include <opencv2/highgui/highgui.hpp>

    #include <opencv/cxcore.h>
#endif

//using namespace std;

using std::cout;
using std::endl;

using namespace flitr;
using std::shared_ptr;

FIP_OpenCVProcessors::FIP_OpenCVProcessors(ImageProducer& upStreamProducer,
               uint32_t images_per_slot,
               uint32_t buffer_size,
               OpenCVProcessors::ProcessorType cvProcessorType,
               std::map<std::string, std::string> cvProcessorParams
):ImageProcessor(upStreamProducer, images_per_slot, buffer_size)
{
    _cvProcessorType = cvProcessorType;
    _cvProcessorParams = cvProcessorParams;

    //Setup image format being produced to downstream.
    for (uint32_t i=0; i<images_per_slot; i++) {
        //ImageFormat(uint32_t w=0, uint32_t h=0, PixelFormat pix_fmt=FLITR_PIX_FMT_Y_8, bool flipV = false, bool flipH = false):
        ImageFormat downStreamFormat(upStreamProducer.getFormat().getWidth(), upStreamProducer.getFormat().getHeight(),
                                     upStreamProducer.getFormat().getPixelFormat());

        ImageFormat_.push_back(downStreamFormat);
    }

    ///Create the instance for OpenCVProcessors()
    _cvProcessors = OpenCVProcessors();

}

FIP_OpenCVProcessors::~FIP_OpenCVProcessors()
{
    //#delete cvImage;
}

bool FIP_OpenCVProcessors::init()
{
    bool rValue=ImageProcessor::init();
    //Note: SharedImageBuffer of downstream producer is initialised with storage in ImageProcessor::init.

    return rValue;
}


using std::shared_ptr;
using namespace flitr;

class BackgroundTriggerThread : public FThread {
public:
    BackgroundTriggerThread(ImageProducer* p) :
    Producer_(p),
    ShouldExit_(false) {}

    void run()
    {
        while(!ShouldExit_)
        {
            //#if (!Producer_->trigger()) FThread::microSleep(5000);
            if (!Producer_->trigger()) FThread::microSleep(5000);
        }
    }

    void setExit() { ShouldExit_ = true; }

private:
    ImageProducer* Producer_;
    bool ShouldExit_;
};


bool FIP_OpenCVProcessors::trigger()
{
    if ((getNumReadSlotsAvailable())&&(getNumWriteSlotsAvailable()))
    {//There are images to consume and the downstream producer has space to produce.
        std::vector<Image**> imvRead=reserveReadSlot();
        std::vector<Image**> imvWrite=reserveWriteSlot();

        //Start stats measurement event.
        ProcessorStats_->tick();

        for (size_t imgNum=0; imgNum<1; ++imgNum)//For now, only process one image in each slot.
        {
            imageNum++;   //# CV image count
            const ImageFormat imFormat=getDownstreamFormat(imgNum);

            //#if (imFormat.getPixelFormat()==ImageFormat::FLITR_PIX_FMT_Y_8){
                Image const * const imRead = *(imvRead[imgNum]);
                Image * const imWrite = *(imvWrite[imgNum]);

                uint8_t const * const dataRead=imRead->data();
                //uint8_t * const dataWrite=imWrite->data();

                uint8_t * const dataWrite = (uint8_t *const) imWrite->data();
                //float * const dataWrite = (float * const)imWrite->data();

                const size_t width=imFormat.getWidth();
                const size_t height=imFormat.getHeight();


                //============================================
                /// Copy flitr image to OpenCV
                flitr::OpenCVHelperFunctions cvHelpers = flitr::OpenCVHelperFunctions();
                Mat cvImage = cvHelpers.copyToOpenCVimage(imvRead, imFormat);

                /// Apply OpenCV processors
                bool runDefaultParams = false;
                {
                    //Use custom processor parameters:
                    if(!_cvProcessorParams.empty()){

                        ///Backgound Subtraction
                        if(_cvProcessorType == OpenCVProcessors::backgroundSubtraction){
                            int backgroundUpdateInterval = atoi(_cvProcessorParams["backgroundUpdateInterval"].c_str());
                            if(backgroundUpdateInterval > 0)
                                _cvProcessors.applyBackgroundSubtraction(cvImage, backgroundUpdateInterval );
                            else
                                runDefaultParams = true;
                        }


                    }else{
                        runDefaultParams = true;
                    }

                    //Use default processor parameters:
                    if(runDefaultParams)
                        _cvProcessors.applyDefaultProcessor(cvImage, _cvProcessorType );

                }


                /// Copy read image back to OSG viewer
                cvHelpers.copyToFlitrImage(cvImage, imvWrite,imFormat);


           //# if}

        }

        //Stop stats measurement event.
        ProcessorStats_->tock();

        releaseWriteSlot();
        releaseReadSlot();

        return true;
    }

    return false;
}
