#include "BufferManager.hpp"


BufferManager::BufferManager(uint64_t size)
  : size(size)
{

}



BufferFrame& BufferManager::fixPage(uint64_t pageId, bool exclusive)
{
	//return the BufferFrame pageId
	//set the locks at the buffer frame
	
	//lock hashtable frames
	
	//check if pageId is already in buffer
	//if not, create it
	//if no free frame available, free one
	
	BufferFrame bf = frames[pageId];
	
	//set read/write lock
	//  exclusive == true --> write lock
	//  exclusive == false --> read lock
	
	//unlock hashtable
}


void BufferManager::unfixPage(BufferFrame& frame, bool isDirty)
{
	
}

