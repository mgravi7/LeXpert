#include "pch.h"
#include "Trie.h"
#include "LxpStdLib.h"
#include "Dawg.h"

#include <assert.h>

namespace LxpStd
{
	// CONSTRUCTOR
	Trie::Trie() :
		blockMemory(65536)	// getting 64K memory blocks
	{
		this->diagnostics.numNodes = 0;
		this->diagnostics.numWords = 0;
		this->diagnostics.numWordLetters = 0;
		this->diagnostics.numLetters = 0;
		this->diagnostics.numReversePartWords = 0;

		this->diagnostics.numFirstChildrenAfterCompression = 0;
		this->diagnostics.numFirstChildrenBeforeCompression = 0;
		this->diagnostics.numNodesAfterCompression = 0;

		// get the special nodes initialized
		this->pRootNode = AllocateNewNode();
		this->pForwardWordNode = AllocateNewNode(this->pRootNode, Dawg::FORWARD_WORD_DAWG_SYMBOL, false);
		this->pReversePartWordNode = AllocateNewNode(this->pRootNode, Dawg::REVERSE_PARTWORD_DAWG_SYMBOL, false);

		this->pRootNode->pFirstChild = this->pForwardWordNode;
		this->pForwardWordNode->pNextSibling = this->pReversePartWordNode;

		// state and rest
		this->state = TrieState::ADDING_WORDS;
		this->firstChildrenCompressNodeIdx = 0;
	}

	// DESTRUCTOR
	Trie::~Trie()
	{
		this->blockMemory.DeallocateAll();
	}

	// ADD CHILD NODE
	TrieNode* Trie::AddChildNode(
		TrieNode* pParentNode,
		char childLetter,
		bool isWordTerminal)
	{
		// validation
		assert(pParentNode != NULL);
		assert(Trie::IsValidLetter(childLetter));

		// increment letter count
		this->diagnostics.numLetters++;

		// does the parent have a child?
		if (pParentNode->pFirstChild == NULL)
		{
			pParentNode->pFirstChild = AllocateNewNode(pParentNode, childLetter, isWordTerminal);
			return pParentNode->pFirstChild;
		}

		// child already present, need to insert in the right place
		TrieNode* pCurNode = pParentNode->pFirstChild;
		TrieNode* pPrevSibling = NULL;
		TrieNode* pNewNode = NULL;
		while (pCurNode != NULL)
		{
			// if the letter is already there
			if (pCurNode->letter == childLetter)
			{
				if (isWordTerminal)	// true always trumps what is already there
					pCurNode->isWordTerminal = isWordTerminal;
				return pCurNode;
			}

			// is the letter to be inserted before curNode?
			if (childLetter < pCurNode->letter)
			{
				// create a new node and set its sibling
				pNewNode = AllocateNewNode(pParentNode, childLetter, isWordTerminal);
				pNewNode->pNextSibling = pCurNode;

				// need to link the newNode to its previous sibling or
				// to parent (in the case of this being the first child)
				if (pPrevSibling == NULL)
					pParentNode->pFirstChild = pNewNode;
				else
					pPrevSibling->pNextSibling = pNewNode;

				return pNewNode;
			}

			// advance to next sibling
			pPrevSibling = pCurNode;
			pCurNode = pCurNode->pNextSibling;
		}

		// node to be created is the last child, link it to previous sibling
		assert(pCurNode == NULL);
		pNewNode = AllocateNewNode(pParentNode, childLetter, isWordTerminal);
		pPrevSibling->pNextSibling = pNewNode;
		return pNewNode;
	}

	// ADD WORD
	void Trie::AddWord(const char* pWord) throw(...)
	{
		// validation
		assert(pWord != NULL);
		
		// validate state for addition
		if (this->state != TrieState::ADDING_WORDS)
			throw(std::exception("Trie must be in ADDING_WORDS state!"));
		
		// initializations
		const char* pNextChar = pWord;
		char curChar = *pNextChar++;
		TrieNode* pCurNode = this->pForwardWordNode;
		bool isWordTerminal = false;
		unsigned int wordLength = 0;
		
		// add child node
		while (curChar != '\0')
		{
			wordLength++;

			// are we at the last letter of the word?
			if (*pNextChar == '\0')
				isWordTerminal = true;

			pCurNode = AddChildNode(pCurNode, curChar, isWordTerminal);
			curChar = *pNextChar++;
		}

		// diagnostics
		if (wordLength > 0)
		{
			this->diagnostics.numWords++;
			this->diagnostics.numWordLetters += wordLength;
		}

		// add the reversed part word
		AddReversedPartWords(pWord, wordLength);
	}

