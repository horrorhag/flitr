#ifndef CAFFE_RCNN_FUNCTIONS_H
#define CAFFE_RCNN_FUNCTIONS_H 1

#ifdef FLITR_USE_CAFFE


#include <iosfwd>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <iostream>
#include <stdio.h>




/*Caffe and OpenCV*/
#include <opencv2/core/core.hpp>  //# Seems like I dont need this, wasn't there from dnn example
#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>


#include <caffe/caffe.hpp>
#include <algorithm>
#include <iosfwd>

//# From my dnn
#include <fstream>
#include <cstdlib>

//using namespace std;

const size_t width = 300;
const size_t height = 300;
using namespace cv;
using namespace cv::dnn;
using cv::String;
//using namespace std;
using std::ifstream;
using std::cout; using std::endl;
using std::cerr;
//using std::shared_ptr;


using namespace caffe;  // NOLINT(build/namespaces)


class Classifier_RCNN {
 public:

    const char* about = "This sample uses Single-Shot Detector "
                        "(https://arxiv.org/abs/1512.02325)"
                        "to detect objects on image\n"; // TODO: link

    const char* params = "{ help           | false | print usage         }"
          "{ proto          |       | model configuration }"
          "{ model          |       | model weights       }"
          "{ image          |       | image for detection }"
          "{ min_confidence | 0.5   | min confidence      }";


    //Public function
    void classify(Mat &image);
    Classifier_RCNN(const string& model_file,const string& trained_file,const string& labels_filename);

 private:
  //Stores a trained SSD or RCNN caffe model
  //#Try using a shared pointer
  //std::shared_ptr<dnn::Net> net;
  dnn::Net net;

  std::vector<string> ClassNames;     //Assume theres less than 100 labels

  Mat getMean(const size_t& imageHeight, const size_t& imageWidth);

  Mat preprocess(const Mat &frame);

};


Classifier_RCNN::Classifier_RCNN(const string& model_file,const string& proto_text,const string& labels_filename){
    cv::dnn::initModule();          //Required if OpenCV is built as static libs

    //caffemodel
    string modelBinary= model_file;
    //prototxt
    string modelConfiguration = proto_text;

#if 0 == 1
    //caffemodel
    string ModelPath = "../models/VOC0712/SSD_300x300/";
    string modelBinary = ModelPath;
    modelBinary +=   "VGG_VOC0712_SSD_300x300_iter_120000.caffemodel";

    //prototxt
    string protoText = "_deploy.prototxt" ;
    String modelConfiguration = ModelPath + protoText;

    //Image
    string imagePath = "/home/dumisani/Pictures/images_classification/";
    String imageFile = imagePath + "personA.jpg";
#endif

    //! [Create the importer of Caffe model]
    Ptr<dnn::Importer> importer;

    // Import Caffe SSD model
    try
    {
        importer = dnn::createCaffeImporter(modelConfiguration, modelBinary);
    }
    catch (const cv::Exception &err) //Importer can throw errors, we will catch them
    {
        cerr << err.msg << endl;
    }
    //! [Create the importer of Caffe model]

    if (!importer)
    {
        cerr << "Can't load network by using the following files: " << endl;
        cerr << "prototxt:   " << modelConfiguration << endl;
        cerr << "caffemodel: " << modelBinary << endl;
        cerr << "Models can be downloaded here:" << endl;
        cerr << "https://github.com/weiliu89/caffe/tree/ssd#models" << endl;
        exit(-1);
    }

    //! [Initialize network]
    //#dnn::Net net;
    importer->populateNet(net);
    importer.release();          //We don't need importer anymore
    //! [Initialize network]


    //![ Load class names for the index labels]
    //Read Labels textfile
    string textLine;
    ifstream labels_file(labels_filename);

    if(labels_file.is_open()){
        while(getline(labels_file,textLine)){
            ClassNames.push_back(textLine);
        }
    }
    labels_file.close();
    cout<<"Read ..." << ClassNames.size() <<" labels" <<endl;
    //![ Load class names for the index labels]
}


