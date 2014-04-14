CC      = clang++
CFLAGS  = -std=c++11 -march=native -O3 -Weverything -Wno-c++98-compat -Werror

all: clean sort

sort: test/sort_test.cpp src/sort.cpp
	$(CC) $(CFLAGS) -o bin/sort test/sort_test.cpp src/sort.cpp

clean:
	rm -rf bin/*
