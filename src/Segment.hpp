#ifndef SEGMENT_H_
#define SEGMENT_H_

#include "BufferManager.hpp"

class Segment {
  protected:
    uint64_t        id;
    size_t          size; // size in pages
    BufferManager&  bm;

  public:
    Segment(BufferManager& bm, uint64_t id) : id(id), bm(bm) {

    }

    uint64_t getID() {
        return id;
    }
};

#endif  // SEGMENT_H_
