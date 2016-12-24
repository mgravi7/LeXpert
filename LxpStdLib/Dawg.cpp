#include "pch.h"
#include "Dawg.h"
#include "LxpStdLib.h"

#include <assert.h>
#include <cstring>
#include <ctime>
#include <string>
#include <fstream>

using namespace std;

namespace LxpStd
{
	// CONSTRUCTOR
	DawgCreator::DawgCreator(const string& lexiconName, unsigned int numNodes, unsigned int numWords)
	{
		CreateHeader(lexiconName, numNodes, numWords);
		this->pNodes = new DawgNode[numNodes];
		this->numAddedNodes = 0;
	}

	// DESTRUCTOR
	DawgCreator::~DawgCreator()
	{
		delete[] this->pNodes;
	}

	// ADD NODE
	void DawgCreator::AddNode(DawgNode& dawgNode)
	{
		assert(this->numAddedNodes < this->header.numNodes);
		assert(this->pNodes != NULL);

		this->pNodes[this->numAddedNodes++] = dawgNode;
	}

	// CREATE HEADER
	void DawgCreator::CreateHeader(const string& lexiconName, unsigned int numNodes, unsigned int numWords)
	{
		// fill the date string
		time_t nowTime;
		struct tm* pLocaltime;
		time(&nowTime);
		pLocaltime = localtime(&nowTime);
		strftime(this->header.date, Dawg::HEADER_DATE_LENGTH, "%d %B %Y", pLocaltime);

		// lexicon name
		int lexiconNameLength = lexiconName.length();
		if (lexiconNameLength > Dawg::HEADER_LEXICON_NAME_LENGTH)
		{
			lexiconName._Copy_s(this->header.lexiconName, Dawg::HEADER_LEXICON_NAME_LENGTH,
				Dawg::HEADER_LEXICON_NAME_LENGTH, 0);
		}
		else
		{
			lexiconName._Copy_s(this->header.lexiconName, Dawg::HEADER_LEXICON_NAME_LENGTH,
				lexiconNameLength, 0);
			this->header.lexiconName[lexiconNameLength] = '\0';
		}

		// other attributes!
		this->header.numNodes = numNodes;
		this->header.numWords = numWords;
		this->header.size = sizeof DawgHeader;
	}

	// SAVE DAWG
	void DawgCreator::SaveDawg(const string& fileName) throw(...)
	{
		// validation
		assert(this->pNodes != NULL);
		if (this->numAddedNodes != this->header.numNodes)
			throw(std::exception("Requested number of nodes not added!"));

		// create file in binary mode
		ofstream dawgStream;
		dawgStream.open(fileName, ofstream::out | ofstream::binary);

		// write header
		dawgStream.write((const char*)(&(this->header)), sizeof(this->header));

		// write nodes
		dawgStream.write((const char*)(this->pNodes), sizeof(DawgNode) * this->header.numNodes);

		// close the stream
		dawgStream.close();
	}
};

namespace LxpStd
{
	// CONSTRUCTOR
	Dawg::Dawg()
	{
		this->pNodes = NULL;
		this->numReversePartWords = 0;
	}

	// DESTRUCTOR
	Dawg::~Dawg()
	{
		Cleanup();
	}

	// CLEAN UP
	void Dawg::Cleanup()
	{
		if (this->pNodes != NULL)
		{
			delete[] this->pNodes;
			this->pNodes = NULL;
		}
		this->numReversePartWords = 0;

		// header
		memset(this->header.date, '\0', Dawg::HEADER_DATE_LENGTH);
		memset(this->header.lexiconName, '\0', Dawg::HEADER_LEXICON_NAME_LENGTH);
		this->header.numNodes = 0;
		this->header.numWords = 0;
		this->header.size = 0;
	}

	// COUNT NUM REVERSE PART WORDS
	unsigned int Dawg::CountNumReversePartWords() const
	{
		return CountNumWordFragmentsForTree(Dawg::REVERSE_PARTWORD_NODE_ID);
	}

	// COUNT NUM WORDS
	unsigned int Dawg::CountNumWords() const
	{
		return CountNumWordFragmentsForTree(Dawg::FORWARD_WORD_NODE_ID);
	}

