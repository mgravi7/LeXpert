// Dawg.h

#ifndef DAWG_H
#define DAWG_H

#include <string>

namespace LxpStd
{
	typedef struct DawgNodeStruct	DawgNode;
	typedef struct DawgHeaderStruct	DawgHeader;

	// NOTES FROM OEIGINAL DAWG.HPP (1990s?)
	// Several tests were conducted (making childNodeId a long and the
	// other members char for speed) and that didn't alter the performance.
	// Another test was conducted (similar to above except childNodeId was
	// made a pointer instead of int) and the performance improved by
	// 2 or 3% which is not signifcant.
	struct DawgNodeStruct
	{
		unsigned int	childNodeId : 22;
				 int	letter : 8;
		unsigned int	isTerminal : 1;
		unsigned int	isLastChild : 1;
	};

	// Used for writing the header information in Dawg files
	// (original layout from 1990s)
	struct DawgHeaderStruct
	{
		unsigned int	size;
		unsigned int	numNodes;
		char			lexiconName[32];
		char			date[20];
		unsigned int	numWords;
	} ;

	// The following class is used for constructing the DAWG.
	// Trie is the class that performs all the addition of words
	// and compression. Typically, it will use the following class
	// to create a DAWG and save it
	class DawgCreator
	{
	public:
		// Existence
		DawgCreator(const std::string& lexiconName, unsigned int numNodes, unsigned int numWords);
											// lexiconName will be truncated to first 32 characters
		~DawgCreator();

		// Methods
		void AddNode(DawgNode& dawgNode);				// sequential addition is implied
		void SaveDawg(const std::string& fileName) throw(...);	// all nodes must be added before this call

	private:
		// Implementation
		void	CreateHeader(const std::string& lexiconName, unsigned int numNodes, unsigned int numWords);

		// Data
		DawgNode*		pNodes;
		DawgHeader		header;
		unsigned int	numAddedNodes;
	};

	class Dawg
	{
	public:
		// common constants
		static const int	MAX_WORD_LENGTH = 32;	// lexicon should reject words greater than this length
		static const int	HEADER_LEXICON_NAME_LENGTH = 32;
		static const int	HEADER_DATE_LENGTH = 20;

		static const char	WILDCARD_CHAR = '?';
		static const char	MULTI_CHAR_MATCH_SYMBOL = '*';
		static const char	NOT_MATCH_SYMBOL = '^';

		static const char	START_LETTER = 'A';		// starting letter for use in lexicon
		static const char	END_LETTER = 'Z';		// final letter for use in lexicon
		static const char	DEFAULT_LETTER = ' ';	// used for initialization

		static const char	FORWARD_WORD_DAWG_SYMBOL = '*';
		static const char	REVERSE_PARTWORD_DAWG_SYMBOL = '<';

		// Existence
		Dawg();
		~Dawg();
		void	Initialize(const std::string& fileName) throw(...);	// initializes from a saved Dawg file

		// Access
		void			GetHeader(DawgHeader& header) const;
		unsigned int	NumReversePartWords() const;

		// Matching
		bool	IsWord(const std::string& word) const;
		bool	IsReversePartWord(const std::string& reversePartWord) const;

	private:
		// common constants
		static const unsigned int	ROOT_NODE_ID = 0;
		static const unsigned int	FORWARD_WORD_NODE_ID = 1;
		static const unsigned int	REVERSE_PARTWORD_NODE_ID = 2;
		static const unsigned int	MINIMUM_NUMBER_OF_NODES = 3;	// Root, forward and reverse

		// Implementation
		void			Cleanup();	// cleans up existing stuff!
		unsigned int	CountNumReversePartWords() const;
		unsigned int	CountNumWords() const;
		unsigned int	CountNumWordFragmentsForTree(unsigned int nodeId) const;	// includes word and part words
		bool			IsWordFragment(const std::string& wordFragment, unsigned int nodeId, unsigned int matchedLength) const;

		// Data
		DawgNode*		pNodes;
		DawgHeader		header;
		unsigned int	numReversePartWords;
	};
}
#endif // !DAWG_H