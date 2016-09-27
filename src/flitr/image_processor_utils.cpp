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

#define _USE_MATH_DEFINES
#include <memory>
#include <cmath>
#include <cstring>

#include <flitr/image_processor_utils.h>
#include <sstream>

using namespace flitr;
using std::shared_ptr;


//=========== BoxFilter ==========//

BoxFilter::BoxFilter(const size_t kernelWidth) :
kernelWidth_(kernelWidth|1)//Make sure the kernel width is odd.
{}

BoxFilter::~BoxFilter() {}

void BoxFilter::setKernelWidth(const int kernelWidth)
{
    kernelWidth_=kernelWidth|1;
}

bool BoxFilter::filter(float * const dataWriteDS, float const * const dataReadUS,
                       const size_t width, const size_t height,
                       float * const dataScratch)
{
    const size_t widthMinusKernel=width-kernelWidth_;
    const size_t heightMinusKernel=height-kernelWidth_;
    const float recipKernelWidth=1.0f/kernelWidth_;
    const size_t halfKernelWidth=(kernelWidth_>>1);
    const size_t widthMinusHalfKernel=width-halfKernelWidth;
    
    for (size_t y=0; y<height; ++y)
    {
        const size_t lineOffsetFS=y * width + halfKernelWidth;
        const size_t lineOffsetUS=y * width;
        
        for (size_t x=0; x<widthMinusKernel; ++x)
        {
            float xFiltValue=0.0f;
            
            for (size_t j=0; j<kernelWidth_; ++j)
            {
                xFiltValue += dataReadUS[(lineOffsetUS + x) + j];
            }
            
            dataScratch[lineOffsetFS + x]=xFiltValue*recipKernelWidth;
        }
    }
    
    for (size_t y=0; y<heightMinusKernel; ++y)
    {
        const size_t lineOffsetDS=(y + halfKernelWidth) * width;
        const size_t lineOffsetFS=y * width;
        
        for (size_t x=halfKernelWidth; x<widthMinusHalfKernel; ++x)
        {
            float filtValue=0.0f;
            
            for (size_t j=0; j<kernelWidth_; ++j)
            {
                filtValue += dataScratch[(lineOffsetFS + x) + j*width];
                //Performance note: The XCode profiler shows that the above multiply by width has little performance overhead compared to the loop in x above.
                //                  Is the code memory bandwidth limited?
            }
            
            dataWriteDS[lineOffsetDS + x]=filtValue*recipKernelWidth;
        }
    }
    
    return true;
}

bool BoxFilter::filterRGB(float * const dataWriteDS, float const * const dataReadUS,
                       const size_t width, const size_t height,
                       float * const dataScratch)
{
    const size_t widthMinusKernel=width-kernelWidth_;
    const size_t heightMinusKernel=height-kernelWidth_;
    const float recipKernelWidth=1.0f/kernelWidth_;
    const size_t halfKernelWidth=(kernelWidth_>>1);
    const size_t widthMinusHalfKernel=width-halfKernelWidth;
    
    for (size_t y=0; y<height; ++y)
    {
        const size_t lineOffsetFS=y * width + halfKernelWidth;
        const size_t lineOffsetUS=y * width;
        
        for (size_t x=0; x<widthMinusKernel; ++x)
        {
            float xFiltValueR=0.0f;
            float xFiltValueG=0.0f;
            float xFiltValueB=0.0f;
            
            for (size_t j=0; j<kernelWidth_; ++j)
            {
                xFiltValueR += dataReadUS[((lineOffsetUS + x) + j)*3 + 0];
                xFiltValueG += dataReadUS[((lineOffsetUS + x) + j)*3 + 1];
                xFiltValueB += dataReadUS[((lineOffsetUS + x) + j)*3 + 2];
            }
            
            dataScratch[(lineOffsetFS + x)*3 + 0]=xFiltValueR*recipKernelWidth;
            dataScratch[(lineOffsetFS + x)*3 + 1]=xFiltValueG*recipKernelWidth;
            dataScratch[(lineOffsetFS + x)*3 + 2]=xFiltValueB*recipKernelWidth;
        }
    }
    
    for (size_t y=0; y<heightMinusKernel; ++y)
    {
        const size_t lineOffsetDS=(y + halfKernelWidth) * width;
        const size_t lineOffsetFS=y * width;
        
        for (size_t x=halfKernelWidth; x<widthMinusHalfKernel; ++x)
        {
            float filtValueR=0.0f;
            float filtValueG=0.0f;
            float filtValueB=0.0f;
            
            for (size_t j=0; j<kernelWidth_; ++j)
            {
                filtValueR += dataScratch[((lineOffsetFS + x) + j*width)*3 + 0];
                filtValueG += dataScratch[((lineOffsetFS + x) + j*width)*3 + 1];
                filtValueB += dataScratch[((lineOffsetFS + x) + j*width)*3 + 2];
            }
            
            dataWriteDS[(lineOffsetDS + x)*3 + 0]=filtValueR*recipKernelWidth;
            dataWriteDS[(lineOffsetDS + x)*3 + 1]=filtValueG*recipKernelWidth;
            dataWriteDS[(lineOffsetDS + x)*3 + 2]=filtValueB*recipKernelWidth;
        }
    }
    
    return true;
}

