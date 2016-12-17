#include "pch.h"
#include "CppUnitTest.h"

#include "BlockMemory.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace LxpStd;

namespace LxpStdLibUnitTest
{
	TEST_CLASS(BlockMemoryUnitTest)
	{
	private:
		const unsigned int DEFAULT_ALLOC_SIZE = 10;
		const int NUM_MULTIPLE_ALLOCS = (BlockMemory::DEFAULT_BLOCK_SIZE / DEFAULT_ALLOC_SIZE) - 1;
		const int NUM_MULTIPLE_ALLOCS_FOR_MULTIPLE_BLOCKS = (BlockMemory::DEFAULT_BLOCK_SIZE * 2) / DEFAULT_ALLOC_SIZE;

	public:
		TEST_METHOD(BlockMemory_AllocateSingle)
		{
			BlockMemory blockMemory;
			Assert::IsNotNull(blockMemory.Allocate(DEFAULT_ALLOC_SIZE), L"Single allocation failed");
		}

		TEST_METHOD(BlockMemory_AllocateMultiple)
		{
			BlockMemory blockMemory;

			for (int idx = 0; idx < NUM_MULTIPLE_ALLOCS; idx++)
			{
				Assert::IsNotNull(blockMemory.Allocate(DEFAULT_ALLOC_SIZE), L"Multiple allocation for single block failed");
			}
		}

		TEST_METHOD(BlockMemory_AllocateMultipleBlocks)
		{
			BlockMemory blockMemory;

			for (int idx = 0; idx < NUM_MULTIPLE_ALLOCS_FOR_MULTIPLE_BLOCKS; idx++)
			{
				Assert::IsNotNull(blockMemory.Allocate(DEFAULT_ALLOC_SIZE), L"Multiple allocation for multiple blocks failed");
			}
		}
	};
}