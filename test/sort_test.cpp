#include <iostream>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>

#include "../src/sort.hpp"

using namespace std;

bool checkOrder(int fd, size_t fsize);

int main(int argc, char* argv[]) {
    if(argc < 4) {
        cerr << "Usage: " << argv[0]
             << " <inputFile> <outputFile> <memoryBufferInMiB>" << endl;
        exit(EXIT_FAILURE);
    }

    // check the buffer size value first
    uint64_t memSize = strtoull(argv[3], NULL, 10);
    if(memSize < 1) {
        cerr << "Argument 3 <memoryBufferInMiB> invalid: "
             << "Value must be positive integer" << endl;
        exit(EXIT_FAILURE);
    }
    memSize *= 1024*1024; // MiB to B

    // then try to open the files with the given file paths
    int fdInput, fdOutput;
    if((fdInput = open(argv[1], O_RDONLY)) < 0) {
        perror("Can not open input file");
        exit(EXIT_FAILURE);
    }
    if((fdOutput = open(argv[2], O_RDWR | O_CREAT)) < 0) {
        perror("Can not open output file");
        close(fdInput);
        exit(EXIT_FAILURE);
    }

    // calculate number of values from filesize
    struct stat fs;
    if(stat(argv[1], &fs) < 0) {
        perror("input file stat failed");
        exit(EXIT_FAILURE);
    }
    uint64_t size = (uint64_t)fs.st_size / sizeof(uint64_t);


    // call external sort function (test entity)
    externalSort(fdInput, size, fdOutput, memSize);

    // check file size of output file
    off_t fsizeIn, fsizeOut;
    fsizeIn = fs.st_size;
    if(stat(argv[2], &fs) < 0) {
        perror("output file stat failed");
        exit(EXIT_FAILURE);
    }
    fsizeOut = fs.st_size;
    if(fsizeIn != fsizeOut) {
        cerr << "filesize of output file incorrect: in "
             << fsizeIn << "B, out " << fsizeOut << "B" << endl;
    }

    // check if the file is sorted correctly
    bool sorted = checkOrder(fdOutput, (size_t)fsizeOut);

    // close file descriptors of input and output file
    close(fdInput);
    close(fdOutput);

    if(!sorted) {
        cerr << "List is unsorted" << endl;
        exit(EXIT_FAILURE);
    }
    cout << "List is sorted" << endl;

    return EXIT_SUCCESS;
}

//Checks if the file is in ascending ordering
bool checkOrder(int fd, size_t fsize) {
    size_t result = 0;

    uint64_t prev    = 0;
    uint64_t current = 0;

    // rewind
    if(lseek(fd, 0, SEEK_SET) < 0) {
        perror("rewinding output file failed");
        return false;
    }

    if(fsize < 2*sizeof(uint64_t)) {
        // skip, need min 2 values
        return true;
    }

    // read first value
    result += (size_t)read(fd, &prev, sizeof(uint64_t));

    // read rest
    while(result < fsize) {
        prev = current;
        result += (size_t)read(fd, &current, sizeof(uint64_t));

        if(prev > current) {
            return false;
        }
    }

    return true;
}
