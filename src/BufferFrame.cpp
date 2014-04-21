#include "BufferFrame.hpp"

#define DEBUG 1

BufferFrame::BufferFrame(uint64_t pageNo)
  : pageNo(pageNo), 
    state(state_t::New), 
	data(NULL), 
	lsm(0), 
	latch(NULL), 
	blocksize(8192)	//8KByte
{
	//the page number, 64 bit
	//  first 16bit: filename
	//  48bit: actual page no
    filename = pageNo >> 48;
	//blocksize * realPageNo, realPageNo=0,1,2,...
	position = blocksize * (pageNo & 0x0000FFFFFFFFFFFF);	
}


BufferFrame::~BufferFrame()
{
	//write all changes, if page is dirty
	if (state == state_t::Dirty)
		writeData();

	//Deallocate data, if necessary	
	if (data != NULL)	// == state != state_t::New
		delete data;
}


void* BufferFrame::getData()
{
	//if not already loaded (dont load on Clean, Dirty)
	if (state == state_t::New)
		data = loadData;
	
	return data;
}

void* BufferFrame::loadData()
{
    //Load data from HDD	
	
#ifdef DEBUG
	if (state == state_t::Dirty)
		std::cout << "WARNING: Data loss on load? (state == Dirty)" << std::endl;
	else if (state == state_t::Clean)
		std::cout << "WARNING: Unnecessary data load (state == Dirty)" << std::endl;
#endif	
	
	//load from file filename
	//at position 
	//with pread
	//TODO
	std::cout << "TODO: Load data from file " << filename << " at " << position << "Bytes" << std::endl;
	void* newData = malloc (blocksize);	//TODO
	
	state = state_t::Clean;
	
	//return pointer to first memory position
	return newData;
}

void BufferFrame::writeData()
{
	//TODO
	std::cout << "TODO: Write data to file " << filename << " at " << position << "Bytes" << std::endl;
	
	state = state_t::Clean;
}