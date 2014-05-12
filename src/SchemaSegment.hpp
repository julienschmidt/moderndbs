#ifndef SCHEMASEGMENT_H_
#define SCHEMASEGMENT_H_

#include "Schema.hpp"
#include "Segment.hpp"

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
