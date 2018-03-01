#if !defined (XBITSET_H)
#define XBITSET_H
//------------------------------------
//  (c) Reliable Software, 2000 - 2005
//------------------------------------

#include "Transactable.h"

class XBitSet : public Transactable
{
public:
	XBitSet () {}
	XBitSet (unsigned long long value)
		: _original (value)
	{}

	void XSet (unsigned int bit)
	{
		_backup.set (bit);
	}

	void XReset (unsigned int bit)
	{
		_backup.reset (bit);
	}

	void XSet (unsigned int bit, bool flag)
	{
		_backup.set (bit, flag);
	}

	void XFlip (unsigned int bit)
	{
		_backup.flip (bit);
	}

	bool Test (unsigned int bit) const
	{
		return _original.test (bit);
	}

	bool XTest (unsigned int bit) const
	{
		return _backup.test (bit);
	}

	// Transactable interface

    void BeginTransaction () { _backup = _original; }
    void CommitTransaction () throw ()
    {
		_original = _backup;
        _backup.reset ();
    }
    void AbortTransaction () { _backup.reset (); }
    void Clear () throw () { _original.reset (); }

    void Serialize (Serializer& out) const;
    void Deserialize (Deserializer& in, int version);

private:
	std::bitset<std::numeric_limits<unsigned long>::digits>	_original;
	std::bitset<std::numeric_limits<unsigned long>::digits>	_backup;
};

#endif