bool BoxFilter::filter(uint8_t * const dataWriteDS, uint8_t const * const dataReadUS,
                       const size_t width, const size_t height,
                       uint8_t * const dataScratch)
{
    const size_t widthMinusKernel=width-kernelWidth_;
    const size_t heightMinusKernel=height-kernelWidth_;
    const size_t halfKernelWidth=(kernelWidth_>>1);
    const size_t widthMinusHalfKernel=width-halfKernelWidth;
    
    for (size_t y=0; y<height; ++y)
    {
        const size_t lineOffsetFS=y * width + halfKernelWidth;
        const size_t lineOffsetUS=y * width;
        
        for (size_t x=0; x<widthMinusKernel; ++x)
        {
            //Variable to hold sum across kernel. Need more bits than the 8 of the uint8_t.
            size_t xFiltValue=0.0f;
            
            for (size_t j=0; j<kernelWidth_; ++j)
            {
                xFiltValue += dataReadUS[(lineOffsetUS + x) + j];
            }
            
            dataScratch[lineOffsetFS + x]=xFiltValue/kernelWidth_;
        }
    }
    
    for (size_t y=0; y<heightMinusKernel; ++y)
    {
        const size_t lineOffsetDS=(y + halfKernelWidth) * width;
        const size_t lineOffsetFS=y * width;
        
        for (size_t x=halfKernelWidth; x<widthMinusHalfKernel; ++x)
        {
            //Variable to hold sum across kernel. Need more bits than the 8 of the uint8_t.
            size_t filtValue=0.0f;
            
            for (size_t j=0; j<kernelWidth_; ++j)
            {
                filtValue += dataScratch[(lineOffsetFS + x) + j*width];
                //Performance note: The XCode profiler shows that the above multiply by width has little performance overhead compared to the loop in x above.
                //                  Is the code memory bandwidth limited?
            }
            
            dataWriteDS[lineOffsetDS + x]=filtValue/kernelWidth_;
        }
    }
    
    return true;
}

bool BoxFilter::filterRGB(uint8_t * const dataWriteDS, uint8_t const * const dataReadUS,
                          const size_t width, const size_t height,
                          uint8_t * const dataScratch)
{
    const size_t widthMinusKernel=width-kernelWidth_;
    const size_t heightMinusKernel=height-kernelWidth_;
    const size_t halfKernelWidth=(kernelWidth_>>1);
    const size_t widthMinusHalfKernel=width-halfKernelWidth;
    
    for (size_t y=0; y<height; ++y)
    {
        const size_t lineOffsetFS=y * width + halfKernelWidth;
        const size_t lineOffsetUS=y * width;
        
        for (size_t x=0; x<widthMinusKernel; ++x)
        {
            //Variable to hold sum across kernel. Need more bits than the 8 of the uint8_t.
            size_t xFiltValueR=0.0f;
            size_t xFiltValueG=0.0f;
            size_t xFiltValueB=0.0f;
            
            for (size_t j=0; j<kernelWidth_; ++j)
            {
                xFiltValueR += dataReadUS[((lineOffsetUS + x) + j)*3 + 0];
                xFiltValueG += dataReadUS[((lineOffsetUS + x) + j)*3 + 1];
                xFiltValueB += dataReadUS[((lineOffsetUS + x) + j)*3 + 2];
            }
            
            dataScratch[(lineOffsetFS + x)*3 + 0]=xFiltValueR/kernelWidth_;
            dataScratch[(lineOffsetFS + x)*3 + 1]=xFiltValueG/kernelWidth_;
            dataScratch[(lineOffsetFS + x)*3 + 2]=xFiltValueB/kernelWidth_;
        }
    }
    
    for (size_t y=0; y<heightMinusKernel; ++y)
    {
        const size_t lineOffsetDS=(y + halfKernelWidth) * width;
        const size_t lineOffsetFS=y * width;
        
        for (size_t x=halfKernelWidth; x<widthMinusHalfKernel; ++x)
        {
            //Variable to hold sum across kernel. Need more bits than the 8 of the uint8_t.
            size_t filtValueR=0.0f;
            size_t filtValueG=0.0f;
            size_t filtValueB=0.0f;
            
            for (size_t j=0; j<kernelWidth_; ++j)
            {
                filtValueR += dataScratch[((lineOffsetFS + x) + j*width)*3 + 0];
                filtValueG += dataScratch[((lineOffsetFS + x) + j*width)*3 + 1];
                filtValueB += dataScratch[((lineOffsetFS + x) + j*width)*3 + 2];
            }
            
            dataWriteDS[(lineOffsetDS + x)*3 + 0]=filtValueR/kernelWidth_;
            dataWriteDS[(lineOffsetDS + x)*3 + 1]=filtValueG/kernelWidth_;
            dataWriteDS[(lineOffsetDS + x)*3 + 2]=filtValueB/kernelWidth_;
        }
    }
    
    return true;
}

//=========================================//



//=========== BoxFilterII ==========//

BoxFilterII::BoxFilterII(const size_t kernelWidth) :
kernelWidth_(kernelWidth|1)//Make sure the kernel width is odd.
{}

BoxFilterII::~BoxFilterII() {}

void BoxFilterII::setKernelWidth(const int kernelWidth)
{
    kernelWidth_=kernelWidth|1;
}

bool BoxFilterII::filter(float * const dataWriteDS, float const * const dataReadUS,
                          const size_t width, const size_t height,
                          double * const IIDoubleScratch,
                          const bool recalcIntegralImage)
{
    if (recalcIntegralImage)
    {
        integralImage_.process(IIDoubleScratch, dataReadUS, width, height);
    }
    
    const size_t kernelWidth=kernelWidth_;
    const size_t halfKernelWidth=(kernelWidth_>>1);
    const size_t widthMinusKernel=width - kernelWidth_;
    const size_t heightMinusKernel=height - kernelWidth_;
    const float recipKernelWidthSq=1.0f / (kernelWidth_*kernelWidth_);
    const size_t widthTimesKernelWidth = width*kernelWidth_;
    
    for (size_t y=0; y<heightMinusKernel; ++y)
    {
        size_t lineOffset=(y+halfKernelWidth+1) * width + (halfKernelWidth+1);
        size_t lineOffsetII=(y+kernelWidth) * width + kernelWidth;
        
        for (size_t x=0; x<widthMinusKernel; ++x)
        {
            const float c=IIDoubleScratch[lineOffsetII]
            - IIDoubleScratch[lineOffsetII - kernelWidth]
            - IIDoubleScratch[lineOffsetII - widthTimesKernelWidth]
            + IIDoubleScratch[lineOffsetII - kernelWidth - widthTimesKernelWidth];
            
            dataWriteDS[lineOffset]=c * recipKernelWidthSq;
            
            ++lineOffset;
            ++lineOffsetII;
        }
    }
    
    return true;
}

