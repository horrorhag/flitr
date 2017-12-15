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

#ifndef FIP_CLASSIFICATION_H
#define FIP_CLASSIFICATION_H 1


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

#include <flitr/modules/flitr_image_processors/classification/caffe_cnn_functions.h>
#include <flitr/modules/opencv_helpers/opencv_helpers.h>


using namespace caffe;  // NOLINT(build/namespaces)
typedef std::pair<string, float> Prediction; //Pair (label, confidence) representing a prediction.


namespace flitr {


    /*! Compute the DPT of image. Currently assumes 8-bit mono input! */
    class FLITR_EXPORT FIPClassify : public ImageProcessor
    {
    private:

    public:
        Mat *cvImage;
        int imageNum = -1;

        //Load the trained Net info
        String caffe_root = "/home/dumisani/dev/caffe/";
        string caffeModelDir = caffe_root + "models/bvlc_reference_caffenet/";

        String model_file = caffeModelDir + "deploy.prototxt";
        String trained_file = caffeModelDir + "bvlc_reference_caffenet.caffemodel";
        String labels_file = caffe_root + "data/ilsvrc12/synset_words.txt";
        String mean_file =  caffe_root + "data/ilsvrc12/imagenet_mean.binaryproto";

        Classifier *classifier = classifier = new Classifier(model_file, trained_file, mean_file, labels_file);

        /*! Constructor given the upstream producer.
         *@param upStreamProducer The upstream image producer.
         *@param images_per_slot The number of images per image slot from the upstream producer.
         *@param buffer_size The size of the shared image buffer of the downstream producer.*/
        FIPClassify(ImageProducer& upStreamProducer,
               uint32_t images_per_slot,
               uint32_t buffer_size=FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS);

        /*! Virtual destructor */
        virtual ~FIPClassify();

        /*! Method to initialise the object.
         *@return Boolean result flag. True indicates successful initialisation.*/
        virtual bool init();

        /*!Synchronous trigger method. Called automatically by the trigger thread in ImageProcessor base class if started.
         *@sa ImageProcessor::startTriggerThread*/
        virtual bool trigger();


    };

}

#endif //FIP_CLASSIFICATION_H
