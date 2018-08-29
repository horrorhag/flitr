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

#include <flitr/modules/flitr_image_processors/classification/fip_classification.h>


/*Caffe and OpenCV*/
//#include <caffe.pb.h>
#ifdef FLITR_USE_CAFFE
    #include <caffe/caffe.hpp>
    #include <caffe/proto/caffe.pb.h>
#endif

#ifdef FLITR_USE_OPENCV
    #include <opencv2/core/core.hpp>
    #include <opencv2/highgui/highgui.hpp>
    #include <opencv2/imgproc/imgproc.hpp>
    using namespace cv;
    //using namespace cv::dnn
#endif

#include <algorithm>
#include <iosfwd>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <iostream>
#include <stdio.h>

using namespace flitr;
//using std::shared_ptr;
using std::cout; using std::endl;

using namespace caffe;  // NOLINT(build/namespaces)


using cv::String;

FIPClassify::FIPClassify(ImageProducer& upStreamProducer,
               uint32_t images_per_slot,
               uint32_t buffer_size) :
ImageProcessor(upStreamProducer, images_per_slot, buffer_size)
{
    //Setup image format being produced to downstream.
    for (uint32_t i=0; i<images_per_slot; i++) {
        //ImageFormat(uint32_t w=0, uint32_t h=0, PixelFormat pix_fmt=FLITR_PIX_FMT_Y_8, bool flipV = false, bool flipH = false):
        ImageFormat downStreamFormat(upStreamProducer.getFormat().getWidth(), upStreamProducer.getFormat().getHeight(),
                                     upStreamProducer.getFormat().getPixelFormat());

        ImageFormat_.push_back(downStreamFormat);

        if(classifier == NULL){
            cerr<<"\n\n Initialising the classifier!! ........ \n\n";
            //cout<<"\n\n Initialising the classifier!! ........ \n\n";
            classifier = new Classifier(model_file, trained_file, mean_file, labels_file);
            //caffe.set_mode_gpu();
        }

    }

}

FIPClassify::~FIPClassify()
{
}

bool FIPClassify::init()
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


bool FIPClassify::trigger()
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

                //memset(dataWrite, 0, width*height);//Clear the downstream image.


                //Copy flitr image to OpenCV
                int opencvFormat = flitr::OpenCVHelperFunctions::getOpenCVFormat(imFormat.getPixelFormat());

                cvImage = new Mat( (int)height,(int)width, opencvFormat) ;
                cvImage->data = (**imvRead[imgNum]).data();




                /*
                 * //Save cv image to file
                string filename = "./OpenCV Images/image"+ std::to_string(imageNum);
                filename += ".png";
                cout<< "imageNUm: " << imageNum <<endl;
                //printf("10,10 pixel: %d \n", cvImage->at<uint8_t>(10,10) );
                imwrite(filename.c_str(),*cvImage);
                */

                std::vector<Prediction> predictions = classifier->Classify(*cvImage);

                /* Print the top N predictions. */

                for (size_t i = 0; i < predictions.size(); ++i) {
                  Prediction p = predictions[i];
                  std::cout << std::fixed << std::setprecision(4) << p.second << " - \""
                            << p.first << "\"" << std::endl;
                }
                printf("\n");


                //Copy the classification results image back to OSG viewer
                memcpy((**imvWrite[imgNum]).data(), (uchar*) cvImage->data , (imFormat.getBytesPerPixel()) * (int) (width*height) ) ;




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

