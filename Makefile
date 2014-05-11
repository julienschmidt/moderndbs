CC      = clang++
CFLAGS  = -std=c++11 -march=native -O3 -Wall -pthread

BUFFER_O = src/BufferManager.cpp src/BufferFrame.cpp

all: clean sort buffer schema

sort: test/sort_test.cpp src/sort.cpp
	$(CC) $(CFLAGS) -o bin/sort test/sort_test.cpp src/sort.cpp

buffer: test/buffer_test.cpp $(BUFFER_O)
	$(CC) $(CFLAGS) -o bin/buffer test/buffer_test.cpp $(BUFFER_O)

schema: test/schema_test.cpp src/Parser.cpp src/Schema.cpp src/SchemaSegment.cpp $(BUFFER_O)
	$(CC) $(CFLAGS) -o bin/schema test/schema_test.cpp src/Parser.cpp src/Schema.cpp src/SchemaSegment.cpp $(BUFFER_O)

clean:
	rm -rf bin/*
