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

#ifndef IMAGE_CONSUMER_H
#define IMAGE_CONSUMER_H 1

namespace flitr {
//Prototype of class defined in this file in case of circular includes.	
  class ImageConsumer;
}


#include <flitr/shared_image_buffer.h>

namespace flitr {
	
/**
 * Base class for image consumers.
 * 
 * ImageConsumer is the base class for image consumers. At
 * construction it connects the consumer with a producer and shared
 * buffer.
 */
class FLITR_EXPORT ImageConsumer {
    friend class SharedImageBuffer;
  public:
    /** 
     * Construct a consumer.
     * 
     * \param producer The producer we are connecting to. The producer's method to add a consumer is protected, and can only be called via this consumer contructor.
     */
    ImageConsumer(ImageProducer& producer);
    
    virtual ~ImageConsumer() {}

	/** 
     * Obtain the number of image slots that are available for
     * reading.
     * 
     * \return Number of slots available for reading.
     */	
    virtual uint32_t getNumReadSlotsAvailable()
    {
        return SharedImageBuffer_->getNumReadSlotsAvailable(*this);
    }

    /** 
     * Obtain the number of slots that a consumer has reserved for
     * reading but has not yet released.
     * 
     * \return The number of slots reserved for reading.
     */
    virtual uint32_t getNumReadSlotsReserved()
    {
        return SharedImageBuffer_->getNumReadSlotsReserved(*this);
    }

    /** 
     * Reserve (obtain) a slot where image data has already been written. Multiple
     * slots can be reserved before any are released.
     * 
     * \return A vector with pointers to image pointers where data can
     * be read from. The size would match the number of images per
     * slot for this buffer. An empty vector if no slot could be
     * obtained (buffer full or reader busy with all slots).
     */
    virtual std::vector<Image**> reserveReadSlot()
    {
        return SharedImageBuffer_->reserveReadSlot(*this);
    }

    /** 
     * Indicate that the consumer has finished with a reserved write
     * slot. Should only be called after a slot has been reserved and
     * once for each slot that was reserved.
     */  
    virtual void releaseReadSlot()
    {
        SharedImageBuffer_->releaseReadSlot(*this);
    }
		
    virtual bool init() { return true; }

  protected:
    /// Called once we get added as a consumer.
    void setSharedImageBuffer(SharedImageBuffer& b) { SharedImageBuffer_ = &b; }

  private:
    // \todo good place for observer pointers
    /// Pointer to the shared buffer. The buffer resides in the producer.
    SharedImageBuffer *SharedImageBuffer_;
    /// Pointer to the producer we are connected to.
    ImageProducer *ImageProducer_;
};

}
#endif //IMAGE_CONSUMER_H
