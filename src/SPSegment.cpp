#include <new>
#include <queue>
#include <vector>

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

    // try all existing pages
    for (pageID=0; pageID < size; pageID++) {
        // open page for read only and read the header
        BufferFrame& rbf = bm.fixPage(segPfx | pageID, false);
        data   = static_cast<char*>(rbf.getData());
        header = reinterpret_cast<Header*>(data);
        size_t freeSpace = header->freeSpace;

        // enough free space in the current page?
        if(recLen+sizeof(Slot) <= freeSpace) {
            off_t headEnd   = sizeof(Header) + (header->slotCount+1)*sizeof(Slot);
            off_t dataStart = header->dataStart;
            bm.unfixPage(rbf, false);

            if (dataStart <= headEnd || recLen > (dataStart - headEnd)) {
                //must compact the page
                compactPage(pageID);
            }
            break; // found a page with enough space
        }
        bm.unfixPage(rbf, false);
    }

    // open page for writing
    BufferFrame& bf = bm.fixPage(segPfx | pageID, true);
    data = static_cast<char*>(bf.getData());

    if (pageID == size) { // new page
        size++;

        // insert new Header
        header = new (data) Header();

        // insert new Slot
        slotID = 0;
        slot   = new (data+sizeof(Header)) Slot();
        header->slotCount     = 1;
        header->firstFreeSlot = 1;

        // update free space
        header->freeSpace -= sizeof(Slot) + recLen;

    } else { // existing page
        uint32_t slotCount = header->slotCount;

        // look for a existing free slot
        if (header->firstFreeSlot < slotCount) {
            slotID = header->firstFreeSlot;

            // search for the next free slot
            uint32_t i = slotID+1;
            for(; i < slotCount; i++) {
                Slot& otherSlot = reinterpret_cast<Slot*>(data+sizeof(Header))[i];
                if (otherSlot.isFree())
                    break;
            }
            header->firstFreeSlot = i;

            // update free space
            header->freeSpace -= recLen;

        // insert new Slot
        } else {
            slotID = slotCount;

            header->slotCount++;
            header->firstFreeSlot = header->slotCount;

            // update free space
            header->freeSpace -= sizeof(Slot) + recLen;
        }

        slot = new (data + sizeof(Header) + slotID*sizeof(Slot)) Slot();
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
    // open page for writing
    BufferFrame& bf = bm.fixPage((id << 48) | tid.pageID, true);
    char* data = static_cast<char*>(bf.getData());

    // read page
    Header& header = reinterpret_cast<Header*>(data)[0];
    Slot&   slot   = reinterpret_cast<Slot*>(data+sizeof(Header))[tid.slotID];

    // update the first free slot cache
    if (tid.slotID < header.firstFreeSlot)
        header.firstFreeSlot = tid.slotID;

    if(slot.isIndirection()) {
        TID itid = slot.getIndirectionTID();

        // mark this slot as empty
        slot.length = 0;
        slot.offset = 0;

        // close page and recursively remove the indirected TID
        bm.unfixPage(bf, true);
        return remove(itid);

    } else {
        // if this slots contains the last data block, adjust header->dataStart
        if (slot.offset == header.dataStart)
            header.dataStart += slot.length;

        // update free space
        header.freeSpace += slot.length;

        // mark this slot as empty
        slot.length = 0;
        slot.offset = 0;

        // close page and return
        bm.unfixPage(bf, true);
        return true;
    }
}

// Returns the read-only record associated with TID tid
Record SPSegment::lookup(TID tid) {
    // open page for reading
    BufferFrame& bf = bm.fixPage((id << 48) | tid.pageID, false);
    char* data = static_cast<char*>(bf.getData());

    // read page
    Slot& slot = reinterpret_cast<Slot*>(data+sizeof(Header))[tid.slotID];

    if(slot.isIndirection()) {
        // recursively lookup
        TID itid = slot.getIndirectionTID();
        bm.unfixPage(bf, false);
        return lookup(itid);

    } else {
        Record r(slot.length, data+slot.offset);

        // close page and return
        bm.unfixPage(bf, false);
        return std::move(r);
    }
}

