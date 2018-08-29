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

#ifndef FIP_PHOTOMETRIC_EQUALISE_H
#define FIP_PHOTOMETRIC_EQUALISE_H 1

#include <flitr/image_processor.h>
#include <flitr/image_processor_utils.h>

#include <algorithm>

namespace flitr {
    
    /*! Applies photometric equalisation to the image stream. */
    class FLITR_EXPORT FIPPhotometricEqualise : public ImageProcessor
    {
    public:
        
        /*! Constructor given the upstream producer.
         *@param upStreamProducer The upstream image producer.
         *@param images_per_slot The number of images per image slot from the upstream producer.
         *@param targetAverage The average image brightness the image is transformed to.
         *@param buffer_size The size of the shared image buffer of the downstream producer.*/
        FIPPhotometricEqualise(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                               float targetAverage,
                               uint32_t buffer_size=FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS);
        
        /*! Virtual destructor */
        virtual ~FIPPhotometricEqualise();
        
        /*! Method to initialise the object.
         *@return Boolean result flag. True indicates successful initialisation.*/
        virtual bool init();
        
        /*!Synchronous trigger method. Called automatically by the trigger thread in ImageProcessor base class if started.
         *@sa ImageProcessor::startTriggerThread*/
        virtual bool trigger();

        virtual void enable(bool state=true)
        {
            enable_ = state;
        }

        virtual bool isEnabled()
        {
            return enable_;
        }
        

        virtual void setTargetAverage(const float f)
        {
            targetAverage_= std::min<float>(std::max<float>(f, 0.0f),1.0f);
        }

        virtual float getTargetAverage() {
            return targetAverage_;
        }

        virtual int getNumberOfParms()
        {
            return 1;
        }

        virtual flitr::Parameters::EParmType getParmType(int id)
        {
            return flitr::Parameters::PARM_FLOAT;
        }

        virtual std::string getParmName(int id)
        {
            switch (id)
            {
            case 0 :return std::string("Target Average");
            }
            return std::string("???");
        }

        virtual float getFloat(int id)
        {
            switch (id)
            {
            case 0 : return getTargetAverage()*100.0f;
            }

            return 0.0f;
        }

        virtual bool getFloatRange(int id, float &low, float &high)
        {
            if (id==0)
            {
                low=1.0; high=100.0;
                return true;
            }

            return false;
        }

        virtual bool setFloat(int id, float v)
        {
            switch (id)
            {
                case 0 : setTargetAverage(v/100.0f); return true;
            }

            return false;
        }

        virtual std::string getTitle()
        {
            return Title_;
        }

    private:
        
        //!Template method that does the equalisation. Pixel format agnostic, but pixel data type templated!
        template<typename T>
        void process(T * const dataWrite, T const * const dataRead,
                     const size_t componentsPerLine, const size_t height)
        {
            
            //Could be line parallel!
            for (size_t y=0; y<height; y++)
            {
                const size_t lineOffset=y * componentsPerLine;
                
                double *lineSum = lineSumArray_ + y;
                *lineSum=0.0;
                
                for (size_t compNum=0; compNum<componentsPerLine; compNum++)
                {
                    (*lineSum)+=dataRead[lineOffset + compNum];
                }
            }
            
            
            double imageSum=0.0;
            
            //Single threaded!
            for (size_t y=0; y<height; y++)
            {
                imageSum+=lineSumArray_[y];
            }
            
            
            const size_t componentsPerImage=componentsPerLine*height;
            const float average = imageSum / ((double)componentsPerImage);

            if(average > 0.0)
            {
                const float eScale=targetAverage_ / average;

                //Could be pixel parallel.
                for (size_t y=0; y<height; ++y)
                {
                    const size_t lineOffset=y * componentsPerLine;

                    for (size_t compNum=0; compNum<componentsPerLine; ++compNum)
                    {
                        dataWrite[lineOffset + compNum]=dataRead[lineOffset + compNum] * eScale;
                    }
                }
            }
        }
        
        float targetAverage_;
        std::string Title_;

        /*! The sum per line in an image. */
        double * lineSumArray_;

        bool    enable_;
    };
    
    
    /*! Applies LOCAL photometric equalisation to the image stream. */
    class FLITR_EXPORT FIPLocalPhotometricEqualise : public ImageProcessor
    {
    public:
        
        /*! Constructor given the upstream producer.
         *@param upStreamProducer The upstream image producer.
         *@param images_per_slot The number of images per image slot from the upstream producer.
         *@param targetAverage The average image brightness the image is transformed to.
         *@param windowSize The local window size taken into account during equalisation of a pixel.
         *@param buffer_size The size of the shared image buffer of the downstream producer.*/
        FIPLocalPhotometricEqualise(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                                    const float targetAverage, const size_t windowSize,
                                    uint32_t buffer_size=FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS);
        
        /*! Virtual destructor */
        virtual ~FIPLocalPhotometricEqualise();
        
        /*! Method to initialise the object.
         *@return Boolean result flag. True indicates successful initialisation.*/
        virtual bool init();
        
        /*!Synchronous trigger method. Called automatically by the trigger thread in ImageProcessor base class if started.
         *@sa ImageProcessor::startTriggerThread*/
        virtual bool trigger();

        virtual void enable(bool state=true)
        {
            enable_ = state;
        }

        virtual bool isEnabled()
        {
            return enable_;
        }


        virtual void setTargetAverage(const float f)
        {
            targetAverage_= std::min<float>(std::max<float>(f, 0.0f),1.0f);
        }

