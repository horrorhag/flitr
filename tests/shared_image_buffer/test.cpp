#include <iostream>
#include <string>

#include <flitr/image_consumer.h>
#include <flitr/image_producer.h>

using std::tr1::shared_ptr;
using namespace flitr;

void checkCondition(bool condition, std::string message)
{
    if (!condition) {
        std::cerr << message;
        exit(-1);
    }
}

#define BUFFER_SZ 10
#define BUFFER_FRAG 4

class TestProducer : public ImageProducer {
  public:
    TestProducer()
    {
        bufferSize_ = BUFFER_SZ;
    }
    bool init()
    {
        ImageFormat imf(1024,2048);
        ImageFormat_.push_back(imf);

        // Allocate storage
        SharedImageBuffer_ = shared_ptr<SharedImageBuffer>(new SharedImageBuffer(*this, bufferSize_, 1));
        SharedImageBuffer_->initWithStorage();
        
        return true;
    }

    void testOverflow()
    {
        // we should be able to reserve all the buffers
        for (uint32_t i=0; i<bufferSize_; i++) {
            std::vector<Image**> iv = reserveWriteSlot();
            checkCondition((iv.size() != 0), "testOverflow: Expected an open slot\n");
        }
        //std::cerr << "testOverflow: pass - get all buffers\n";
        // another reserve should fail
        {
            std::vector<Image**> iv = reserveWriteSlot();
            checkCondition((iv.size() == 0), "testOverflow: Expected reserved failure\n");
        }
        //std::cerr << "testOverflow: pass - cannot reserve more than size\n";
        // release all again
        for (uint32_t i=0; i<bufferSize_; i++) {
            releaseWriteSlot();
        }
    }

    bool reserveOne()
    {
        std::vector<Image**> iv = reserveWriteSlot();
        return (iv.size()!=0);
    }
    bool releaseOne()
    {
        releaseWriteSlot();
        return true;
    }
    void releaseReadSlotCallback() 
    {
        notified_ = true;
    }
    void resetNotified() { notified_ = false; }
    bool getNotified() { return notified_; }

  private:
    uint32_t bufferSize_;
    bool notified_;

};

class TestConsumer : public ImageConsumer {
  public:
    TestConsumer(ImageProducer& producer) :
        ImageConsumer(producer)
    {
        
    }
    bool init()
    {
        return true;
    }
    
    bool reserveOne()
    {
        std::vector<Image**> iv = reserveReadSlot();
        return (iv.size()!=0);
    }
    bool releaseOne()
    {
        releaseReadSlot();
        return true;
    }
};

