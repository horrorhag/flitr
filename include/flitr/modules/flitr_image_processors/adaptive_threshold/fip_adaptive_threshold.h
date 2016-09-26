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

#ifndef FIP_ADAPTIVE_THRESHOLD_H
#define FIP_ADAPTIVE_THRESHOLD_H 1

#include <flitr/image_processor.h>
#include <flitr/image_processor_utils.h>
#include <math.h>
#include <atomic>

namespace flitr {
    
    /*! Applies Gaussian adaptive threshold filter. */
    class FLITR_EXPORT FIPAdaptiveThreshold : public ImageProcessor
    {
    public:
        
        /*! Constructor given the upstream producer.
         *@param upStreamProducer The upstream image producer.
         *@param images_per_slot The number of images per image slot from the upstream producer.
         *@param buffer_size The size of the shared image buffer of the downstream producer.
         *@param thresholdOffset The offset [0..1] (fraction of max pixel value) of the threshold below the local average.
         */
        FIPAdaptiveThreshold(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                             const int kernelWidth,//Width of filter kernel in pixels.
                             const short numIntegralImageLevels,
                             const float thresholdOffset,
                             uint32_t buffer_size=FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS);
        
        /*! Virtual destructor */
        virtual ~FIPAdaptiveThreshold();
        
        /*!Sets the kernel width of both the Gaussian and the approx BoxFilter Gaussian! Also affects the BoxFilter Gaussian's standard deviation
         @sa getStandardDeviation */
        virtual void setKernelWidth(const int kernelWidth);
        
        /*! Method to initialise the object.
         *@return Boolean result flag. True indicates successful initialisation.*/
        virtual bool init();
        
        /*!Synchronous trigger method. Called automatically by the trigger thread in ImageProcessor base class if started.
         *@sa ImageProcessor::startTriggerThread*/
        virtual bool trigger();
        
        
        void setWidth(int r)
        {
            _kernelWidth = r;
            setKernelWidth(r);
        }
        
        int getWidth() const
        {
            return _kernelWidth;
        }
        
        void setThresholdOffset(float r)
        {
            _thresholdOffset = r;
        }
        
        float getThresholdOffset() const
        {
            return _thresholdOffset;
        }
        
        virtual int getNumberOfParms()
        {
            return 2;
        }
        
        virtual flitr::Parameters::EParmType getParmType(int id)
        {
            switch (id)
            {
                case 0 : return flitr::Parameters::PARM_INT;
                case 1 : return flitr::Parameters::PARM_FLOAT;
            }
            
            return flitr::Parameters::PARM_INT;
        }
        
        virtual std::string getParmName(int id)
        {
            switch (id)
            {
                case 0 :return std::string("Kernel Width");
                case 1 :return std::string("Threshold Offset");
            }
            
            return std::string("???");
        }
        
        void setTitle(const std::string &title)
        {
            _title=title;
        }
        
        virtual std::string getTitle()
        {
            return _title;
        }
        
        virtual void enable(bool state=true)
        {
            _enabled = state;
        }
        
        virtual bool isEnabled()
        {
            return _enabled;
        }
        
        float getThresholdAvrg()
        {
            return _thresholdAvrg.load();
        }
        
    private:
        short _numIntegralImageLevels;
        
        BoxFilterII _noiseFilter; //No significant state associated with this.
        uint8_t *_noiseFilteredInputData;
        
        uint8_t *_scratchData;
        double *_integralImageScratchData;
        
        BoxFilterII _boxFilter; //No significant state associated with this.
        
        bool _enabled;
        std::string _title;
        int _kernelWidth;
        
        float _thresholdOffset;
        
        std::atomic<float> _thresholdAvrg;
    };
    
}

#endif //FIP_GAUSSIAN_FILTER_H
