CC      = clang++
CFLAGS  = -Weverything -std=c++11 -march=native -O3

all: sort

sort: src/sort.hpp src/sort.cpp
	$(CC) $(CFLAGS) -o bin/sort src/sort.cpp

clean:
	rm bin/*
