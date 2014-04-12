CC      = clang++
CFLAGS  = -std=c++11 -march=native -O3 -Weverything -Wno-c++98-compat -Werror

all: sort

sort: src/sort.hpp src/sort.cpp
	$(CC) $(CFLAGS) -o bin/sort src/sort.cpp

clean:
	rm bin/*
