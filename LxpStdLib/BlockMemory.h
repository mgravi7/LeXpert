// BlockMemory.h

#ifndef BLOCK_MEMORY_H
#define BLOCK_MEMORY_H

#include <vector>

namespace LxpStd
{
	// This class is useful when a lot of small memory allocations are needed.
	// There is no overhead for a single memory allocation call (almost none).
	// There is overhead for the whole class but it is very small. There
	// is no need to call any method to free the small memory allocations.
	// That is because, memory allocations are not kept track of.
	//
	// For example if we are allocating 10,000 strings of about 10 letter
	// long (on average) and if there are going to be few deletions, using
	// this class as a memory allocator for the whole word list will be
	// efficient.
	//
	// The returned address is not guaranteed to be aligned on anything other
	// than a char

	class BlockMemory
	{
	public:
		// constants
		static const unsigned int	DEFAULT_BLOCK_SIZE = 4096;

		// Existence
		BlockMemory(unsigned int blockSize = DEFAULT_BLOCK_SIZE);
		~BlockMemory();

		// Methods
		void*	Allocate(unsigned int size);	// Allocates requested size memory
		void	DeallocateAll();				// Deallocates all the allocations

	private:
		// Implementation
		void	AllocateNewBlock(void);

		// Not Implemented (copy constructor and equal operator)
		BlockMemory(const BlockMemory& blockMemory);
		BlockMemory& operator=(const BlockMemory& blockMemory);

		// Data
		const unsigned int	blockSize;
		char*				freePtr;
		unsigned int		availableMemory;
		std::vector<void*>	newedMemoryVector;
	};

}
#endif // !BLOCK_MEMORY_H

