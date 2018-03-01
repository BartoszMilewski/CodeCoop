#if !defined COMPRESSOR_H
#define COMPRESSOR_H
//-------------------------------------
// (c) Reliable Software 1999, 2000, 01
// ------------------------------------

#include "BlockCoder.h"
#include "BitStream.h"

#include <auto_vector.h>

class InNamedBlock;
class Statistics;
class CodingTables;

class Compressor
{
public:
	Compressor (std::vector<InNamedBlock> const & sources)
		: _sources (sources) 
	{}

	void Compress ();
	typedef ByteDequeIter Iterator;
	Iterator begin () const { return _out.begin (); }
	Iterator end () const { return _out.end (); }
	unsigned int GetPackedSize () const { return _out.size (); }

private:
	void Read (Statistics & stats);
	void Write (CodingTables const & codingTables);

private:
	std::vector<InNamedBlock> const &	_sources;
    auto_vector<BlockCoder>				_blockCoders;
	OutputBitStream						_out;
};

#endif
