#if !defined TRANSACTABLE_H
#define TRANSACTABLE_H
//-------------------------------------
//  transactable.h
//  (c) Reliable Software, 1996 -- 2002
//-------------------------------------

#include "Serialize.h"

class SoftTransactable
{
friend class ClearTransaction;
friend class Transaction;
friend class TransactableContainer;

protected:
	virtual ~SoftTransactable () {}

    virtual void BeginTransaction ()  = 0;
	virtual void BeginClearTransaction () {}
    virtual void CommitTransaction ()  throw () = 0;
	virtual void PostCommitTransaction () throw () {}
    virtual void AbortTransaction ()  = 0;
	virtual void Clear () throw () = 0;
};

class Transactable : public SoftTransactable, public Serializable
{
};

class TransactableContainer : public Transactable
{
protected:
	void AddTransactableMember (SoftTransactable & transactable)
	{
		_vTransactable.push_back (&transactable);
	}
    void BeginTransaction ()
	{
		Execute (&SoftTransactable::BeginTransaction);
	}
	void BeginClearTransaction ()
	{
		Execute (&SoftTransactable::BeginClearTransaction);
	}
    void CommitTransaction ()  throw ()
	{
		Execute (&SoftTransactable::CommitTransaction);
	}
	void PostCommitTransaction () throw ()
	{
		Execute (&SoftTransactable::PostCommitTransaction);
	}
    void AbortTransaction ()
	{
		Execute (&SoftTransactable::AbortTransaction);
	}
	void Clear () throw ()
	{
		Execute (&SoftTransactable::Clear);
	}

private:
	void Execute (void (SoftTransactable::*method)())
	{
		for (unsigned i = 0; i < _vTransactable.size (); ++i)
		{
			(_vTransactable [i]->*method) ();
		}
	}
	std::vector<SoftTransactable *> _vTransactable;
};

class TransactableSwitch : public Transactable
{
    enum SwitchValue
    {
        // Some arbitrary bit patterns
        OneValid = 0xc6c6c6c6,
        TwoValid = 0x3a3a3a3a,
        Invalid = 0xffffffff
    };

public:
    TransactableSwitch ()
        : _value (OneValid),
          _xValue (Invalid)
    {}

    int GetCurrent () const;
    int XGetCurrent () const;
    int GetPrevious () const;
    void XSwitch ();

    void BeginTransaction ()  { _xValue = _value; }
    void CommitTransaction () throw () { _value = _xValue; }
    void AbortTransaction ()  { _xValue = Invalid; }
	void Clear () throw () { _value = OneValid; }

    void Serialize (Serializer& out) const  { out.PutLong (_xValue); }
    void Deserialize (Deserializer& in, int version);

private:
    bool IsValid (int value) const;

    int  _value;
    int  _xValue;
};

#endif
