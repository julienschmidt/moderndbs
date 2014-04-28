# moderndbs

Team: Andreas Geiger, Julien Schmidt

## Assignment 01: External Sorting

Run with:
```bash
$ make sort
$ ./bin/sort <inputFile> <outputFile> <memoryBufferInMiB>
```

For example:
```bash
$ make sort
$ ./bin/sort ./test/data/test_4MiB ./out 1
```


## Assignment 02: Buffer Manager

Run with:
```bash
$ make buffer
$ ./bin/buffer <pagesOnDisk> <pagesInRAM> <threads>
```

For example:
```bash
$ make buffer
$ ./bin/buffer 11 10 9
```

The following adjustments to the buffer test were made:
* ignored compiler warnings (using `#pragma`)
* use frame references instead of copies (copy operator might not exists since copies of BufferFrames don't make any sense)
* added sanity checks for the input args

Since asynchronous write back does not need to be implemented at this point, pages are only written back when the BufferManager or the respective BufferFrames are destructed. Moreover `flush()` can be called manually on BufferFrames to write back the data.