bool BoxFilterII::filterRGB(float * const dataWriteDS, float const * const dataReadUS,
                             const size_t width, const size_t height,
                             double * const IIDoubleScratch,
                             const bool recalcIntegralImage)
{
    if (recalcIntegralImage)
    {
        integralImage_.processRGB(IIDoubleScratch, dataReadUS, width, height);
    }
    
    const size_t kernelWidth=kernelWidth_;
    const size_t halfKernelWidth=(kernelWidth_>>1);
    const size_t widthMinusKernel=width - kernelWidth_;
    const size_t heightMinusKernel=height - kernelWidth_;
    const float recipKernelWidthSq=1.0f / (kernelWidth_*kernelWidth_);
    const size_t widthTimesKernelWidth = width*kernelWidth_;
    
    for (size_t y=0; y<heightMinusKernel; ++y)
    {
        size_t lineOffset=(y+halfKernelWidth+1) * width + (halfKernelWidth+1);
        size_t lineOffsetII=(y+kernelWidth) * width + kernelWidth_;
        
        for (size_t x=0; x<widthMinusKernel; ++x)
        {
            const float R=IIDoubleScratch[lineOffsetII*3 + 0]
            - IIDoubleScratch[(lineOffsetII - kernelWidth)*3 + 0]
            - IIDoubleScratch[(lineOffsetII - widthTimesKernelWidth)*3 + 0]
            + IIDoubleScratch[(lineOffsetII - kernelWidth - widthTimesKernelWidth)*3 + 0];
            const float G=IIDoubleScratch[lineOffsetII*3 + 1]
            - IIDoubleScratch[(lineOffsetII - kernelWidth)*3 + 1]
            - IIDoubleScratch[(lineOffsetII - widthTimesKernelWidth)*3 + 1]
            + IIDoubleScratch[(lineOffsetII - kernelWidth - widthTimesKernelWidth)*3 + 1];
            const float B=IIDoubleScratch[lineOffsetII*3 + 2]
            - IIDoubleScratch[(lineOffsetII - kernelWidth)*3 + 2]
            - IIDoubleScratch[(lineOffsetII - widthTimesKernelWidth)*3 + 2]
            + IIDoubleScratch[(lineOffsetII - kernelWidth - widthTimesKernelWidth)*3 + 2];
            
            dataWriteDS[lineOffset*3 + 0]=R * recipKernelWidthSq;
            dataWriteDS[lineOffset*3 + 1]=G * recipKernelWidthSq;
            dataWriteDS[lineOffset*3 + 2]=B * recipKernelWidthSq;
            
            ++lineOffset;
            ++lineOffsetII;
        }
    }
    
    return true;
}

bool BoxFilterII::filter(uint8_t * const dataWriteDS, uint8_t const * const dataReadUS,
                         const size_t width, const size_t height,
                         double * const IIDoubleScratch,
                         const bool recalcIntegralImage)
{
    if (recalcIntegralImage)
    {
        integralImage_.process(IIDoubleScratch, dataReadUS, width, height);
    }
    
    const size_t kernelWidth=kernelWidth_;
    const size_t halfKernelWidth=(kernelWidth_>>1);
    const size_t widthMinusKernel=width - kernelWidth_;
    const size_t heightMinusKernel=height - kernelWidth_;
    const float recipKernelWidthSq=1.0f / (kernelWidth_*kernelWidth_);
    const size_t widthTimesKernelWidth = width*kernelWidth_;
    
    for (size_t y=0; y<heightMinusKernel; ++y)
    {
        size_t lineOffset=(y+halfKernelWidth+1) * width + (halfKernelWidth+1);
        size_t lineOffsetII=(y+kernelWidth) * width + kernelWidth;
        
        for (size_t x=0; x<widthMinusKernel; ++x)
        {
            const float c=IIDoubleScratch[lineOffsetII]
            - IIDoubleScratch[lineOffsetII - kernelWidth]
            - IIDoubleScratch[lineOffsetII - widthTimesKernelWidth]
            + IIDoubleScratch[lineOffsetII - kernelWidth - widthTimesKernelWidth];
            
            dataWriteDS[lineOffset]=uint8_t(c * recipKernelWidthSq + 0.5f);
            
            ++lineOffset;
            ++lineOffsetII;
        }
    }
    
    return true;
}

