#include "precompiled.h"
#include "Repetitions.h"

void Repetitions::Add (RepetitionRecord const & repetition)
{
	if (_current >= _records.size ())
		_records.resize (_current * 2);
	_records [_current].Init (repetition);
	_current++;
}

std::vector <RepetitionRecord> const & Repetitions::GetRecords ()
{
	_records.resize (_current);
	return _records;
}

