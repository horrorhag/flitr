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

#ifndef FIP_MORPHOLOGICAL_FILTER_H
#define FIP_MORPHOLOGICAL_FILTER_H 1

#include <flitr/image_processor.h>
#include <flitr/image_processor_utils.h>
#include <vector>

namespace flitr {
    
    /*! Applies Gaussian filter with radius approx. 5 pixels. */
    class FLITR_EXPORT FIPMorphologicalFilter : public ImageProcessor
    {
    public:
        enum class MorphoPass {ERODE, DILATE, SOURCE_MINUS, MINUS_SOURCE, THRESHOLD};
        
        /*! Constructor given the upstream producer.
         *@param upStreamProducer The upstream image producer.
         *@param images_per_slot The number of images per image slot from the upstream producer.
         *@param buffer_size The size of the shared image buffer of the downstream producer.*/
        FIPMorphologicalFilter(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                               const size_t structuringElementSize,
                               const float threshold,
                               const float binaryMax,
                               uint32_t buffer_size=FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS);
        
        /*! Virtual destructor */
        virtual ~FIPMorphologicalFilter();
        
        /*! Method to initialise the object.
         *@return Boolean result flag. True indicates successful initialisation.*/
        virtual bool init();
        
        void addMorphoPass(MorphoPass morphoPass)
        {
            morphoPassVec_.push_back(morphoPass);
        }
        
        void clearMorphoPassVec()
        {
            morphoPassVec_.clear();
        }
        
        /*!Synchronous trigger method. Called automatically by the trigger thread in ImageProcessor base class if started.
         *@sa ImageProcessor::startTriggerThread*/
        virtual bool trigger();
        
    private:
        template<typename T>
        void doMorphoPasses(T const * const dataReadUS, T * const dataWriteDS,
                            const size_t width, const size_t height)
        {
            const size_t numMorphoPasses=morphoPassVec_.size();
            T const * tempReadUS=dataReadUS;
            
            for (size_t morphoPassNum=0; morphoPassNum<numMorphoPasses; ++morphoPassNum)
            {
                const MorphoPass morphoPass=morphoPassVec_[morphoPassNum];
                
                T * tempWriteDS=(morphoPassNum==(numMorphoPasses-1)) ? dataWriteDS : ((T *)passScratchData_[morphoPassNum]);
                
                switch (morphoPass)
                {
                    case MorphoPass::ERODE:
                        morphologicalFilter_.erode(tempWriteDS, tempReadUS,
                                                   structuringElementSize_,
                                                   width, height, (T *)tmpScratchData_);
                        break;
                    case MorphoPass::DILATE:
                        morphologicalFilter_.dilate(tempWriteDS, tempReadUS,
                                                    structuringElementSize_,
                                                    width, height, (T *)tmpScratchData_);
                        break;
                    case MorphoPass::SOURCE_MINUS:
                        morphologicalFilter_.difference(tempWriteDS,
                                                        dataReadUS,//source
                                                        tempReadUS,//previous result
                                                        width, height);
                        break;
                    case MorphoPass::MINUS_SOURCE:
                        morphologicalFilter_.difference(tempWriteDS,
                                                        tempReadUS,//previous result
                                                        dataReadUS,//source
                                                        width, height);
                        break;
                    case MorphoPass::THRESHOLD:
                        morphologicalFilter_.threshold(tempWriteDS, tempReadUS,
                                                       T(threshold_),
                                                       T(binaryMax_),
                                                       width, height);
                        break;
                }
                
                tempReadUS=tempWriteDS;
            }
        }
        
        template<typename T>
        void doMorphoPassesRGB(T const * const dataReadUS, T * const dataWriteDS,
                               const size_t width, const size_t height)
        {
            const size_t numMorphoPasses=morphoPassVec_.size();
            T const * tempReadUS=dataReadUS;
            
            for (size_t morphoPassNum=0; morphoPassNum<numMorphoPasses; ++morphoPassNum)
            {
                const MorphoPass morphoPass=morphoPassVec_[morphoPassNum];
                
                T * tempWriteDS=(morphoPassNum==(numMorphoPasses-1)) ? dataWriteDS : ((T *)passScratchData_[morphoPassNum]);
                
                switch (morphoPass)
                {
                    case MorphoPass::ERODE:
                        morphologicalFilter_.erodeRGB(tempWriteDS, tempReadUS,
                                                      structuringElementSize_,
                                                      width, height, (T *)tmpScratchData_);
                        break;
                    case MorphoPass::DILATE:
                        morphologicalFilter_.dilateRGB(tempWriteDS, tempReadUS,
                                                       structuringElementSize_,
                                                       width, height, (T *)tmpScratchData_);
                        break;
                    case MorphoPass::SOURCE_MINUS:
                        morphologicalFilter_.differenceRGB(tempWriteDS,
                                                           dataReadUS,//source
                                                           tempReadUS,//previous result
                                                           width, height);
                    case MorphoPass::MINUS_SOURCE:
                        morphologicalFilter_.differenceRGB(tempWriteDS,
                                                           tempReadUS,//previous result
                                                           dataReadUS,//source
                                                           width, height);
                        break;
                    case MorphoPass::THRESHOLD:
                        morphologicalFilter_.thresholdRGB(tempWriteDS, tempReadUS,
                                                          T(threshold_),
                                                          T(binaryMax_),
                                                          width, height);
                        break;
                }
                
                tempReadUS=tempWriteDS;
            }
        }
        
        
        size_t structuringElementSize_;
        
        float threshold_;
        float binaryMax_;
        
        MorphologicalFilter morphologicalFilter_; //No significant state associated with this.
        
        uint8_t *passScratchData_[2];
        
        uint8_t *tmpScratchData_;
        
        std::vector<MorphoPass> morphoPassVec_;
    };
    
}

#endif //FIP_MORPHOLOGICAL_FILTER_H
