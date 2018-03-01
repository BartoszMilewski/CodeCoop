#if !defined (PTRUTIL_H)
#define PTRUTIL_H
//--------------------------------
// (c) Reliable Software 1998-2005
//--------------------------------

class RefCounted
{
public:
	RefCounted (int startCount = 1) : _refCount (startCount) {}
	int AddRef () const { return ::InterlockedIncrement (&_refCount); }
	int SubRef () const { return ::InterlockedDecrement (&_refCount); }
	long GetRefCount () const { return _refCount; }
	bool IsNonZero () const { return _refCount != 0; }
private:
	mutable volatile long _refCount;
};

// Note: This object is not thread-safe!
template<typename T>
class RefPtr
{
public:
	RefPtr (T * ref = 0): _ref (ref) 
	{}
	RefPtr (RefPtr<T> const & p)
		: _ref (p._ref)
	{
		if (_ref != 0)
			_ref->AddRef ();
	}
	~RefPtr ()
	{
		Release ();
	}
	RefPtr const & operator= (RefPtr const & p)
	{
		if (this != &p)
		{
			Release ();
			_ref = p._ref;
			if (_ref != 0)
				_ref->AddRef ();
		}
		return *this;
	}
	void reset () { Release (); }
	void reset (T * p)
	{
		Release ();
		_ref = p;
	}
	bool empty () const { return _ref == 0;	}
	T * operator-> () { return _ref; }
	T const * operator-> () const { return _ref; }
	T * get () { return _ref; }
	T const * get () const { return _ref; }

	template<typename U>
		U * GetPtr ()
	{
		return static_cast<U *> (_ref);
	}
	template<typename U>
		U const * GetPtr () const
	{
		return static_cast<U *> (_ref);
	}
protected:
	void Release ()
	{
		if (_ref && _ref->SubRef () == 0)
			delete _ref;
	}
private:
	T * _ref;
};

// Use it this way:
// T * sharedPtr;
// {
//     T substObject;
//     TempSubstPtr<T> tmp (sharedPtr);
//     tmp.Switch (&substObject);
//     ... everybody who uses sharedPtr, sees substObject
// }   // destructor switches them back

template<class T>
class TempSubstPtr
{
public:
	TempSubstPtr (T * & pPrev) 
		: _target (pPrev)
	{
		_pPrev = pPrev;
	}
	~TempSubstPtr ()
	{
		_target = _pPrev;
	}
	void Switch (T * pNew)
	{
		_target = pNew;
	}
private:
	T * &	_target;
	T *		_pPrev;
};

// Use it this way:
// T sharedVal;
// {
//     TempSubstVal<T> tmp (sharedVal);
//     tmp.Switch (substValue);
//     ... everybody who keeps reference to sharedVal, sees substValue
// }   // destructor switches them back

template<class T>
class TempSubstVal
{
public:
	TempSubstVal (T  & prev) 
		: _target (prev)
	{
		_oldVal = prev;
	}
	~TempSubstVal ()
	{
		_target = _oldVal;
	}
	void Switch (T  newVal)
	{
		_target = newVal;
	}
private:
	T     & _target;
	T		_oldVal;
};

#endif
