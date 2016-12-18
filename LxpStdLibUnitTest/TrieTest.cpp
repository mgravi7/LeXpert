#include "pch.h"
#include "CppUnitTest.h"
#include "Trie.h"
#include <cstring>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace LxpStd;

namespace LxpStdLibUnitTest
{
	TEST_CLASS(TrieUnitTest)
	{
	private:
		static const unsigned int NUM_OVERHEAD_NODES = 3;	// implementation assumed (root node, forward word node, reverse part word node)

		static const int numWordsInLexicon = 7;
		const char* lexicon[numWordsInLexicon] = { "BAT", "BATS", "CAR", "CARS", "CAT", "CATS", "FAT" };
		static const unsigned int expectedNumNodesBeforeCompression = 38 + 1;	// +1 for root node
		static const unsigned int expectedNumNodesAfterCompression = 31 + 1;	// +1 for root node
		static const unsigned int expectedNumFirstChildrenBeforeCompression = 22;
		static const unsigned int expectedNumFirstChildrenAfterCompression = 17;

	public:
		// numNodes can't be filled in expectedDiagnostics as it is very data dependent
		void GetExpectedDiagnostics(const char* words[], int numWords, TrieDiagnostics& expectedDiagnostics)
		{
			// initialization
			expectedDiagnostics.numLetters = 0;
			expectedDiagnostics.numReversePartWords = 0;
			expectedDiagnostics.numWordLetters = 0;
			expectedDiagnostics.numWords = numWords;

			// add for each word
			for (int idx = 0; idx < numWords; idx++)
			{
				int wordLength = strlen(words[idx]);
				expectedDiagnostics.numLetters += wordLength + (wordLength * (wordLength + 1)) / 2;
				expectedDiagnostics.numReversePartWords += wordLength;
				expectedDiagnostics.numWordLetters += wordLength;
			}
		}

		TEST_METHOD(Trie_AddOneWord)
		{
			const char* word = "BATH";
			unsigned int wordLength = strlen(word);
			unsigned int numExpectedNodes = NUM_OVERHEAD_NODES +
											wordLength +
											(wordLength * (wordLength + 1))/2;	// summation formula

			Trie trie;
			TrieDiagnostics diagnostics;

			trie.AddWord(word);
			trie.GetDiagnostics(diagnostics);

			// check number of words, number of letters, number of nodes
			Assert::AreEqual(1U, diagnostics.numWords, L"diagnostics.numWords does not match!");
			Assert::AreEqual(wordLength, diagnostics.numWordLetters, L"diagnostics.numWordLetters does not match!");
			Assert::AreEqual(numExpectedNodes, diagnostics.numNodes, L"diagnostics.numNodes does not match!");
		}

		TEST_METHOD(Trie_AddWords)
		{
			Trie trie;
			TrieDiagnostics expected;
			TrieDiagnostics diagnostics;

			// build the Trie
			for (int idx = 0; idx < numWordsInLexicon; idx++)
				trie.AddWord(lexicon[idx]);
			
			GetExpectedDiagnostics(lexicon, numWordsInLexicon, expected);
			expected.numNodes = expectedNumNodesBeforeCompression;	// based on illustration in "DAWG Samples" spreadsheet ("2way Dawg" sheet)

			trie.GetDiagnostics(diagnostics);
			Assert::AreEqual(expected.numWords, diagnostics.numWords, L"diagnostics.numWords does not match!");
			Assert::AreEqual(expected.numWordLetters, diagnostics.numWordLetters, L"diagnostics.numWordLetters does not match!");
			Assert::AreEqual(expected.numNodes, diagnostics.numNodes, L"diagnostics.numNodes does not match!");
			Assert::AreEqual(expected.numReversePartWords, diagnostics.numReversePartWords, L"diagnostics.numReversePartWords does not match!");
			Assert::AreEqual(expected.numLetters, diagnostics.numLetters, L"diagnostics.numLetters does not match!");
		}

		TEST_METHOD(Trie_Compress)
		{
			Trie trie;
			TrieDiagnostics diagnostics;

			// build the Trie
			for (int idx = 0; idx < numWordsInLexicon; idx++)
				trie.AddWord(lexicon[idx]);

			// compress until it is done
			while (trie.Compress() == false)
			{
				// do nothing
			}

			// we will assert values related to compression
			trie.GetDiagnostics(diagnostics);
			Assert::AreEqual(expectedNumFirstChildrenAfterCompression, diagnostics.numFirstChildrenAfterCompression,
							 L"diagnostics.numFirstChildrenAfterCompression does not match!");
			Assert::AreEqual(expectedNumFirstChildrenBeforeCompression, diagnostics.numFirstChildrenBeforeCompression,
							 L"diagnostics.numFirstChildrenBeforeCompression does not match!");
			Assert::AreEqual(expectedNumNodesAfterCompression, diagnostics.numNodesAfterCompression,
							 L"diagnostics.numNodesAfterCompression does not match!");
		}
	};
}