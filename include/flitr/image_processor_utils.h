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
#include <math.h>
#include <stdint.h>

#include <iostream>
#include <vector>

#ifdef FLITR_OPENCL_FOUND
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif
#endif

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
        
        /*! Constructor
         @param kernelWidth Width of filter kernel in pixels in US image.
         */
        BoxFilter(const size_t kernelWidth);
        
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
        
        //!Set the width of the box filter.
        void setKernelWidth(const int kernelWidth);
        
        float getStandardDeviation() const
        {
            return sqrtf((kernelWidth_*kernelWidth_ - 1) * (1.0f/12.0f));
        }
        
        size_t getKernelWidth() const
        {
            return kernelWidth_;
        }
        
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
        
        /*! Constructor
         @param kernelWidth Width of filter kernel in pixels in US image.
         */
        BoxFilterII(const size_t kernelWidth);
        
        /*! Destructor */
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
        
        //!Sets the width of the box filter.
        void setKernelWidth(const int kernelWidth);
        
        //!Gets the standard deviation of the average filter.
        float getStandardDeviation() const
        {
            return sqrtf((kernelWidth_*kernelWidth_ - 1) * (1.0f/12.0f));
        }
        
        //!Get the kernel width.
        size_t getKernelWidth() const
        {
            return kernelWidth_;
        }
        
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
    
    
    //! General purpose Box filter USING A RUNNING SUM.
    class FLITR_EXPORT BoxFilterRS
    {
    public:
        
        /*! Constructor
         @param kernelWidth Width of filter kernel in pixels in US image.
         */
        BoxFilterRS(const size_t kernelWidth);
        
        /*! Destructor */
        ~BoxFilterRS();
        
        //! Copy constructor
        BoxFilterRS(const BoxFilterRS& rh) :
        kernelWidth_(rh.kernelWidth_)
        {}
        
        //! Assignment operator
        BoxFilterRS& operator=(const BoxFilterRS& rh)
        {
            if (this == &rh)
            {
                return *this;
            }
            
            kernelWidth_=rh.kernelWidth_;
            
            return *this;
        }
        
        //!Set the width of the box filter.
        void setKernelWidth(const int kernelWidth);
        
        float getStandardDeviation() const
        {
            return sqrtf((kernelWidth_*kernelWidth_ - 1) * (1.0f/12.0f));
        }
        
        size_t getKernelWidth() const
        {
            return kernelWidth_;
        }
        
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
        float *historyRing_;
    };
    
    
    //! General purpose Gaussian filter.
    class FLITR_EXPORT GaussianFilter
    {
    public:
        
        /*! Constructor
         @param kernelWidth Width of filter kernel in pixels in US image.
         */
        GaussianFilter(const float filterRadius,//filterRadius = standardDeviation * 2.0 in US image.
                       const size_t kernelWidth);
        
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
        
        //!Sets the radius of the Gaussian.
        void setFilterRadius(const float filterRadius);
        
        //!Set the width of the convolution kernel.
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
        
        /*! Constructor
         @param kernelWidth Width of filter kernel in pixels in US image.
         */
        GaussianDownsample(const float filterRadius,//filterRadius = standardDeviation * 2.0 in US image.
                           const size_t kernelWidth);
        
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
        
        //!Sets the radius of the Gaussian.
        void setFilterRadius(const float filterRadius);
        
        //!Set the width of the convolution kernel.
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
    
    
    
    //struct Image
    //{
    //    std::vector<char> pixel;
    //    int width, height;
    //};
    
    
    //! General purpose Morphological filter.
    class FLITR_EXPORT MorphologicalFilter
    {
    public:
        MorphologicalFilter()
        {
            /*
             // http://www.khronos.org/registry/cl/sdk/1.1/docs/man/xhtml/clGetPlatformIDs.html
             cl_uint platformIdCount = 0;
             clGetPlatformIDs (0, nullptr, &platformIdCount);
             
             if (platformIdCount == 0) {
             std::cerr << "No OpenCL platform found" << std::endl;
             } else {
             std::cout << "Found " << platformIdCount << " platform(s)" << std::endl;
             }
             
             std::vector<cl_platform_id> platformIds (platformIdCount);
             clGetPlatformIDs (platformIdCount, platformIds.data (), nullptr);
             
             for (cl_uint i = 0; i < platformIdCount; ++i) {
             std::cout << "\t (" << (i+1) << ") : " << GetPlatformName (platformIds [i]) << std::endl;
             }
             
             // http://www.khronos.org/registry/cl/sdk/1.1/docs/man/xhtml/clGetDeviceIDs.html
             cl_uint deviceIdCount = 0;
             clGetDeviceIDs (platformIds [0], CL_DEVICE_TYPE_ALL, 0, nullptr,
             &deviceIdCount);
             
             if (deviceIdCount == 0) {
             std::cerr << "No OpenCL devices found" << std::endl;
             } else {
             std::cout << "Found " << deviceIdCount << " device(s)" << std::endl;
             }
             
             std::vector<cl_device_id> deviceIds (deviceIdCount);
             clGetDeviceIDs (platformIds [0], CL_DEVICE_TYPE_ALL, deviceIdCount,
             deviceIds.data (), nullptr);
             
             for (cl_uint i = 0; i < deviceIdCount; ++i) {
             std::cout << "\t (" << (i+1) << ") : " << GetDeviceName (deviceIds [i]) << std::endl;
             }
             
             // http://www.khronos.org/registry/cl/sdk/1.1/docs/man/xhtml/clCreateContext.html
             const cl_context_properties contextProperties [] =
             {
             CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties> (platformIds [0]),
             0, 0
             };
             
             cl_int error = CL_SUCCESS;
             cl_context context = clCreateContext (contextProperties, deviceIdCount,
             deviceIds.data (), nullptr, nullptr, &error);
             CheckError (error);
             
             std::cout << "Context created" << std::endl;
             */
            
            /*
             // Simple Gaussian blur filter
             float filter [] = {
             1, 2, 1,
             2, 4, 2,
             1, 2, 1
             };
             
             // Normalize the filter
             for (int i = 0; i < 9; ++i) {
             filter [i] /= 16.0f;
             }
             
             // Create a program from source
             cl_program program = CreateProgram (LoadKernel ("kernels/image.cl"),
             context);
             
             CheckError (clBuildProgram (program, deviceIdCount, deviceIds.data (),
             "-D FILTER_SIZE=1", nullptr, nullptr));
             
             // http://www.khronos.org/registry/cl/sdk/1.1/docs/man/xhtml/clCreateKernel.html
             cl_kernel kernel = clCreateKernel (program, "Filter", &error);
             CheckError (error);
             
             // OpenCL only supports RGBA, so we need to convert here
             const auto image = RGBtoRGBA (LoadImage ("test.ppm"));
             
             // http://www.khronos.org/registry/cl/sdk/1.1/docs/man/xhtml/clCreateImage2D.html
             static const cl_image_format format = { CL_RGBA, CL_UNORM_INT8 };
             cl_mem inputImage = clCreateImage2D (context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, &format,
             image.width, image.height, 0,
             // This is a bug in the spec
             const_cast<char*> (image.pixel.data ()),
             &error);
             CheckError (error);
             
             cl_mem outputImage = clCreateImage2D (context, CL_MEM_WRITE_ONLY, &format,
             image.width, image.height, 0,
             nullptr, &error);
             CheckError (error);
             
             // Create a buffer for the filter weights
             // http://www.khronos.org/registry/cl/sdk/1.1/docs/man/xhtml/clCreateBuffer.html
             cl_mem filterWeightsBuffer = clCreateBuffer (context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
             sizeof (float) * 9, filter, &error);
             CheckError (error);
             
             // Setup the kernel arguments
             clSetKernelArg (kernel, 0, sizeof (cl_mem), &inputImage);
             clSetKernelArg (kernel, 1, sizeof (cl_mem), &filterWeightsBuffer);
             clSetKernelArg (kernel, 2, sizeof (cl_mem), &outputImage);
             
             // http://www.khronos.org/registry/cl/sdk/1.1/docs/man/xhtml/clCreateCommandQueue.html
             cl_command_queue queue = clCreateCommandQueue (context, deviceIds [0],
             0, &error);
             CheckError (error);
             
             // Run the processing
             // http://www.khronos.org/registry/cl/sdk/1.1/docs/man/xhtml/clEnqueueNDRangeKernel.html
             std::size_t offset [3] = { 0 };
             std::size_t size [3] = { image.width, image.height, 1 };
             CheckError (clEnqueueNDRangeKernel (queue, kernel, 2, offset, size, nullptr,
             0, nullptr, nullptr));
             
             // Prepare the result image, set to black
             Image result = image;
             std::fill (result.pixel.begin (), result.pixel.end (), 0);
             
             // Get the result back to the host
             std::size_t origin [3] = { 0 };
             std::size_t region [3] = { result.width, result.height, 1 };
             clEnqueueReadImage (queue, outputImage, CL_TRUE,
             origin, region, 0, 0,
             result.pixel.data (), 0, nullptr, nullptr);
             
             SaveImage (RGBAtoRGB (result), "output.ppm");
             */
            
            /*
             //clReleaseMemObject (outputImage);
             //clReleaseMemObject (filterWeightsBuffer);
             //clReleaseMemObject (inputImage);
             
             //clReleaseCommandQueue (queue);
             
             //clReleaseKernel (kernel);
             //clReleaseProgram (program);
             
             clReleaseContext (context);
             */
        }
        
        //! Destructor
        ~MorphologicalFilter() {}
        
        /*
         std::string GetDeviceName (cl_device_id id)
         {
         size_t size = 0;
         clGetDeviceInfo (id, CL_DEVICE_NAME, 0, nullptr, &size);
         
         std::string result;
         result.resize (size);
         clGetDeviceInfo (id, CL_DEVICE_NAME, size,
         const_cast<char*> (result.data ()), nullptr);
         
         return result;
         }
         
         std::string GetPlatformName (cl_platform_id id)
         {
         size_t size = 0;
         clGetPlatformInfo (id, CL_PLATFORM_NAME, 0, nullptr, &size);
         
         std::string result;
         result.resize (size);
         clGetPlatformInfo (id, CL_PLATFORM_NAME, size,
         const_cast<char*> (result.data ()), nullptr);
         
         return result;
         }
         
         void CheckError (cl_int error)
         {
         if (error != CL_SUCCESS) {
         std::cerr << "OpenCL call failed with error " << error << std::endl;
         std::exit (1);
         }
         }
         */
        /*
         void SaveImage (const Image& img, const char* path)
         {
         std::ofstream out (path, std::ios::binary);
         
         out << "P6\n";
         out << img.width << " " << img.height << "\n";
         out << "255\n";
         out.write (img.pixel.data (), img.pixel.size ());
         }
         
         Image RGBtoRGBA (const Image& input)
         {
         Image result;
         result.width = input.width;
         result.height = input.height;
         
         for (std::size_t i = 0; i < input.pixel.size (); i += 3) {
         result.pixel.push_back (input.pixel [i + 0]);
         result.pixel.push_back (input.pixel [i + 1]);
         result.pixel.push_back (input.pixel [i + 2]);
         result.pixel.push_back (0);
         }
         
         return result;
         }
         
         Image RGBAtoRGB (const Image& input)
         {
         Image result;
         result.width = input.width;
         result.height = input.height;
         
         for (std::size_t i = 0; i < input.pixel.size (); i += 4) {
         result.pixel.push_back (input.pixel [i + 0]);
         result.pixel.push_back (input.pixel [i + 1]);
         result.pixel.push_back (input.pixel [i + 2]);
         }
         
         return result;
         }
         */
        //!Synchronous process method for T pixel format.
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
                
                T minImgValue;
                int minTTL=0;
                
                for (size_t x=0; x<widthMinusStructElem; ++x)
                {
                    //Use the time to live (TTL) of the min/max and only do a full search of min/max's TTL expires.
                    if (minTTL<=0)
                    {
                        size_t offsetUS=lineOffsetUS + x;
                        minImgValue=dataReadUS[offsetUS];
                        minTTL=0;
                        ++offsetUS;
                        
                        for (size_t i=1; i<structElemWidth; ++i)
                        {
                            const T &imgValue = dataReadUS[offsetUS];
                            
                            if (imgValue<minImgValue)
                            {
                                minImgValue=imgValue;
                                minTTL=i;
                            }
                            
                            ++offsetUS;
                        }
                    } else
                    {
                        const size_t offsetUS=lineOffsetUS + x + (structElemWidth-1);
                        const T &imgValue = dataReadUS[offsetUS];
                        
                        if (imgValue<=minImgValue)
                        {
                            minImgValue=imgValue;
                            minTTL=structElemWidth-1;
                        } else
                        {
                            --minTTL;
                        }
                    }
                    
                    dataScratch[lineOffsetFS + x]=minImgValue;
                }
            }
            
            //Second pass in y.
            T *minImgValueArr=new T[width];
            int *minTTLArr=new int[width];
            for (size_t x=0; x<width; ++x)
            {
                minTTLArr[x]=0;
            }
            
            for (size_t y=0; y<heightMinusStructElem; ++y)
            {
                const size_t lineOffsetDS=(y+halfStructElem) * width;
                const size_t lineOffsetFS=y * width;
                
                for (size_t x=halfStructElem; x<widthMinusHalfStructElem; ++x)
                {
                    //Use the time to live (TTL) of the min/max and only do a full search of min/max's TTL expires.
                    if (minTTLArr[x]<=0)
                    {
                        size_t offsetFS=lineOffsetFS + x;
                        minImgValueArr[x]=dataScratch[offsetFS];
                        minTTLArr[x]=0;
                        offsetFS+=width;
                        
                        for (size_t j=1; j<structElemWidth; ++j)
                        {
                            const T &imgValue = dataScratch[offsetFS];
                            
                            if (imgValue<minImgValueArr[x])
                            {
                                minImgValueArr[x]=imgValue;
                                minTTLArr[x]=j;
                            }
                            
                            offsetFS+=width;
                        }
                    } else
                    {
                        const size_t offsetFS=lineOffsetFS + x + (structElemWidth-1)*width;
                        const T &imgValue = dataScratch[offsetFS];
                        
                        if (imgValue<=minImgValueArr[x])
                        {
                            minImgValueArr[x]=imgValue;
                            minTTLArr[x]=structElemWidth-1;
                        } else
                        {
                            --minTTLArr[x];
                        }
                    }
                    
                    dataWriteDS[lineOffsetDS + x]=minImgValueArr[x];
                }
            }
            
            return true;
        }
        
        //!Synchronous process method for T RGB pixel format.
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
                    //!@todo Add TTL optimisation from grayscale version above.
                    size_t offsetUS=(lineOffsetUS + x)*3;
                    T minImgValueR=dataReadUS[offsetUS+0];
                    T minImgValueG=dataReadUS[offsetUS+1];
                    T minImgValueB=dataReadUS[offsetUS+2];
                    offsetUS+=3;
                    
                    for (size_t i=1; i<structElemWidth; ++i)
                    {
                        const T &imgValueR = dataReadUS[offsetUS + 0];
                        const T &imgValueG = dataReadUS[offsetUS + 1];
                        const T &imgValueB = dataReadUS[offsetUS + 2];
                        
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
                    //!@todo Add TTL optimisation from grayscale version above.
                    size_t offsetFS=(lineOffsetFS + x)*3;
                    T minImgValueR=dataScratch[offsetFS + 0];
                    T minImgValueG=dataScratch[offsetFS + 1];
                    T minImgValueB=dataScratch[offsetFS + 2];
                    offsetFS+=width*3;
                    
                    for (size_t j=1; j<structElemWidth; ++j)
                    {
                        const T &imgValueR = dataScratch[offsetFS + 0];
                        const T &imgValueG = dataScratch[offsetFS + 1];
                        const T &imgValueB = dataScratch[offsetFS + 2];
                        
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
        
        //!Synchronous process method for T pixel format.
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
                
                T maxImgValue;
                int maxTTL=0;
                
                for (size_t x=0; x<widthMinusStructElem; ++x)
                {
                    //Use the time to live (TTL) of the min/max and only do a full search of min/max's TTL expires.
                    if (maxTTL<=0)
                    {
                        size_t offsetUS=lineOffsetUS + x;
                        maxImgValue=dataReadUS[offsetUS];
                        maxTTL=0;
                        ++offsetUS;
                        
                        for (size_t i=1; i<structElemWidth; ++i)
                        {
                            const T &imgValue = dataReadUS[offsetUS];
                            
                            if (imgValue>maxImgValue)
                            {
                                maxImgValue=imgValue;
                                maxTTL=i;
                            }
                            
                            ++offsetUS;
                        }
                    } else
                    {
                        const size_t offsetUS=lineOffsetUS + x + (structElemWidth-1);
                        const T &imgValue = dataReadUS[offsetUS];
                        
                        if (imgValue>=maxImgValue)
                        {
                            maxImgValue=imgValue;
                            maxTTL=structElemWidth-1;
                        } else
                        {
                            --maxTTL;
                        }
                    }
                    
                    dataScratch[lineOffsetFS + x]=maxImgValue;
                }
            }
            
            //Second pass in y.
            T *maxImgValueArr=new T[width];
            int *maxTTLArr=new int[width];
            for (size_t x=0; x<width; ++x)
            {
                maxTTLArr[x]=0;
            }
            
            for (size_t y=0; y<heightMinusStructElem; ++y)
            {
                const size_t lineOffsetDS=(y+halfStructElem) * width;
                const size_t lineOffsetFS=y * width;
                
                for (size_t x=halfStructElem; x<widthMinusHalfStructElem; ++x)
                {
                    //Use the time to live (TTL) of the min/max and only do a full search of min/max's TTL expires.
                    if (maxTTLArr[x]<=0)
                    {
                        size_t offsetFS=lineOffsetFS + x;
                        maxImgValueArr[x]=dataScratch[offsetFS];
                        maxTTLArr[x]=0;
                        offsetFS+=width;
                        
                        for (size_t j=1; j<structElemWidth; ++j)
                        {
                            const T &imgValue = dataScratch[offsetFS];
                            
                            if (imgValue>maxImgValueArr[x])
                            {
                                maxImgValueArr[x]=imgValue;
                                maxTTLArr[x]=j;
                            }
                            
                            offsetFS+=width;
                        }
                    } else
                    {
                        const size_t offsetFS=lineOffsetFS + x + (structElemWidth-1)*width;
                        const T &imgValue = dataScratch[offsetFS];
                        
                        if (imgValue>=maxImgValueArr[x])
                        {
                            maxImgValueArr[x]=imgValue;
                            maxTTLArr[x]=structElemWidth-1;
                        } else
                        {
                            --maxTTLArr[x];
                        }
                    }
                    
                    dataWriteDS[lineOffsetDS + x]=maxImgValueArr[x];
                }
            }
            
            return true;
        }
        
        //!Synchronous process method for T RGB pixel format.
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
                    //!@todo Add TTL optimisation from grayscale version above.
                    size_t offsetUS=(lineOffsetUS + x)*3;
                    T maxImgValueR=dataReadUS[offsetUS+0];
                    T maxImgValueG=dataReadUS[offsetUS+1];
                    T maxImgValueB=dataReadUS[offsetUS+2];
                    offsetUS+=3;
                    
                    for (size_t i=1; i<structElemWidth; ++i)
                    {
                        const T &imgValueR = dataReadUS[offsetUS + 0];
                        const T &imgValueG = dataReadUS[offsetUS + 1];
                        const T &imgValueB = dataReadUS[offsetUS + 2];
                        
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
                    //!@todo Add TTL optimisation from grayscale version above.
                    size_t offsetFS=(lineOffsetFS + x)*3;
                    T maxImgValueR=dataScratch[offsetFS + 0];
                    T maxImgValueG=dataScratch[offsetFS + 1];
                    T maxImgValueB=dataScratch[offsetFS + 2];
                    offsetFS+=width*3;
                    
                    for (size_t j=1; j<structElemWidth; ++j)
                    {
                        const T &imgValueR = dataScratch[offsetFS + 0];
                        const T &imgValueG = dataScratch[offsetFS + 1];
                        const T &imgValueB = dataScratch[offsetFS + 2];
                        
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
        
        
        //!Synchronous process method for T pixel format.
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
        
        //!Synchronous process method for T RGB pixel format.
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
        
        //!Synchronous process method for T pixel format.
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
                    const T &imgValue=dataReadUS[lineOffset+x];
                    
                    dataWriteDS[lineOffset+x] = (imgValue>=t) ? max : T(0);
                }
            }
            
            return true;
        }
        
        //!Synchronous process method for T RGB pixel format.
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
                    const T &imgValueR=dataReadUS[offset + 0];
                    const T &imgValueG=dataReadUS[offset + 1];
                    const T &imgValueB=dataReadUS[offset + 2];
                    
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