Mat Classifier_RCNN::getMean(const size_t& imageHeight, const size_t& imageWidth)
{
    Mat mean;

    const int meanValues[3] = {104, 117, 123};
    vector<Mat> meanChannels;
    for(size_t i = 0; i < 3; i++)
    {
        Mat channel(imageHeight, imageWidth, CV_32F, Scalar(meanValues[i]));
        meanChannels.push_back(channel);
    }
    cv::merge(meanChannels, mean);
    return mean;
}

/*
Mat Classifier::preprocess(const Mat &frame)
{
    Mat preprocessed;

    if(frame.type() == CV_8UC1){
        cvtColor(frame, preprocessed, CV_GRAY2RGB);
        cout<<"Yes!! Gray" <<endl;
    }

    preprocessed.convertTo(preprocessed, CV_32FC3);

    //printf("Old Size: (%d,%d)  ==> (%d,%d)\n", preprocessed.rows, preprocessed.cols, width, height );
    resize(preprocessed, preprocessed, Size(width, height)); //SSD accepts 300x300 RGB-images

    Mat mean = getMean(width, height);
    cv::subtract(preprocessed, mean, preprocessed);

    return preprocessed;
}
*/

//#void Classifier::Classify(Mat &image, float min_confidence=0.5){
void Classifier_RCNN::classify(Mat &image){ //# Put this back : (Mat &image){
    if(image.empty()){
        cerr<<"Could not load the image!!\n\n";
        exit(-1);
    }

    //! [Prepare blob]
    Mat preprocessedFrame;  // = preprocess(image, preprocessedFrame);
    if(image.type() == CV_8UC1){
        cvtColor(image, preprocessedFrame, CV_GRAY2RGB);
        cout<<"Yes!! Gray" <<endl;
    }else{
        preprocessedFrame = image;
    }

    preprocessedFrame.convertTo(preprocessedFrame, CV_32FC3);
    resize(preprocessedFrame, preprocessedFrame, Size(width, height)); //SSD accepts 300x300 RGB-images
    Mat mean = getMean(width, height);
    cv::subtract(preprocessedFrame, mean, preprocessedFrame);
    //printf("C1 type==RBG? %d \n", (preprocessedFrame.type()==CV_32FC3));

    cout<<"Processing image...."<<endl;
    Mat inputBlob = blobFromImage(preprocessedFrame); //Convert Mat to batch of images
    //! [Prepare blob]

    //! [Set input blob]
    net.setBlob(".data", inputBlob);                //set the network input
    //! [Set input blob]

    //! [Make forward pass]
    net.forward();                                  //compute output
    //! [Make forward pass]

    //! [Gather output]
    Mat detection = net.getBlob("detection_out");
    Mat detectionMat(detection.size[2], detection.size[3], CV_32F, detection.ptr<float>());

    //#printf("detection; (%d,%d,%d,%d) \n",detection.size);
    printf("\n");

    float confidenceThreshold = 0.5;  //# Make it default a value on the function parameter "min_confidence"
    for(int i = 0; i < detectionMat.rows; i++)
    {
        float confidence = detectionMat.at<float>(i, 2);

        if(confidence > confidenceThreshold)
        {

            size_t objectClass = detectionMat.at<float>(i, 1);


            float xLeftBottom = detectionMat.at<float>(i, 3) * image.cols;
            float yLeftBottom = detectionMat.at<float>(i, 4) * image.rows;
            float xRightTop = detectionMat.at<float>(i, 5) * image.cols;
            float yRightTop = detectionMat.at<float>(i, 6) * image.rows;

            std::cout << "Class: " << objectClass
                      << " " << ClassNames.at(objectClass) <<std::endl;

            std::cout << "Confidence: " << confidence << std::endl;

            std::cout << " " << xLeftBottom
                      << " " << yLeftBottom
                      << " " << xRightTop
                      << " " << yRightTop << std::endl;

            Rect object(xLeftBottom, yLeftBottom,
                        xRightTop - xLeftBottom,
                        yRightTop - yLeftBottom);

            rectangle(image, object, Scalar(0, 255, 0));
        }
    }
/*
    //#imshow("detections", image);
    //waitKey();
*/
}

#else
int main(int argc, char** argv) {
  cerr << "This example requires CAFFE; compile with FLITR_USE_CAFFE." <<endl;
}
#endif //FLITR_USE_CAFFE



#endif


