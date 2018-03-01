#if !defined (MAPIRESTRICTION_H)
#define MAPIRESTRICTION_H
//
// (c) Reliable Software 1998 -- 2004
//

#include <Dbg/Assert.h>

#include <mapix.h>
#include <mapival.h>

//
// MAPI Filters
//

class BoolPropertyFilter : public SRestriction
{
public:
	BoolPropertyFilter (unsigned long property, unsigned long oper, unsigned short value)
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
	{
		Assert (!FBadRestriction (this));
	}
};

class LongPropertyFilter : public SRestriction
{
public:
	LongPropertyFilter (unsigned long property, unsigned long oper, unsigned long value)
	{
		rt = RES_PROPERTY;						// This restriction applies to MAPI properties
		res.resProperty.ulPropTag = property;	// Restrict values of this property
		res.resProperty.relop = oper;			// Compare properties with provided value using this operator
		res.resProperty.lpProp = &_propValue;	// Property value to compare with
		_propValue.ulPropTag = property;
		_propValue.Value.ul = value;
	}

private:
	SPropValue	_propValue;
};

class FlagPropertyFilter : public SRestriction
{
public:
	FlagPropertyFilter (unsigned long property, unsigned long oper, unsigned long mask)
	{
		rt = RES_BITMASK;						// This restriction applies to MAPI properties
		res.resBitMask.ulPropTag = property;	// Restrict values of this property
		res.resBitMask.relBMR = oper;			// Relational operator describing how the mask
												// specified in the ulMask member should be applied
												// to the property tag.
		res.resBitMask.ulMask  = mask;			// Bitmask to apply to the property identified by ulPropTag.
	}
};

class IsUnreadMessage : public FlagPropertyFilter
{
public:
	IsUnreadMessage ()
		: FlagPropertyFilter (PR_MESSAGE_FLAGS, BMR_EQZ, MSGFLAG_READ)
	{
		Assert (!FBadRestriction (this));
	}
};

#endif
