#include "pch.h"
#include "BlockMemory.h"

#include <exception>

namespace LxpStd
{
	// BLOCKMEMORY
	BlockMemory::BlockMemory(unsigned int blockSize) :
		blockSize(blockSize)
	{
		this->freePtr = NULL;
		this->availableMemory = 0;
	}

	// ~BLOCKMEMORY
	BlockMemory::~BlockMemory()
	{
		DeallocateAll();
	}

	// ALLOCATE
	void* BlockMemory::Allocate(unsigned int size) throw(...)
	{
		// allocation size request must not be greater than block size
		if (size > this->blockSize)
			throw std::exception("Requested allocation size is greater than block size");

		// if there is not enough memory allocate new block
		if (size > this->availableMemory)
			AllocateNewBlock();

		void* newMemory = this->freePtr;
		this->freePtr += size;
		this->availableMemory -= size;

		return newMemory;
	}

	// ALLOCATE NEW BLOCK
	void BlockMemory::AllocateNewBlock(void)
	{
		this->freePtr = new char[this->blockSize];
		this->availableMemory = this->blockSize;
		this->newedMemoryVector.push_back(this->freePtr);
	}

	// DEALLOCATE ALL
	void BlockMemory::DeallocateAll(void)
	{
		// free all the blocks
		for (std::vector<void*>::iterator itr = this->newedMemoryVector.begin();
			itr != this->newedMemoryVector.end(); ++itr)
		{
			delete[](char *) *itr;
		}
		
		this->newedMemoryVector.clear();
		this->freePtr = NULL;
		this->availableMemory = 0;
	}
}