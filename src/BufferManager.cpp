#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <tuple>
#include <unistd.h>


#include "BufferManager.hpp"

BufferManager::BufferManager(size_t size) {
    maxSize = size;
    pthread_rwlock_init(&latch, NULL);

    frames.reserve(size);

    // LRU
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
    // acquire map lock
    rdlock();

    BufferFrame* bf;

    // check whether the page is already buffered
    auto entry = frames.find(pageID);
    if(entry != frames.end()) {
        bf = &entry->second;

        // remove frame from LRU list
        removeLRU(bf);

    } else {
        // frame is not buffered, try to buffer it

        // release read lock and get the write lock
        unlock();
        wrlock();

        // check whether the page was loaded in the meantime
        entry = frames.find(pageID);
        if(entry != frames.end()) {
            bf = &entry->second;

            // remove frame from LRU list
            removeLRU(bf);

        } else {
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
    auto entry = segments.find(segmentID);
    if(entry != segments.end()) {
        fd = entry->second;
    } else {
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

    // only insert into the list if no other user has still fixed this frame
    if((--fp->currentUsers) == 0) {
        if(mru == NULL) {
            lru = fp;
        } else {
            fp->prev  = mru;
            mru->next = fp;
        }
        mru = fp;
    }

    pthread_mutex_unlock(&lruMutex);
}

// remove item from the LRU list
void BufferManager::removeLRU(BufferFrame* fp) {
    pthread_mutex_lock(&lruMutex);

    // only try to remove this frame from the LRU list if no other user has this
    // frame currently fixed (already removed)
    if(fp->currentUsers++ == 0) {
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
    if(lru != NULL) {
        lru = ret->next;
        ret->next = NULL;

        if(lru != NULL)
            lru->prev = NULL;
        else
            mru = NULL;
    }

    pthread_mutex_unlock(&lruMutex);

    return ret;
}
