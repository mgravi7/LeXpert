// Trie.h

#ifndef TRIE_H
#define TRIE_H

#include "BlockMemory.h"
#include <vector>

namespace LxpStd
{
	typedef	struct TrieNodeStruct			TrieNode;
	typedef struct TrieDiagnosticsStruct	TrieDiagnostics;

	// this structure is used in constructing a Trie
	struct TrieNodeStruct
	{
		// these are essential for processing and building the Trie
		TrieNode*	pFirstChild;
		TrieNode*	pNextSibling;
		TrieNode*	pOriginalParent;	// useful for compression
		char		letter;
		bool		isWordTerminal;
		
		// since the above takes 14 bytes, the following is used
		// for optimization when compressing the Trie (makes the struct 16 bytes)
		bool			isCompressed;	// has a pass been made for compression?
										// (applicable to first child only)
		bool			isDuplicate;	// this node is a duplicate and needs to be discarded with descedents
										// (applicable to first child only)
	};

	// this structure is for diagnostics collection for Trie
	struct TrieDiagnosticsStruct
	{
		unsigned int	numNodes;
		unsigned int	numWords;
		unsigned int	numLetters;
	};

	class Trie
	{
	public:
		// Existence
		Trie();
		~Trie();

		// Methods
		void	AddWord(const char* pWord);	// words can be added in any order (see note below)
		bool	Compress(void);				// SHOULD be called after all the words are added 
	
	private:
		enum class TrieState {ADDING_WORDS, COMPRESSING, COMPRESSED};

		// Implementation
		TrieNode*	AddChildNode(TrieNode* pParentNode, char childLetter, bool isWordTerminal);
		TrieNode*	AllocateNewNode(void);
		TrieNode*	AllocateNewNode(TrieNode* pOriginalParent, char letter, bool isWordTerminal);
		bool		AreNodesSimilar(TrieNode* pNode1, TrieNode* pNode2) const;
		void		IdentifyFirstChildren(TrieNode* pParentNode);
		void		RemoveDuplicates(unsigned int firstChildrenNodeIdx);

		// Not Implemented
		Trie(const Trie& trie);
		Trie& operator=(const Trie& trie);

		// Data
		TrieState		state;
		TrieNode*		pRootNode;
		BlockMemory		blockMemory;	// for the Nodes
		TrieDiagnostics	diagnostics;

		std::vector<TrieNode*>	firstChildren;			// vector of all the first children
		unsigned int			compressedNodeIdx;		// where compression has stopped
	};
}

#endif // !TRIE_H


