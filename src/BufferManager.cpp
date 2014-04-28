#include <iostream>
#include <fcntl.h>
#include <stdexcept>
#include <tuple>
#include <unistd.h>

#include "BufferManager.hpp"

BufferManager::BufferManager(unsigned size) {
    maxSize = size;
    pthread_rwlock_init(&latch, NULL);

    frames.reserve(size);

    // LRU
    pthread_mutex_init(&lruMutex, NULL);
    lru = mru = NULL;
    lruLength = 0;
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
    // acquire map lock
    rdlock();

    BufferFrame* bf;

    // check whether the page is already buffered
    try {
        bf = &frames.at(pageID);

        // remove frame from LRU list
        removeLRU(bf);
    } catch (const std::out_of_range) {
        // frame is not buffered, try to buffer it

        // release read lock and get the write lock
        unlock();
        wrlock();

        // check whether the page was loaded in the meantime
        try {
            bf = &frames.at(pageID);

            // remove frame from LRU list
            removeLRU(bf);
        } catch (const std::out_of_range) {
            // check whether the the buffer is full and we need to unload a frame
            if (frames.size() >= maxSize) {
                // select the LRU frame as our unload victim
                BufferFrame* victim = popLRU();
                if (victim == NULL) { // LRU list is empty
                    throw std::runtime_error("could not find free frame");
                }

                frames.erase(victim->id);
            }

            // page ID (64 bit):
            //   first 16bit: segment (=filename)
            //   48bit: actual page ID
            int fd = getSegmentFd(pageID >> 48);

            // create a new frame in the map (with key pageID)
            auto ret = frames.emplace(std::piecewise_construct,
                  std::forward_as_tuple(pageID),
                  std::forward_as_tuple(fd, pageID)
            );
            bf = &ret.first->second;

            bf->currentUsers++;
        }
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

    // check if the file descriptor was already created
    try {
        fd = segments.at(segmentID);
    } catch (const std::out_of_range) {
        // must create a new one

        // create filename from segmentID
        char filename[15];
        sprintf(filename, "%d", segmentID);

        // open the segment file
        fd = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR); // | O_NOATIME | O_SYNC
        if (fd < 0) {
            throw std::runtime_error(std::strerror(errno));
        }

        // save the fd to cache and share it
        segments[segmentID] = fd;
    }

    return fd;
}

void BufferManager::unfixPage(BufferFrame& frame, bool isDirty) {
    if(isDirty)
        frame.markDirty();

    // TODO: flush to file?
    //frame.flush();

    // release the lock on the frame
    frame.unlock();

    putLRU(&frame);
}

// insert item at the end of the LRU list (MRU)
void BufferManager::putLRU(BufferFrame* fp) {
    pthread_mutex_lock(&lruMutex);

    fp->currentUsers--;
    if(fp->currentUsers == 0) {
        if(mru == NULL) {
            if(lru != NULL) {
                std::cerr << "mru NULL and lru not" << std::endl;
                exit(1);
            }

            mru = fp;
            lru = fp;
        } else if(lru == NULL) {
            std::cerr << "lru NULL and mru not" << std::endl;
            exit(1);
        } else {
            fp->prev  = mru;
            mru->next = fp;
            mru = fp;
        }

        lruLength++;

        if (lruLength > (int)maxSize) {
            std::cerr << "LRU too long! " << lruLength << std::endl;
            exit(1);
        }
    }

    pthread_mutex_unlock(&lruMutex);
}

// remove item from the LRU list
void BufferManager::removeLRU(BufferFrame* fp) {
    pthread_mutex_lock(&lruMutex);

    if(fp->currentUsers++ == 0) {
        lruLength--;

        if (fp->prev != NULL)
            fp->prev->next = fp->next;

        if (fp->next != NULL)
            fp->next->prev = fp->prev;

        if (lru == fp)
            lru = fp->next;

        if (mru == fp)
            mru = fp->prev;

        fp->next = fp->prev = NULL;
    }

    pthread_mutex_unlock(&lruMutex);
}

// get and remove the LRU item from the list
BufferFrame* BufferManager::popLRU() {
    pthread_mutex_lock(&lruMutex);

    BufferFrame* ret = lru;
    if(lru == NULL) {
        if(mru != NULL) {
            std::cerr << "lru NULL and mru not" << std::endl;
            exit(1);
        }
    } else if(mru == NULL) {
        std::cerr << "mru NULL and lru not" << std::endl;
        exit(1);
    } else {
        lru = ret->next;
        ret->next = NULL;

        if(lru != NULL) {
            lru->prev = NULL;
        } else {
            if(mru != ret) {
                std::cerr << " mru!=ret " << mru << " != " << ret << std::endl;
                exit(1);
            }
            mru = NULL;
        }

        lruLength--;
    }

    pthread_mutex_unlock(&lruMutex);

    return ret;
}
