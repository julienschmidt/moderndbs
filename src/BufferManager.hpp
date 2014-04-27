#ifndef BUFFERMANAGER_H_
#define BUFFERMANAGER_H_

#include <pthread.h>
#include <unordered_map>

#include "BufferFrame.hpp"


class BufferManager {
  public:
    // Create a new instance that keeps up to size frames in main memory
    BufferManager(unsigned size);

	// Destructor. Write all dirty frames to disk and free all resources
    ~BufferManager();

    // A method to retrieve frames given a page ID and indicating whether the
    // page will be held exclusively by this thread or not.
    // The method can fail if no free frame is available an no used frame can be
    // freed.
    // The page ID is split into a segment ID and the actual page ID.
    // Each page is stored on disk in a file with the same name as its segment
    // ID (e.g. "1")
    BufferFrame& fixPage(uint64_t pageID, bool exclusive);

    // Return a frame to the buffer manager indicating whether it is dirty or not.
    // If dirty, the page manager must write it back to disk. It does not have
    // to write it back immediately but must not write it back before unfixPage
    // is called.
    void unfixPage(BufferFrame& frame, bool isDirty);

    // writes all modified data back to disk
    void flush();

  private:
    inline void rdlock() { pthread_rwlock_rdlock(&latch); }
    inline void wrlock() { pthread_rwlock_wrlock(&latch); }
    inline void unlock() { pthread_rwlock_unlock(&latch); }

    int getSegmentFd(unsigned segmentID);

    // max number of buffered pages
    unsigned maxSize;

    pthread_rwlock_t latch;

	// hashmap containing all frames
	std::unordered_map<uint64_t, BufferFrame> frames;

    // hashmap containing all file descriptors of segment files
    std::unordered_map<unsigned, int> segments;
};

#endif  // BUFFERMANAGER_H_
