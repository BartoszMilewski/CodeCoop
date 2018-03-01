#if !defined (SCRIPTSTATUS_H)
#define SCRIPTSTATUS_H
// -------------------------------
// (c) Reliable Software, 1999
// -------------------------------
#include <Dbg/Assert.h>

class ScriptStatus
{
public:
	struct Direction
	{
		enum Bits
		{
			In = 0,
			Out,
			Unknown
		};
	};
	struct Read
	{
		enum Bits
		{
			Absent = 0,
			Ok,
			NoHeader,
			AccessDenied,
			Corrupted
		};
	};
	struct Dispatch
	{
		enum Bits
		{
			ToBeDone = 0,
			InProgress,
			NoNetwork,
			NoEmail,
			ServerTimeout,
			NoTmpCopy,
			NoDisk,
			Ignored
		};
	};

public:
	ScriptStatus (unsigned long value)
		: _value (value)
	{}
	ScriptStatus (Direction::Bits dir = Direction::Unknown)
        : _value (0)
    {
		SetDirection (dir);
	}
	void Set(ScriptStatus status)
	{
		_value = status.GetValue();
	}
	void SetDirection (Direction::Bits dir) 
	{
		Assert (dir < 4); // must fit in 2 bits
		_stateBits._direction = dir; 
	}
	void ClearReadStatus () { _stateBits._read = Read::Absent; }
	void SetPresent ()		{ _stateBits._read = Read::Ok; }
    void SetIsCorrupted  () { _stateBits._read = Read::Corrupted; }
    void SetNoHeader     () { _stateBits._read = Read::NoHeader; }
    void SetAccessDenied () { _stateBits._read = Read::AccessDenied; }
	// Returns true if status changed
	bool SetDispatchStatus (Dispatch::Bits status)
	{
		bool changed = _stateBits._dispatch != static_cast<unsigned long>(status); 
		_stateBits._dispatch = status; 
		return changed;
	}
	void SetInProgress () { _stateBits._dispatch = Dispatch::InProgress; }
	void ClearInProgress ()
	{
		if (_stateBits._dispatch == Dispatch::InProgress)
			_stateBits._dispatch = Dispatch::ToBeDone;
	}
	unsigned long GetValue () const { return _value; }
	Direction::Bits GetDirection () const 
	{
		return static_cast<Direction::Bits> (_stateBits._direction);
	}
	Read::Bits GetReadStatus () const
	{
		return static_cast<Read::Bits> (_stateBits._read);
	}
	Dispatch::Bits GetDispatchStatus () const
	{
		return static_cast<Dispatch::Bits> (_stateBits._dispatch);
	}
	char const * LongDescription () const;
	char const * ShortDescription () const;
	char const * GetDirectionStr () const;
private:
    union
    {
        unsigned long _value;  // for initialisation and copy constructor
        struct
        {
            unsigned long _read			: 8;
            unsigned long _dispatch     : 8;
            unsigned long _direction    : 2;
        } _stateBits;
    };
};

#endif
