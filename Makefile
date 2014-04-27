CC      = clang++
CFLAGS  = -std=c++11 -march=native -O3 -Weverything -Wno-c++98-compat -Werror -Wno-padded

all: clean sort buffer

sort: test/sort_test.cpp src/sort.cpp
	$(CC) $(CFLAGS) -o bin/sort test/sort_test.cpp src/sort.cpp

buffer: test/buffer_test.cpp src/BufferManager.cpp src/BufferFrame.cpp
	$(CC) $(CFLAGS) -o bin/buffer test/buffer_test.cpp src/BufferManager.cpp src/BufferFrame.cpp

clean:
	rm -rf bin/*
