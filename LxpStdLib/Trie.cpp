#include "pch.h"
#include "Trie.h"
#include "LxpStdLib.h"
#include "Dawg.h"

#include <assert.h>
#include <string>

using namespace std;

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

	// ADD TREE TO DAWG
	// *** To be called on first child only ***
	int Trie::AddTreeToDawg(TrieNode* pNode, DawgCreator& dawgCreator, int lastSavedNodeNumber) const
	{
		// recursion stop conditions
		if (pNode == NULL)
			return lastSavedNodeNumber;

		if (pNode->nodeNumber == Trie::DEFAULT_NODE_NUMBER)
			return lastSavedNodeNumber;

		// node numbers are saved sequentially. Due to
		// cyclical nature of DAWG, previous added nodes may be
		// asked to be added again. Guard against that!
		if (pNode->nodeNumber <= lastSavedNodeNumber)
			return lastSavedNodeNumber;
		
		// save all the siblings first
		TrieNode* pSaveNode = pNode;
		DawgNode dawgNode;
		while (pSaveNode != NULL)
		{
			assert(pSaveNode->nodeNumber == lastSavedNodeNumber + 1); // verifies sequencing
			Trie::TrieNodeToDawgNode(pSaveNode, dawgNode);
			dawgCreator.AddNode(dawgNode);

			lastSavedNodeNumber = pSaveNode->nodeNumber;
			pSaveNode = pSaveNode->pNextSibling;
		}

		// save all the first children and their tree of the current tree
		pSaveNode = pNode;
		while (pSaveNode != NULL)
		{
			lastSavedNodeNumber = AddTreeToDawg(pSaveNode->pFirstChild, dawgCreator, lastSavedNodeNumber);
			pSaveNode = pSaveNode->pNextSibling;
		}

		return lastSavedNodeNumber;
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

	// ASSIGN NODE NUMBER FOR TREE
	// *** To be called for first children only ***
	int Trie::AssignNodeNumberForTree(TrieNode* pNode, int nextNodeNumber)
	{
		// recursion stop condition
		if (pNode == NULL)
			return nextNodeNumber;

		// if this is a duplicate don't touch!
		if (pNode->isDuplicate)
			return nextNodeNumber;

		// if the node is numbered return
		if (pNode->nodeNumber != Trie::DEFAULT_NODE_NUMBER)
			return nextNodeNumber;

		// first child and siblings need to be contiguous
		pNode->nodeNumber = nextNodeNumber++;
		TrieNode* pSibling = pNode->pNextSibling;
		while (pSibling != NULL)
		{
			pSibling->nodeNumber = nextNodeNumber++;
			pSibling = pSibling->pNextSibling;
		}

		// need to have the first child of the tree numbered too
		nextNodeNumber = AssignNodeNumberForTree(pNode->pFirstChild, nextNodeNumber);
		pSibling = pNode->pNextSibling;
		while (pSibling != NULL)
		{
			nextNodeNumber = AssignNodeNumberForTree(pSibling->pFirstChild, nextNodeNumber);
			pSibling = pSibling->pNextSibling;
		}

		return nextNodeNumber;
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
				// compression finished
				this->state = TrieState::COMPRESSED;

				// get nodes numbered
				SetDefaultNodeNumberForTree(this->pRootNode);
				int startNodeNumber = 0;
				unsigned int numNodes = AssignNodeNumberForTree(this->pRootNode, startNodeNumber);

				// diagnostics
				UpdateAfterCompressionDiagnostics();
				assert(numNodes == this->diagnostics.numNodesAfterCompression);		// different methods of computing num nodes

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

	// SAVE AS DAWG
	void Trie::SaveAsDawg(string fileName, string lexiconName) const
	{
		DawgCreator dawgCreator(lexiconName, this->diagnostics.numNodesAfterCompression, this->diagnostics.numWords);
		AddTreeToDawg(this->pRootNode, dawgCreator, -1);
		dawgCreator.SaveDawg(fileName);
	}

	// SET DEFAULT NODE NUMBER FOR TREE
	void Trie::SetDefaultNodeNumberForTree(TrieNode* pNode)
	{
		// recursion stop condition
		if (pNode == NULL)
			return;

		pNode->nodeNumber = Trie::DEFAULT_NODE_NUMBER;
		SetDefaultNodeNumberForTree(pNode->pNextSibling);
		SetDefaultNodeNumberForTree(pNode->pFirstChild);
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

	// TRIE NODE TO DAWG NODE
	void Trie::TrieNodeToDawgNode(const TrieNode* pTrieNode, DawgNode& dawgNode) throw(...)
	{
		// validation
		if (pTrieNode == NULL)
			throw(std::exception("pTrieNode is NULL!"));;

		// one field at a time!
		dawgNode.letter = pTrieNode->letter;

		if (pTrieNode->pFirstChild != NULL)
			dawgNode.childNodeId = pTrieNode->pFirstChild->nodeNumber;
		else
			dawgNode.childNodeId = 0;

		// needed as the two types are different (bool vs unsigned int with 1 bit length)
		if (pTrieNode->isWordTerminal)
			dawgNode.isTerminal = TRUE;
		else
			dawgNode.isTerminal = FALSE;

		if (pTrieNode->pNextSibling == NULL)
			dawgNode.isLastChild = TRUE;
		else
			dawgNode.isLastChild = FALSE;
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

