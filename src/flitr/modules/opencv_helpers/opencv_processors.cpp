

#include <flitr/modules/opencv_helpers/opencv_processors.h>
#include <flitr/modules/opencv_helpers/opencv_helpers.h>


using std::cout; using std::endl;
using std::string;


OpenCVProcessors::OpenCVProcessors(){
    ;
}

void OpenCVProcessors::applyDefaultProcessor(cv::Mat &image, ProcessorType proc = laplace){
    int imageFormat = image.type();
    int kernelSize = 3;

    if(proc == laplace){
        cv::Laplacian(image,image,imageFormat,kernelSize);
    }
    else if(proc == histEqualisation){
        if(!(imageFormat == CV_8UC1 || imageFormat == CV_32FC1))
            cvtColor( image, image, CV_BGR2GRAY );      // Convert to grayscale

        cv::equalizeHist(image,image);

        if(!(imageFormat == CV_8UC1 || imageFormat == CV_32FC1))
            cvtColor( image, image, CV_GRAY2BGR);      // Convert to BGR
    }
    else if(proc == clahe){
        if(!(imageFormat == CV_8UC1 || imageFormat == CV_32FC1))
            cvtColor( image, image, CV_BGR2GRAY );      // Convert to grayscale

        int clipLimit=2.0;
        cv::Size tileGridSize = cv::Size(8,8);
        cv::Ptr<cv::CLAHE> claheProcessor = cv::createCLAHE(clipLimit, tileGridSize);

        claheProcessor->apply(image,image);

        if(!(imageFormat == CV_8UC1 || imageFormat == CV_32FC1))
            cvtColor( image, image, CV_GRAY2BGR);      // Convert to rgb
    }
    else if(proc == backgroundSubtraction){
        _bgSubtraction.backgroundSubtraction(image,200);
    }


}


void OpenCVProcessors::applyBackgroundSubtraction(cv::Mat &image, int backgroundUpdateInterval){
    _bgSubtraction.backgroundSubtraction(image,backgroundUpdateInterval);
}

