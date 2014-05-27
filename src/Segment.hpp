#ifndef SEGMENT_H_
#define SEGMENT_H_

#include <atomic>

#include "BufferManager.hpp"

class Segment {
  protected:
    uint64_t            id;
    std::atomic<size_t> size; // size in pages
    BufferManager&      bm;

  public:
    Segment(BufferManager& bm, uint64_t id) : id(id), size(0), bm(bm) {}

    uint64_t getID() {
        return id;
    }

    size_t getSize() {
        return size.load();
    }
};

#endif  // SEGMENT_H_
