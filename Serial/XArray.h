#if !defined (XARRAY_H)
#define XARRAY_H
//------------------------------------
//  (c) Reliable Software, 1997 - 2005
//------------------------------------

#include "Transactable.h"

#include <Dbg/Assert.h>

//--------------------
// Transactable arrays
//--------------------

// Assumptions about class T:
//   - class T has to have a copy constructor used by the XGetEdit method

template <class T>
class SoftXArray : public SoftTransactable
{
public:
    ~SoftXArray ()
    {
	  size_t i;
        for (i = 0; i < _data.size (); i++)
            delete _data [i];
        // Tricky: we might own some items in _xData
        for (i = 0; i < _xData.size (); i++)
        {
            if (OwnedByTransaction (i))
                delete _xData [i];
        }
    }

	// Revisit: use the following for defining the end position
	// static unsigned int EndPos ()
	// {
	//	   return std::numeric_limits<unsigned int>::max();
	// }

	enum { npos = 0xffffffff };

	// Iterators
	typedef typename std::vector<T *>::const_iterator const_iterator;
	const_iterator xbegin () const { return _xData.begin (); }
	const_iterator xend () const   { return _xData.end ();   }

	const_iterator begin () const { return _data.begin (); }
	const_iterator end () const   { return _data.end (); }
	unsigned XtoIndex (const_iterator it) const { return it - _xData.begin (); }
	unsigned ToIndex (const_iterator it) const { return it - _data.begin (); }

    T const * Get  (unsigned int i) const { return _data [i]; }
    T const * XGet  (unsigned int i) const { return _xData [i]; }
    unsigned int Count () const { return _data.size (); }
    unsigned int XCount () const { return _xData.size (); }
	unsigned int XActualCount () const { return XCount () - DeletedCount (); }

    T * XGetEdit (unsigned int i);
    void XSubstitute (unsigned int i, std::unique_ptr<T> newItem);
    unsigned int XAppend (std::unique_ptr<T> item);
	// inserts the item at index i, items above i are moved up
	// Warning, expensive operation, invalidates iterators
	void XInsert (unsigned int i, std::unique_ptr<T> item);

    void XMarkDeleted (unsigned int i);
    void Clear () throw ();
    void XClear () throw ();

    // Soft Transactable interface

    void BeginTransaction ();
	void BeginClearTransaction ();
    void CommitTransaction () throw ();
    void AbortTransaction ();

protected:
    bool OwnedByTransaction (unsigned int i);
    unsigned int  DeletedCount () const;

	std::vector<T *> _data;
    // Tricky: doesn't own data, unless OwnedByTransaction is true
	std::vector<T *>  _xData;
};

template <class T>
bool SoftXArray <T>::OwnedByTransaction (unsigned int i)
{
    // The item is owned by this transaction if it is a New item (i >= _data.size ())
    // or it has been subustituted (_data [i] != _xData [i])
    return (i >= _data.size () || _data [i] != _xData [i]);
}

template <class T>
unsigned int SoftXArray <T>::DeletedCount () const
{
	unsigned int deletedCount = 0;
    for (size_t i = 0; i < _xData.size (); ++i)
    {
        if (_xData [i] == 0)
            deletedCount++;
    }
    return deletedCount;
}

template <class T>
unsigned int SoftXArray <T>::XAppend (std::unique_ptr<T> item)
{
    // Make sure there is space allocated
    // for this item before we release it
    _xData.reserve (_xData.size () + 1);
    _xData.push_back (item.release ());
    // item is now OwnedByTransaction
    Assert (OwnedByTransaction (_xData.size () - 1));
    return _xData.size () - 1;
}

template <class T>
void SoftXArray <T>::XSubstitute (unsigned int i, std::unique_ptr<T> newItem)
{
    // Delete the old item only if it's been added
    // under this transaction
    if (OwnedByTransaction (i))
        delete _xData [i];
    _xData [i] = newItem.release ();
    Assert (OwnedByTransaction (i));
}

template <class T>
void SoftXArray <T>::XInsert (unsigned int i, std::unique_ptr<T> item)
{
	// Example: insert x at i = 3, size = 6
	// 0 1 2 3 4 5 
	// a a a b b b     before
	// a a a x b b b   after
	unsigned oldSize = _xData.size ();
	if (i == oldSize)
	{
		XAppend (std::move(item));
	}
	else
	{
		Assert (i < oldSize);
		_xData.resize (oldSize + 1);
		// all elements starting at i will be owned by transaction
		for (unsigned dst = oldSize; dst != i; --dst)
		{
			Assert (dst != 0);
			if (OwnedByTransaction (dst - 1))
				_xData [dst] = _xData [dst - 1]; // move up
			else
				_xData [dst] = new T (*_xData [dst - 1]); // clone it
		}
		_xData [i] = item.release ();
	}
    Assert (OwnedByTransaction (i));
}

