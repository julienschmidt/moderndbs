#include <iostream>
#include <fcntl.h>
#include <unistd.h>

#include "BufferManager.hpp"

#define DEBUG

BufferManager::BufferManager(unsigned size) {
    maxSize = size*10; // TODO: change back
    pthread_rwlock_init(&latch, NULL);

    frames.reserve(size);
}

BufferManager::~BufferManager() {
    // wait until all tasks are finished
    wrlock();

    pthread_rwlock_destroy(&latch);

    // write dirty pages back to file
    for (auto& kv: frames)
        kv.second.flush();

    // close segment file descriptors
    for (auto& kv: segments)
        close(kv.second);
}


BufferFrame& BufferManager::fixPage(uint64_t pageID, bool exclusive) {
    // acquire map lock
    rdlock();

    BufferFrame* bf;

    // check whether the page is already buffered
    try {
        bf = &frames.at(pageID);
    } catch (const std::out_of_range) {
        // frame is not buffered, try to buffer it

    #ifdef DEBUG
        std::cout << "page " << pageID << " not buffered; map size: " << frames.size() << std::endl;
    #endif

        // check whether the the buffer is full
        if (frames.size() >= maxSize) {
            std::cout << "buffer is full: size " << frames.size() << " maxSize " << maxSize << std::endl;

            // TODO: try to free frames
            throw; // runtime_error("Could not find free frame");
        }

        // release read lock and get the write lock
        unlock();
        wrlock();

        // page ID (64 bit):
        //   first 16bit: segment (=filename)
        //   48bit: actual page ID
        int fd = getSegmentFd(pageID >> 48);
        int pageNo = pageID & 0x0000FFFFFFFFFFFF;

        // create a new frame in the map (with key pageID)
        // we get a reference to the existing frame if another thread created it
        // in the meantime
        auto ret = frames.emplace(std::piecewise_construct,
              std::forward_as_tuple(pageID),
              std::forward_as_tuple(fd, pageNo)
        );
        bf = &ret.first->second;
    }

    // release the lock on the map
    unlock();

    // acquire lock on the frame
    bf->lock(exclusive);

    // return frame reference
    return *bf;
}

// must be protected by locks
int BufferManager::getSegmentFd(unsigned segmentID) {
    int fd;

    // check if the fd was already created
    try {
        fd = segments.at(segmentID);
        std::cout << "fd " << segmentID << " from buffer" << std::endl;
    } catch (const std::out_of_range) {
        // must create a new one

        char filename[255];
        sprintf(filename, "%d", segmentID);

        fd = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR); // | O_NOATIME | O_SYNC
        if (fd < 0) {
            perror("opening the segment file failed");
            throw; // TODO: useful exception
        }

        segments[segmentID] = fd;

        std::cout << "fd " << segmentID << " created" << std::endl;
    }

    return fd;
}

void BufferManager::unfixPage(BufferFrame& frame, bool isDirty) {
    if(isDirty)
        frame.markDirty();

    // TODO flush to file?

    // release the lock on the frame
    frame.unlock();
}
