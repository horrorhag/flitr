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

#ifndef FIP_OpenCVProcessors_H
#define FIP_OpenCVProcessors_H 1

#include <flitr/image_processor.h>

#include <algorithm>
#include <iosfwd>
#include <memory>
#include <string>
#include <utility>
#include <vector>

/////////////////////////////////////////
/*OpenCV*/
#ifdef FLITR_USE_OPENCV
    #include <opencv2/core/core.hpp>
    #include <opencv2/highgui/highgui.hpp>
#endif
using namespace cv;
using cv::Mat;

#include <flitr/modules/opencv_helpers/opencv_processors.h>
#include <flitr/modules/opencv_helpers/opencv_helpers.h>



namespace flitr {

    /*! Compute the DPT of image. Currently assumes 8-bit mono input! */
    class FLITR_EXPORT FIP_OpenCVProcessors : public ImageProcessor
    {
    private:

    public:
        int imageNum = -1;
        OpenCVProcessors::ProcessorType _cvProcessorType;
        OpenCVProcessors _cvProcessors;
        std::map<std::string, std::string> _cvProcessorParams;


        /*! Constructor given the upstream producer.
         *@param upStreamProducer The upstream image producer.
         *@param images_per_slot The number of images per image slot from the upstream producer.
         *@param buffer_size The size of the shared image buffer of the downstream producer.*/
        FIP_OpenCVProcessors(ImageProducer& upStreamProducer,
                             uint32_t images_per_slot,
                             uint32_t buffer_size=FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS,
                             OpenCVProcessors::ProcessorType cvProcessorType = OpenCVProcessors::laplace,
                             std::map<std::string, std::string> cvProcessorParams = std::map<std::string, std::string>()
        );

        /*! Virtual destructor */
        virtual ~FIP_OpenCVProcessors();

        /*! Method to initialise the object.
         *@return Boolean result flag. True indicates successful initialisation.*/
        virtual bool init();

        /*!Synchronous trigger method. Called automatically by the trigger thread in ImageProcessor base class if started.
         *@sa ImageProcessor::startTriggerThread*/
        virtual bool trigger();


        //virtual std::string getTitle()
        /*! The following functions overwrite the flitr::Parameters virtual functions */
        virtual std::string getTitle(){return _title;   }
        virtual void setTitle(const std::string &title){_title=title;}

        virtual int getNumberOfParms(){
            return 0;
        }

    private:
        std::string _title = "OpenCV Processors";
    };


}

#endif //FIP_OpenCVProcessors_H