int main(void)
{
    shared_ptr<TestProducer> tp(new TestProducer());
    tp->init();

    // just advance writer a bit
    for (int i=0; i<17; i++) {
        tp->reserveOne();
        tp->releaseOne();
    }

    // check with no readers
    tp->testOverflow();

    shared_ptr<TestConsumer> tc1(new TestConsumer(*tp));
    tc1->init();

    // check overflow when reader not advancing
    for (int i=0; i<BUFFER_SZ; i++) {
        bool reserveOK = tp->reserveOne();
        checkCondition(reserveOK, "Expected reserve OK\n");
        tp->releaseOne();
    }
    // now the reserve should fail
    {
        bool reserveOK = tp->reserveOne();
        checkCondition(!reserveOK, "Expected reserve to fail\n");
    }
    
    {
        // read one
        bool reserveOK = tc1->reserveOne();
        checkCondition(reserveOK, "Expected read reserve OK\n");
        tc1->releaseOne();
    }

    // now write reserve should be OK again
    {
        bool reserveOK = tp->reserveOne();
        checkCondition(reserveOK, "Expected reserve OK\n");
        tp->releaseOne();
    }
    // now the reserve should fail
    {
        bool reserveOK = tp->reserveOne();
        checkCondition(!reserveOK, "Expected reserve to fail\n");
    }

    // we should have BUFFER_SZ readable
    {
        uint32_t num_avail = tc1->getNumReadSlotsAvailable();
        checkCondition((num_avail == BUFFER_SZ), "Expected full buffer available\n");
    }
    // we should be able to reserve all
    {
        for (int i=0; i<BUFFER_SZ; i++) {
            bool reserveOK = tc1->reserveOne();
            checkCondition(reserveOK, "Expected read reserve OK\n");
        }
    }
    // now finish with all
    {
        for (int i=0; i<BUFFER_SZ; i++) {
            tc1->releaseOne();
        }
    }
    
    // check notify
    {
        {
            bool reserveOK = tp->reserveOne();
            checkCondition(reserveOK, "Expected reserve OK\n");
            tp->releaseOne();
            tp->resetNotified();
        }
        {
            bool reserveOK = tc1->reserveOne();
            checkCondition(reserveOK, "Expected read reserve OK\n");
            tc1->releaseOne();
        }
        bool notified = tp->getNotified();
        checkCondition(notified, "Expected notify OK\n");
    }

    // write a few
    for (int i=0; i<BUFFER_FRAG; i++) {
        bool reserveOK = tp->reserveOne();
        checkCondition(reserveOK, "Expected reserve OK\n");
        tp->releaseOne();
    }

    // add another consumer
    shared_ptr<TestConsumer> tc2(new TestConsumer(*tp));
    tc2->init();

    // 1 should have BUFFER_FRAG avail 
    {
        uint32_t num_avail = tc1->getNumReadSlotsAvailable();
        checkCondition((num_avail == BUFFER_FRAG), "Expected some available\n");
    }
    // and 2 should have 0
    {
        uint32_t num_avail = tc2->getNumReadSlotsAvailable();
        checkCondition((num_avail == 0), "Expected none available\n");
    }
    // write one more
    {
        bool reserveOK = tp->reserveOne();
        checkCondition(reserveOK, "Expected reserve OK\n");
        tp->releaseOne();
    }
    // 1 should have BUFFER_FRAG+1 avail 
    {
        uint32_t num_avail = tc1->getNumReadSlotsAvailable();
        checkCondition((num_avail == BUFFER_FRAG+1), "Expected some available\n");
    }
    // and 2 should have 0+1
    {
        uint32_t num_avail = tc2->getNumReadSlotsAvailable();
        checkCondition((num_avail == 0+1), "Expected some available\n");
    }
    // now pop BUFFER_FRAG from c1, we should get a notify for each one
    for (int i=0; i<BUFFER_FRAG; i++) {
        tp->resetNotified();
        bool reserveOK = tc1->reserveOne();
        checkCondition(reserveOK, "Expected read reserve OK\n");
        tc1->releaseOne();
        bool notified = tp->getNotified();
        checkCondition(notified, "Expected notify OK\n");
    }
    // pop another from c1, but now we don't expect a notify cos c2 is behind
    {
        tp->resetNotified();
        bool reserveOK = tc1->reserveOne();
        checkCondition(reserveOK, "Expected read reserve OK\n");
        tc1->releaseOne();
        bool notified = tp->getNotified();
        checkCondition(!notified, "Expected no notify\n");
    }
    // now if we pop c2, we expect a notify
    {
        tp->resetNotified();
        bool reserveOK = tc2->reserveOne();
        checkCondition(reserveOK, "Expected read reserve OK\n");
        tc2->releaseOne();
        bool notified = tp->getNotified();
        checkCondition(notified, "Expected notify OK\n");
    }
    // c1 and c2 should have 0 available now
    {
        uint32_t num_avail = tc1->getNumReadSlotsAvailable();
        checkCondition((num_avail == 0), "Expected none available\n");
    }
    {
        uint32_t num_avail = tc2->getNumReadSlotsAvailable();
        checkCondition((num_avail == 0), "Expected none available\n");
    }
}
