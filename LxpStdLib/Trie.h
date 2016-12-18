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
		
		// since the above takes 14 bytes, the following two bytes
		// are used for special purposes (makes the struct 16 bytes)
		bool			isCounted;		// has a pass been made for counting descendents?
		bool			isDuplicate;	// this node is a duplicate and needs to be discarded with descedents
										// (applicable to first child only)
	};

	// this structure is for diagnostics collection for Trie
	struct TrieDiagnosticsStruct
	{
		unsigned int	numNodes;				// running count
		unsigned int	numWords;				// running count
		unsigned int	numWordLetters;			// running count - only forward word letters
		unsigned int	numLetters;				// running count
		unsigned int	numReversePartWords;	// running count

		unsigned int	numFirstChildrenBeforeCompression;	// available after compression starts
		unsigned int	numFirstChildrenAfterCompression;	// available after compression ends
		unsigned int	numNodesAfterCompression;			// available after compression ends
	};

	// ** Trie can be in three states only ***
	// Initially, it is in ADDING_WORDS state. In this stage, add words by calling AddWord method.
	// When the first Compress() method is called, it is in COMPRESSING state and returns "true"
	// if compression is finished. The caller should keep calling until "true" is returned.
	// At that stage, the state of the Trie becomes COMPRESSED. No more words can be added.
	// Compression is a long running process. Hence, control is returned to the caller for
	// processing other (UI) requests.

	class Trie
	{
	public:
		// Existence
		Trie();
		~Trie();

		// Methods
		void	AddWord(const char* pWord);	// words can be added in any order (see note below)
		bool	Compress(void);				// SHOULD be called after all the words are added

		// Diagnostics
		void	GetDiagnostics(TrieDiagnostics& diagnostics) const;
	
	private:
		enum class TrieState {ADDING_WORDS, COMPRESSING, COMPRESSED};

		// Implementation
		TrieNode*		AddChildNode(TrieNode* pParentNode, char childLetter, bool isWordTerminal);
		void			AddReversedPartWords(const char* pWord, unsigned int wordLength);
		TrieNode*		AllocateNewNode(void);
		TrieNode*		AllocateNewNode(TrieNode* pOriginalParent, char letter, bool isWordTerminal);
		bool			AreNodesSimilar(TrieNode* pNode1, TrieNode* pNode2) const;

		unsigned int	GetNodeCountForTree(TrieNode* pNode);
		void			IdentifyFirstChildren(TrieNode* pParentNode);
		unsigned int	Length();	// returns number of nodes in the Trie
		void			RemoveDuplicates(unsigned int firstChildrenNodeIdx);
		void			SetIsCountedStateForTree(TrieNode* pNode, bool isCounted);
		void			UpdateAfterCompressionDiagnostics();

		// static methods
		static bool		IsValidLetter(char letter);

		// Not Implemented
		Trie(const Trie& trie);
		Trie& operator=(const Trie& trie);

		// Data
		TrieState		state;
		TrieNode*		pRootNode;
		TrieNode*		pForwardWordNode;
		TrieNode*		pReversePartWordNode;	// reverse partials and reverse words
		BlockMemory		blockMemory;			// for the Nodes
		TrieDiagnostics	diagnostics;

		std::vector<TrieNode*>	firstChildren;					// vector of all the first children
		unsigned int			firstChildrenCompressNodeIdx;	// where compression needs to start
	};
}

#endif // !TRIE_H