	// ADD REVERSED PART WORDS
	// For example, if the word is CATS, and the wordLength is 4, the following
	// part words are added to the reverse part word part of the DAWG:
	//
	// STAC
	// TAC
	// AC
	// C
	void Trie::AddReversedPartWords(const char* pWord, unsigned int wordLength) throw(...)
	{
		// validation
		assert(pWord != NULL);
		assert(this->state == TrieState::ADDING_WORDS);

		// recursion stopper
		if (wordLength == 0)
			return;

		// add letters in reverse starting at wordLength - 1
		TrieNode* pCurNode = this->pReversePartWordNode;
		bool isWordTerminal = false;
		for (int idx = wordLength - 1; idx >= 0; idx--)
		{
			// final letter?
			if (idx == 0)
				isWordTerminal = true;
			pCurNode = AddChildNode(pCurNode, pWord[idx], isWordTerminal);
		}

		// increment reverse part word count
		this->diagnostics.numReversePartWords++;

		// need to add reverse part word starting at wordLength - 1
		AddReversedPartWords(pWord, wordLength - 1);
	}

	// ALLOCATE NEW NODE
	TrieNode* Trie::AllocateNewNode(void)
	{
		// get new memory and initialize values
		TrieNode* pNewNode = (TrieNode*) this->blockMemory.Allocate(sizeof(TrieNode));
		this->diagnostics.numNodes++;

		pNewNode->pFirstChild = NULL;
		pNewNode->pNextSibling = NULL;
		pNewNode->pOriginalParent = NULL;
		pNewNode->letter = Dawg::DEFAULT_LETTER;
		pNewNode->isWordTerminal = false;

		pNewNode->isCounted = false;
		pNewNode->isDuplicate = false;

		return pNewNode;
	}

	// ALLOCATE NEW NODE
	TrieNode* Trie::AllocateNewNode(
		TrieNode*	pOriginalParent,
		char		letter,
		bool		isWordTerminal)
	{
		assert(Trie::IsValidLetter(letter));

		TrieNode* pNewNode = AllocateNewNode();
		pNewNode->pOriginalParent = pOriginalParent;
		pNewNode->letter = letter;
		pNewNode->isWordTerminal = isWordTerminal;

		return pNewNode;
	}

	// ARE NODES SIMILAR
	bool Trie::AreNodesSimilar(TrieNode* pNode1, TrieNode* pNode2) const
	{
		// are these the same nodes or are both of them NULL?
		if (pNode1 == pNode2)
			return true;

		// one of them NULL?
		if (pNode1 == NULL || pNode2 == NULL)
			return false;

		// different letters?
		if (pNode1->letter != pNode2->letter)
			return false;

		// different word terminal setting?
		if (pNode1->isWordTerminal != pNode2->isWordTerminal)
			return false;

		// is the next sibling similar
		if (!AreNodesSimilar(pNode1->pNextSibling, pNode2->pNextSibling))
			return false;

		// is the first child similar
		return AreNodesSimilar(pNode1->pFirstChild, pNode2->pFirstChild);
	}

	// COMPRESS
	bool Trie::Compress(void) throw(...)
	{
		// validate state for addition
		if (this->state == TrieState::COMPRESSED)
			throw(std::exception("Trie is already COMPRESSED!"));

		// adding words state
		if (this->state == TrieState::ADDING_WORDS)
		{
			this->state = TrieState::COMPRESSING;

			// need to identify and collect first children
			IdentifyFirstChildren(this->pRootNode);
			this->firstChildrenCompressNodeIdx = 0;
			this->diagnostics.numFirstChildrenBeforeCompression = this->firstChildren.size();
		}

		// compression in progress
		if (this->state == TrieState::COMPRESSING)
		{
			// are we done?
			if (this->firstChildrenCompressNodeIdx >= this->firstChildren.size() - 1)
			{
				this->state = TrieState::COMPRESSED;
				UpdateAfterCompressionDiagnostics();
				return true;	// done compressing
			}
			else
			{
				// need to remove duplicates for the current node
				RemoveDuplicates(this->firstChildrenCompressNodeIdx);
				this->firstChildrenCompressNodeIdx++;

				return false;	// more work is left
			}
		}

		assert(this->state == TrieState::COMPRESSED);
		return true;	// nothing to do, state must be COMPRESSED
	}

