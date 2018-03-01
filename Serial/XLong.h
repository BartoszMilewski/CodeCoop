#if !defined (XLONG_H)
#define XLONG_H
//---------------------------------------
//  XLong.h
//  (c) Reliable Software, 2002
//---------------------------------------

#include "Transact.h"
#include "Dbg/Assert.h"

template <unsigned long defaultValue>
class XLongWithDefault : public Transactable
{
public:
	XLongWithDefault (unsigned long value = defaultValue)
	: _original (value), _backup (0)
	{
	}
	static unsigned long GetDefaultValue ()
	{
		return defaultValue;
	}
    unsigned long XGetOriginal () const
	{
		Assert (_inXaction);
		return _original;
	}
	unsigned long Get () const
	{
		//	if we're in a transaction and calling Get, values better be equal
		FutureAssert (!_inXaction || (_original == _backup));
		return _original;
	}
    unsigned long XGet() const
	{
		Assert (_inXaction);
		return _backup;
	}
	void XSet (unsigned long value)
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
		_backup = defaultValue;
#if !defined (NDEBUG)
		_inXaction.Set ();
#endif
	}
	void CommitTransaction () throw ()
	{
		Assert (_inXaction);
		_original = _backup;
		_backup = 0;
#if !defined (NDEBUG)
		_inXaction.Clear ();
#endif
	}
	void AbortTransaction ()
	{
		Assert (_inXaction);
		_backup = 0;
#if !defined (NDEBUG)
		_inXaction.Clear ();
#endif
	}
    void Clear () throw ()
	{
		Assert (!_inXaction);
		_original = defaultValue;
	}

    void Serialize (Serializer& out) const
	{
		Assert (_inXaction);
		out.PutLong (_backup);
	}
    void Deserialize (Deserializer& in, int version)
	{
		Assert (_inXaction);
		_backup = in.GetLong ();
	}

private:
	unsigned long	_original;
	unsigned long	_backup;
#if !defined (NDEBUG)
	boolDbg			_inXaction;
#endif
};

typedef XLongWithDefault<0> XLong;

#endif
