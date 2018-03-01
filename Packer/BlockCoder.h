#if !defined BLOCKCODER_H
#define BLOCKCODER_H

//-------------------------------------
// (c) Reliable Software 1999, 2000, 01
// ------------------------------------

#include "Repetitions.h"

class ForgetfulHashTable;
class OutputBitStream;
class InNamedBlock;
class Statistics;
class CodingTables;

class BlockCoder
{
public:
	BlockCoder (InNamedBlock const & inBlock);
	void Read (ForgetfulHashTable & hash, Statistics & stats);
	void Write (OutputBitStream & output, CodingTables const & codingTables);

private:
    Repetitions				_repetitions;	
	InNamedBlock const &	_inBlock;
};

#endif
