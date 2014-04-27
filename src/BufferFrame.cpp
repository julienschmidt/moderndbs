#include <iostream>
#include <pthread.h>
#include <unistd.h>

#include "BufferFrame.hpp"

#define DEBUG

BufferFrame::BufferFrame(int segmentFd, unsigned pageNo) {
    pthread_rwlock_init(&rwlock, NULL);

    state  = state_t::New;
    offset = blocksize * pageNo;
    fd     = segmentFd;
}

BufferFrame::~BufferFrame() {
    lock(true); // get exclusive lock / wait until all tasks are finished
    pthread_rwlock_destroy(&rwlock);

    flush();

    // deallocate data, if necessary
    if (data != NULL)
        free(data);
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
}

void BufferFrame::writeData() {
    // write data back to file on disk
    pwrite(fd, data, blocksize, offset);

    state = state_t::Clean;
}
