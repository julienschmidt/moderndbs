CXX:=g++
CXXFLAGS:=-std=c++0x -g -O2 -Wall

all: parser

Schema.o: Schema.cpp Schema.hpp Makefile
	$(CXX) $(CXXFLAGS) -c Schema.cpp -o Schema.o

Parser.o: Parser.cpp Parser.hpp Makefile
	$(CXX) $(CXXFLAGS) -c Parser.cpp -o Parser.o

parser: Schema.o Parser.o Schema.hpp Types.hpp parser.cpp Makefile
	$(CXX) $(CXXFLAGS) Parser.o Schema.o parser.cpp -o parser

clean:
	rm -rf *.o parser
