#ifndef SPSEGMENT_H_
#define SPSEGMENT_H_

#include "Record.hpp"
#include "Segment.hpp"
#include "TID.hpp"

// A slotted page consists of three parts: A header, the slots and the
// (variable-length) records. Records are addressed by TIDs (tuple identifier),
// consisting of a page ID and a slotID
class SPSegment : public Segment {
  public:
    // exported for the test
    struct Header {
        // LSN for recovery
        uint16_t slotCount;     // number of used slots
        uint16_t firstFreeSlot; // cache to speed up locating free slots
        off_t    dataStart;     // lower end of the data
        size_t   freeSpace;     // space that would be available after compaction
        Header() : slotCount(0), firstFreeSlot(0), dataStart(blocksize), freeSpace(blocksize-sizeof(Header)) {}
    };
    struct Slot {
        size_t length;   // length of the data item
        //size_t alloc;  // number of allocated bytes
        off_t  offset;   // start of the data item
        // free slot: oﬀset = 0, length = 0
        // zero-length data item: oﬀset > 0, length = 0
        Slot() : length(0), offset(0) {}
    };

    SPSegment(BufferManager& bm, uint64_t id) : Segment(bm, id) {};

    // Searches through the segment's pages looking for a page with enough space
    // to store r. Returns the TID identifying the location where r was stored
    TID insert(const Record& r);

    // Deletes the record pointed to by tid and updates the page header
    // accordingly
    bool remove(TID tid);

    // Returns the read-only record associated with TID tid
    Record lookup(TID tid);

    // Updates the record pointed to by tid with the content of record r
    bool update(TID tid, const Record& r);

  private:


    //std::map<uint16_t,std::list<off_t>> blockMap;
};

#endif  // SPSEGMENT_H_