        virtual float getTargetAverage() {
            return targetAverage_;
        }

        virtual int getNumberOfParms()
        {
            return 1;
        }

        virtual flitr::Parameters::EParmType getParmType(int id)
        {
            return flitr::Parameters::PARM_FLOAT;
        }

        virtual std::string getParmName(int id)
        {
            switch (id)
            {
            case 0 :return std::string("Target Average");
            }
            return std::string("???");
        }

        virtual float getFloat(int id)
        {
            switch (id)
            {
            case 0 : return getTargetAverage()*100.0f;
            }

            return 0.0f;
        }

        virtual bool getFloatRange(int id, float &low, float &high)
        {
            if (id==0)
            {
                low=1.0; high=100.0;
                return true;
            }

            return false;
        }

        virtual bool setFloat(int id, float v)
        {
            switch (id)
            {
                case 0 : setTargetAverage(v/100.0f); return true;
            }

            return false;
        }

        virtual std::string getTitle()
        {
            return Title_;
        }


    private:
        
        //!Template method that does the equalisation. Pixel format agnostic, but pixel data type templated.
        template<typename T>
        void process(T * const dataWrite, T const * const dataRead,
                     const size_t width, const size_t height)
        {
            const size_t halfWindowSize=windowSize_>>1;
            const size_t widthMinusWindowSize=width - windowSize_;
            const size_t heightMinusWindowSize=height - windowSize_;
            const double recipWindowSizeSquared=1.0f / float(windowSize_*windowSize_);
            
            for (size_t y=1; y<heightMinusWindowSize; ++y)
            {
                const size_t lineOffsetUS=y * width;
                const size_t lineOffsetDS=(y+halfWindowSize) * width + halfWindowSize;
                
                for (size_t x=1; x<widthMinusWindowSize; ++x)
                {
                    const double windowSum=integralImageData_[(lineOffsetUS+x) + (windowSize_-1) + (windowSize_-1)*width]
                    - integralImageData_[(lineOffsetUS+x) + (windowSize_-1) - width]
                    - integralImageData_[(lineOffsetUS+x) - 1 + (windowSize_-1)*width]
                    + integralImageData_[(lineOffsetUS+x) - 1 - width];
                    
                    const float windowAvrg=float(windowSum * recipWindowSizeSquared);
                    
                    const float eScale=targetAverage_ / windowAvrg;
                    
                    dataWrite[lineOffsetDS + x]=dataRead[lineOffsetDS + x] * eScale;
                }
            }
        }
        
        template<typename T>
        void processRGB(T * const dataWrite, T const * const dataRead,
                        const size_t width, const size_t height)
        {
            const size_t halfWindowSize=windowSize_>>1;
            const size_t widthMinusWindowSize=width - windowSize_;
            const size_t heightMinusWindowSize=height - windowSize_;
            const double recipWindowSizeSquared=1.0f / float(windowSize_*windowSize_);
            
            for (size_t y=1; y<heightMinusWindowSize; ++y)
            {
                size_t lineOffsetTL=((y-1)*width + 0) * 3;
                size_t lineOffsetTR=((y-1)*width + windowSize_) * 3;
                size_t lineOffsetBL=((y-1)*width + windowSize_*width) * 3;
                size_t lineOffsetBR=((y-1)*width + windowSize_*width + windowSize_) * 3;
                
                size_t lineOffset=((y+halfWindowSize) * width + halfWindowSize) * 3 + 3;
                
                for (size_t x=1; x<widthMinusWindowSize; ++x)
                {
                    const double windowSumR=integralImageData_[lineOffsetBR + 0]
                    - integralImageData_[lineOffsetBL + 0]
                    - integralImageData_[lineOffsetTR + 0]
                    + integralImageData_[lineOffsetTL + 0];
                    
                    const double windowSumG=integralImageData_[lineOffsetBR + 1]
                    - integralImageData_[lineOffsetBL + 1]
                    - integralImageData_[lineOffsetTR + 1]
                    + integralImageData_[lineOffsetTL + 1];
                    
                    const double windowSumB=integralImageData_[lineOffsetBR + 2]
                    - integralImageData_[lineOffsetBL + 2]
                    - integralImageData_[lineOffsetTR + 2]
                    + integralImageData_[lineOffsetTL + 2];
                    
                    const float windowAvrgR=float(windowSumR * recipWindowSizeSquared);
                    const float windowAvrgG=float(windowSumG * recipWindowSizeSquared);
                    const float windowAvrgB=float(windowSumB * recipWindowSizeSquared);
                    
                    const float eScaleR=targetAverage_ / windowAvrgR;
                    const float eScaleG=targetAverage_ / windowAvrgG;
                    const float eScaleB=targetAverage_ / windowAvrgB;
                    
                    dataWrite[lineOffset + 0]=dataRead[lineOffset + 0] * eScaleR;
                    dataWrite[lineOffset + 1]=dataRead[lineOffset + 1] * eScaleG;
                    dataWrite[lineOffset + 2]=dataRead[lineOffset + 2] * eScaleB;
                    
                    lineOffset+=3;
                    
                    lineOffsetTL+=3;
                    lineOffsetTR+=3;
                    lineOffsetBL+=3;
                    lineOffsetBR+=3;
                }
            }
        }
        
        float targetAverage_;
        const size_t windowSize_;
        std::string Title_;

        IntegralImage integralImage_;
        double *integralImageData_;

        bool enable_;
    };
    
}

#endif //FIP_PHOTOMETRIC_EQUALISE_H