template <class T>
T * SoftXArray <T>::XGetEdit (unsigned int i)
{
    Assert (i < _xData.size ());
    if (OwnedByTransaction (i))
		return _xData [i];

    T * editItem = new T (*_xData [i]);
    // What follows this allocation is exception safe!
    // now it's OwnedByTransaction
    _xData [i] = editItem;
    Assert (OwnedByTransaction (i));
    return editItem;
}

template <class T>
void SoftXArray <T>::XMarkDeleted (unsigned int i)
{
    if (OwnedByTransaction (i))
        delete _xData[i];
    _xData [i] = 0;
}

// Soft Transactable interface
template <class T>
void SoftXArray <T>::BeginClearTransaction ()
{
    _xData.clear ();
}

template <class T>
void SoftXArray <T>::BeginTransaction ()
{
    _xData.clear ();
    // These are not owned by transaction
	std::copy (_data.begin (), _data.end (), std::back_inserter (_xData));
}

template <class T>
void SoftXArray <T>::CommitTransaction () throw ()
{
    size_t newCount = _xData.size () - DeletedCount ();

	// ripple copy _xData and delete items in _data
	size_t trg = 0;
	size_t src = 0;
	for (; src < _data.size (); ++src)
	{
        // Delete original items when substituted during transaction
        if (OwnedByTransaction (src))
		{
            delete _data [src];
			_data [src] = 0;
		}
		if (_xData [src] != 0)
        {
            _xData [trg] = _xData [src];
            ++trg;
        }
	}

    // Copy all items added during this transaction
	for (; src < _xData.size (); ++src)
	{
        Assert (OwnedByTransaction (src));
		if (_xData [src] != 0)
		{
            _xData [trg] = _xData [src];
            ++trg;
        }
	}

	Assert (trg == newCount);
	Assert (newCount <= _xData.size ());
	_xData.resize (newCount);
	_data.swap (_xData);
	std::vector<T *> empty;
	_xData.swap (empty);
}

template <class T>
void SoftXArray <T>::AbortTransaction ()
{
    for (size_t i = 0; i < _xData.size (); i++)
    {
        if (OwnedByTransaction (i))
            delete _xData [i];
    }
    _xData.clear ();
}

template <class T>
void SoftXArray <T>::Clear () throw ()
{
    for (size_t i = 0; i < _data.size (); i++)
    {
        delete _data [i];
    }
    _data.clear ();
}

template <class T>
void SoftXArray <T>::XClear () throw ()
{
    for (size_t i = 0; i < _xData.size (); i++)
    {
        XMarkDeleted (i);
    }
}

//--------------------
// Fully transactable
// Serializable array
//--------------------


template <class T>
class TransactableArray : public SoftXArray <T>, public Serializable
{
public:
    TransactableArray ()
        : SoftXArray <T> ()
    {}

	// Iterators
	const_iterator xbegin () const { return SoftXArray<T>::xbegin (); }
	const_iterator xend () const   { return SoftXArray<T>::xend ();   }

	const_iterator begin () const { return SoftXArray<T>::begin (); }
	const_iterator end () const   { return SoftXArray<T>::end (); }

    void Serialize (Serializer& out) const;
    void Deserialize (Deserializer& in, int version);
};

// Serializable interface

template <class T>
void TransactableArray <T>::Serialize (Serializer& out) const
{
    int newCount = _xData.size () - DeletedCount ();
    out.PutLong (newCount);
    int j = 0;
    for (size_t i = 0; i < _xData.size (); i++)
    {
        if (_xData [i] != 0)
        {
            _xData [i]->Serialize (out);
            j++;
        }
    }
    Assert (j == newCount);
}

template <class T>
void TransactableArray <T>::Deserialize (Deserializer& in, int version)
{
    int count = in.GetLong ();
	// Assumption: at least one byte per entry must be read from file
	if (count * sizeof (T*) > in.GetSize ().Low ())
		throw Win::Exception ("Deserializing corrupted file");
	// Mark old items as deleted
	_xData.clear ();
	_xData.resize (_data.size ());
    // Make sure there is space allocated for new items
	_xData.reserve (_xData.size () + count);
    for (int i = 0; i < count; i++)
    {
        T * item = new T(in, version);
		// item is now OwnedByTransaction
        _xData.push_back (item);
        Assert (OwnedByTransaction (i));
    }
}

#endif
