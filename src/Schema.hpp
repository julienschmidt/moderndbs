#ifndef SCHEMA_H_
#define SCHEMA_H_

#include <vector>
#include <string>

#include "Types.hpp"

struct Schema {
   struct Relation {
      struct Attribute {
         std::string name;
         Types::Tag  type;
         size_t      len;
         bool        notNull;

         Attribute() : len(~0), notNull(true) {}
      };

      std::string name;
      std::vector<unsigned>                    primaryKey;
      std::vector<Schema::Relation::Attribute> attributes;
      size_t      size;      // number of pages
      uint16_t    segmentID; // segment is assigned on first write; 0 -> unass.

      Relation(const std::string& name) : name(name), size(0), segmentID(0) {}
   };
   std::vector<Schema::Relation> relations;
   std::string toString() const;
};

#endif // SCHEMA_H_
