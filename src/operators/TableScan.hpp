#ifndef TABLESCAN_H_
#define TABLESCAN_H_

#include <vector>

#include "../Operator.hpp"
#include "../Schema.hpp"
#include "../SPSegment.hpp"

class TableScan: public Operator {
    SPSegment&                               seg;
    BufferManager&                           bm;
    BufferFrame*                             bf;
    char*                                    data;
    uint64_t                                 pageID;
    uint64_t                                 lastID;
    uint32_t                                 slotCount;
    uint32_t                                 slotID;
    SPSegment::Slot*                         slots;
    std::vector<Register*>                   regs;
    std::vector<Schema::Relation::Attribute> attributes;

  public:
    TableScan(Schema::Relation& rel, SPSegment& seg);
    void                   open();
    bool                   next();
    std::vector<Register*> getOutput();
    void                   close();
};

TableScan::TableScan(Schema::Relation& rel, SPSegment& seg) :
    seg(seg), bm(seg.bm), attributes(rel.attributes) {
        regs.resize(attributes.size());
    }

void TableScan::open() {
    // For now, we ignore possible inconsistencies because of writes on other
    // pages during the iteration.
    // Opening all pages at the start of the scan is not an option since there
    // might be more pages than free slots in the buffer manager.
    // TODO: Use e.g. locking

    assert(bf == NULL);

    pageID = (seg.id << 48);
    lastID = pageID+seg.size-1;
}

bool TableScan::next() {
    // loop over pages
    while (true) {
        // load a new page if necessary
        if (bf == NULL) {
            // check if we reached the end of the segment
            if (pageID > lastID) {
                return false;
            }

            // TODO: implement and use an SPSegment iterator instead
            bf   = &bm.fixPage(pageID, false);
            data = static_cast<char*>(bf->getData());
            auto  header = reinterpret_cast<SPSegment::Header*>(data);

            slotCount = header->slotCount;
            slotID    = 0;
            slots     = reinterpret_cast<SPSegment::Slot*>(data+sizeof(SPSegment::Header));

            ++pageID;
        }

        // loop over the slots of the current page
        while(slotID < slotCount) {
            auto slot = slots[slotID];
            ++slotID;

            // skip free and indirection slots
            if (!slot.isRecord()) {
                continue;
            }

            // we assume that all types have a fixed length
            char* recordPtr = data+slot.offset;
            off_t recordOff = 0;
            for (int i = 0; i < regs.size(); ++i) {
                Register* reg = new Register;
                reg->load(attributes[i].type, recordPtr+recordOff);
                regs[i] = reg;
                recordOff += attributes[i].len;
            }
            assert(recordOff == slot.length);
            return true;
        }

        // reached the end of the page, unload
        bm.unfixPage(*bf, false);
        bf = NULL;
    }
}

std::vector<Register*> TableScan::getOutput() {
    return regs;
}

void TableScan::close() {
    if (bf != NULL) {
        bm.unfixPage(*bf, false);
        bf = NULL;
    }
}

#endif  // TABLESCAN_H_
