#if !defined (MAPIRESTRICTION_H)
#define MAPIRESTRICTION_H
//
// (c) Reliable Software 1998
//

#include <mapix.h>

//
// MAPI Filters
//

class BoolPropertyFilter : public SRestriction
{
public:
	BoolPropertyFilter (ULONG property, ULONG oper, USHORT value)
	{
		rt = RES_PROPERTY;						// This restriction applies to MAPI properties
		res.resProperty.ulPropTag = property;	// Restrict values of this property
		res.resProperty.relop = oper;			// Compare properties with provided value using this operator
		res.resProperty.lpProp = &_propValue;	// Property value to compare with
		_propValue.ulPropTag = property;
		_propValue.Value.b = value;
	}

private:
	SPropValue	_propValue;
};

class DefaultStoreFilter : public BoolPropertyFilter
{
public:
	DefaultStoreFilter ()
		: BoolPropertyFilter (PR_DEFAULT_STORE, RELOP_EQ, TRUE)
	{}
};

#endif
