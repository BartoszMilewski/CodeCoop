#if !defined (MAPIPROPERTY_H)
#define MAPIPROPERTY_H
//
// (c) Reliable Software 1998
//

#include "LightString.h"
#include "MapiBuffer.h"

struct _SPropValue;
class MapiBufferSerializer<_SPropValue>;

class MapiProp
{
public:
	virtual ~MapiProp () {}
	virtual void Serialize (MapiBufferSerializer<_SPropValue> & out) const = 0;
};

class AddressType : public MapiProp
{
public:
	AddressType (char const * adrType)
		: _addrType (adrType)
	{}

	void Serialize (MapiBufferSerializer<_SPropValue> & out) const;

private:
	std::string	_addrType;
};

class DisplayName : public MapiProp
{
public:
	DisplayName (char const * name)
		: _displayName (name)
	{}

	void Serialize (MapiBufferSerializer<_SPropValue> & out) const;

private:
	std::string	_displayName;
};

class EmailAddress : public MapiProp
{
public:
	EmailAddress (char const * emailAddress)
		: _emailAddress (emailAddress)
	{}

	void Serialize (MapiBufferSerializer<_SPropValue> & out) const;

private:
	std::string	_emailAddress;
};

class RecipientType : public MapiProp
{
public:
	RecipientType (long recipType)
		: _recipientType (recipType)
	{}

	void Serialize (MapiBufferSerializer<_SPropValue> & out) const;

private:
	long	_recipientType;
};

class RowId : public MapiProp
{
public:
	RowId ()
		: _rowId (0)
	{}
	void Serialize (MapiBufferSerializer<_SPropValue> & out) const;

private:
	long _rowId;
};

#endif
