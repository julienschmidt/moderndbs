# moderndbs

## [Assignment 01: External Sorting](https://github.com/julienschmidt/moderndbs/releases/tag/assignment01)

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


## [Assignment 02: Buffer Manager](https://github.com/julienschmidt/moderndbs/releases/tag/assignment02)

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

The Buffer Manager uses a LRU frame replacement strategy.

Since asynchronous write back does not need to be implemented at this point, pages are only written back when the BufferManager or the respective BufferFrames are destructed. Moreover `flush()` can be called manually on BufferFrames to write back the data.

## [Assignment 03: Segments / Slotted Pages](https://github.com/julienschmidt/moderndbs/releases/tag/assignment03)

### Schema Segments
Run with:
```bash
$ make schema
$ ./bin/schema ./test/sql/schema.sql
```
### Slotted Pages
Run with:
```bash
$ make slotted
$ ./bin/slotted
```

## [Assignment 04: BTree](https://github.com/julienschmidt/moderndbs/releases/tag/assignment04)

Run with:
```bash
$ make btree
$ ./bin/btree [n=1000000]
```

For example:
```bash
$ make btree
$ ./bin/btree 10000
```

## [Assignment 05: Operators](https://github.com/julienschmidt/moderndbs/releases/tag/assignment05)

Run with:
```bash
$ make operators
$ ./bin/operators
```

