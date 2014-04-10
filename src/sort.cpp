#include <algorithm>
#include <fcntl.h>
#include <iostream>
#include <queue>
#include <stdio.h>
#include <unistd.h>

#include "sort.hpp"

using namespace std;

void externalSort(int fdInput, uint64_t size, int fdOutput, uint64_t memSize) {
    if(size == 0) {
        // nothing to do here
        return;
    }

    if(memSize < sizeof(uint64_t)) {
        cerr << "Not enough memory for sorting" << endl;
        return;
    }

    /* Not available on OS X
    // Preallocate file space for output file
    if(posix_fallocate(fdOutput, 0, size) < 0) {
        perror("Output file file space allocation failed");
        return;
    }*/

    // total size of input data in bytes
    //uint64_t dataSize  = size * sizeof(uint64_t);

    // how many values fit into one chunk
    uint64_t chunkSize = min(memSize / sizeof(uint64_t), size);

    // number of chunks we must split the input into
    uint64_t numChunks = (size + chunkSize-1) / chunkSize;

    vector<FILE*> chunkFiles;
    chunkFiles.reserve(numChunks);

    vector<uint64_t> chunkBuf;
    chunkBuf.reserve(chunkSize);

    for(uint64_t i=0; i < numChunks; i++) {
        // read one chunk of input into memory
        size_t valuesToRead = chunkSize;
        if(size - i*chunkSize < valuesToRead) {
            valuesToRead = size - i*chunkSize;
            chunkBuf.resize(valuesToRead);
        }
        size_t bytesToRead  = valuesToRead * sizeof(uint64_t);

        if(read(fdInput, &chunkBuf[0], bytesToRead) < (ssize_t)bytesToRead) {
            perror("Reading chunk of input failed");
            return;
        }


        // sort the values in the chunk
        sort(chunkBuf.begin(), chunkBuf.end());


        // open a new temporary file and save the chunk externally
        FILE* tmpf = NULL;
        tmpf = tmpfile();
        if(tmpf == NULL) {
            cerr << "Creating a temporary file failed!" << endl;
            return;
        }
        chunkFiles.push_back(tmpf);

        fwrite(&chunkBuf[0], sizeof(uint64_t), valuesToRead, tmpf);
    }


    for(FILE* tmpf : chunkFiles) {
        //write(fdOutput, &buffer, nbyte);
        fclose(tmpf);
    }
}
