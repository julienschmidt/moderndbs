#include "BufferFrame.hpp"

void* BufferFrame::getData()
{
	//if not already loaded
	if (data == NULL)
		data = loadData;
	
	return data;
}

void* BufferFrame::loadData()
{
    //Load data from HDD
	
	//the page number, 64 bit
	//  first 16bit: filename
	//  28bit: actual page no
    uint64_t filename = pageNo >> 48;
	uint64_t realPageNo = pageNo & 0x0000FFFFFFFFFFFF;
		
	//load from file filename
	//at position blocksize * realPageNo, realPageNo=0,1,2,...
	//with pread
	
	//return pointer to first memory position
}

void BufferFrame::writeData();
{
	//TODO
}