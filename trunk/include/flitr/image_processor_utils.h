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
    //! General purpose Integral image. Should be moved to FLITR_FIP_UTILS or similar.
    class FLITR_EXPORT IntegralImage
    {
    public:
        
        IntegralImage();
        
        /*! Virtual destructor */
        virtual ~IntegralImage();
        
        //! Copy constructor
        IntegralImage(const IntegralImage& rh)
        {}
        
        //! Assignment operator
        IntegralImage& operator=(const IntegralImage& rh)
        {
            return *this;
        }
        
        /*!Synchronous process method.*/
        virtual bool process(double * const dataWriteDS, float const * const dataReadUS,
                             const size_t width, const size_t height);
        
        /*!Synchronous process method.*/
        //virtual bool processRGB(double * const dataWriteDS, float const * const dataReadUS,
        //                       const size_t width, const size_t height);
    };
    
    
    //! General purpose Box filter. Should be moved to FLITR_FIP_UTILS or similar.
    class FLITR_EXPORT BoxFilter
    {
    public:
        
        BoxFilter(const size_t kernelWidth//Width of filter kernel in pixels in US image.
        );
        
        /*! Virtual destructor */
        virtual ~BoxFilter();
        
        //! Copy constructor
        BoxFilter(const BoxFilter& rh) :
        kernelWidth_(rh.kernelWidth_)
        {}
        
        //! Assignment operator
        BoxFilter& operator=(const BoxFilter& rh)
        {
            if (this == &rh)
            {
                return *this;
            }
            
            kernelWidth_=rh.kernelWidth_;
            
            return *this;
        }
        
        virtual void setKernelWidth(const int kernelWidth);
        
        /*!Synchronous process method.*/
        virtual bool filter(float * const dataWriteDS, float const * const dataReadUS,
                            const size_t width, const size_t height,
                            float * const dataScratch);
        
        /*!Synchronous process method.*/
        //virtual bool filterRGB(float * const dataWriteDS, float const * const dataReadUS,
        //                       const size_t width, const size_t height,
        //                       float * const dataScratch);
        
    private:
        size_t kernelWidth_;
    };
    
    
    //! General purpose Box filter USING AN INTEGRAL IMAGE. Should be moved to FLITR_FIP_UTILS or similar.
    class FLITR_EXPORT BoxFilterII
    {
    public:
        
        BoxFilterII(const size_t kernelWidth//Width of filter kernel in pixels in US image.
        );
        
        /*! Virtual destructor */
        virtual ~BoxFilterII();
        
        //! Copy constructor
        BoxFilterII(const BoxFilterII& rh) :
        kernelWidth_(rh.kernelWidth_)
        {}
        
        //! Assignment operator
        BoxFilterII& operator=(const BoxFilterII& rh)
        {
            if (this == &rh)
            {
                return *this;
            }
            
            kernelWidth_=rh.kernelWidth_;
            
            return *this;
        }
        
        virtual void setKernelWidth(const int kernelWidth);
        
        /*!Synchronous process method.*/
        virtual float filter(float * const dataWriteDS, float const * const dataReadUS,
                             const size_t width, const size_t height,
                             double * const IIDoubleScratch,
                             bool reuseIIScratch);
        
        /*!Synchronous process method.*/
        //virtual bool filterRGB(float * const dataWriteDS, float const * const dataReadUS,
        //                       const size_t width, const size_t height,
        //                       float * const dataScratch);
        
    private:
        size_t kernelWidth_;
        
        IntegralImage integralImage_;
    };
    
    
    //! General purpose Gaussial filter. Should be moved to FLITR_FIP_UTILS or similar.
    class FLITR_EXPORT GaussianFilter
    {
    public:
        
        GaussianFilter(const float filterRadius,//filterRadius = standardDeviation * 2.0 in US image.
                       const size_t kernelWidth//Width of filter kernel in pixels in US image.
        );
        
        /*! Virtual destructor */
        virtual ~GaussianFilter();
        
        //! Copy constructor
        GaussianFilter(const GaussianFilter& rh) :
        kernel1D_(nullptr),
        filterRadius_(rh.filterRadius_),
        kernelWidth_(rh.kernelWidth_)
        {
            updateKernel1D();
        }
        
        //! Assignment operator
        GaussianFilter& operator=(const GaussianFilter& rh)
        {
            if (this == &rh)
            {
                return *this;
            }
            
            filterRadius_=rh.filterRadius_;
            kernelWidth_=rh.kernelWidth_;
            updateKernel1D();
            
            return *this;
        }
        
        virtual void updateKernel1D();
        virtual void setFilterRadius(const float filterRadius);
        virtual void setKernelWidth(const int kernelWidth);
        
        /*!Synchronous process method.*/
        virtual bool filter(float * const dataWriteDS, float const * const dataReadUS,
                            const size_t width, const size_t height,
                            float * const dataScratch);
        
        /*!Synchronous process method.*/
        //virtual bool filterRGB(float * const dataWriteDS, float const * const dataReadUS,
        //                       const size_t width, const size_t height,
        //                       float * const dataScratch);
        
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
        
        //! Copy constructor
        GaussianDownsample(const GaussianDownsample& rh) :
        kernel1D_(nullptr),
        filterRadius_(rh.filterRadius_),
        kernelWidth_(rh.kernelWidth_)
        {
            updateKernel1D();
        }
        
        //! Assignment operator
        GaussianDownsample& operator=(const GaussianDownsample& rh)
        {
            if (this == &rh)
            {
                return *this;
            }
            
            filterRadius_=rh.filterRadius_;
            kernelWidth_=rh.kernelWidth_;
            updateKernel1D();
            
            return *this;
        }
        
        virtual void updateKernel1D();
        virtual void setFilterRadius(const float filterRadius);
        virtual void setKernelWidth(const int kernelWidth);
        
        /*!Synchronous process method.*/
        virtual bool downsample(float * const dataWriteDS, float const * const dataReadUS,
                                const size_t widthUS, const size_t heightUS,
                                float * const dataScratch);
        
        /*!Synchronous process method.*/
        //virtual bool downsampleRGB(float * const dataWriteDS, float const * const dataReadUS,
        //                           const size_t widthUS, const size_t heightUS,
        //                           float * const dataScratch);
        
    private:
        float *kernel1D_;
        
        float filterRadius_;
        size_t kernelWidth_;
    };
}

#endif //IMAGE_PROCESSOR_UTILS_H
