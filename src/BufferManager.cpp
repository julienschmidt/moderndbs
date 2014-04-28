#include <iostream>
#include <fcntl.h>
#include <stdexcept>
#include <unistd.h>

#include "BufferManager.hpp"

#define DEBUG

BufferManager::BufferManager(unsigned size) {
    maxSize = size; // TODO: change back
    pthread_rwlock_init(&latch, NULL);

    frames.reserve(size);

    pthread_mutex_init(&lruMutex, NULL);
    lru = mru = NULL;
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

    pthread_mutex_destroy(&lruMutex);
}


BufferFrame& BufferManager::fixPage(uint64_t pageID, bool exclusive) {
    std::cout << "fix " << pageID << std::endl;
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

        // release read lock and get the write lock
        std::cout << "lockswap un " << pageID << std::endl;
        unlock();
        std::cout << "lockswap wr " << pageID << std::endl;
        wrlock();
        std::cout << "lockswapped " << pageID << std::endl;

        // check whether the the buffer is full
        if (frames.size() >= maxSize) {
            std::cout << "buffer is full: size " << frames.size() << " maxSize " << maxSize << std::endl;

            std::cout << "popLRU " << pageID << std::endl;
            BufferFrame* victim = popLRU();
            std::cout << "poppedLRU " << pageID << std::endl;
            if (victim == NULL) // LRU list is empty
                throw std::runtime_error("could not find free frame");

            // TODO: remove victim
            std::cout << "erase " << victim->id << std::endl;
            frames.erase(victim->id);
            std::cout << "erased" << std::endl;
        }

        // page ID (64 bit):
        //   first 16bit: segment (=filename)
        //   48bit: actual page ID
        int fd = getSegmentFd(pageID >> 48);

        // create a new frame in the map (with key pageID)
        // we get a reference to the existing frame if another thread created it
        // in the meantime
        auto ret = frames.emplace(std::piecewise_construct,
              std::forward_as_tuple(pageID),
              std::forward_as_tuple(fd, pageID)
        );
        bf = &ret.first->second;
    }

    // release the lock on the map
    unlock();

    std::cout << ">> bf lock " << bf->id << " excl: " << exclusive << std::endl;
    // acquire lock on the frame
    bf->lock(exclusive);
    std::cout << "bf locked" << std::endl;

    // remove from LRU list
    removeLRU(bf);
    std::cout << "bf removed from LRU" << std::endl;

    // return frame reference
    return *bf;
}

// must be protected by locks
int BufferManager::getSegmentFd(unsigned segmentID) {
    int fd;

    // check if the file descriptor was already created
    try {
        fd = segments.at(segmentID);
        std::cout << "fd " << segmentID << " from buffer" << std::endl;
    } catch (const std::out_of_range) {
        // must create a new one

        // create filename from segmentID
        char filename[15];
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
    std::cout << "unfix " << frame.id << std::endl;

    if(isDirty)
        frame.markDirty();

    // TODO: flush to file?
    //frame.flush();

    std::cout << "putLRU " << frame.id << std::endl;
    putLRU(&frame);
    std::cout << "puttedLRU " << frame.id << std::endl;

    // release the lock on the frame
    frame.unlock();
    std::cout << "<< unlocked " << frame.id << std::endl;
}

// insert item at the end of the LRU list (MRU)
void BufferManager::putLRU(BufferFrame* fp) {
    pthread_mutex_lock(&lruMutex);

    std::cout << "putLRU locked " << fp->id << std::endl;

    if (mru != NULL) {
        mru->next = fp;
        fp->prev = mru;
    } else { // lru == NULL
        lru = fp;
    }

    mru = fp;

    pthread_mutex_unlock(&lruMutex);

    std::cout << "putLRU unlocked " << fp->id << std::endl;
}

// remove item from the LRU list
void BufferManager::removeLRU(BufferFrame* fp) {
    pthread_mutex_lock(&lruMutex);

    std::cout << "removeLRU locked " << fp->id << std::endl;

    if (fp->prev != NULL)
        fp->prev->next = fp->next;

    if (fp->next != NULL)
        fp->next->prev = fp->prev;

    if (lru == fp)
        lru = fp->next;

    if (mru == fp)
        mru = fp->prev;

    fp->next = fp->prev = NULL;

    pthread_mutex_unlock(&lruMutex);

    std::cout << "removeLRU unlocked " << fp->id << std::endl;
}

// get and remove the LRU item from the list
BufferFrame* BufferManager::popLRU() {
    pthread_mutex_lock(&lruMutex);

    std::cout << "popLRU locked " << std::endl;

    BufferFrame* ret = lru;
    if (lru != NULL) {
        lru = lru->next;

        if(lru != NULL)
            lru->prev = NULL;
    }

    pthread_mutex_unlock(&lruMutex);

    std::cout << "popLRU unlocked " << std::endl;

    return ret;
}
