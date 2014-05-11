#ifndef SCHEMASEGMENT_H_
#define SCHEMASEGMENT_H_

#include "Schema.hpp"
#include "Segment.hpp"

struct Header {
    size_t  numRelations; // number of relations
    void*   dataPtr;      // last data block
};

struct Relation {
    /*
    segmentID
    nameOffset
    nameLength
    size       // size in pages
    ? pkOffset // begin of PK List
    ? pkLength // PK list length
    attrOffset
    attrLength
    */
};

/*struct Atrribute {
    // id // position of the attribute in the relation
    Types::Tag type;
    size_t length; // length in bytes
    bool notNull;
    nameOffset;
    nameLength;
};*/

class SchemaSegment : public Segment {
  public:
    // create segment from new schema
    SchemaSegment(BufferManager& bm, uint64_t id, Schema& schema);

    // create segment and load schema from disk
    SchemaSegment(BufferManager& bm, uint64_t id);

    Schema* getSchema() {
        return &schema;
    }

  private:
    Schema& schema;
    void writeToDisk();
    void loadFromDisk();
};

#endif  // SCHEMASEGMENT_H_
