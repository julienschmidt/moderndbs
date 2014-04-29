#ifndef H_Record_HPP
#define H_Record_HPP

#include <cstring>
#include <cstdlib>

// A simple Record implementation
class Record {
   unsigned len;
   char* data;

public:
   // Assignment Operator: deleted
   Record& operator=(Record& rhs) = delete;
   // Copy Constructor: deleted
   Record(Record& t) = delete;
   // Move Constructor
   Record(Record&& t);
   // Constructor
   Record(unsigned len, const char* const ptr);
   // Destructor
   ~Record();
   // Get pointer to data
   const char* getData() const;
   // Get data size in bytes
   unsigned getLen() const;
};

Record::Record(Record&& t) : len(t.len), data(t.data) {
   t.data = nullptr;
   t.len = 0;
}

Record::Record(unsigned len, const char* const ptr) : len(len) {
   data = static_cast<char*>(malloc(len));
   if (data)
      memcpy(data, ptr, len);
}

const char* Record::getData() const {
   return data;
}

unsigned Record::getLen() const {
   return len;
}

Record::~Record() {
   free(data);
}

#endif