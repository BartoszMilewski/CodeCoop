#if !defined XSTRING_H
#define XSTRING_H

//------------------------------------
// (c) Reliable Software 1997 -- 2003
//------------------------------------

#include "SerString.h"
#include "Transact.h"
#include <Dbg/Out.h>

class XString : public Transactable
{
public:
    XString () {}
    XString (std::string const & str)
    : _original (str)
    {}
    void XSet (std::string const & str)
	{
		Assert (_inXaction);
		_backup.assign (str);
	}
    void XClear ()
	{
		Assert (_inXaction);
		_backup.clear ();
	}
    bool XIsEmpty () const
	{
		Assert (_inXaction);
		return _backup.empty ();
	}
    bool IsEmpty () const
	{
		FutureAssert (!_inXaction);
		return _original.empty ();
	}
	std::string & XGetString ()
	{
		Assert (_inXaction);
		return _backup;
	}
	std::string const & XGet () const
	{
		Assert (_inXaction);
		return _backup;
	}
	operator std::string const & () const 
	{
		//	if we're in a transaction and calling Get, values better be equal
		FutureAssert (!_inXaction || (_original == _backup));
		return _original;
	}
	char const * c_str () const
	{
		FutureAssert (!_inXaction || (_original == _backup));
		return _original.c_str ();
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
		Assert (_backup.empty ());
		_backup.clear ();
#if !defined (NDEBUG)
		_inXaction.Set ();
#endif
	}
    void CommitTransaction () throw ()
    {
		Assert (_inXaction);
		_original.clear ();
        _original.swap (_backup);
#if !defined (NDEBUG)
		_inXaction.Clear ();
#endif
    }
    void AbortTransaction ()
    {
		Assert (_inXaction);
        _backup.clear ();
#if !defined (NDEBUG)
		_inXaction.Clear ();
#endif
    }
    void Clear () throw ()
	{
		Assert (!_inXaction);
		_original.clear ();
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
#if !defined (NDEBUG)
	boolDbg		_inXaction;
#endif
    SerString	_original;
    SerString	_backup;
};

#endif
