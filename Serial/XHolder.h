#if !defined (XHOLDER_H)
#define XHOLDER_H
//---------------------------------------------
// (c) Reliable Software 2004
//---------------------------------------------

#include "XArray.h"

#include <Loki/TypeManip.h>

// Holds the reference to the TransactableArray<T>
template <class T, bool isReadOnlyAccess, bool isInTransaction, typename Container = TransactableArray<T> >
class XHolder
{
public:
	XHolder (Container arr)
		: _arr (arr)
	{}

	typedef typename Loki::Select<isReadOnlyAccess, T const *, T *>::Result ValueType;

	ValueType Get (unsigned int idx) const { return Get (idx, Loki::Int2Type<isInTransaction>()); }
	ValueType GetEdit (unsigned int idx) const { return GetEdit (idx, Loki::Int2Type<isReadOnlyAccess>(), Loki::Int2Type<isInTransaction>()); }

private:
	ValueType Get (unsigned int idx, Loki::Int2Type<true>) const { return _arr.XGet (idx); }
	ValueType Get (unsigned int idx, Loki::Int2Type<false>) const { return _arr.Get (idx); }

	// GetEdit available only for read-write access during transaction
	ValueType GetEdit (unsigned int idx, Loki::Int2Type<false>, Loki::Int2Type<true>) const { return _arr.XGetEdit (idx); }

	typedef typename Loki::Select<isReadOnlyAccess, Container const &, Container &>::Result RefType;

	RefType	_arr;
};

#endif
