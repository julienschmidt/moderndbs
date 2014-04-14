#include <algorithm>
#include <fcntl.h>
#include <iostream>
#include <queue>
#include <stdio.h>
#include <unistd.h>

#include "sort.hpp"

using namespace std;

struct mergeItem {
    uint64_t value;
    uint64_t srcIndex;

    bool operator < (const mergeItem& a) const {
        return a.value < value;
    }
};

template <typename T>
struct fileBuffer {
    FILE*    srcFile;
    T*       buffer;
    uint64_t length;
    uint64_t remaining;
};

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

    /*** STEP 1: Chunking ***/

    // how many values fit into one chunk
    const uint64_t chunkSize = min(memSize / sizeof(uint64_t), size);

    // number of chunks we must split the input into
    const uint64_t numChunks = (size + chunkSize-1) / chunkSize;

    cout << "filesize: " << (size*sizeof(uint64_t)) <<
    " B, memSize: " << memSize <<
    " B, chunkSize: " << chunkSize << ", numChunks: " << numChunks << endl;

    vector<FILE*> chunkFiles;
    chunkFiles.reserve(numChunks);

    vector<uint64_t> memBuf;
    memBuf.resize(chunkSize);

    for(uint64_t i=0; i < numChunks; i++) {
        // read one chunk of input into memory
        size_t valuesToRead = chunkSize;
        if(size - i*chunkSize < valuesToRead) {
            valuesToRead = size - i*chunkSize;
            memBuf.resize(valuesToRead);
        }
        size_t bytesToRead = valuesToRead * sizeof(uint64_t);
        cout << "chunk #" << i << " bytesToRead: " << bytesToRead << endl;

        if(read(fdInput, &memBuf[0], bytesToRead) != (ssize_t)bytesToRead) {
            perror("Reading chunk of input failed");
            return;
        }

        // sort the values in the chunk
        sort(memBuf.begin(), memBuf.end());

        // open a new temporary file and save the chunk externally
        FILE* tmpf = NULL;
        tmpf = tmpfile();
        if(tmpf == NULL) {
            cerr << "Creating a temporary file failed!" << endl;
            return;
        }
        chunkFiles.push_back(tmpf);

        fwrite(&memBuf[0], sizeof(uint64_t), valuesToRead, tmpf);
        if(ferror(tmpf)) {
            cerr << "Writing chunk to file failed!" << endl;
        }
    }


    /*** STEP 2: k-way merge chunks ***/

    // priority queue used for n-way merging values from n chunks
    priority_queue<mergeItem> queue;

    // We reuse the already allocated memory from the memBuf and split the
    // available memory between numChunks chunk buffers and 1 output buffer.
    const size_t bufSize  = memSize/((numChunks+1)*sizeof(uint64_t));
    const size_t bufBytes = bufSize*sizeof(uint64_t);

    // output buffer
    //uint64_t* outBuf = &memBuf[0];

    // input buffers (from chunks)
    vector<fileBuffer<uint64_t>> inBufs;
    inBufs.reserve(numChunks);

    for(uint64_t i=0; i < numChunks; i++) {
        fileBuffer<uint64_t> buf = {
            chunkFiles[i],       /* srcFile   */
            &memBuf[i*bufBytes], /* buffer    */
            bufSize,             /* length    */
            chunkSize            /* remaining */
        };
        inBufs.push_back(buf);

        rewind(chunkFiles[i]);
        if(fread(&memBuf[0]+i*bufBytes, sizeof(uint64_t), 1, chunkFiles[i]) != 1) {
            cerr << "Reading values from tmp chunk file #" << i << " failed, ";
            if(feof(chunkFiles[i]))
                cerr << "file is too short!" << endl;
            else
                cerr << "unknown error!" << endl;
            return;
        }
        queue.push(mergeItem{buf.buffer[0], i});
    }

    // TODO: actually use the out buffer
    while(!queue.empty()) {
        mergeItem top = queue.top();
        queue.pop();
        write(fdOutput, &(top.value), sizeof(uint64_t));

        uint64_t value;
        if(fread(&value, sizeof(uint64_t), 1, chunkFiles[top.srcIndex]) == 1) {
            queue.push(mergeItem{value, top.srcIndex});
        } else {
            cout << "merged all data from chunk #" << top.srcIndex << endl;
            fclose(chunkFiles[top.srcIndex]);
        }
    }
}
