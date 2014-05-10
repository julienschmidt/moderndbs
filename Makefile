CC      = clang++
CFLAGS  = -std=c++11 -march=native -O3 -Wall -pthread

all: clean sort buffer schema

sort: test/sort_test.cpp src/sort.cpp
	$(CC) $(CFLAGS) -o bin/sort test/sort_test.cpp src/sort.cpp

buffer: test/buffer_test.cpp src/BufferManager.cpp src/BufferFrame.cpp
	$(CC) $(CFLAGS) -o bin/buffer test/buffer_test.cpp src/BufferManager.cpp src/BufferFrame.cpp

schema: test/schema_test.cpp src/Parser.cpp src/Schema.cpp
	$(CC) $(CFLAGS) -o bin/schema test/schema_test.cpp src/Parser.cpp src/Schema.cpp

clean:
	rm -rf bin/*
