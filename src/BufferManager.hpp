
#ifndef BUFFERMANAGER_H_
#define BUFFERMANAGER_H_

#include "BufferFrame.hpp"



class BufferManager
{
  public:
    /**
     * Create a new instance that keeps up to size frames in main memory
     */
    BufferManager(int size) ;
		
    /**
	 * Copy-Konstruktor
	 */
    BufferManager(const BufferManager& a);   // Copy-Konstruktor
	
	/**
	 * Destructor. Write all dirty frames to disk and free all resources
	 */
    ~BufferManager();
 
 
    BufferFrame& fixPage(uint64_t pageId, bool exclusive);
 
    void unfixPage(BufferFrame& frame, bool isDirty);
	
	
	
  private:
    int size;
	
	//TODO
	//Hashtable mit allen Frames
	
};

#endif  // BUFFERMANAGER_H_