	// COUNT NUM TERMINALS FOR TREE
	unsigned int Dawg::CountNumWordFragmentsForTree(unsigned int nodeId) const
	{
		unsigned int numWordFragments = 0;

		// recursion stop condition
		if (nodeId == 0 || nodeId >= this->header.numNodes)
			return numWordFragments;

		// is the current node terminal?
		if (this->pNodes[nodeId].isTerminal == TRUE)
			numWordFragments++;

		// traverse the first child tree
		numWordFragments += CountNumWordFragmentsForTree(this->pNodes[nodeId].childNodeId);

		// traverse the next sibling tree (if this is not the last child)
		if (this->pNodes[nodeId].isLastChild != TRUE)
			numWordFragments += CountNumWordFragmentsForTree(nodeId + 1);

		return numWordFragments;
	}

	// GET HEADER
	void Dawg::GetHeader(DawgHeader& header) const
	{
		header = this->header;
	}

	// INITIALIZE
	void Dawg::Initialize(const string& fileName) throw(...)
	{
		// clean up first
		Cleanup();

		// open the file and read header
		ifstream dawgStream;
		dawgStream.open(fileName, ifstream::in | ifstream::binary);

		// find the length of file
		dawgStream.seekg(0, dawgStream.end);
		unsigned int fileLength = dawgStream.tellg();
		dawgStream.seekg(0, dawgStream.beg);
		if (fileLength < sizeof DawgHeader)
			throw(std::exception("File length is smaller than Header information! Bug or file corruption?"));

		// read header and allocate memory for reading nodes
		dawgStream.read((char*)(&(this->header)), sizeof(this->header));
		unsigned int expectedFileLength = sizeof DawgHeader + (sizeof DawgNode * this->header.numNodes);
		if (fileLength < expectedFileLength)
			throw(std::exception("File length is smaller than expected! Bug or file corruption?"));

		this->pNodes = new DawgNode[this->header.numNodes];

		// read nodes and close the stream
		dawgStream.read((char*)(this->pNodes), sizeof(DawgNode) * this->header.numNodes);
		dawgStream.close();

		// count words and reverse part words
		unsigned int numWords = CountNumWords();
		this->numReversePartWords = CountNumReversePartWords();

		// validate that the number of words match
		assert(numWords == this->header.numWords);
		if (numWords != this->header.numWords)
			throw(std::exception("Number of words in Dawg does not match what is in the header! Bug or file corruption?"));

		// validate the number of nodes match the minimum
		if (this->header.numNodes < Dawg::MINIMUM_NUMBER_OF_NODES)
			throw(std::exception("Number of nodes in Dawg does not match the minimum! Bug or file corruption?"));
	}

	// IS REVERSE PART WORD
	bool Dawg::IsReversePartWord(const string& reversePartWord) const
	{
		assert(this->pNodes != NULL);
		return IsWordFragment(reversePartWord, this->pNodes[Dawg::REVERSE_PARTWORD_NODE_ID].childNodeId, 0);
	}

	// IS WORD
	bool Dawg::IsWord(const string& word) const
	{
		assert(this->pNodes != NULL);
		return IsWordFragment(word, this->pNodes[Dawg::FORWARD_WORD_NODE_ID].childNodeId, 0);
	}

	// IS WORD FRAGMENT
	bool Dawg::IsWordFragment(const string& wordFragment, unsigned int nodeId, unsigned int matchedLength) const
	{
		// length check for edge cases (length = 0)
		if (wordFragment.length() == 0)
			return false;	// we can't match empty string

		// safety check!
		assert(matchedLength < wordFragment.length());

		// is the letter available in the current node or its sibling
		char letterToMatch = wordFragment[matchedLength];
		do
		{
			if (this->pNodes[nodeId].letter == letterToMatch)
			{
				matchedLength++;

				// is this the last letter in the wordFragment?
				if (wordFragment.length() == matchedLength)
				{
					if (this->pNodes[nodeId].isTerminal == TRUE)
						return true;
					else
						return false;
				}

				// not the last letter, need to match child letters
				return IsWordFragment(wordFragment, this->pNodes[nodeId].childNodeId, matchedLength);
			}
		} while (this->pNodes[nodeId].isLastChild != TRUE);

		// letter not found
		return false;
	}

	// NUM REVERSE PART WORDS
	unsigned int Dawg::NumReversePartWords() const
	{
		return this->numReversePartWords;
	}
};