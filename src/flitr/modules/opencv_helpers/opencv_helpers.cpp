

#include <flitr/modules/opencv_helpers/opencv_helpers.h>


/** Get openCV format from FLitr ImageFormat */
int flitr::OpenCVHelperFunctions::getOpenCVFormat(flitr::ImageFormat::PixelFormat pixelFormat)
{
        int opencvFormat;
        switch(pixelFormat)
        {

        case flitr::ImageFormat::FLITR_PIX_FMT_Y_8:
                opencvFormat=CV_8UC1;
                break;

        case flitr::ImageFormat::FLITR_PIX_FMT_RGB_8:
                opencvFormat=CV_8UC3;
                break;

        case flitr::ImageFormat::FLITR_PIX_FMT_BGR:
                opencvFormat=CV_8UC3;
                break;

        case flitr::ImageFormat::FLITR_PIX_FMT_Y_F32:
                opencvFormat=CV_32FC1;
                break;
        case flitr::ImageFormat::FLITR_PIX_FMT_RGB_F32:
                opencvFormat=CV_32FC3;
                break;


        default:
                opencvFormat=CV_8UC3;
                std::cout << "Unsupported Image Format, default CV_8UC3 Image Format will be used.\n";
                break;

        }

        return opencvFormat;
}

/** Copy Flitr image to OpenCV image */
Mat flitr::OpenCVHelperFunctions::copyToOpenCVimage(std::vector<Image**> sourceFlitr, ImageFormat imFormat){
    const size_t width=imFormat.getWidth();
    const size_t height=imFormat.getHeight();
    int opencvFormat =  getOpenCVFormat(imFormat.getPixelFormat());

    /// Copy flitr image to OpenCV
    Mat cvImage = Mat( (int)height,(int)width, opencvFormat) ;  //#Use CV_8UC1
    memcpy(cvImage.data, (**sourceFlitr[0]).data() , imFormat.getBytesPerImage() ) ;

    return cvImage;
}

/** Copy OpenCV image to flitr */
void flitr::OpenCVHelperFunctions::copyToFlitrImage(Mat &sourceOpenCV, std::vector<Image**> destFlitr, ImageFormat imFormat){
    memcpy((**destFlitr[0]).data(), sourceOpenCV.data, imFormat.getBytesPerImage() );
}


/** Assign flitr image to an OpenCV.data()*/
Mat flitr::OpenCVHelperFunctions::assignToOpenCVimage(std::vector<Image**> sourceFlitr, ImageFormat imFormat){
    const size_t width=imFormat.getWidth();
    const size_t height=imFormat.getHeight();
    int opencvFormat =  getOpenCVFormat(imFormat.getPixelFormat());

    /// Copy flitr image to OpenCV
    Mat cvImage = Mat( (int)height,(int)width, opencvFormat) ;  //#Use CV_8UC1
    cvImage.data = (**sourceFlitr[0]).data();

    return cvImage;
}