bool BoxFilterII::filterRGB(uint8_t * const dataWriteDS, uint8_t const * const dataReadUS,
                            const size_t width, const size_t height,
                            double * const IIDoubleScratch,
                            const bool recalcIntegralImage)
{
    if (recalcIntegralImage)
    {
        integralImage_.processRGB(IIDoubleScratch, dataReadUS, width, height);
    }
    
    const size_t kernelWidth=kernelWidth_;
    const size_t halfKernelWidth=(kernelWidth_>>1);
    const size_t widthMinusKernel=width - kernelWidth_;
    const size_t heightMinusKernel=height - kernelWidth_;
    const float recipKernelWidthSq=1.0f / (kernelWidth_*kernelWidth_);
    const size_t widthTimesKernelWidth = width*kernelWidth_;
    
    for (size_t y=0; y<heightMinusKernel; ++y)
    {
        size_t lineOffset=(y+halfKernelWidth+1) * width + (halfKernelWidth+1);
        size_t lineOffsetII=(y+kernelWidth) * width + kernelWidth;
        
        for (size_t x=0; x<widthMinusKernel; ++x)
        {
            const float R=IIDoubleScratch[lineOffsetII*3 + 0]
            - IIDoubleScratch[(lineOffsetII - kernelWidth)*3 + 0]
            - IIDoubleScratch[(lineOffsetII - widthTimesKernelWidth)*3 + 0]
            + IIDoubleScratch[(lineOffsetII - kernelWidth - widthTimesKernelWidth)*3 + 0];
            const float G=IIDoubleScratch[lineOffsetII*3 + 1]
            - IIDoubleScratch[(lineOffsetII - kernelWidth)*3 + 1]
            - IIDoubleScratch[(lineOffsetII - widthTimesKernelWidth)*3 + 1]
            + IIDoubleScratch[(lineOffsetII - kernelWidth - widthTimesKernelWidth)*3 + 1];
            const float B=IIDoubleScratch[lineOffsetII*3 + 2]
            - IIDoubleScratch[(lineOffsetII - kernelWidth)*3 + 2]
            - IIDoubleScratch[(lineOffsetII - widthTimesKernelWidth)*3 + 2]
            + IIDoubleScratch[(lineOffsetII - kernelWidth - widthTimesKernelWidth)*3 + 2];
            
            dataWriteDS[lineOffset*3 + 0]=uint8_t(R * recipKernelWidthSq + 0.5f);
            dataWriteDS[lineOffset*3 + 1]=uint8_t(G * recipKernelWidthSq + 0.5f);
            dataWriteDS[lineOffset*3 + 2]=uint8_t(B * recipKernelWidthSq + 0.5f);
            
            ++lineOffset;
            ++lineOffsetII;
        }
    }
    
    return true;
}

//=========================================//
//=========== BoxFilterRS ==========//

BoxFilterRS::BoxFilterRS(const size_t kernelWidth) :
historyRing_(nullptr)
{
    setKernelWidth(kernelWidth);
}

BoxFilterRS::~BoxFilterRS()
{
    delete [] historyRing_;
}

void BoxFilterRS::setKernelWidth(const int kernelWidth)
{
    kernelWidth_=kernelWidth|1;//Make sure the kernel width is odd.
    
    if (historyRing_!=nullptr) delete [] historyRing_;
    
    historyRing_=new float[kernelWidth_];
    
    memset(historyRing_, 0, kernelWidth_ * sizeof(float));
}

bool BoxFilterRS::filter(float * const dataWriteDS, float const * const dataReadUS,
                       const size_t width, const size_t height,
                       float * const dataScratch)
{
    const size_t heightMinusKernel=height-kernelWidth_;
    const float recipKernelWidth=1.0f/kernelWidth_;
    const size_t halfKernelWidth=(kernelWidth_>>1);
    const size_t widthMinusHalfKernel=width-halfKernelWidth;
    

    for (size_t y=0; y<height; ++y)
    {
        const size_t lineOffsetUS=y * width;
        const size_t lineOffsetFS=y * width - halfKernelWidth;
        
        float rs=0.0;
        size_t historyPos=0;
        
        for (size_t x=0; x<(kernelWidth_-1); ++x)
        {
            rs += dataReadUS[lineOffsetUS + x];
            historyRing_[historyPos++]=dataReadUS[lineOffsetUS + x];
        }
        
        for (size_t x=(kernelWidth_-1); x<width; ++x)
        {
            rs += dataReadUS[lineOffsetUS + x];
            historyRing_[historyPos]=dataReadUS[lineOffsetUS + x];
            
            //historyPos=(historyPos+1)%kernelWidth_;
            ++historyPos;
            if (historyPos>=kernelWidth_) historyPos=0;
            
            dataScratch[lineOffsetFS + x]=rs * recipKernelWidth;
            
            rs -= historyRing_[historyPos];
        }
    }
    
    float yHistoryRing[width * kernelWidth_];
    float yRS[width];
    size_t yHistoryPos=0;
    
    for (size_t x=halfKernelWidth; x<widthMinusHalfKernel; ++x)
    {
        yRS[x] = 0.0;
    }
    
    for (size_t y=0; y<(kernelWidth_-1); ++y)
    {
        const size_t lineOffsetFS=y * width;
        
        const size_t historyLineOffset=(yHistoryPos++) * width;

        for (size_t x=halfKernelWidth; x<widthMinusHalfKernel; ++x)
        {
            yRS[x] += dataScratch[lineOffsetFS + x];
            yHistoryRing[historyLineOffset + x]=dataScratch[lineOffsetFS + x];
        }
    }

    for (size_t y=(kernelWidth_-1); y<height; ++y)
    {
        const size_t lineOffsetFS=y * width;
        const size_t lineOffsetDS=(y - halfKernelWidth) * width;
        
        const size_t historyLineOffset=yHistoryPos*width;

        const size_t yNextHistoryPos=(yHistoryPos+1)%kernelWidth_;
        const size_t nextHistoryLineOffset=yNextHistoryPos*width;
        
        for (size_t x=halfKernelWidth; x<widthMinusHalfKernel; ++x)
        {
            yRS[x] += dataScratch[lineOffsetFS + x];
            yHistoryRing[historyLineOffset + x]=dataScratch[lineOffsetFS + x];
            
            dataWriteDS[lineOffsetDS + x]=yRS[x] * recipKernelWidth;
            
            yRS[x] -= yHistoryRing[nextHistoryLineOffset + x];
        }
        
        yHistoryPos = yNextHistoryPos;
    }
    
    return true;
}

