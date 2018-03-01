#if !defined (SCRIPTKIND_H)
#define SCRIPTKIND_H
//------------------------------------
//  (c) Reliable Software, 2003 - 2007
//------------------------------------

#include <Dbg/Assert.h>

#include <iosfwd>

class ScriptKind
{
public:
	ScriptKind ()
		: _value (0)
	{}
	ScriptKind (ScriptKind const & state) 
	{ 
		_value = state._value;
	}
	ScriptKind (unsigned long value)
		: _value (value)
	{}
	void operator= (ScriptKind const & state) 
	{ 
		_value = state._value;
	}

	void InitFromOldScriptKind (unsigned long oldKind);

	unsigned long GetValue () const { return _value; }

	// Includes packages and full synch
	bool IsControl () const { return TestLevel0 (true); }
	bool IsPureControl () const { return TestLevel0 (true) && TestLevel1 (true); }

	bool IsScript () const { return IsControl () && TestLevel1 (true); }
	bool IsPackage () const { return IsControl () && TestLevel1 (false); }

	bool IsFullSynch () const { return IsPackage () && TestLevel2 (true); }
	bool IsRegularPackage () const { return IsPackage () && TestLevel2 (false); }
	bool IsVerificationPackage () const { return IsRegularPackage () && TestLevel3 (true); }

	bool IsAck () const { return IsScript () && TestLevel2 (false); }
	bool IsRequest () const { return IsScript () && TestLevel2 (true); }
	bool IsJoinRequest () const { return IsRequest () && TestLevel3 (true); }
	bool IsResendRequest () const { return IsRequest () && TestLevel3 (false); }
	bool IsScriptResendRequest () const { return IsResendRequest () && TestLevel4 (false); }
	bool IsFullSynchResendRequest () const { return IsResendRequest () && TestLevel4 (true); }

	bool IsData () const { return TestLevel0 (false); }

	bool IsCumulative () const { return IsData () && TestLevel1 (true); }
	bool IsSetChange () const { return IsCumulative () && TestLevel2 (true); }
	bool IsUnitChange () const { return IsCumulative () && TestLevel2 (false); }

	bool IsMergeable () const { return IsData () && TestLevel1 (false); }
	bool IsDrastic () const { return IsMergeable () && TestLevel2 (true); }
	bool IsEditMember () const { return IsMergeable () && TestLevel2 (false); }
	bool IsDeleteMember () const { return IsDrastic () && TestLevel3 (true); }
	bool IsAddMember () const { return IsDrastic () && TestLevel3 (false); }

	bool Verify (bool isFromVersion40) const;

private:
	bool TestLevel0 (bool bit) const { return _bits._level0 == bit; }
	bool TestLevel1 (bool bit) const { return _bits._level1 == bit; }
	bool TestLevel2 (bool bit) const { return _bits._level2 == bit; }
	bool TestLevel3 (bool bit) const { return _bits._level3 == bit; }
	bool TestLevel4 (bool bit) const { return _bits._level4 == bit; }

private:
	union
	{
		unsigned long _value;		// for quick serialization
		// ScriptKind is implemented as bit tree. The bits correspond
		// to the bit tree depth levels. For details see Components\Scripts\Script.doc
		// in the Documentation project.
		struct
		{
			unsigned long _level0:1; // least significant
			unsigned long _level1:1;
			unsigned long _level2:1;
			unsigned long _level3:1;
			unsigned long _level4:1;
		} _bits;
	};
};

class ScriptKindSetChange : public ScriptKind
{
public:
	ScriptKindSetChange ()
		: ScriptKind (0x6)	// 0110
	{
		Assert (IsSetChange ());
	}
};

class ScriptKindUnitChange : public ScriptKind
{
public:
	ScriptKindUnitChange ()
		: ScriptKind (0x2)	// 0010
	{
		Assert (IsUnitChange ());
	}
};

class ScriptKindFullSynch : public ScriptKind
{
public:
	ScriptKindFullSynch ()
		: ScriptKind (0x5)	// 0101
	{
		Assert (IsFullSynch ());
	}
};

class ScriptKindPackage : public ScriptKind
{
public:
	ScriptKindPackage ()
		: ScriptKind (0x1)	// 0001
	{
		Assert (IsRegularPackage ());
	}
};

class ScriptKindVerificationPackage : public ScriptKind
{
public:
	ScriptKindVerificationPackage ()
		: ScriptKind (0x9)	// 1001
	{
		Assert (IsVerificationPackage ());
	}
};

class ScriptKindEditMember : public ScriptKind
{
public:
	ScriptKindEditMember ()
		: ScriptKind (0x0)	// 0000
	{
		Assert (IsEditMember ());
	}
};

class ScriptKindAddMember : public ScriptKind
{
public:
	ScriptKindAddMember ()
		: ScriptKind (0x4)	// 0100
	{
		Assert (IsAddMember ());
	}
};

class ScriptKindDeleteMember : public ScriptKind
{
public:
	ScriptKindDeleteMember ()
		: ScriptKind (0xc)	// 1100
	{
		Assert (IsDeleteMember ());
	}
};

class ScriptKindAck : public ScriptKind
{
public:
	ScriptKindAck ()
		: ScriptKind (0x3)	// 0011
	{
		Assert (IsAck ());
	}
};

class ScriptKindJoinRequest : public ScriptKind
{
public:
	ScriptKindJoinRequest ()
		: ScriptKind (0xf)	// 1111
	{
		Assert (IsJoinRequest ());
	}
};

class ScriptKindResendRequest : public ScriptKind
{
public:
	ScriptKindResendRequest ()
		: ScriptKind (0x7)	// 0111
	{
		Assert (IsScriptResendRequest ());
	}
};

class ScriptKindFullSynchResendRequest : public ScriptKind
{
public:
	ScriptKindFullSynchResendRequest ()
		: ScriptKind (0x17)	// 10111
	{
		Assert (IsFullSynchResendRequest ());
	}
};

std::ostream & operator<<(std::ostream & os, ScriptKind kind);

#endif
