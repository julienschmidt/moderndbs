#include <iostream>
#include <new>

#include "SPSegment.hpp"


// Searches through the segment's pages looking for a page with enough space to
// store r. Returns the TID identifying the location where r was stored
TID SPSegment::insert(const Record& r) {
    char*    data;
    Header*  header;
    Slot*    slot;
    uint32_t pageID;
    uint32_t slotID;
    size_t   recLen = r.getLen();

    // segmentID prefix for the pageID
    uint64_t segPfx = (id << 48);

    // Try all existing pages
    for (pageID=0; pageID < size; pageID++) {
        // open page for read only and read the header
        BufferFrame& rbf = bm.fixPage(segPfx | pageID, false);
        data   = static_cast<char*>(rbf.getData());
        header = reinterpret_cast<Header*>(data);

        off_t headEnd   = sizeof(Header) + (header->slotCount+1)*sizeof(Slot);
        off_t dataStart = header->dataStart;

        bm.unfixPage(rbf, false);

        // enough free space in the current page?
        // TODO: check header->freeSpace instead (needs compaction)
        if (dataStart > headEnd && recLen <= (dataStart - headEnd)) {
            break; // found a page with enough space
        }
    }

    // open page for writing
    BufferFrame& bf = bm.fixPage(segPfx | pageID, true);
    data = static_cast<char*>(bf.getData());

    if (pageID == size) { // new page
        slotID = 0;
        size++;

        // Insert new Header
        header = new (data) Header();

        // Insert new Slot
        slot = new (data+sizeof(Header)) Slot();
        header->slotCount     = 1;
        header->firstFreeSlot = 1;
        header->freeSpace -= sizeof(Slot) + recLen;

        std::cout << "new page " << pageID << std::endl;
    } else {
        slotID = header->slotCount;

        // TODO: look for a existing free slot

        // Insert new Slot
        slot = new (data + sizeof(Header) + slotID*sizeof(Slot)) Slot();
        header->slotCount++;
        header->firstFreeSlot++;
        header->freeSpace -= sizeof(Slot) + recLen;

        std::cout << "reused page " << pageID << std::endl;
    }

    // Insert Record Data
    header->dataStart -= recLen;
    slot->offset = header->dataStart;
    slot->length = recLen;
    char* recPtr = data + slot->offset;
    memcpy(recPtr, r.getData(), recLen);

    bm.unfixPage(bf, true);

    return TID{pageID, slotID};
}

// Deletes the record pointed to by tid and updates the page header accordingly
bool SPSegment::remove(TID tid) {
    std::cout << "::remove " << tid.pageID << " " << tid.slotID << std::endl;
    return true;
}

// Returns the read-only record associated with TID tid
Record SPSegment::lookup(TID tid) {
    // open page
    uint64_t segPfx = (id << 48);
    BufferFrame& bf = bm.fixPage(segPfx | tid.pageID, false);
    char* data = static_cast<char*>(bf.getData());

    // read page
    Slot* slot = reinterpret_cast<Slot*>(data+sizeof(Header)+tid.slotID*sizeof(Slot));
    Record r(slot->length, data+slot->offset);

    // close page and return
    bm.unfixPage(bf, false);
    return std::move(r);
}

// Updates the record pointed to by tid with the content of record r
bool SPSegment::update(TID tid, const Record& r) {
    std::cout << "::update " << tid.pageID << " " << tid.slotID << std::endl;
    return false;
}