bool BoxFilterRS::filterRGB(float * const dataWriteDS, float const * const dataReadUS,
                          const size_t width, const size_t height,
                          float * const dataScratch)
{
    const size_t widthMinusKernel=width-kernelWidth_;
    const size_t heightMinusKernel=height-kernelWidth_;
    const float recipKernelWidth=1.0f/kernelWidth_;
    const size_t halfKernelWidth=(kernelWidth_>>1);
    const size_t widthMinusHalfKernel=width-halfKernelWidth;
    
    for (size_t y=0; y<height; ++y)
    {
        const size_t lineOffsetFS=y * width + halfKernelWidth;
        const size_t lineOffsetUS=y * width;
        
        for (size_t x=0; x<widthMinusKernel; ++x)
        {
            float xFiltValueR=0.0f;
            float xFiltValueG=0.0f;
            float xFiltValueB=0.0f;
            
            for (size_t j=0; j<kernelWidth_; ++j)
            {
                xFiltValueR += dataReadUS[((lineOffsetUS + x) + j)*3 + 0];
                xFiltValueG += dataReadUS[((lineOffsetUS + x) + j)*3 + 1];
                xFiltValueB += dataReadUS[((lineOffsetUS + x) + j)*3 + 2];
            }
            
            dataScratch[(lineOffsetFS + x)*3 + 0]=xFiltValueR*recipKernelWidth;
            dataScratch[(lineOffsetFS + x)*3 + 1]=xFiltValueG*recipKernelWidth;
            dataScratch[(lineOffsetFS + x)*3 + 2]=xFiltValueB*recipKernelWidth;
        }
    }
    
    for (size_t y=0; y<heightMinusKernel; ++y)
    {
        const size_t lineOffsetDS=(y + halfKernelWidth) * width;
        const size_t lineOffsetFS=y * width;
        
        for (size_t x=halfKernelWidth; x<widthMinusHalfKernel; ++x)
        {
            float filtValueR=0.0f;
            float filtValueG=0.0f;
            float filtValueB=0.0f;
            
            for (size_t j=0; j<kernelWidth_; ++j)
            {
                filtValueR += dataScratch[((lineOffsetFS + x) + j*width)*3 + 0];
                filtValueG += dataScratch[((lineOffsetFS + x) + j*width)*3 + 1];
                filtValueB += dataScratch[((lineOffsetFS + x) + j*width)*3 + 2];
            }
            
            dataWriteDS[(lineOffsetDS + x)*3 + 0]=filtValueR*recipKernelWidth;
            dataWriteDS[(lineOffsetDS + x)*3 + 1]=filtValueG*recipKernelWidth;
            dataWriteDS[(lineOffsetDS + x)*3 + 2]=filtValueB*recipKernelWidth;
        }
    }
    
    return true;
}

bool BoxFilterRS::filter(uint8_t * const dataWriteDS, uint8_t const * const dataReadUS,
                       const size_t width, const size_t height,
                       uint8_t * const dataScratch)
{
    const size_t widthMinusKernel=width-kernelWidth_;
    const size_t heightMinusKernel=height-kernelWidth_;
    const size_t halfKernelWidth=(kernelWidth_>>1);
    const size_t widthMinusHalfKernel=width-halfKernelWidth;
    
    for (size_t y=0; y<height; ++y)
    {
        const size_t lineOffsetFS=y * width + halfKernelWidth;
        const size_t lineOffsetUS=y * width;
        
        for (size_t x=0; x<widthMinusKernel; ++x)
        {
            //Variable to hold sum across kernel. Need more bits than the 8 of the uint8_t.
            size_t xFiltValue=0.0f;
            
            for (size_t j=0; j<kernelWidth_; ++j)
            {
                xFiltValue += dataReadUS[(lineOffsetUS + x) + j];
            }
            
            dataScratch[lineOffsetFS + x]=xFiltValue/kernelWidth_;
        }
    }
    
    for (size_t y=0; y<heightMinusKernel; ++y)
    {
        const size_t lineOffsetDS=(y + halfKernelWidth) * width;
        const size_t lineOffsetFS=y * width;
        
        for (size_t x=halfKernelWidth; x<widthMinusHalfKernel; ++x)
        {
            //Variable to hold sum across kernel. Need more bits than the 8 of the uint8_t.
            size_t filtValue=0.0f;
            
            for (size_t j=0; j<kernelWidth_; ++j)
            {
                filtValue += dataScratch[(lineOffsetFS + x) + j*width];
                //Performance note: The XCode profiler shows that the above multiply by width has little performance overhead compared to the loop in x above.
                //                  Is the code memory bandwidth limited?
            }
            
            dataWriteDS[lineOffsetDS + x]=filtValue/kernelWidth_;
        }
    }
    
    return true;
}

