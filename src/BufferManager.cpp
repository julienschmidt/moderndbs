#include <iostream>

#include "BufferManager.hpp"

#define DEBUG

BufferManager::BufferManager(unsigned size) {
    maxSize = size*10; // TODO: change back
    pthread_rwlock_init(&framesLatch, NULL);

    frames.reserve(size);
}

BufferManager::~BufferManager() {
    flush();

    pthread_rwlock_destroy(&framesLatch);
}


BufferFrame& BufferManager::fixPage(uint64_t pageID, bool exclusive) {
    // acquire map lock
    rdlock();
    //wrlock();

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
            throw;
        }

        // release read lock and get the write lock
        unlock();
        wrlock();

        // create a new frame in the map (with key pageID)
        // we get a reference to the existing frame if another thread created it
        // in the meantime
        auto ret = frames.emplace(pageID, pageID);
        if (!ret.second) {
            std::cerr << "frame could not be inserted!" << std::endl;
            throw;
        }

        // reference of the newly inserted frame
        bf = &ret.first->second;
    }

    // release the lock on the map
    unlock();

    // acquire lock on the frame
    bf->lock(exclusive);

    // return frame reference
    return *bf;
}


void BufferManager::unfixPage(BufferFrame& frame, bool isDirty) {
    //std::cout << "::unfixPage([";
    //frame.print();
    //std::cout <<"],"<<isDirty<<")"<<std::endl;

    if(isDirty)
        frame.markDirty();

    // TODO flush to file?

    // release the lock on the frame
    frame.unlock();
}


void BufferManager::flush() {
    std::cout << "::flush()"<<std::endl;
    // TODO: write all dirty pages
}

