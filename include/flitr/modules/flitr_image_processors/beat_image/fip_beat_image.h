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

#ifndef FIP_BEAT_IMAGE_H
#define FIP_BEAT_IMAGE_H 1

#include <flitr/image_processor.h>

namespace flitr {
    
    /*! Calculates the beat image on the CPU. The performance is independent of the number of frames. */
    class FLITR_EXPORT FIPBeatImage : public ImageProcessor
    {
    public:
        
        /*! Constructor given the upstream producer.
         *@param upStreamProducer The upstream image producer.
         *@param images_per_slot The number of images per image slot from the upstream producer.
         *@param buffer_size The size of the shared image buffer of the downstream producer.*/
        FIPBeatImage(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                        uint8_t base2WindowLength,
                        uint32_t buffer_size=FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS);
        
        /*! Virtual destructor */
        virtual ~FIPBeatImage();
        
        /*! Method to initialise the object.
         *@return Boolean result flag. True indicates successful initialisation.*/
        virtual bool init();
        
        /*!Synchronous trigger method. Called automatically by the trigger thread if started.
         *@sa ImageProcessor::startTriggerThread*/
        virtual bool trigger();
        
    private:
        
        /*! Replaces data[1..2*nn] by its discrete Fourier transform, if isign is input as 1; or 
         replaces data[1..2*nn] by nn times its inverse discrete Fourier transform, if isign is 
         input as −1. data is a complex array of length nn or, equivalently, a real array of length 
         2*nn. nn MUST be an integer power of 2 (this is not checked for!). */
        void four1(float * const data, uint32_t nn, int32_t isign)
        {
            unsigned long n,mmax,m,j,istep,i;
            double wtemp,wr,wpr,wpi,wi,theta;
            float tempr,tempi;
            
            n=nn << 1;
            j=1;
            
            for (i=1;i<n;i+=2)
            {
                if (j > i)
                {
                    std::swap(data[j],data[i]);
                    std::swap(data[j+1],data[i+1]);
                }
                m=nn;
                while (m >= 2 && j > m)
                {
                    j -= m;
                    m >>= 1;
                }
                
                j += m;
            }
            
            mmax=2;
            
            while (n > mmax)
            {
                istep=mmax << 1; theta=isign*(6.28318530717959/mmax); wtemp=sin(0.5*theta);
                wpr = -2.0*wtemp*wtemp;
                
                wpi=sin(theta);
                wr=1.0;
                wi=0.0;
                
                for (m=1;m<mmax;m+=2)
                {
                    for (i=m;i<=n;i+=istep)
                    {
                        j=i+mmax;
                        tempr=wr*data[j]-wi*data[j+1];
                        tempi=wr*data[j+1]+wi*data[j];
                        data[j]=data[i]-tempr;
                        data[j+1]=data[i+1]-tempi;
                        data[i] += tempr;
                        data[i+1] += tempi;
                    }
                    
                    wr=(wtemp=wr)*wpr-wi*wpi+wr;
                    wi=wi*wpr+wtemp*wpi+wi;
                }
                
                mmax=istep;
            }
        }
        
        const uint8_t base2WindowLength_;
        const size_t windowLength_;
        
        /*! The history ring buffer/vector for each image in the slot. Ring buffer slots next to each other in memory!*/
        std::vector<float * > historyImageVec_;
        
        /*! The beat ring buffer/vector for each image in the slot. */
        std::vector<float * > beatImageVec_;
        
        size_t oldestHistorySlot_;
    };
    
}

#endif //FIP_AVERAGE_IMAGE_H
