//-------------------------------------
// (c) Reliable Software 1999-2003
// ------------------------------------
#include "precompiled.h"
#include "RepetitionFinder.h"
#include "Repetitions.h"
#include "ForgetfulHashTable.h"
#include "Statistics.h"
#include "Bucketizer.h"
#include <Ex/WinEx.h>

RepetitionFinder::RepetitionFinder (char const * buf,
									File::Size size,
									Repetitions & repetitions,
								    ForgetfulHashTable & hashtable)
	: _buf (buf),
	  _size (size.Low ()),
	  _scanEnd (size.Low () - MinRepetitionLength - 1),
	  _repetitions (repetitions),
	  _hashtable (hashtable),
	  _current (0)      
{
	if (size.IsLarge ())
		throw Win::Exception ("File size exceeds 4GB", 0, 0);
	if (_size <= MinRepetitionLength)
		_scanEnd = _size - 1;
}

void RepetitionFinder::Find (Statistics & stats)
{
	while (_current < _scanEnd)
	{
		if (FoundRepetition ())
		{				
			_repetitions.Add (_curRepetition);
			stats.RecordRepetition (_curRepetition.GetLen (), _curRepetition.GetBackwardDist ());
			_current += _curRepetition.GetLen ();
		}
		else
		{
			stats.RecordLiteral (_buf [_current++]);
		}
	}

	while (_current < _size)
	{
		stats.RecordLiteral (_buf [_current++]);
	}
}

bool RepetitionFinder::FoundRepetition ()
{
	_curRepetition.SetTarget (_current);
	_curRepetition.SetLen (MinRepetitionLength - 1);
	ForgetfulHashTable::PositionListIter iter = _hashtable.Save (Hash (), _current);

	// Go over all found so far repetitions and check if they match the current byte sequence
	for ( ; !iter.AtEnd () && _curRepetition.GetLen () < 10; iter.Advance ())
	{
		unsigned int repStart = iter.GetPosition ();
		Assert (_current > repStart);
		unsigned int backwardDist = _current - repStart;
		if (backwardDist > RepDistBucketizer::GetMaxRepetitionDistance ())
		{
			// Repetition is too far away from the current position in the buffer.
			// Remove repetitions that are too far away from the hash table.
			iter.ShortenList ();
			break;
		}
		// Close enough -- check if long enough		
		if (_buf [_current + _curRepetition.GetLen ()] == _buf [repStart + _curRepetition.GetLen ()])
		{
			unsigned int thisRepLen = 0;
			for ( ; thisRepLen < RepLenBucketizer::GetMaxRepetitionLength (); ++thisRepLen)
			{
				if ((_current + thisRepLen == _scanEnd) ||
					(_buf [repStart + thisRepLen] != _buf [_current + thisRepLen]))
				{
					break;
				}
			}
			if (thisRepLen > _curRepetition.GetLen ())
			{
				_curRepetition.SetLen (thisRepLen);
				_curRepetition.SetFrom (repStart);
			}
		}
	}
	// We have found repetition if it is long enough and at economical
	// distance for repetitions of lenght greater then 3
	return _curRepetition.GetLen () >= MinRepetitionLength &&
		   (_curRepetition.GetLen () > 3 || _curRepetition.GetBackwardDist () < EconomicalDistanceFor3ByteRepetitionLength);
}
