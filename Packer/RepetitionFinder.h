#if !defined REPETITIONFINDER_H
#define REPETITIONFINDER_H
//--------------------------------
// (c) Reliable Software 1999-2003
// -------------------------------
#include "Repetitions.h"
#include <File/File.h>

class ForgetfulHashTable;
class Statistics;

class RepetitionFinder
{
public:
	RepetitionFinder (char const * buf,
					  File::Size size,
					  Repetitions & repetitions,
					  ForgetfulHashTable & hashtable);

	void Find (Statistics & stats);
		    
private:
	enum
	{
		MinRepetitionLength = 3,
		EconomicalDistanceFor3ByteRepetitionLength = 8192
	};

	bool FoundRepetition ();	
	unsigned int Hash ()
	{
		unsigned long hash = static_cast <unsigned char>(_buf[_current]) << 16;
		hash += static_cast <unsigned char>(_buf[_current + 1]) << 8;
		hash += static_cast <unsigned char>(_buf[_current + 2]);
		return hash;
	}

private:
	ForgetfulHashTable &	_hashtable;
	const char * const		_buf;
	const unsigned int		_size;
	unsigned int			_scanEnd;
	unsigned int			_current;
	RepetitionRecord		_curRepetition;
	Repetitions &			_repetitions;
};

#endif
