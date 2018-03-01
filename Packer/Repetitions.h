#if !defined REPETITIONS_H
#define REPETITIONS_H
//-------------------------------------
// (c) Reliable Software 1999, 2000, 01
// ------------------------------------

class RepetitionRecord
{
public :
	void Init (RepetitionRecord const & repetition)
	{
		_target = repetition._target;
		_from = repetition._from;
		_len = repetition._len;
	}

	void SetLen (unsigned int len) { _len = len; }
	void SetFrom (unsigned int from) { _from = from; }
	void SetTarget (unsigned int target) { _target = target; }

	unsigned int GetTarget () const { return _target;}
	unsigned int GetFrom () const { return _from;}
	unsigned int GetLen () const { return _len; }
	unsigned int GetBackwardDist () const { return _target - _from; }

private :
     unsigned int _target;	// Where the repetition has to occur in the buffer
	 unsigned int _from;	// Where the repetition already appears in the buffer _from < _target
	 unsigned int _len;
};

class Repetitions
{
public:
	Repetitions (unsigned int scannedBlockSize)
		: _current (0)
	{
		// Preallocating estimated repetition count speeds up repetition finding.
		unsigned int repetitionCountEstimate = scannedBlockSize / 20 + 50;
		_records.resize (repetitionCountEstimate);
	}

	void Add (RepetitionRecord const & repetition);
    std::vector<RepetitionRecord> const & GetRecords ();

private:
	std::vector<RepetitionRecord>	_records;
	unsigned int					_current;
};

#endif
