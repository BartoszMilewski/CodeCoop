#if !defined (XFILEOFFSET_H)
#define XFILEOFFSET_H
// (c) Reliable Software, 2003
#include "Transact.h"
#include <File/File.h>

class SerFileOffset : public File::Offset, public Serializable
{
public:
	SerFileOffset () : File::Offset (0, 0) {}
	SerFileOffset (LargeInteger li) : File::Offset (li.Low (), li.High ()) {}
    void Serialize (Serializer& out) const
	{
		out.PutLong (Low ());
		out.PutLong (High ());
	}
    void Deserialize (Deserializer& in, int version)
	{
		_value.LowPart = in.GetLong ();
		if (version > 43)
			_value.HighPart = in.GetLong ();
		else
		{
			_value.HighPart = 0;
			if (_value.LowPart == -1)
				*this = File::Offset::Invalid;
		}
	}
};

class XFileOffset : public Transactable
{
public:
	XFileOffset ()
	: _original (0, 0)
	{
	}
	File::Offset XGetOriginal () const
	{
		Assert (_inXaction);
		return _original;
	}
	File::Offset Get () const
	{
		//	if we're in a transaction and calling Get, values better be equal
		FutureAssert (!_inXaction || (_original == _backup));
		return _original;
	}
    File::Offset XGet() const
	{
		Assert (_inXaction);
		return _backup;
	}
	void XSet (File::Offset value)
	{
		Assert (_inXaction);
		_backup = value;
	}

    // Transactable interface

	void BeginTransaction ()
	{
		Assert (!_inXaction);
		_backup = _original;
#if !defined (NDEBUG)
		_inXaction.Set ();
#endif
	}
	void BeginClearTransaction ()
	{
		Assert (!_inXaction);
		_backup.Reset ();
#if !defined (NDEBUG)
		_inXaction.Set ();
#endif
	}
	void CommitTransaction () throw ()
	{
		Assert (_inXaction);
		_original = _backup;
		_backup.Reset ();
#if !defined (NDEBUG)
		_inXaction.Clear ();
#endif
	}
	void AbortTransaction ()
	{
		Assert (_inXaction);
		_backup.Reset ();
#if !defined (NDEBUG)
		_inXaction.Clear ();
#endif
	}
    void Clear () throw ()
	{
		Assert (!_inXaction);
		_original.Reset ();
	}

    void Serialize (Serializer& out) const
	{
		Assert (_inXaction);
		_backup.Serialize (out);
	}
    void Deserialize (Deserializer& in, int version)
	{
		Assert (_inXaction);
		_backup.Deserialize (in, version);
	}

private:
	File::Offset	_original;
	SerFileOffset	_backup;
#if !defined (NDEBUG)
	boolDbg			_inXaction;
#endif
};

#endif