	// GET DIAGNOSTICS
	void Trie::GetDiagnostics(TrieDiagnostics& diagnostics) const
	{
		diagnostics = this->diagnostics;
	}

	// GET NODE COUNT FOR TREE
	unsigned int Trie::GetNodeCountForTree(TrieNode* pNode) 
	{
		unsigned int count = 0;

		// recursion stop condition
		if (pNode == NULL)
			return count;
		
		// count the node (if not counted), sibling and first child
		if (!pNode->isCounted)
		{
			count = 1;
			pNode->isCounted = true;
		}
		
		count += GetNodeCountForTree(pNode->pNextSibling);
		count += GetNodeCountForTree(pNode->pFirstChild);

		return count;
	}

	// IDENTIFY FIRST CHILDREN
	void Trie::IdentifyFirstChildren(TrieNode* pParentNode)
	{
		// nice recursive call to traverse the first child
		// and next sibling

		// is the parent null?
		if (pParentNode == NULL)
			return;

		// add the first child if there is one
		// and then its child and siblings
		if (pParentNode->pFirstChild != NULL)
		{
			this->firstChildren.push_back(pParentNode->pFirstChild);
			IdentifyFirstChildren(pParentNode->pFirstChild);
		}
			
		// need to do identify the next sibling's first child
		if (pParentNode->pNextSibling != NULL)
			IdentifyFirstChildren(pParentNode->pNextSibling);
	}

	// IS VALID LETTER
	bool Trie::IsValidLetter(char letter)
	{
		// allowed letter range?
		if (letter >= Dawg::START_LETTER && letter <= Dawg::END_LETTER)
			return true;

		// other special letters
		if (letter == Dawg::FORWARD_WORD_DAWG_SYMBOL)
			return true;

		if (letter == Dawg::REVERSE_PARTWORD_DAWG_SYMBOL)
			return true;

		// no match
		return false;
	}

	// LENGTH
	unsigned int Trie::Length()
	{
		// clear the counted state
		SetIsCountedStateForTree(this->pRootNode, false);
		return GetNodeCountForTree(this->pRootNode);
	}

	// REMOVE DUPLICATES
	void Trie::RemoveDuplicates(unsigned int node1Index)
	{
		// removing duplicates for only one node (node1Index)
		int numFirstChildren = this->firstChildren.size();
		
		TrieNode* pNode1 = this->firstChildren[node1Index];

		for (int node2Index = node1Index + 1; node2Index < numFirstChildren; node2Index++)
		{
			TrieNode* pNode2 = this->firstChildren[node2Index];
			if (!pNode2->isDuplicate && AreNodesSimilar(pNode1, pNode2))
			{
				// mark node2 as duplicate and relink the parent's first child
				pNode2->isDuplicate = true;
				pNode2->pOriginalParent->pFirstChild = pNode1;
			}
		}
	}

	// SET IS COUNTED STATE FOR TREE
	void Trie::SetIsCountedStateForTree(TrieNode* pNode, bool isCounted)
	{
		// recursion stop condition
		if (pNode == NULL)
			return;

		pNode->isCounted = isCounted;
		SetIsCountedStateForTree(pNode->pNextSibling, isCounted);
		SetIsCountedStateForTree(pNode->pFirstChild, isCounted);
	}

	// UPDATE COMPRESSION DIAGNOSTICS
	void Trie::UpdateAfterCompressionDiagnostics()
	{
		// first children nodes
		int numDuplicates = 0;
		for (int idx = 0; idx < this->firstChildren.size(); idx++)
		{
			if (this->firstChildren[idx]->isDuplicate)
				numDuplicates++;
		}
		this->diagnostics.numFirstChildrenAfterCompression = this->firstChildren.size() - numDuplicates;

		// number of nodes
		this->diagnostics.numNodesAfterCompression = Length();
	}
}

