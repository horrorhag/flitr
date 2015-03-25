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

#ifndef IMAGE_PROCESSOR_UTILS_H
#define IMAGE_PROCESSOR_UTILS_H 1

#include <cstdlib>
#include <flitr/flitr_export.h>

namespace flitr {
    //! General purpose Gaussial filter. Should be moved to FLITR_FIP_UTILS or similar.
    class FLITR_EXPORT GaussianFilter
    {
    public:
        
        GaussianFilter(const float filterRadius,//filterRadius = standardDeviation * 2.0 in US image.
                       const size_t kernelWidth//Width of filter kernel in pixels in US image.
        );
        
        /*! Virtual destructor */
        virtual ~GaussianFilter();
        
        virtual void updateKernel1D();
        virtual void setFilterRadius(const float filterRadius);
        virtual void setKernelWidth(const int kernelWidth);
        
        /*!Synchronous process method.*/
        virtual bool filter(float * const dataWriteDS, float const * const dataReadUS,
                            const size_t width, const size_t height,
                            float * const dataScratch);
        
    private:
        float *kernel1D_;
        
        float filterRadius_;
        size_t kernelWidth_;
    };
    
    
    //! General purpose Gaussial filter. Should be moved to FLITR_FIP_UTILS or similar.
    class FLITR_EXPORT GaussianDownsample
    {
    public:
        
        GaussianDownsample(const float filterRadius,//filterRadius = standardDeviation * 2.0 in US image.
                           const size_t kernelWidth//Width of filter kernel in pixels in US image.
        );
        
        /*! Virtual destructor */
        virtual ~GaussianDownsample();
        
        virtual void updateKernel1D();
        virtual void setFilterRadius(const float filterRadius);
        virtual void setKernelWidth(const int kernelWidth);
        
        /*!Synchronous process method.*/
        virtual bool downsample(float * const dataWriteDS, float const * const dataReadUS,
                                const size_t widthUS, const size_t heightUS,
                                float * const dataScratch);
        
    private:
        float *kernel1D_;
        
        float filterRadius_;
        size_t kernelWidth_;
    };
}

#endif //IMAGE_PROCESSOR_UTILS_H
