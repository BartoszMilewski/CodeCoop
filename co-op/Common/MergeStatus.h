#if !defined (MERGESTATUS_H)
#define MERGESTATUS_H
//------------------------------------
//  (c) Reliable Software, 2006 - 2007
//------------------------------------

#include <Bit.h>

class MergeStatus
{
public:
	MergeStatus ()
	{
		_bits.init (0);
	}
	MergeStatus (unsigned long value)
	{
		_bits.init (value);
	}

	void SetIdentical (bool bit)		{ _bits.set (Identical, bit); }
	void SetDifferent (bool bit)		{ _bits.set (Different, bit); }
	void SetCreated (bool bit)			{ _bits.set (Created, bit); }
	void SetDeletedAtTarget (bool bit)	{ _bits.set (DeletedAtTarget, bit); }
	void SetDeletedAtSource (bool bit)	{ _bits.set (DeletedAtSource, bit); }
	void SetAbsent (bool bit)			{ _bits.set (Absent, bit); }
	void SetMergeParent (bool bit)		{ _bits.set (MergeParent, bit); }
	void SetMerged (bool bit)			{ _bits.set (Merged, bit); }
	void SetConflict (bool bit)			{ _bits.set (Conflict, bit); }

	unsigned long GetValue () const { return _bits.to_ulong (); }
	char const * GetStatusName () const;
 
	bool IsValid () const				{ return GetValue () != 0; }
	bool IsIdentical () const			{ return _bits.test (Identical); }
	bool IsDifferent () const			{ return _bits.test (Different); }
	bool IsCreated () const				{ return _bits.test (Created); }
	bool IsDeletedAtTarget () const		{ return _bits.test (DeletedAtTarget); }
	bool IsDeletedAtSource () const		{ return _bits.test (DeletedAtSource); }
	bool IsAbsent () const				{ return _bits.test (Absent); }
	bool IsMergeParent () const			{ return _bits.test (MergeParent); }
	bool IsMerged () const				{ return _bits.test (Merged); }
	bool IsConflict () const			{ return _bits.test (Conflict); }

private:
	enum MergeBits
	{
		MergeParent,		// Parent folder needs merging
		Conflict,			// Automatic merge with conflicts
		Different,			// Different at source and target
		DeletedAtTarget,	// Deleted at target
		DeletedAtSource,	// Deleted at source
		Created,			// New at source
		Merged,				// Automatic merge succeeded
		Identical,			// Identical at source and target
		Absent,				// No longer a project item
	};

private:
	BitSet<MergeBits>	_bits;
};

#endif