bool BoxFilterRS::filterRGB(uint8_t * const dataWriteDS, uint8_t const * const dataReadUS,
                          const size_t width, const size_t height,
                          uint8_t * const dataScratch)
{
    const size_t widthMinusKernel=width-kernelWidth_;
    const size_t heightMinusKernel=height-kernelWidth_;
    const size_t halfKernelWidth=(kernelWidth_>>1);
    const size_t widthMinusHalfKernel=width-halfKernelWidth;
    
    for (size_t y=0; y<height; ++y)
    {
        const size_t lineOffsetFS=y * width + halfKernelWidth;
        const size_t lineOffsetUS=y * width;
        
        for (size_t x=0; x<widthMinusKernel; ++x)
        {
            //Variable to hold sum across kernel. Need more bits than the 8 of the uint8_t.
            size_t xFiltValueR=0.0f;
            size_t xFiltValueG=0.0f;
            size_t xFiltValueB=0.0f;
            
            for (size_t j=0; j<kernelWidth_; ++j)
            {
                xFiltValueR += dataReadUS[((lineOffsetUS + x) + j)*3 + 0];
                xFiltValueG += dataReadUS[((lineOffsetUS + x) + j)*3 + 1];
                xFiltValueB += dataReadUS[((lineOffsetUS + x) + j)*3 + 2];
            }
            
            dataScratch[(lineOffsetFS + x)*3 + 0]=xFiltValueR/kernelWidth_;
            dataScratch[(lineOffsetFS + x)*3 + 1]=xFiltValueG/kernelWidth_;
            dataScratch[(lineOffsetFS + x)*3 + 2]=xFiltValueB/kernelWidth_;
        }
    }
    
    for (size_t y=0; y<heightMinusKernel; ++y)
    {
        const size_t lineOffsetDS=(y + halfKernelWidth) * width;
        const size_t lineOffsetFS=y * width;
        
        for (size_t x=halfKernelWidth; x<widthMinusHalfKernel; ++x)
        {
            //Variable to hold sum across kernel. Need more bits than the 8 of the uint8_t.
            size_t filtValueR=0.0f;
            size_t filtValueG=0.0f;
            size_t filtValueB=0.0f;
            
            for (size_t j=0; j<kernelWidth_; ++j)
            {
                filtValueR += dataScratch[((lineOffsetFS + x) + j*width)*3 + 0];
                filtValueG += dataScratch[((lineOffsetFS + x) + j*width)*3 + 1];
                filtValueB += dataScratch[((lineOffsetFS + x) + j*width)*3 + 2];
            }
            
            dataWriteDS[(lineOffsetDS + x)*3 + 0]=filtValueR/kernelWidth_;
            dataWriteDS[(lineOffsetDS + x)*3 + 1]=filtValueG/kernelWidth_;
            dataWriteDS[(lineOffsetDS + x)*3 + 2]=filtValueB/kernelWidth_;
        }
    }
    
    return true;
}

//=========================================//
//=========================================//



//=========== GaussianFilter ==========//

GaussianFilter::GaussianFilter(const float filterRadius,
                               const size_t kernelWidth) :
kernel1D_(nullptr),
filterRadius_(filterRadius),
kernelWidth_(kernelWidth|1)//Make sure the kernel width is odd.
{
    updateKernel1D();
}

GaussianFilter::~GaussianFilter()
{
    delete [] kernel1D_;
}

void GaussianFilter::updateKernel1D()
{
    if (kernel1D_!=nullptr) delete [] kernel1D_;
    
    kernel1D_=new float[kernelWidth_];
    
    const float kernelCentre=kernelWidth_ * 0.5f;
    const float sigma=filterRadius_ * 0.5f;
    
    const float twoSigmaSquared=2.0f * (sigma*sigma);
    const float stdNorm=1.0f / (sqrt(2.0f*float(M_PI))*sigma);
    
    float kernelSum=0.0f;
    
    for (size_t i=0; i<kernelWidth_; ++i)
    {
        const float r=(i + 0.5f) - kernelCentre;
        const float g=stdNorm * exp(-(r*r)/twoSigmaSquared);
        kernel1D_[i]=g;
        kernelSum+=g;
    }
    
    const float recipKernelSum=1.0f / kernelSum;
    
    //=== Ensure that the kernel is normalised within the specified kernel width!
    for (size_t i=0; i<kernelWidth_; ++i)
    {
        kernel1D_[i] *= recipKernelSum;
    }
}

void GaussianFilter::setFilterRadius(const float filterRadius)
{
    filterRadius_=filterRadius;
    updateKernel1D();
}

void GaussianFilter::setKernelWidth(const int kernelWidth)
{
    kernelWidth_=kernelWidth|1;
    updateKernel1D();
}

bool GaussianFilter::filter(float * const dataWriteDS, float const * const dataReadUS,
                            const size_t width, const size_t height,
                            float * const dataScratch)
{
    const size_t kernelWidth=kernelWidth_;
    const size_t widthMinusKernel=width-kernelWidth_;
    const size_t heightMinusKernel=height-kernelWidth_;
    
    const size_t halfKernelWidth=(kernelWidth_>>1);
    const size_t widthMinusHalfKernel=width-halfKernelWidth;
    
    for (size_t y=0; y<height; ++y)
    {
        const size_t lineOffsetFS=y * width + halfKernelWidth;
        const size_t lineOffsetUS=y * width;
        
        for (size_t x=0; x<widthMinusKernel; ++x)
        {
            float xFiltValue=0.0f;
            
            for (size_t j=0; j<kernelWidth; ++j)
            {
                xFiltValue += dataReadUS[(lineOffsetUS + x) + j] * kernel1D_[j];
            }
            
            dataScratch[lineOffsetFS + x]=xFiltValue;
        }
    }
    
    for (size_t y=0; y<heightMinusKernel; ++y)
    {
        const size_t lineOffsetDS=(y + halfKernelWidth) * width;
        const size_t lineOffsetFS=y * width;
        
        for (size_t x=halfKernelWidth; x<widthMinusHalfKernel; ++x)
        {
            float filtValue=0.0f;
            
            for (size_t j=0; j<kernelWidth; ++j)
            {
                filtValue += dataScratch[(lineOffsetFS + x) + j*width] * kernel1D_[j];
                //Performance note: The XCode profiler shows that the above multiply by width has little performance overhead compared to the loop in x above.
                //                  Is the code memory bandwidth limited?
            }
            
            dataWriteDS[lineOffsetDS + x]=filtValue;
        }
    }
    
    return true;
}

