
#ifndef BUFFERFRAME_H_
#define BUFFERFRAME_H_

#include <mutex>

// frame state
enum state_t {
    New,   // no data loaded
    Clean, // data loaded and no data changed
    Dirty  // data loaded and changed
};

const unsigned blocksize = 8192;

class BufferFrame {
  public:
    BufferFrame(uint64_t pageID);
    ~BufferFrame();

    // delete the copy operator. The frames have fields that should not be copied
    //BufferFrame(const BufferFrame& other) = delete;

    //BufferFrame(BufferFrame&& other);

    void lock(bool exclusive);
    void unlock();

    // Returns the actual data contained on the page
    void* getData();

    void markDirty() { state = state_t::Dirty; }

    void print();

    //uint64_t getPageNo() { return pageNo; }
    //pthread_rwlock_t getLatch() { return latch; }
    //uint64_t getLsm() { return lsm; }
    //state_t getState() { return state; }

  private:
    void loadData();
    void writeData();

    uint64_t id;

    // a read/writer lock to protect the page
    pthread_rwlock_t rwlock;

    // clean/dirty/newly created etc.
    state_t state;

    //the page number, 64 bit
    //  first 16bit: filename
    //  48bit: actual page id
    //uint64_t pageID;

    //LSN of the last change, for recovery
    //Not needed?
    //uint64_t lsn;

    // pointer to loaded data
    void* data;

    //filename and position in bytes
    uint64_t filename;
    uint64_t offset;

    //blocksize in Byte
    //uint64_t blocksize;

    //BufferFrame* next;
};

#endif  // BUFFERFRAME_H_
