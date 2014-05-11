#ifndef SCHEMA_H_
#define SCHEMA_H_

#include <vector>
#include <string>

#include "Types.hpp"

struct Schema {
   struct Relation {
      struct Attribute {
         std::string name;
         Types::Tag type;
         size_t len;
         bool notNull;
         Attribute() : len(~0), notNull(true) {}
      };
      std::string name;
      std::vector<Schema::Relation::Attribute> attributes;
      std::vector<unsigned> primaryKey;
      Relation(const std::string& name) : name(name) {}
   };
   std::vector<Schema::Relation> relations;
   std::string toString() const;
};

#endif // SCHEMA_H_
