#if !defined (MAPIRECIPIENT_H)
#define MAPIRECIPIENT_H
//
// (c) Reliable Software 1998
//

#include "MapiProperty.h"

#include <mapix.h>

class MapiRecipient
{
public:
	MapiRecipient ()
	{
		auto_ptr<MapiProp> addrType (new AddressType ("SMTP"));
		_props.push_back (addrType);
		auto_ptr<MapiProp> recipType (new RecipientType (MAPI_TO));
		_props.push_back (recipType);
	}

	void AddDisplayName (char const * displayName);
	int	GetPropertyCount () const { return _props.size (); }
	void SerializeProps (MapiBufferSerializer<SPropValue> & out) const;

private:
	auto_vector<MapiProp>	_props;
};

#endif
