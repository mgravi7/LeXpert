#include "pch.h"
#include "CppUnitTest.h"
#include "Dawg.h"
#include <string>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace LxpStd;
using namespace std;

namespace LxpStdLibUnitTest
{
	TEST_CLASS(DawgCreatorUnitTest)
	{
	private:
		static const int numNodes = 6;
		static const int numWords = 2;
		DawgNode nodes[numNodes] =
		{
			{ 1U, ' ', 0U, 1U },
			{ 2U, '*', 0U, 1U },
			{ 3U, 'B', 0U, 1U },
			{ 4U, 'A', 0U, 1U },
			{ 5U, 'T', 1U, 1U },
			{ 0U, 'S', 1U, 1U }
		};
		
	public:
		TEST_METHOD(DawgCreator_AddNodes)
		{
			string lexiconName("Unit test lexicon");
			string fileName("UnitTestDawg.lxd");

			DawgCreator dawgCreator(lexiconName, numNodes, numWords);
			
			for (int idx = 0; idx < numNodes; idx++)
				dawgCreator.AddNode(nodes[idx]);

			dawgCreator.SaveDawg(fileName);

			// not sure what can be asserted!
			// how about no exception thrown?
		}
	};
}