bool GaussianFilter::filterRGB(float * const dataWriteDS, float const * const dataReadUS,
                               const size_t width, const size_t height,
                               float * const dataScratch)
{
    const size_t kernelWidth=kernelWidth_;
    const size_t widthMinusKernel=width-kernelWidth_;
    const size_t heightMinusKernel=height-kernelWidth_;
    
    const size_t halfKernelWidth=(kernelWidth_>>1);
    const size_t widthMinusHalfKernel=width-halfKernelWidth;
    
    for (size_t y=0; y<height; ++y)
    {
        const size_t lineOffsetFS=y * width + halfKernelWidth;
        const size_t lineOffsetUS=y * width;
        
        for (size_t x=0; x<widthMinusKernel; ++x)
        {
            float xFiltValueR=0.0f;
            float xFiltValueG=0.0f;
            float xFiltValueB=0.0f;
            
            for (size_t j=0; j<kernelWidth; ++j)
            {
                const size_t xOffset=((lineOffsetUS + x) + j)*3;
                
                xFiltValueR += dataReadUS[xOffset + 0] * kernel1D_[j];
                xFiltValueG += dataReadUS[xOffset + 1] * kernel1D_[j];
                xFiltValueB += dataReadUS[xOffset + 2] * kernel1D_[j];
            }
            
            const size_t xOffset=(lineOffsetFS + x)*3;
            
            dataScratch[xOffset + 0]=xFiltValueR;
            dataScratch[xOffset + 1]=xFiltValueG;
            dataScratch[xOffset + 2]=xFiltValueB;
        }
    }
    
    for (size_t y=0; y<heightMinusKernel; ++y)
    {
        const size_t lineOffsetDS=(y + halfKernelWidth) * width;
        const size_t lineOffsetFS=y * width;
        
        for (size_t x=halfKernelWidth; x<widthMinusHalfKernel; ++x)
        {
            float filtValueR=0.0f;
            float filtValueG=0.0f;
            float filtValueB=0.0f;
            
            for (size_t j=0; j<kernelWidth; ++j)
            {
                const size_t xOffset=((lineOffsetFS + x) + j*width)*3;
                
                filtValueR += dataScratch[xOffset + 0] * kernel1D_[j];
                filtValueG += dataScratch[xOffset + 1] * kernel1D_[j];
                filtValueB += dataScratch[xOffset + 2] * kernel1D_[j];
            }
            
            const size_t xOffset=(lineOffsetDS + x)*3;
            
            dataWriteDS[xOffset + 0]=filtValueR;
            dataWriteDS[xOffset + 1]=filtValueG;
            dataWriteDS[xOffset + 2]=filtValueB;
        }
    }
    
    return true;
}

bool GaussianFilter::filter(uint8_t * const dataWriteDS, uint8_t const * const dataReadUS,
                            const size_t width, const size_t height,
                            uint8_t * const dataScratch)
{
    const size_t kernelWidth=kernelWidth_;
    const size_t widthMinusKernel=width - kernelWidth_;
    const size_t heightMinusKernel=height - kernelWidth_;
    
    const size_t halfKernelWidth=(kernelWidth_>>1);
    const size_t widthMinusHalfKernel=width-halfKernelWidth;
    
    for (size_t y=0; y<height; ++y)
    {
        const size_t lineOffsetFS=y * width + halfKernelWidth;
        const size_t lineOffsetUS=y * width;
        
        for (size_t x=0; x<widthMinusKernel; ++x)
        {
            float xFiltValue=0.0f;
            
            for (size_t j=0; j<kernelWidth; ++j)
            {
                xFiltValue += dataReadUS[(lineOffsetUS + x) + j] * kernel1D_[j];
            }
            
            dataScratch[lineOffsetFS + x]=uint8_t(xFiltValue+0.5f);
        }
    }
    
    for (size_t y=0; y<heightMinusKernel; ++y)
    {
        const size_t lineOffsetDS=(y + halfKernelWidth) * width;
        const size_t lineOffsetFS=y * width;
        
        for (size_t x=halfKernelWidth; x<widthMinusHalfKernel; ++x)
        {
            float filtValue=0.0f;
            
            for (size_t j=0; j<kernelWidth; ++j)
            {
                filtValue += dataScratch[(lineOffsetFS + x) + j*width] * kernel1D_[j];
                //Performance note: The XCode profiler shows that the above multiply by width has little performance overhead compared to the loop in x above.
                //                  Is the code memory bandwidth limited?
            }
            
            dataWriteDS[lineOffsetDS + x]=uint8_t(filtValue+0.5f);
        }
    }
    
    return true;
}

