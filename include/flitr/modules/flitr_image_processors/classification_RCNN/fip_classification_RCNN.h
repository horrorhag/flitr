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

#ifndef FIP_CLASSIFICATION_RCNN_H
#define FIP_CLASSIFICATION_RCNN_H 1


#include <flitr/image_processor.h>
#ifdef FLITR_USE_OPENCV
    #include <opencv2/core/core.hpp>
    #include <opencv2/highgui/highgui.hpp>
    #include <opencv2/imgproc/imgproc.hpp>
    using namespace cv;
#endif

#include <algorithm>
#include <iosfwd>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <caffe/caffe.hpp>
#include <caffe/proto/caffe.pb.h>
#include "flitr/modules/flitr_image_processors/classification_RCNN/caffe_rcnn_functions.h"
#include <flitr/modules/opencv_helpers/opencv_helpers.h>

using namespace caffe;  // NOLINT(build/namespaces)
typedef std::pair<string, float> Prediction; //Pair (label, confidence) representing a prediction.


namespace flitr {
    /*! Compute the DPT of image. Currently assumes 8-bit mono input! */
    class FLITR_EXPORT FIPClassifyRCNN : public ImageProcessor
    {
    private:

    public:       
        Mat *cvImage;
        int imageNum = -1;

        //=========== Load the Caffe Network =====================
        //caffemodel
        String ModelPath = "/home/dumisani/dev/caffe/models/VOC0712/SSD_300x300/";
        String trained_modelBinary = ModelPath + "VGG_VOC0712_SSD_300x300_iter_120000.caffemodel";

        //prototxt
        String protoText = "_deploy.prototxt" ;
        String modelConfiguration = ModelPath + protoText;

        //Labels textfile
        string labels_file = ModelPath + "VOCO_ClassNames.txt";

        Classifier_RCNN *classifier;

        //Returns the corresponding opencv imageformat, given a flitr ImageFormat
        int getOpenCVFormat(flitr::ImageFormat::PixelFormat pixelFormat);

        /*! Constructor given the upstream producer.
         *@param upStreamProducer The upstream image producer.
         *@param images_per_slot The number of images per image slot from the upstream producer.
         *@param buffer_size The size of the shared image buffer of the downstream producer.*/
        FIPClassifyRCNN(ImageProducer& upStreamProducer,
               uint32_t images_per_slot,
               uint32_t buffer_size=FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS);

        /*! Virtual destructor */
        virtual ~FIPClassifyRCNN();

        /*! Method to initialise the object.
         *@return Boolean result flag. True indicates successful initialisation.*/
        virtual bool init();

        /*!Synchronous trigger method. Called automatically by the trigger thread in ImageProcessor base class if started.
         *@sa ImageProcessor::startTriggerThread*/
        virtual bool trigger();


        /*! The following functions overwrite the flitr::Parameters virtual functions */
        virtual std::string getTitle(){return _title;   }
        virtual void setTitle(const std::string &title){_title=title;}

        virtual int getNumberOfParms(){
            return 0;
        }


    private:
        std::string _title = "Regions with CNNs";
    };


}



#endif //FIP_CLASSIFICATION_H
