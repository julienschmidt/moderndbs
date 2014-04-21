
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
    BufferManager(const BufferManager& a);
	
	/**
	 * Destructor. Write all dirty frames to disk and free all resources
	 */
    ~BufferManager();
 
 
    BufferFrame& fixPage(uint64_t pageId, bool exclusive);
 
    void unfixPage(BufferFrame& frame, bool isDirty);
	
	
	
  private:
    int size;
	
	//TODO, initialize with "size" elements max
	//Hashtable with all Frames	
	std::unordered_map<uint64_t, BufferFrame> frames;
	
	
	bool freeFrame();
	
};

#endif  // BUFFERMANAGER_H_