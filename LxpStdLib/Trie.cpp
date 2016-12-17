#include "pch.h"
#include "Trie.h"
#include "LxpStdLib.h"

#include <assert.h>

namespace LxpStd
{
	// CONSTRUCTOR
	Trie::Trie() :
		blockMemory(65536)	// getting 64K memory blocks
	{
		this->diagnostics.numNodes = 0;
		this->diagnostics.numWords = 0;
		this->diagnostics.numLetters = 0;

		// get the root node initialized
		this->pRootNode = AllocateNewNode();
		this->state = TrieState::ADDING_WORDS;
		this->compressedNodeIdx = 0;
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
		assert(childLetter >= LxpStd::START_LETTER && childLetter <= LxpStd::END_LETTER);

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
				// create a new node and set it sibling
				pNewNode = AllocateNewNode(pParentNode, childLetter, isWordTerminal);
				pNewNode->pNextSibling = pCurNode;

				// need to link the newNode to its previous sibling or
				// to parent (in the case of this being the first child)
				if (pPrevSibling == NULL)
					pParentNode->pFirstChild = pNewNode;
				else
					pPrevSibling->pNextSibling = pNewNode;

				// is the new node parent's first child now?
				if (pPrevSibling == NULL)
					pParentNode->pFirstChild = pNewNode;

				return pNewNode;
			}

			// advance to next sibling
			pPrevSibling = pCurNode;
			pCurNode = pCurNode->pNextSibling;
		}

		// this is the last child, link it to previous sibling
		pNewNode = AllocateNewNode(pParentNode, childLetter, isWordTerminal);
		pPrevSibling->pNextSibling = pNewNode;
		return pNewNode;
	}

	// ADD WORD
	void Trie::AddWord(const char* pWord)
	{
		// TODO Validate state for addition

		// validation
		assert(pWord != NULL);

		// initializations
		char curChar = *pWord++;
		TrieNode* pCurNode = this->pRootNode;
		bool isWordTerminal = FALSE;
		
		// add child node
		while (curChar != '\0')
		{
			// are we at the last letter of the word?
			if (*pWord == '\0')
				isWordTerminal = TRUE;

			pCurNode = AddChildNode(pCurNode, curChar, isWordTerminal);
			curChar = *pWord++;
		}

		// increment word count if we did add a word
		if (isWordTerminal)
			this->diagnostics.numWords++;
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
		pNewNode->letter = LxpStd::DEFAULT_LETTER;
		pNewNode->isWordTerminal = FALSE;

		pNewNode->isCompressed = FALSE;
		pNewNode->isDuplicate = FALSE;

		return pNewNode;
	}

	// ALLOCATE NEW NODE
	TrieNode* Trie::AllocateNewNode(
		TrieNode*	pOriginalParent,
		char		letter,
		bool		isWordTerminal)
	{
		assert(letter >= LxpStd::START_LETTER && letter <= LxpStd::END_LETTER);

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
			return TRUE;

		// one of them NULL?
		if (pNode1 == NULL || pNode2 == NULL)
			return FALSE;

		// different letters?
		if (pNode1->letter != pNode2->letter)
			return FALSE;

		// different word terminal setting?
		if (pNode1->isWordTerminal != pNode2->isWordTerminal)
			return FALSE;

		// is the next sibling similar
		if (!AreNodesSimilar(pNode1->pNextSibling, pNode2->pNextSibling))
			return FALSE;

		// is the first child similar
		return AreNodesSimilar(pNode1->pFirstChild, pNode2->pFirstChild);
	}

	// COMPRESS
	bool Trie::Compress(void)
	{
		// adding words state
		if (this->state == TrieState::ADDING_WORDS)
		{
			this->state = TrieState::COMPRESSING;

			// need to identify and collect first children
			IdentifyFirstChildren(this->pRootNode);
			this->compressedNodeIdx = 0;
		}

		// compression in progress
		if (this->state == TrieState::COMPRESSING)
		{
			// need to remove duplicates for the current node
			RemoveDuplicates(this->compressedNodeIdx);
			this->compressedNodeIdx++;

			// are we done?
			if (this->compressedNodeIdx >= this->firstChildren.size() - 1)
			{
				this->state = TrieState::COMPRESSED;
				return TRUE;	// done compressing
			}
			else
				return FALSE;	// more work is left
		}

		assert(this->state == TrieState::COMPRESSED);
		return TRUE;	// nothing to do, state must be COMPRESSED
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

	// REMOVE DUPLICATES
	void Trie::RemoveDuplicates(unsigned int node1Index)
	{
		// removing duplicates for one one node (node1Index)
		int numFirstChildren = this->firstChildren.size();
		
		TrieNode* pNode1 = this->firstChildren[node1Index];
		if (!pNode1->isCompressed)
		{
			for (int node2Index = node1Index + 1; node2Index < numFirstChildren; node2Index++)
			{
				TrieNode* pNode2 = this->firstChildren[node2Index];
				if (!pNode2->isDuplicate && AreNodesSimilar(pNode1, pNode2))
				{
					// mark node2 as duplicate and relink the parent's first child
					pNode2->isDuplicate = TRUE;
					pNode2->pOriginalParent->pFirstChild = pNode1;
				}
			}
			pNode1->isCompressed = TRUE;
		}
	}
}

