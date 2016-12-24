#include "pch.h"
#include "CppUnitTest.h"

#include "Trie.h"
#include "Dawg.h"
#include <string>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace LxpStd;
using namespace std;

namespace LxpStdLibUnitTest
{
    TEST_CLASS(IntegrationTest)
    {
    public:
        TEST_METHOD(Int_TrieSaveDawgRead)
        {
			const int numWordsInLexicon = 7;
			const unsigned int numUniqueReversePartWordsInLexicon = 13;
			const char* lexicon[numWordsInLexicon] = { "BAT", "BATS", "CAR", "CARS", "CAT", "CATS", "FAT" };

			string lexiconName("Integration test lexicon");
			string fileName("IntTestDawg.lxd");

			Trie trie;

			// build the Trie
			for (int idx = 0; idx < numWordsInLexicon; idx++)
				trie.AddWord(lexicon[idx]);

			// compress until it is done and save
			while (trie.Compress() == false)
			{
				// do nothing
			}
			trie.SaveAsDawg(fileName, lexiconName);

			// read the file in Dawg
			Dawg dawg;
			dawg.Initialize(fileName);

			Assert::AreEqual(dawg.NumReversePartWords(), numUniqueReversePartWordsInLexicon, L"dawg.NumReversePartWords do not match!");

			// verify each word is there
			for (int idx = 0; idx < numWordsInLexicon; idx++)
				Assert::IsTrue(dawg.IsWord(string(lexicon[idx])));
        }
    };
}