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
        uint32_t slotCount;     // number of used slots
        uint32_t firstFreeSlot; // cache to speed up locating free slots
        off_t    dataStart;     // lower end of the data
        size_t   freeSpace;     // space that would be available after compaction
        Header() : slotCount(0), firstFreeSlot(0), dataStart(blocksize), freeSpace(blocksize-sizeof(Header)) {}
    };

    struct Slot {
        off_t    offset; // start of the data item
        uint64_t length; // length of the data item

        Slot() : offset(0), length(0) {}

        // offset = 0 => free
        inline bool isFree() {
            return (offset == 0);
        }

        // offset = 1 => indirection
        inline bool isIndirection() {
            return (offset == 1);
        }

        inline TID getIndirectionTID() {
            return TID{(uint32_t) (length >> 32), (uint32_t) length};
        }

        inline void setIndirection(TID tid) {
            offset = 1;
            length = (uint64_t) (tid.pageID) << 32 | tid.slotID;
        }
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
    // compacts the given page by moving records
    void compactPage(uint32_t pageID);
};

#endif  // SPSEGMENT_H_



