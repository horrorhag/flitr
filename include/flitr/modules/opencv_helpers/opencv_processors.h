#ifndef OPENCV_PROCESSORS_H
#define OPENCV_PROCESSORS_H

/**
OpenCVProcessors implements any opencv processors for easier use in Flitr or IPFramework.
To add a processor specifier to the enum "ProcessorType"
*/



#include <iostream>
#include <string>
#include <map>


#ifdef FLITR_USE_OPENCV
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
using namespace cv;



using std::count; using std::endl;
using std::string;

class OpenCVProcessors
{


///====================OpenCV Algorithms =================
public:
    class BackgroundSubtraction{
    public:
        cv::Mat backgroundImage;
        int intervalFrameCount = 0;

        void backgroundSubtraction(cv::Mat &image, int backgroundUpdateInterval=50){
            int imageType = image.type();

            if(!(imageType == CV_8UC1 || imageType == CV_32FC1))
                cvtColor( image, image, CV_BGR2GRAY);      // Convert to grayscale



            if(intervalFrameCount == backgroundUpdateInterval || intervalFrameCount==0 ){
                image.copyTo(backgroundImage);
                intervalFrameCount = 0;
            }
            intervalFrameCount++;

            //TODO: Check if the scene has changed. (Perhaps the significant num of pixels) or Vector Distance

            //Subtract images
            cv::absdiff(image,backgroundImage,image);// Absolute differences between the 2 images
            cv::threshold(image,image,70,255,CV_THRESH_BINARY); // set threshold to ignore small differences you can also use inrange function

            if(!(imageType == CV_8UC1 || imageType == CV_32FC1))
                cvtColor( image, image, CV_GRAY2BGR);      // Convert to BGR
        }

        void resetBackgroundImage(){
            //backgroundImage.release();  //this will remove Mat m from memory
            backgroundImage = Mat();
        }
    };



/// Public User Functions
public:
        OpenCVProcessors();

        enum ProcessorType {laplace,
                            histEqualisation,
                            backgroundSubtraction,
                            clahe
                           };
        /// Apply the selected processor, using default parameters
        void applyDefaultProcessor(cv::Mat &image, enum ProcessorType);


        /// Custom Processors: ================================================
        BackgroundSubtraction _bgSubtraction;
        void applyBackgroundSubtraction(cv::Mat &image, int backgroundUpdateInterval=50);

};



#else
    std::cout<<"openCV_Processor requires openCV, set the compiler definition 'Flitr_USE_OPENCV' to ON "
#endif
#endif // CONVERTTOOPENCV_H





