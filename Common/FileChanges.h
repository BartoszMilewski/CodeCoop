#if !defined (FILECHANGES_H)
#define FILECHANGES_H
//---------------------------------------
//  FileChanges.h
//  (c) Reliable Software, 2002
//---------------------------------------

#include <bitset>
#include <limits>

class FileChanges
{
public:
	FileChanges () {}
	FileChanges (unsigned long long value)
		: _bits (value)
	{}

	void SetLocalEdit (bool bit)		{ _bits.set (localEdit, bit); }
	void SetLocalMove (bool bit)		{ _bits.set (localMove, bit); }
	void SetLocalRename (bool bit)		{ _bits.set (localRename, bit); }
	void SetLocalTypeChange (bool bit)	{ _bits.set (localTypeChange, bit); }
	void SetLocalDelete (bool bit)		{ _bits.set (localDelete, bit); }
	void SetLocalNew (bool bit)			{ _bits.set (localNew, bit); }
	void SetLocalRollback (bool bit)	{ _bits.set (localRollback, bit); }
    void SetSynchEdit (bool bit)		{ _bits.set (synchEdit, bit); }
	void SetSynchMove (bool bit)		{ _bits.set (synchMove, bit); }
	void SetSynchRename (bool bit)		{ _bits.set (synchRename, bit); }
	void SetSynchTypeChange (bool bit)	{ _bits.set (synchTypeChange, bit); }
	void SetSynchDelete (bool bit)		{ _bits.set (synchDelete, bit); }
	void SetSynchNew (bool bit)			{ _bits.set (synchNew, bit); }

	void SetHistorical (bool bit)		{ _bits.set (historical, bit); }

	unsigned long GetValue () const { return _bits.to_ulong (); }
	void Clear () { _bits.reset (); }
	void ClearLocal ()
	{
		SetLocalEdit (false);
		SetLocalMove (false);
		SetLocalRename (false);
		SetLocalTypeChange (false);
		SetLocalDelete (false);
		SetLocalNew (false);
		SetLocalRollback (false);
	}

    bool IsLocalEdit () const			{ return _bits.test (localEdit); }
	bool IsLocalMove () const			{ return _bits.test (localMove); }
	bool IsLocalRename () const			{ return _bits.test (localRename); }
	bool IsLocalTypeChange () const		{ return _bits.test (localTypeChange); }
	bool IsLocalDelete () const			{ return _bits.test (localDelete); }
	bool IsLocalNew () const			{ return _bits.test (localNew); }
	bool IsLocalRollback () const		{ return _bits.test (localRollback); }
    bool IsSynchEdit () const			{ return _bits.test (synchEdit); }
	bool IsSynchMove () const			{ return _bits.test (synchMove); }
	bool IsSynchRename () const			{ return _bits.test (synchRename); }
	bool IsSynchTypeChange () const		{ return _bits.test (synchTypeChange); }
	bool IsSynchDelete () const			{ return _bits.test (synchDelete); }
	bool IsSynchNew () const			{ return _bits.test (synchNew); }

	bool IsHistorical () const			{ return _bits.test (historical); }

	bool AnyLocalChanges () const		{ return IsLocalEdit () ||
												 IsLocalMove () ||
												 IsLocalRename () ||
												 IsLocalTypeChange () ||
												 IsLocalDelete () ||
												 IsLocalNew () ||
												 IsLocalRollback (); }

	bool AnySynchChanges () const		{ return IsSynchEdit () ||
												 IsSynchMove () ||
												 IsSynchRename () ||
												 IsSynchTypeChange () ||
												 IsSynchDelete () ||
												 IsSynchNew (); }
private:
	enum
	{
		localEdit,			// File edited by the user
		localMove,			// File moved by the user
		localRename,		// File renamed by the user
		localTypeChange,	// File type changed by the user
		localDelete,		// File deleted by the user
		localNew,			// File added by the user
		localRollback,		// File changes has been locally rolled back due to script conflict
		synchEdit,			// File edited by the synch
		synchMove,			// File moved by the synch
		synchRename,		// File renamed by the synch
		synchTypeChange,	// File type changed by the synch
		synchDelete,		// File deleted by the synch
		synchNew,			// File added by the synch
		historical			// Showing historical file changes
	};

private:
	std::bitset<std::numeric_limits<unsigned long>::digits>	_bits;
};

#endif
