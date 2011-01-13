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

#ifndef SHARED_IMAGE_BUFFER_H
#define SHARED_IMAGE_BUFFER_H 1

#include <flitr/image.h>

#include <map>
#include <vector>

#include <OpenThreads/ScopedLock>
#include <OpenThreads/Mutex>

#include <boost/tr1/memory.hpp>

namespace flitr {

class ImageConsumer;
class ImageProducer;

/**
 * \brief Class for passing images between producers and consumers. 
 * 
 * The SharedImageBuffer class manages the transfer of images between
 * a producer and potentially multiple consumers. A circular (ring)
 * buffer is created where the producer can store images. Consumers
 * can read data as soon as available. Producers reserve available
 * slots in the buffer for writing, while consumers can reserve
 * available buffers for reading. While reserved, the producer can
 * write directly to the buffer and consumers can read from the
 * already written buffers. The structure therefore attempts to
 * minimise copying of image data. Access to the buffer is serialised
 * using mutexes.
 *
 * A slot contains a single image or a group of images, typically
 * captured at the same time. This allows consumers to obtain a
 * synchronised group of images.
 *
 * Multiple slots can be reserved for reading and writing. This allows
 * a consumer to e.g. keep access to a range of images if it's
 * interested in a time range (history) of images.
 */
class FLITR_EXPORT SharedImageBuffer {
  public:
    /** 
     * Creates a shared buffer without allocating storage.
     * 
     * \param my_producer Reference to the producer this buffer is
     * associated with.
     *
     * \param num_slots The number of slots in the circular buffer.
     *
     * \param images_per_slot The number of images in a slot (group of
     * synchronised images)
     */
    SharedImageBuffer(ImageProducer& my_producer, 
                      uint32_t num_slots, 
                      uint32_t images_per_slot);

    ~SharedImageBuffer();

    /** 
     * Initialise the buffer and allocates storage for the images we
     * are to contain.
     * 
     * \return True on successful initialisation.
     */
    bool initWithStorage();

    /** 
     * Initialise the buffer, but do not allocate storage any
     * images. This is typically used if our elements just point to
     * elements in another shared buffer.
     * 
     * \return True on successful initialisation. 
     */
    bool initWithoutStorage();
    
    /** 
     * Reserve (obtain) a slot for writing new image data. Multiple
     * slots can be reserved before any are released.
     * 
     * \return A vector with pointers to image pointers where data can
     * be written. The size would match the number of images per slot
     * for this buffer. An empty vector if no slot could be obtained
     * (buffer full or readers busy with all slots). 
     */
    virtual std::vector<Image**> reserveWriteSlot();
    //virtual std::vector<Image**> getWritable();
    

    /** 
     * Indicate that the producer has finished with a reserved write
     * slot. Should only be called after a slot has been reserved and
     * once for each slot that was reserved.
     * 
     */
    virtual void releaseWriteSlot();
    //virtual void pushWritable();

    /** 
     * Find the least number of readable image slots available between
     * all consumers.
     * 
     * \return Least number of read slots available.
     */
    virtual uint32_t getLeastNumReadSlotsAvailable();

    /** 
     * Obtain the number of image slots that are available for
     * reading.
     * 
     * \param consumer Reference to the consumer for which the query
     * is being made.
     * 
     * \return Number of slots available for reading.
     */
    virtual uint32_t getNumReadSlotsAvailable(const ImageConsumer& consumer);
    //virtual uint32_t getNumReadable(const ImageConsumer& consumer);

    /** 
     * Obtain the number of slots that a consumer has reserved for
     * reading but has not yet released.
     * 
     * \param consumer Reference to the consumer for which the query
     * is being made.
     * 
     * \return The number of slots reserved for reading.
     */
    virtual uint32_t getNumReadSlotsReserved(const ImageConsumer& consumer);
    //virtual uint32_t getNumReadableBusy(const ImageConsumer& consumer);

    /** 
     * Reserve (obtain) a slot where image data has already been written. Multiple
     * slots can be reserved before any are released.
     * 
     * \param consumer Reference to the consumer for which the query
     * is being made.
     * 
     * \return A vector with pointers to image pointers where data can
     * be read from. The size would match the number of images per
     * slot for this buffer. An empty vector if no slot could be
     * obtained (buffer full or reader busy with all slots).
     */
    virtual std::vector<Image**> reserveReadSlot(const ImageConsumer& consumer);
    //virtual std::vector<Image**> getReadable(const ImageConsumer& consumer);

    /** 
     * Indicate that the consumer has finished with a reserved write
     * slot. Should only be called after a slot has been reserved and
     * once for each slot that was reserved.
     * 
     * \param consumer Reference to the consumer that has completed
     * with a read.
     */    
    virtual void releaseReadSlot(const ImageConsumer& consumer);
    //virtual void popReadable(const ImageConsumer& consumer);
	
    /** 
     * Add a new consumer to this buffer. The consumer's read position
     * would be set to the latest image to be written.
     * 
     * \param consumer Reference to the consumer to be added.
     * 
     * \return True if successfully added.
     */
    virtual bool addConsumer(ImageConsumer& consumer);

  private:
    /// Returns true if there is no more space in the buffer for writing.
    bool isFull();
    
    /** 
     * See if, after the consumer that just released a slot, all
     * consumers are done with the oldest slot. If we depend on
     * another buffer for our data we can then notify that we are done
     * with the oldest slot.
     * 
     * \param consumer The consumer that just released a slot.
     * 
     * \return True if all consumers are now done with the oldest used
     * slot.
     */
    bool tailPopped(const ImageConsumer& consumer);

    /** 
     * Utility function to find out how many readable slots exist for a consumer.
     * 
     * \param consumer Consumer to find the number of readable slots for.
     * 
     * \return The number of available read slots.
     */
    uint32_t numAvailable(const ImageConsumer& consumer);

    /// The producer we are a member of.
    ImageProducer *ImageProducer_;
		
    /// Protects read and write positions
    OpenThreads::Mutex BufferMutex_;

    /// The number of slots in the buffer.
    uint32_t NumSlots_;
    /// Number of images in each slot of the buffer.
    uint32_t ImagesPerSlot_;
		
    /// One past where we have already written
    uint32_t WriteTail_;
    /// Point where we are about to write.
    uint32_t WriteHead_;
    /// How many slots have been reserved for writing.
    uint32_t NumWriteReserved_;

    /// Map of consumers to one past where they have completed reading.
    std::map< const ImageConsumer*, uint32_t > ReadTails_;
    /// Map of consumers to where they are about to read.
    std::map< const ImageConsumer*, uint32_t > ReadHeads_;
    /// Map of consumers to the number of read slots reserved.
    std::map< const ImageConsumer*, uint32_t > NumReadReserved_;

    /// The actual ring buffer. Contains only pointers.
    std::vector< std::vector< Image* > > Buffer_;

    /// Indicates whether we have reserved storage for the images in
    /// the buffer.
    bool HasStorage_;
};

}

#endif //SHARED_IMAGE_BUFFER_H
