
#ifndef BUFFERFRAME_H_
#define BUFFERFRAME_H_

#include <pthread.h>  //for locks/latches

//state clean/dirty/newly created etc.
enum state_t {Clean, Dirty, New};

class BufferFrame
{
  public:
    BufferFrame();
    BufferFrame(const BufferFrame& a);
    ~BufferFrame();
 
    /**
	 * Returns the actual data contained on the page
	 */
    void* getData();
	
	uint64_t getPageNo();
	pthread_rwlock_t getLatch();
	uint64_t getLsm();
	state_t getState();
 
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
	void* data = NULL;
	
	void* loadData();
	void* readData();
};

#endif  // BUFFERFRAME_H_