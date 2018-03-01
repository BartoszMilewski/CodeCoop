//-------------------------------------
// (c) Reliable Software 1999-2003
// ------------------------------------

#include "precompiled.h"
#include "BlockCoder.h"
#include "RepetitionFinder.h"
#include "BitStream.h"
#include "NamedBlock.h"
#include "ForgetfulHashTable.h"
#include "Statistics.h"
#include "CodingTables.h"
#include <Ex/WinEx.h>

BlockCoder::BlockCoder (InNamedBlock const & inBlock)
	: _inBlock (inBlock),
	  _repetitions (inBlock.GetSize ().Low ())
{
	if (inBlock.GetSize ().IsLarge ())
		throw Win::Exception ("File size exceeds 4GB", 0, 0);
}

void BlockCoder::Read (ForgetfulHashTable & hash, Statistics & stats)
{
	std::string const & name = _inBlock.GetName ();
	for (std::string::const_iterator it = name.begin (); it != name.end (); ++it)
	{
		stats.RecordLiteral (*it);
	}
	if (!_inBlock.GetSize ().IsZero ())
	{
		RepetitionFinder finder (_inBlock.GetBuf (), _inBlock.GetSize (), _repetitions, hash);
		finder.Find (stats);
	}
	stats.RecordLenCode (256);
	hash.Clear ();
}

void BlockCoder::Write (OutputBitStream & output, CodingTables const & codingTables)
{	
	std::string const & name = _inBlock.GetName ();
	PackedULong nameLen (name.length ());
	nameLen.Write (output);
	for (std::string::const_iterator iter = name.begin (); iter != name.end (); ++iter)
	{
		BitCode literalCode = codingTables.GetLiteralCode (*iter);
		output.Write (literalCode);
	}

    char const * const  buf = _inBlock.GetBuf ();
	unsigned int current = 0;
    std::vector<RepetitionRecord> const & records = _repetitions.GetRecords ();
	std::vector<RepetitionRecord>::const_iterator it;

	for (it = records.begin (); it != records.end (); ++it)	
	{		
		unsigned int repTarget = it->GetTarget ();

		// While not at repetition target position copy bytes from
		// the input byte stream to the output bit stream
		while (current < repTarget)
		{
			char ch = buf [current];
			BitCode literalCode = codingTables.GetLiteralCode (ch);
			output.Write (literalCode);
			current++;
		}

		unsigned int repLen = it->GetLen ();
		unsigned int backwardDist = it->GetBackwardDist ();
		BitCode lenCode = codingTables.GetLengthCode (repLen);
		output.Write (lenCode);
		BitCode distCode = codingTables.GetDistanceCode (backwardDist);
		output.Write (distCode);
		current += repLen;
	}

	File::Size inputSize = _inBlock.GetSize ();
	// While not at input byte stream end copy bytes
	// to the output bit stream
	if (inputSize.IsLarge ())
		throw Win::Exception ("File size exceeds 4GB", 0, 0);
	unsigned int size = inputSize.Low ();
	while (current < size)
	{
		char ch = buf [current];
		BitCode literalCode = codingTables.GetLiteralCode (ch);
		output.Write (literalCode);
		current++;
	}
	BitCode literalCode = codingTables.GetEndOfBlockCode ();
	output.Write (literalCode);
}