// Updates the record pointed to by tid with the content of record r
bool SPSegment::update(TID tid, const Record& r) {
    // open page for writing
    BufferFrame& bf = bm.fixPage((id << 48) | tid.pageID, true);
    char* data = static_cast<char*>(bf.getData());
    Slot& slot = reinterpret_cast<Slot*>(data+sizeof(Header))[tid.slotID];

    if (slot.isIndirection()) {
        TID itid = slot.getIndirectionTID();
        bm.unfixPage(bf, false);

        // update recursively
        if(!update(itid, r))
            return false;

        // handle double indirections
        BufferFrame& ibf = bm.fixPage((id << 48) | itid.pageID, true);
        char* idata = static_cast<char*>(ibf.getData());
        Slot& islot = reinterpret_cast<Slot*>(idata+sizeof(Header))[itid.slotID];

        if (!islot.isIndirection()) {
            bm.unfixPage(ibf, false);
            return true;

        } else { // double indirected
            // skip first indirection
            BufferFrame& bf = bm.fixPage((id << 48) | tid.pageID, true);
            data = static_cast<char*>(bf.getData());
            Slot& slot = reinterpret_cast<Slot*>(data+sizeof(Header))[tid.slotID];
            slot.setIndirection(islot.getIndirectionTID());
            bm.unfixPage(bf, true);

            // mark first indirection slot as free
            islot.offset = 0;
            islot.length = 0;

            // update first free slot cache
            Header& iheader = reinterpret_cast<Header*>(idata)[0];
            if (itid.slotID < iheader.firstFreeSlot)
                iheader.firstFreeSlot = itid.slotID;

            bm.unfixPage(ibf, true);
            return true;
        }
    } else {
        Header& header = reinterpret_cast<Header*>(data)[0];

        unsigned newLen = r.getLen();
        unsigned oldLen = slot.length;

        // check if able to replace in-place
        if (newLen <= oldLen) {
            // update length and free space
            slot.length = newLen;
            header.freeSpace += oldLen - newLen;

            // copy the data
            char* recPtr = data + slot.offset;
            memcpy(recPtr, r.getData(), newLen);

            // close page and return
            bm.unfixPage(bf, true);
            return true;

        } else {
            // remove current record
            slot.length = 0;
            header.freeSpace += oldLen;

            // not in the mood for deadlocks today?
            bm.unfixPage(bf, true);

            // insert again
            TID newtid = insert(r);

            // open same page again for writing
            BufferFrame& bf2 = bm.fixPage((id << 48) | tid.pageID, true);
            char* data2 = static_cast<char*>(bf2.getData());
            Slot& slot2 = reinterpret_cast<Slot*>(data2+sizeof(Header))[tid.slotID];

            // handle indirection to same page
            if (newtid.pageID == tid.pageID) {
                Slot& newslot = reinterpret_cast<Slot*>(data2+sizeof(Header))[newtid.slotID];
                slot2.offset = newslot.offset;
                slot2.length = newslot.length;

                // mark new slot as free
                newslot.offset = 0;
                newslot.length = 0;

                // update first free slot cache
                Header& header = reinterpret_cast<Header*>(data2)[0];
                if (newtid.slotID < header.firstFreeSlot)
                    header.firstFreeSlot = newtid.slotID;
            } else {
                slot2.setIndirection(newtid);
            }

            // close page and return
            bm.unfixPage(bf2, true);
            return true;
        }
    }

    return false;
}

void SPSegment::compactPage(uint32_t pageID) {
    // open the page for writing
    BufferFrame& bf     = bm.fixPage((id << 48) | pageID, true);
    char*        data   = static_cast<char*>(bf.getData());
    Header*      header = reinterpret_cast<Header*>(data);

    uint32_t slotCount = header->slotCount;
    Slot*    slots     = reinterpret_cast<Slot*>(data+sizeof(Header));

    // put all non-free and non-indirection slots in a priority queue
    struct SlotPtrCmpOffst {
        bool operator() (Slot* const &a, Slot* const &b) {
            return a->offset < b->offset;
        }
    };
    std::priority_queue<Slot*, std::vector<Slot*>, SlotPtrCmpOffst> slotPtrs;
    for(uint32_t slotID = 0; slotID < slotCount; slotID++) {
        Slot& slot = slots[slotID];

        if (!slot.isIndirection() && !slot.isFree())
            slotPtrs.push(&slot);
    }

    off_t offset = blocksize;

    // move records
    while (!slotPtrs.empty()) {
        Slot* slot = slotPtrs.top();
        slotPtrs.pop();

        // nothing to move, if the record length is 0 bytes
        if (slot->length == 0)
            continue;

        // move the data
        offset -= slot->length;
        memmove(data+offset, data+slot->offset, slot->length);
        slot->offset = offset;
    }

    header->dataStart = offset;

    // close page and return
    bm.unfixPage(bf, true);
}