bool GaussianFilter::filterRGB(uint8_t * const dataWriteDS, uint8_t const * const dataReadUS,
                               const size_t width, const size_t height,
                               uint8_t * const dataScratch)
{
    const size_t kernelWidth=kernelWidth_;
    const size_t widthMinusKernel=width - kernelWidth_;
    const size_t heightMinusKernel=height - kernelWidth_;
    
    const size_t halfKernelWidth=(kernelWidth_>>1);
    const size_t widthMinusHalfKernel=width-halfKernelWidth;
    
    for (size_t y=0; y<height; ++y)
    {
        const size_t lineOffsetFS=y * width + halfKernelWidth;
        const size_t lineOffsetUS=y * width;
        
        for (size_t x=0; x<widthMinusKernel; ++x)
        {
            float xFiltValueR=0.0f;
            float xFiltValueG=0.0f;
            float xFiltValueB=0.0f;
            
            for (size_t j=0; j<kernelWidth; ++j)
            {
                const size_t xOffset=((lineOffsetUS + x) + j)*3;
                
                xFiltValueR += float(dataReadUS[xOffset + 0]) * kernel1D_[j];
                xFiltValueG += float(dataReadUS[xOffset + 1]) * kernel1D_[j];
                xFiltValueB += float(dataReadUS[xOffset + 2]) * kernel1D_[j];
            }
            
            const size_t xOffset=(lineOffsetFS + x)*3;
            
            dataScratch[xOffset + 0]=uint8_t(xFiltValueR+0.5f);
            dataScratch[xOffset + 1]=uint8_t(xFiltValueG+0.5f);
            dataScratch[xOffset + 2]=uint8_t(xFiltValueB+0.5f);
        }
    }
    
    for (size_t y=0; y<heightMinusKernel; ++y)
    {
        const size_t lineOffsetDS=(y + halfKernelWidth) * width;
        const size_t lineOffsetFS=y * width;
        
        for (size_t x=halfKernelWidth; x<widthMinusHalfKernel; ++x)
        {
            float filtValueR=0.0f;
            float filtValueG=0.0f;
            float filtValueB=0.0f;
            
            for (size_t j=0; j<kernelWidth; ++j)
            {
                const size_t xOffset=((lineOffsetFS + x) + j*width)*3;
                
                filtValueR += float(dataScratch[xOffset + 0]) * kernel1D_[j];
                filtValueG += float(dataScratch[xOffset + 1]) * kernel1D_[j];
                filtValueB += float(dataScratch[xOffset + 2]) * kernel1D_[j];
            }
            
            const size_t xOffset=(lineOffsetDS + x)*3;
            
            dataWriteDS[xOffset + 0]=uint8_t(filtValueR+0.5f);
            dataWriteDS[xOffset + 1]=uint8_t(filtValueG+0.5f);
            dataWriteDS[xOffset + 2]=uint8_t(filtValueB+0.5f);
        }
    }
    
    return true;
}
//=========================================//



//=========== GaussianDownsample ==========//
GaussianDownsample::GaussianDownsample(const float filterRadius,
                                       const size_t kernelWidth) :
kernel1D_(nullptr),
filterRadius_(filterRadius),
kernelWidth_((kernelWidth>>1)<<1)//Make sure the kernel width is even.
{
    updateKernel1D();
}

GaussianDownsample::~GaussianDownsample()
{
    delete [] kernel1D_;
}

void GaussianDownsample::updateKernel1D()
{
    if (kernel1D_!=nullptr) delete [] kernel1D_;
    
    kernel1D_=new float[kernelWidth_];
    
    const float kernelCentre=kernelWidth_ * 0.5f;
    const float sigma=filterRadius_ * 0.5f;
    
    float kernelSum=0.0f;
    
    for (size_t i=0; i<kernelWidth_; ++i)
    {
        const float r=((i + 0.5f) - kernelCentre);
        kernel1D_[i]=(1.0f/(sqrt(2.0f*float(M_PI))*sigma)) * exp(-(r*r)/(2.0f*sigma*sigma));
        kernelSum+=kernel1D_[i];
    }
    
    //=== Ensure that the kernel is normalised!
    for (size_t i=0; i<kernelWidth_; ++i)
    {
        kernel1D_[i]/=kernelSum;
    }
}

void GaussianDownsample::setFilterRadius(const float filterRadius)
{
    filterRadius_=filterRadius;
    updateKernel1D();
}

void GaussianDownsample::setKernelWidth(const int kernelWidth)
{
    kernelWidth_=(kernelWidth>>1)<<1;
    updateKernel1D();
}

bool GaussianDownsample::downsample(float * const dataWriteUS, float const * const dataReadUS,
                                    const size_t widthUS, const size_t heightUS,
                                    float * const dataScratch)
{
    const size_t widthDS=widthUS>>1;
    //const size_t heightDS=widthUS>>1;
    
    const size_t widthUSMinusKernel=widthUS - kernelWidth_;
    const size_t heightUSMinusKernel=heightUS - kernelWidth_;
    
    const size_t halfKernelWidth=(kernelWidth_>>1);//kernelWidth_ is even!
    
    for (size_t y=0; y<heightUS; ++y)
    {
        const size_t lineOffsetFS=y * widthDS + (halfKernelWidth>>1);
        const size_t lineOffsetUS=y * widthUS;
        
        for (size_t xUS=0; xUS<widthUSMinusKernel; xUS+=2)
        {
            float xFiltValue=0.0f;
            
            for (size_t j=0; j<kernelWidth_; ++j)
            {
                xFiltValue+=dataReadUS[(lineOffsetUS + xUS) + j] * kernel1D_[j];
            }
            
            dataScratch[lineOffsetFS + (xUS>>1)]=xFiltValue;
        }
    }
    
    for (size_t y=0; y<heightUSMinusKernel; y+=2)
    {
        const size_t lineOffsetDS=((y>>1) + (halfKernelWidth>>1)) * widthDS;
        const size_t lineOffsetFS=y * widthDS;
        
        for (size_t xFS=0; xFS<widthDS; ++xFS)
        {
            float filtValue=0.0f;
            
            for (size_t j=0; j<kernelWidth_; ++j)
            {
                filtValue+=dataScratch[(lineOffsetFS + xFS) + j*widthDS] * kernel1D_[j];
                //Performance note: The XCode profiler shows that the above multiply by width has little performance overhead compared to the loop in x above.
                //                  Is the code memory bandwidth limited?
            }
            
            dataWriteUS[lineOffsetDS + xFS]=filtValue;
        }
    }
    
    return true;
}
//=========================================//



