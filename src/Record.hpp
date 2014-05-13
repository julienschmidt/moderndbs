#ifndef RECORD_H_
#define RECORD_H_

#include <stdlib.h>
#include <string.h>

// A simple Record implementation
class Record {
   unsigned len;
   char*    data;

public:
   // Assignment Operator: deleted
   Record& operator=(Record& rhs) = delete;

   // Copy Constructor: deleted
   Record(Record& t) = delete;

   // Move Constructor
   Record(Record&& t) : len(t.len), data(t.data) {
      t.data = nullptr;
      t.len = 0;
   }

   // Constructor
   Record(unsigned len, const char* const ptr) : len(len) {
      data = static_cast<char*>(malloc(len));
      if (data)
         memcpy(data, ptr, len);
   }

   // Destructor
   ~Record() {
      free(data);
   }

   // Get pointer to data
   const char* getData() const {
      return data;
   }

   // Get data size in bytes
   unsigned getLen() const {
      return len;
   }
};

#endif // RECORD_H_
