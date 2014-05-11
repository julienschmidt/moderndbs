#include <iostream>

#include "BufferManager.hpp"
#include "SchemaSegment.hpp"

SchemaSegment::SchemaSegment(BufferManager& bm, uint64_t id, Schema& schema) : Segment(bm, id), schema(schema) {
    writeToDisk();
}

SchemaSegment::SchemaSegment(BufferManager& bm, uint64_t id) : Segment(bm, id), schema(*(new Schema())) {
    loadFromDisk();
}

void SchemaSegment::writeToDisk() {
    BufferFrame& bf = bm.fixPage(id, true);
    void* dataPtr = bf.getData();

    // WARNING: nasty pointer action behind this line
    // Java or Ruby hackers, last chance to turn back!

    // number of relations
    size_t  relCount = schema.relations.size();
    size_t* stPtr    = (size_t*) dataPtr;
    *stPtr           = relCount;
    ++stPtr;

    dataPtr = (void*) stPtr;

    // serialize relations
    for(size_t i=0; i < relCount; ++i) {
        Schema::Relation& relation = schema.relations[i];

        // name length
        size_t nameLen = relation.name.length();
        stPtr = (size_t*) dataPtr;
        *stPtr = nameLen;
        ++stPtr;

        // name data
        char* charPtr = (char*) stPtr;
        memcpy(charPtr, relation.name.data(), nameLen);
        charPtr += nameLen;

        // primary key length
        size_t pkLen = relation.primaryKey.size();
        stPtr = (size_t*) charPtr;
        *stPtr = pkLen;
        ++stPtr;

        // primary key data
        unsigned* usgPtr = (unsigned*) stPtr;
        memcpy(usgPtr, &relation.primaryKey[0], pkLen*sizeof(unsigned));
        usgPtr += pkLen;

        // number of attributes
        size_t attrCount = relation.attributes.size();
        stPtr = (size_t*) usgPtr;
        *stPtr = attrCount;
        ++stPtr;

        dataPtr = (void*) stPtr;

        // serialize attributes
        for(size_t j=0; j < attrCount; ++j) {
            Schema::Relation::Attribute& attr = relation.attributes[j];

            // name length
            nameLen = attr.name.length();
            stPtr = (size_t*) dataPtr;
            *stPtr = nameLen;
            ++stPtr;

            // name data
            char* charPtr = (char*) stPtr;
            memcpy(charPtr, attr.name.data(), nameLen);
            charPtr += nameLen;

            // type
            Types::Tag* typePtr = (Types::Tag*) charPtr;
            *typePtr = attr.type;
            ++typePtr;

            // len
            stPtr = (size_t*) typePtr;
            *stPtr = attr.len;
            ++stPtr;

            // notNull
            // TODO: pack this into a bit field or something
            charPtr = (char*) stPtr;
            *charPtr = (char) attr.notNull;
            ++charPtr;

            dataPtr = (void*) charPtr;
        }
    }

    bf.flush();
    bm.unfixPage(bf, true);
}

void SchemaSegment::loadFromDisk() {
    BufferFrame& bf = bm.fixPage(id, false);
    void* dataPtr = bf.getData();

    // WARNING: nasty pointer action behind this line
    // Java or Ruby hackers, last chance to turn back!

    // number of relations
    size_t* stPtr    = (size_t*) dataPtr;
    size_t  relCount = *stPtr;
    ++stPtr;
    schema.relations.reserve(relCount);

    dataPtr = (void*) stPtr;

    // deserialize relations
    for(size_t i=0; i < relCount; ++i) {
        // name
        stPtr = (size_t*) dataPtr;
        size_t nameLen = *stPtr;
        ++stPtr;
        char* charPtr = (char*) stPtr;
        schema.relations.push_back(Schema::Relation(
            std::string(charPtr, nameLen)
        ));
        charPtr += nameLen;

        Schema::Relation& relation = schema.relations[i];

        // primary key
        stPtr = (size_t*) charPtr;
        size_t pkLen = *stPtr;
        ++stPtr;
        unsigned* usgPtr = (unsigned*) stPtr;
        relation.primaryKey.assign(usgPtr, usgPtr+pkLen);
        usgPtr+=pkLen;

        // number of attributes
        stPtr = (size_t*) usgPtr;
        size_t attrCount = *stPtr;
        ++stPtr;
        relation.attributes.resize(attrCount);

        dataPtr = (void*) stPtr;

        // deserialize attributes
        for(size_t j=0; j < attrCount; ++j) {
            Schema::Relation::Attribute& attr = relation.attributes[j];

            // name
            stPtr = (size_t*) dataPtr;
            size_t nameLen = *stPtr;
            ++stPtr;
            char* charPtr = (char*) stPtr;
            attr.name = std::string(charPtr, nameLen);
            charPtr += nameLen;

            // type
            Types::Tag* typePtr = (Types::Tag*) charPtr;
            attr.type = *typePtr;
            ++typePtr;

            // len
            stPtr = (size_t*) typePtr;
            attr.len = *stPtr;
            ++stPtr;

            // notNull
            // TODO: pack this into a bit field or something
            charPtr = (char*) stPtr;
            *charPtr = (char) attr.notNull;
            ++charPtr;

            dataPtr = (void*) charPtr;
        }
    }

    bm.unfixPage(bf, false);
}
