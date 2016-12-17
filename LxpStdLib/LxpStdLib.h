// LxpStdLib.h

#ifndef LXPSTDLIB_H
#define LXPSTDLIB_H

namespace LxpStd
{
	// common typedefs
	typedef unsigned char	uchar;
	typedef unsigned int	uint;
	typedef unsigned long	ulong;

	// common constants
	const int	MAX_WORD_LENGTH = 32;	// lexicon should reject words greater than this

	const char	WILDCARD_CHAR = '?';
	const char	MULTI_CHAR_MATCH_SYMBOL = '*';
	const char	NOT_MATCH_SYMBOL = '^';

	const char	START_LETTER = 'A';		// starting letter for use in lexicon
	const char	END_LETTER = 'Z';		// final letter for use in lexicon
	const char	DEFAULT_LETTER = ' ';	// used for initialization

}
#endif // !LXPSTDLIB_H

