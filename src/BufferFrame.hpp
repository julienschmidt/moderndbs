
#ifndef BUFFERFRAME_H_
#define BUFFERFRAME_H_

// frame state
enum state_t {
    New,   // no data loaded
    Clean, // data loaded and no data changed
    Dirty  // data loaded and changed
};

const unsigned blocksize = 8192;

class BufferFrame {
  public:
    BufferFrame(int segmentFd, unsigned pageNo);
    ~BufferFrame();

    void lock(bool exclusive);
    void unlock();
    void* getData(); // returns the actual data contained on the page
    void markDirty() { state = state_t::Dirty; }
    void flush();

  private:
    void loadData();
    void writeData();

    // pointer to loaded data
    void* data;

    // page state: clean/dirty/newly created etc.
    state_t state;

    // offset in the segment file
    off_t offset;

    // file descriptor of the file the segment is mapped to
    int fd;

    // a read/writer lock to protect the page
    pthread_rwlock_t rwlock;
};

#endif  // BUFFERFRAME_H_
