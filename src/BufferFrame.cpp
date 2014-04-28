#include <iostream>
#include <pthread.h>
#include <unistd.h>

#include "BufferFrame.hpp"

#define DEBUG

BufferFrame::BufferFrame(int segmentFd, uint64_t pageID) {
    pthread_rwlock_init(&rwlock, NULL);

    id     = pageID;
    data   = NULL;
    state  = state_t::New;
    offset = blocksize * (pageID & 0x0000FFFFFFFFFFFF);
    fd     = segmentFd;

    prev = next = NULL;

    std::cout << "** constructed Frame " << id << std::endl;
}

BufferFrame::~BufferFrame() {
    std::cout << "##1 destruct Frame " << id << std::endl;
    lock(true); // get exclusive lock / wait until all tasks are finished
    std::cout << "##2 destruct Frame " << id << std::endl;
    pthread_rwlock_destroy(&rwlock);
    std::cout << "##3 destruct Frame " << id << std::endl;

    flush();

    // deallocate data, if necessary
    if (data != NULL)
        free(data);
}

void BufferFrame::lock(bool exclusive) {
    if (exclusive) {
        std::cout << ">>> lockin exc " << id << std::endl;
        pthread_rwlock_wrlock(&rwlock);
        std::cout << ">>> locked exc " << id << std::endl;
    } else {
        std::cout << ">>> lockin shr " << id << std::endl;
        pthread_rwlock_rdlock(&rwlock);
        std::cout << ">>> locked shr " << id << std::endl;
    }
}

void BufferFrame::unlock() {
    pthread_rwlock_unlock(&rwlock);
}


void* BufferFrame::getData() {
    std::cout << "getData " << id << std::endl;

    // load data, if not already loaded
    if (state == state_t::New)
        loadData();

    return data;
}

void BufferFrame::flush() {
    // write all changes, if page is dirty
    if (state == state_t::Dirty)
        writeData();
}

// load data from disk
void BufferFrame::loadData() {
    std::cout << "loadData " << id << std::endl;

#ifdef DEBUG
    if (state == state_t::Dirty)
        std::cout << "WARNING: Data loss on load? (state == Dirty)" << std::endl;
    else if (state == state_t::Clean)
        std::cout << "WARNING: Unnecessary data load (state == Clean)" << std::endl;
#endif

    // in-memory buffer
    data = malloc(blocksize);

    // read data from file to buffer
    pread(fd, data, blocksize, offset);

    state = state_t::Clean;

    std::cout << "loaded " << id << std::endl;
}

void BufferFrame::writeData() {
    // write data back to file on disk
    pwrite(fd, data, blocksize, offset);

    state = state_t::Clean;
}
