
#ifndef BUFFERFRAME_H_
#define BUFFERFRAME_H_

#include <pthread.h>  //for locks/latches

//state clean/dirty/newly created etc.
enum state_t {
	Clean, 	//Data loaded and no data changed
	Dirty, 	//Data laoded and changed --> write it back
	New		//No Data loaded
};

class BufferFrame
{
  public:
    BufferFrame(uint64_t pageNo);
    BufferFrame(const BufferFrame& a);
    ~BufferFrame();
 
    /**
	 * Returns the actual data contained on the page
	 */
    void* getData();
	
	uint64_t getPageNo() { return pageNo; }
	pthread_rwlock_t getLatch() { return latch; }
	uint64_t getLsm() { return lsm; }
	state_t getState() { return state; }
	
	void setStateDirty() { state = state_t::Dirty; }
 
  private:
    //the page number, 64 bit
	//  first 16bit: filename
	//  28bit: actual page id
    uint64_t pageNo;
	
	//a read/writer lock to protect the page
	pthread_rwlock_t latch;
	
	//LSN of the last change, for recovery
	//Not needed?
	uint64_t lsm;
	
	//clean/dirty/newly created etc.
	state_t state;
	
	//Pointer to loaded data
	void* data;
	
	//filename and position in bytes
	uint64_t filename;
	uint64_t position;
	
	//blocksize in Byte
	uint64_t blocksize;
	
	void* loadData();
	void* readData();
};

#endif  // BUFFERFRAME_H_