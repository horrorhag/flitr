
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
Mat flitr::OpenCVHelperFunctions::copyToOpenCVimage(Image** sourceFlitr, ImageFormat imFormat){
    const size_t width=imFormat.getWidth();
    const size_t height=imFormat.getHeight();
    int opencvFormat =  getOpenCVFormat(imFormat.getPixelFormat());


    /// Copy flitr image to OpenCV
    Mat cvImage = Mat( (int)height,(int)width, opencvFormat) ;  //#Use CV_8UC1
    memcpy(cvImage.data, (**sourceFlitr).data() , imFormat.getBytesPerImage() ) ;

    if(imFormat.getPixelFormat() == ImageFormat::FLITR_PIX_FMT_RGB_8 ||
       imFormat.getPixelFormat() == ImageFormat::FLITR_PIX_FMT_RGB_F32){
        cvtColor(cvImage,cvImage,CV_RGB2BGR);
    }


    return cvImage;
}

/** Copy OpenCV image to flitr */
void flitr::OpenCVHelperFunctions::copyToFlitrImage(Mat &sourceOpenCV, Image** destFlitr, ImageFormat imFormat){
    if(imFormat.getPixelFormat() == ImageFormat::FLITR_PIX_FMT_RGB_8 ||
         imFormat.getPixelFormat() == ImageFormat::FLITR_PIX_FMT_RGB_F32){
        cvtColor(sourceOpenCV,sourceOpenCV,CV_BGR2RGB);
    }

    memcpy((**destFlitr).data(), sourceOpenCV.data, imFormat.getBytesPerImage() );
}


/** Assign flitr image to an OpenCV.data()*/
Mat flitr::OpenCVHelperFunctions::assignToOpenCVimage(Image** sourceFlitr, ImageFormat imFormat){
    const size_t width=imFormat.getWidth();
    const size_t height=imFormat.getHeight();
    int opencvFormat =  getOpenCVFormat(imFormat.getPixelFormat());



    /// Copy flitr image to OpenCV
    Mat cvImage = Mat( (int)height,(int)width, opencvFormat, (**sourceFlitr).data() ) ;  //#Use CV_8UC1

    if(imFormat.getPixelFormat() == ImageFormat::FLITR_PIX_FMT_RGB_8 ||
         imFormat.getPixelFormat() == ImageFormat::FLITR_PIX_FMT_RGB_F32){
        cvtColor(cvImage,cvImage,CV_BGR2RGB);
    }

    return cvImage;
}



