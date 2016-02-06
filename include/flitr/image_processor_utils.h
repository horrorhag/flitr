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
    //! General purpose Integral image.
    class FLITR_EXPORT IntegralImage
    {
    public:
        
        IntegralImage() {}
        
        /*! destructor */
        ~IntegralImage() {}
        
        //! Copy constructor
        IntegralImage(const IntegralImage& rh) {}
        
        //! Assignment operator
        IntegralImage& operator=(const IntegralImage& rh)
        {
            return *this;
        }
        
        /*!Synchronous process method for float pixel format.*/
        template<typename T>
        bool process(double * const dataWriteDS, T const * const dataReadUS, const size_t width, const size_t height)
        {
            dataWriteDS[0]=dataReadUS[0];
            
            for (size_t x=1; x<width; ++x)
            {
                dataWriteDS[x]=dataReadUS[x] + dataWriteDS[x-1];
            }
            
            size_t offset=width;
            for (size_t y=1; y<height; ++y)
            {
                dataWriteDS[offset]=dataReadUS[offset] + dataWriteDS[offset - width];
                offset+=width;
            }
            
            for (size_t y=1; y<height; ++y)
            {
                offset=y*width;
                double lineSum=dataReadUS[offset];
                ++offset;
                
                for (size_t x=1; x<width; ++x)
                {
                    lineSum+=dataReadUS[offset];
                    
                    dataWriteDS[offset]=lineSum + dataWriteDS[offset - width];
                    
                    ++offset;
                }
            }
            
            return true;
        }
        
        template<typename T>
        bool processRGB(double * const dataWriteDS, T const * const dataReadUS, const size_t width, const size_t height)
        {
            const size_t widthTimeThree=width*3;
            
            dataWriteDS[0]=dataReadUS[0];
            dataWriteDS[1]=dataReadUS[1];
            dataWriteDS[2]=dataReadUS[2];
            
            for (size_t offset=3; offset<widthTimeThree; offset+=3)
            {
                dataWriteDS[offset + 0]=dataReadUS[offset + 0] + dataWriteDS[offset-3];
                dataWriteDS[offset + 1]=dataReadUS[offset + 1] + dataWriteDS[offset-2];
                dataWriteDS[offset + 2]=dataReadUS[offset + 2] + dataWriteDS[offset-1];
            }
            
            size_t offset=widthTimeThree;
            for (size_t y=1; y<height; ++y)
            {
                dataWriteDS[offset + 0]=dataReadUS[offset + 0] + dataWriteDS[offset - widthTimeThree + 0];
                dataWriteDS[offset + 1]=dataReadUS[offset + 1] + dataWriteDS[offset - widthTimeThree + 1];
                dataWriteDS[offset + 2]=dataReadUS[offset + 2] + dataWriteDS[offset - widthTimeThree + 2];
                offset+=widthTimeThree;
            }
            
            for (size_t y=1; y<height; ++y)
            {
                offset=y*widthTimeThree;
                
                double lineSumR=dataReadUS[offset + 0];
                double lineSumG=dataReadUS[offset + 1];
                double lineSumB=dataReadUS[offset + 2];
                
                ++offset;
                
                for (size_t x=1; x<width; ++x)
                {
                    lineSumR+=dataReadUS[offset + 0];
                    lineSumG+=dataReadUS[offset + 1];
                    lineSumB+=dataReadUS[offset + 2];
                    
                    const size_t offsetMinusWidthTimeThree=offset - widthTimeThree;
                    
                    dataWriteDS[offset + 0]=lineSumR + dataWriteDS[offsetMinusWidthTimeThree + 0];
                    dataWriteDS[offset + 1]=lineSumG + dataWriteDS[offsetMinusWidthTimeThree + 1];
                    dataWriteDS[offset + 2]=lineSumB + dataWriteDS[offsetMinusWidthTimeThree + 2];
                    
                    offset+=3;
                }
            }
            
            return true;
        }
    };
    
    
    //! General purpose Box filter.
    class FLITR_EXPORT BoxFilter
    {
    public:
        
        BoxFilter(const size_t kernelWidth//Width of filter kernel in pixels in US image.
        );
        
        /*! destructor */
        ~BoxFilter();
        
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
        
        void setKernelWidth(const int kernelWidth);
        
        /*!Synchronous process method for float pixel format.*/
        bool filter(float * const dataWriteDS, float const * const dataReadUS,
                    const size_t width, const size_t height,
                    float * const dataScratch);
        
        /*!Synchronous process method for float RGB pixel format.*/
        bool filterRGB(float * const dataWriteDS, float const * const dataReadUS,
                       const size_t width, const size_t height,
                       float * const dataScratch);
        
        /*!Synchronous process method for uint8_t pixel format.*/
        bool filter(uint8_t * const dataWriteDS, uint8_t const * const dataReadUS,
                    const size_t width, const size_t height,
                    uint8_t * const dataScratch);
        
        /*!Synchronous process method for uint8_t RGB pixel format.*/
        bool filterRGB(uint8_t * const dataWriteDS, uint8_t const * const dataReadUS,
                       const size_t width, const size_t height,
                       uint8_t * const dataScratch);
        
    private:
        size_t kernelWidth_;
    };
    
    
    //! General purpose Box filter USING AN INTEGRAL IMAGE..
    class FLITR_EXPORT BoxFilterII
    {
    public:
        
        BoxFilterII(const size_t kernelWidth//Width of filter kernel in pixels in US image.
        );
        
        /*! destructor */
        ~BoxFilterII();
        
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
        
        void setKernelWidth(const int kernelWidth);
        
        /*!Synchronous process method for float pixel format..*/
        bool filter(float * const dataWriteDS, float const * const dataReadUS,
                    const size_t width, const size_t height,
                    double * const IIDoubleScratch,
                    const bool recalcIntegralImage);
        
        /*!Synchronous process method for float RGB pixel format..*/
        bool filterRGB(float * const dataWriteDS, float const * const dataReadUS,
                       const size_t width, const size_t height,
                       double * const IIDoubleScratch,
                       const bool recalcIntegralImage);
        
        /*!Synchronous process method for uint8_t pixel format.*/
        bool filter(uint8_t * const dataWriteDS, uint8_t const * const dataReadUS,
                    const size_t width, const size_t height,
                    double * const IIDoubleScratch,
                    const bool recalcIntegralImage);
        
        /*!Synchronous process method for uint8_t RGB pixel format.*/
        bool filterRGB(uint8_t * const dataWriteDS, uint8_t const * const dataReadUS,
                       const size_t width, const size_t height,
                       double * const IIDoubleScratch,
                       const bool recalcIntegralImage);
        
    private:
        size_t kernelWidth_;
        
        IntegralImage integralImage_;
    };
    
    
    //! General purpose Gaussian filter.
    class FLITR_EXPORT GaussianFilter
    {
    public:
        
        GaussianFilter(const float filterRadius,//filterRadius = standardDeviation * 2.0 in US image.
                       const size_t kernelWidth//Width of filter kernel in pixels in US image.
        );
        
        /*! destructor */
        ~GaussianFilter();
        
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
        
        void setFilterRadius(const float filterRadius);
        void setKernelWidth(const int kernelWidth);
        
        float getStandardDeviation() const
        {
            return filterRadius_ * 0.5f;
        }
        
        /*!Synchronous process method for float pixel format..*/
        bool filter(float * const dataWriteDS, float const * const dataReadUS,
                    const size_t width, const size_t height,
                    float * const dataScratch);
        
        /*!Synchronous process method for float RGB pixel format..*/
        bool filterRGB(float * const dataWriteDS, float const * const dataReadUS,
                       const size_t width, const size_t height,
                       float * const dataScratch);
        
        /*!Synchronous process method for uint8_t pixel format.*/
        bool filter(uint8_t * const dataWriteDS, uint8_t const * const dataReadUS,
                    const size_t width, const size_t height,
                    uint8_t * const dataScratch);
        
        /*!Synchronous process method for uint8_t RGB pixel format.*/
        bool filterRGB(uint8_t * const dataWriteDS, uint8_t const * const dataReadUS,
                       const size_t width, const size_t height,
                       uint8_t * const dataScratch);
        
        
    private:
        void updateKernel1D();
        
        float *kernel1D_;
        
        float filterRadius_;
        size_t kernelWidth_;
    };
    
    
    //! General purpose Gaussian donwsample filter.
    class FLITR_EXPORT GaussianDownsample
    {
    public:
        
        GaussianDownsample(const float filterRadius,//filterRadius = standardDeviation * 2.0 in US image.
                           const size_t kernelWidth//Width of filter kernel in pixels in US image.
        );
        
        /*! destructor */
        ~GaussianDownsample();
        
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
        
        void setFilterRadius(const float filterRadius);
        void setKernelWidth(const int kernelWidth);
        
        /*!Synchronous process method for float pixel format..*/
        bool downsample(float * const dataWriteDS, float const * const dataReadUS,
                        const size_t widthUS, const size_t heightUS,
                        float * const dataScratch);
        
        /*!Synchronous process method for float RGB pixel format..*/
        //bool downsampleRGB(float * const dataWriteDS, float const * const dataReadUS,
        //                           const size_t widthUS, const size_t heightUS,
        //                           float * const dataScratch);
        
    private:
        void updateKernel1D();
        
        float *kernel1D_;
        
        float filterRadius_;
        size_t kernelWidth_;
    };
    
    
    //! General purpose Morphological filter.
    class FLITR_EXPORT MorphologicalFilter
    {
    public:
        MorphologicalFilter()
        {}
        
        //! Destructor
        ~MorphologicalFilter() {}
        
        //!Synchronous process method for float pixel format.
        template<typename T>
        bool erode(T * const dataWriteDS, T const * const dataReadUS,
                   size_t structElemWidth,
                   const size_t width, const size_t height,
                   T * const dataScratch)
        {
            structElemWidth=structElemWidth|1;//Make structuring element's width is odd.
            
            const size_t widthMinusStructElem=width-structElemWidth;
            const size_t heightMinusStructElem=height-structElemWidth;
            const size_t halfStructElem=(structElemWidth>>1);
            const size_t widthMinusHalfStructElem=width-halfStructElem;
            
            //First pass in x.
            for (size_t y=0; y<height; ++y)
            {
                const size_t lineOffsetFS=y * width + halfStructElem;
                const size_t lineOffsetUS=y * width;
                
                for (size_t x=0; x<widthMinusStructElem; ++x)
                {
                    size_t offsetUS=lineOffsetUS + x;
                    T minImgValue=dataReadUS[offsetUS];
                    ++offsetUS;
                    
                    for (size_t i=1; i<structElemWidth; ++i)
                    {
                        const T imgValue = dataReadUS[offsetUS];
                        if (imgValue<minImgValue) minImgValue=imgValue;
                        ++offsetUS;
                    }
                    
                    dataScratch[lineOffsetFS + x]=minImgValue;
                }
            }
            
            //Second pass in y.
            for (size_t y=0; y<heightMinusStructElem; ++y)
            {
                const size_t lineOffsetDS=(y+halfStructElem) * width;
                const size_t lineOffsetFS=y * width;
                
                for (size_t x=halfStructElem; x<widthMinusHalfStructElem; ++x)
                {
                    size_t offsetFS=lineOffsetFS + x;
                    T minImgValue=dataScratch[offsetFS];
                    offsetFS+=width;
                    
                    for (size_t j=1; j<structElemWidth; ++j)
                    {
                        const T imgValue = dataScratch[offsetFS];
                        if (imgValue<minImgValue) minImgValue=imgValue;
                        offsetFS+=width;
                    }
                    
                    dataWriteDS[lineOffsetDS + x]=minImgValue;
                }
            }
            
            return true;
        }
        
        //!Synchronous process method for float RGB pixel format.
        template<typename T>
        bool erodeRGB(T * const dataWriteDS, T const * const dataReadUS,
                      size_t structElemWidth,
                      const size_t width, const size_t height,
                      T * const dataScratch)
        {
            structElemWidth=structElemWidth|1;//Make structuring element's width is odd.
            
            const size_t widthMinusStructElem=width-structElemWidth;
            const size_t heightMinusStructElem=height-structElemWidth;
            const size_t halfStructElem=(structElemWidth>>1);
            const size_t widthMinusHalfStructElem=width-halfStructElem;
            
            //First pass in x.
            for (size_t y=0; y<height; ++y)
            {
                const size_t lineOffsetFS=y * width + halfStructElem;
                const size_t lineOffsetUS=y * width;
                
                for (size_t x=0; x<widthMinusStructElem; ++x)
                {
                    size_t offsetUS=(lineOffsetUS + x)*3;
                    T minImgValueR=dataReadUS[offsetUS+0];
                    T minImgValueG=dataReadUS[offsetUS+1];
                    T minImgValueB=dataReadUS[offsetUS+2];
                    offsetUS+=3;
                    
                    for (size_t i=1; i<structElemWidth; ++i)
                    {
                        const T imgValueR = dataReadUS[offsetUS + 0];
                        const T imgValueG = dataReadUS[offsetUS + 1];
                        const T imgValueB = dataReadUS[offsetUS + 2];
                        
                        if (imgValueR<minImgValueR) minImgValueR=imgValueR;
                        if (imgValueG<minImgValueG) minImgValueG=imgValueG;
                        if (imgValueB<minImgValueB) minImgValueB=imgValueB;
                        
                        offsetUS+=3;
                    }
                    
                    dataScratch[(lineOffsetFS + x)*3 + 0]=minImgValueR;
                    dataScratch[(lineOffsetFS + x)*3 + 1]=minImgValueG;
                    dataScratch[(lineOffsetFS + x)*3 + 2]=minImgValueB;
                }
            }
            
            //Second pass in y.
            for (size_t y=0; y<heightMinusStructElem; ++y)
            {
                const size_t lineOffsetDS=(y+halfStructElem) * width;
                const size_t lineOffsetFS=y * width;
                
                for (size_t x=halfStructElem; x<widthMinusHalfStructElem; ++x)
                {
                    size_t offsetFS=(lineOffsetFS + x)*3;
                    T minImgValueR=dataScratch[offsetFS + 0];
                    T minImgValueG=dataScratch[offsetFS + 1];
                    T minImgValueB=dataScratch[offsetFS + 2];
                    offsetFS+=width*3;
                    
                    for (size_t j=1; j<structElemWidth; ++j)
                    {
                        const T imgValueR = dataScratch[offsetFS + 0];
                        const T imgValueG = dataScratch[offsetFS + 1];
                        const T imgValueB = dataScratch[offsetFS + 2];
                        
                        if (imgValueR<minImgValueR) minImgValueR=imgValueR;
                        if (imgValueG<minImgValueG) minImgValueG=imgValueG;
                        if (imgValueB<minImgValueB) minImgValueB=imgValueB;
                        
                        offsetFS+=(width*3);
                    }
                    
                    dataWriteDS[(lineOffsetDS + x)*3 + 0]=minImgValueR;
                    dataWriteDS[(lineOffsetDS + x)*3 + 1]=minImgValueG;
                    dataWriteDS[(lineOffsetDS + x)*3 + 2]=minImgValueB;
                }
            }
            
            return true;
        }
        
        //!Synchronous process method for float pixel format.
        template<typename T>
        bool dilate(T * const dataWriteDS, T const * const dataReadUS,
                    size_t structElemWidth,
                    const size_t width, const size_t height,
                    T * const dataScratch)
        {
            structElemWidth=structElemWidth|1;//Make structuring element's width is odd.
            
            const size_t widthMinusStructElem=width-structElemWidth;
            const size_t heightMinusStructElem=height-structElemWidth;
            const size_t halfStructElem=(structElemWidth>>1);
            const size_t widthMinusHalfStructElem=width-halfStructElem;
            
            //First pass in x.
            for (size_t y=0; y<height; ++y)
            {
                const size_t lineOffsetFS=y * width + halfStructElem;
                const size_t lineOffsetUS=y * width;
                
                for (size_t x=0; x<widthMinusStructElem; ++x)
                {
                    size_t offsetUS=lineOffsetUS + x;
                    T maxImgValue=dataReadUS[offsetUS];
                    ++offsetUS;
                    
                    for (size_t i=1; i<structElemWidth; ++i)
                    {
                        const T imgValue = dataReadUS[offsetUS];
                        if (imgValue>maxImgValue) maxImgValue=imgValue;
                        ++offsetUS;
                    }
                    
                    dataScratch[lineOffsetFS + x]=maxImgValue;
                }
            }
            
            //Second pass in y.
            for (size_t y=0; y<heightMinusStructElem; ++y)
            {
                const size_t lineOffsetDS=(y+halfStructElem) * width;
                const size_t lineOffsetFS=y * width;
                
                for (size_t x=halfStructElem; x<widthMinusHalfStructElem; ++x)
                {
                    size_t offsetFS=lineOffsetFS + x;
                    T maxImgValue=dataScratch[offsetFS];
                    offsetFS+=width;
                    
                    for (size_t j=1; j<structElemWidth; ++j)
                    {
                        const T imgValue = dataScratch[offsetFS];
                        if (imgValue>maxImgValue) maxImgValue=imgValue;
                        offsetFS+=width;
                    }
                    
                    dataWriteDS[lineOffsetDS + x]=maxImgValue;
                }
            }
            
            return true;
        }
        
        //!Synchronous process method for float RGB pixel format.
        template<typename T>
        bool dilateRGB(T * const dataWriteDS, T const * const dataReadUS,
                       size_t structElemWidth,
                       const size_t width, const size_t height,
                       T * const dataScratch)
        {
            structElemWidth=structElemWidth|1;//Make structuring element's width is odd.
            
            const size_t widthMinusStructElem=width-structElemWidth;
            const size_t heightMinusStructElem=height-structElemWidth;
            const size_t halfStructElem=(structElemWidth>>1);
            const size_t widthMinusHalfStructElem=width-halfStructElem;
            
            //First pass in x.
            for (size_t y=0; y<height; ++y)
            {
                const size_t lineOffsetFS=y * width + halfStructElem;
                const size_t lineOffsetUS=y * width;
                
                for (size_t x=0; x<widthMinusStructElem; ++x)
                {
                    size_t offsetUS=(lineOffsetUS + x)*3;
                    T maxImgValueR=dataReadUS[offsetUS+0];
                    T maxImgValueG=dataReadUS[offsetUS+1];
                    T maxImgValueB=dataReadUS[offsetUS+2];
                    offsetUS+=3;
                    
                    for (size_t i=1; i<structElemWidth; ++i)
                    {
                        const T imgValueR = dataReadUS[offsetUS + 0];
                        const T imgValueG = dataReadUS[offsetUS + 1];
                        const T imgValueB = dataReadUS[offsetUS + 2];
                        
                        if (imgValueR>maxImgValueR) maxImgValueR=imgValueR;
                        if (imgValueG>maxImgValueG) maxImgValueG=imgValueG;
                        if (imgValueB>maxImgValueB) maxImgValueB=imgValueB;
                        
                        offsetUS+=3;
                    }
                    
                    dataScratch[(lineOffsetFS + x)*3 + 0]=maxImgValueR;
                    dataScratch[(lineOffsetFS + x)*3 + 1]=maxImgValueG;
                    dataScratch[(lineOffsetFS + x)*3 + 2]=maxImgValueB;
                }
            }
            
            //Second pass in y.
            for (size_t y=0; y<heightMinusStructElem; ++y)
            {
                const size_t lineOffsetDS=(y+halfStructElem) * width;
                const size_t lineOffsetFS=y * width;
                
                for (size_t x=halfStructElem; x<widthMinusHalfStructElem; ++x)
                {
                    size_t offsetFS=(lineOffsetFS + x)*3;
                    T maxImgValueR=dataScratch[offsetFS + 0];
                    T maxImgValueG=dataScratch[offsetFS + 1];
                    T maxImgValueB=dataScratch[offsetFS + 2];
                    offsetFS+=width*3;
                    
                    for (size_t j=1; j<structElemWidth; ++j)
                    {
                        const T imgValueR = dataScratch[offsetFS + 0];
                        const T imgValueG = dataScratch[offsetFS + 1];
                        const T imgValueB = dataScratch[offsetFS + 2];
                        
                        if (imgValueR>maxImgValueR) maxImgValueR=imgValueR;
                        if (imgValueG>maxImgValueG) maxImgValueG=imgValueG;
                        if (imgValueB>maxImgValueB) maxImgValueB=imgValueB;
                        
                        offsetFS+=(width*3);
                    }
                    
                    dataWriteDS[(lineOffsetDS + x)*3 + 0]=maxImgValueR;
                    dataWriteDS[(lineOffsetDS + x)*3 + 1]=maxImgValueG;
                    dataWriteDS[(lineOffsetDS + x)*3 + 2]=maxImgValueB;
                }
            }
            
            return true;
        }
        
        
        //!Synchronous process method for float pixel format.
        template<typename T>
        bool difference(T * const dataWriteDS,
                        T const * const dataReadUS_A,//Will implement A-B
                        T const * const dataReadUS_B,
                        const size_t width, const size_t height)
        {
            for (size_t y=0; y<height; ++y)
            {
                const size_t lineOffset=y * width;
                
                for (size_t x=0; x<width; ++x)
                {
                    dataWriteDS[lineOffset+x] = std::abs(int16_t(dataReadUS_A[lineOffset+x]) - int16_t(dataReadUS_B[lineOffset+x]));
                }
            }
            
            return true;
        }
        
        //!Synchronous process method for float RGB pixel format.
        template<typename T>
        bool differenceRGB(T * const dataWriteDS,
                           T const * const dataReadUS_A,//Will implement A-B
                           T const * const dataReadUS_B,
                           const size_t width, const size_t height)
        {
            for (size_t y=0; y<height; ++y)
            {
                size_t offset=y * (width * 3);
                
                for (size_t x=0; x<width; ++x)
                {
                    dataWriteDS[offset + 0] = std::abs(int16_t(dataReadUS_A[offset + 0]) - int16_t(dataReadUS_B[offset + 0]));
                    dataWriteDS[offset + 1] = std::abs(int16_t(dataReadUS_A[offset + 1]) - int16_t(dataReadUS_B[offset + 1]));
                    dataWriteDS[offset + 2] = std::abs(int16_t(dataReadUS_A[offset + 2]) - int16_t(dataReadUS_B[offset + 2]));
                    
                    offset+=3;
                }
            }
            
            return true;
        }
        
        //!Synchronous process method for float pixel format.
        template<typename T>
        bool threshold(T * const dataWriteDS,
                       T const * const dataReadUS,
                       const T t,
                       const T max,
                       const size_t width, const size_t height)
        {
            for (size_t y=0; y<height; ++y)
            {
                const size_t lineOffset=y * width;
                
                for (size_t x=0; x<width; ++x)
                {
                    const T imgValue=dataReadUS[lineOffset+x];
                    
                    dataWriteDS[lineOffset+x] = (imgValue>=t) ? max : T(0);
                }
            }
            
            return true;
        }
        
        //!Synchronous process method for float RGB pixel format.
        template<typename T>
        bool thresholdRGB(T * const dataWriteDS,
                          T const * const dataReadUS,
                          const T t,
                          const T max,
                          const size_t width, const size_t height)
        {
            for (size_t y=0; y<height; ++y)
            {
                size_t offset=(y * width)*3;
                
                for (size_t x=0; x<width; ++x)
                {
                    const T imgValueR=dataReadUS[offset + 0];
                    const T imgValueG=dataReadUS[offset + 1];
                    const T imgValueB=dataReadUS[offset + 2];
                    
                    dataWriteDS[offset + 0] = (imgValueR>=t) ? max : T(0);
                    dataWriteDS[offset + 1] = (imgValueG>=t) ? max : T(0);
                    dataWriteDS[offset + 2] = (imgValueB>=t) ? max : T(0);
                    
                    offset+=3;
                }
            }
            
            return true;
        }
    };
    
}

#endif //IMAGE_PROCESSOR_UTILS_H
