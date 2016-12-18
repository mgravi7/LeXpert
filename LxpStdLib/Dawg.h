// Dawg.h

#ifndef DAWG_H
#define DAWG_H

namespace LxpStd
{
	class Dawg
	{
	public:
		// common constants
		static const int	MAX_WORD_LENGTH = 32;	// lexicon should reject words greater than this length

		static const char	WILDCARD_CHAR = '?';
		static const char	MULTI_CHAR_MATCH_SYMBOL = '*';
		static const char	NOT_MATCH_SYMBOL = '^';

		static const char	START_LETTER = 'A';		// starting letter for use in lexicon
		static const char	END_LETTER = 'Z';		// final letter for use in lexicon
		static const char	DEFAULT_LETTER = ' ';	// used for initialization

		static const char	FORWARD_WORD_DAWG_SYMBOL = '*';
		static const char	REVERSE_PARTWORD_DAWG_SYMBOL = '<';
	};
}
#endif // !DAWG_H