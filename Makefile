CC      = clang++
CFLAGS  = -std=c++11 -march=native -O3 -Wall -pthread

BUFFER_O = src/BufferManager.cpp src/BufferFrame.cpp

all: clean sort buffer btree operators schema slotted

sort: test/sort_test.cpp src/sort.cpp
	$(CC) $(CFLAGS) -o bin/sort test/sort_test.cpp src/sort.cpp

buffer: test/buffer_test.cpp $(BUFFER_O)
	$(CC) $(CFLAGS) -o bin/buffer test/buffer_test.cpp $(BUFFER_O)

btree: test/btree_test.cpp $(BUFFER_O)
	$(CC) $(CFLAGS) -o bin/btree test/btree_test.cpp $(BUFFER_O)

operators: test/operators_test.cpp src/SPSegment.cpp
	$(CC) $(CFLAGS) -o bin/operators test/operators_test.cpp src/SPSegment.cpp $(BUFFER_O)

schema: test/schema_test.cpp src/Parser.cpp src/Schema.cpp src/SchemaSegment.cpp $(BUFFER_O)
	$(CC) $(CFLAGS) -o bin/schema test/schema_test.cpp src/Parser.cpp src/Schema.cpp src/SchemaSegment.cpp $(BUFFER_O)

slotted: test/slotted_test.cpp src/SPSegment.cpp $(BUFFER_O)
	$(CC) $(CFLAGS) -o bin/slotted test/slotted_test.cpp src/SPSegment.cpp $(BUFFER_O)

clean:
	rm -rf bin/*
