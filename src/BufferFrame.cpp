#include <iostream>
#include <pthread.h>

#include "BufferFrame.hpp"

#define DEBUG

BufferFrame::BufferFrame(uint64_t pageID) {
    id = pageID;

    pthread_rwlock_init(&rwlock, NULL);

    this->state = state_t::New;

    // page ID (64 bit):
    //   first 16bit: segment (=filename)
    //   48bit: actual page ID
    filename = pageID >> 48;

    std::cout << "make frame with pageID " << pageID << "; filename " << filename << std::endl;

    // blocksize * realPageNo (realPageNo=0,1,2,...)
    offset = blocksize * (pageID & 0x0000FFFFFFFFFFFF);
}

BufferFrame::~BufferFrame() {
    //pthread_rwlock_destroy(&rwlock);

    // write all changes, if page is dirty
    if (state == state_t::Dirty)
        writeData();

    // deallocate data, if necessary
    //if (data != NULL)   // == state != state_t::New
    //    free data;
}

void BufferFrame::print() {
    std::cout << id <<  " filename: " << filename << " state: " << state;
}

void BufferFrame::lock(bool exclusive) {
    if (exclusive)
        pthread_rwlock_wrlock(&rwlock);
    else
        pthread_rwlock_rdlock(&rwlock);
}

void BufferFrame::unlock() {
    pthread_rwlock_unlock(&rwlock);
}


void* BufferFrame::getData() {
    // load data, if not already loaded (don't load on Clean, Dirty)
    if (state == state_t::New)
        loadData();

    return data;
}

// load data from disk
void BufferFrame::loadData() {
#ifdef DEBUG
    if (state == state_t::Dirty)
        std::cout << "WARNING: Data loss on load? (state == Dirty)" << std::endl;
    else if (state == state_t::Clean)
        std::cout << "WARNING: Unnecessary data load (state == Dirty)" << std::endl;
#endif

    //load from file filename
    //at position
    //with pread
    //TODO
    std::cout << "TODO: Load data from file " << filename << " at " << offset << "Bytes" << std::endl;
    data = malloc(blocksize);

    state = state_t::Clean;
}

//TODO
void BufferFrame::writeData() {
    std::cout << "TODO: Write data to file " << filename << " at " << offset << "Bytes" << std::endl;

    state = state_t::Clean;
}
