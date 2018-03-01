//
// (c) Reliable Software 1998
//

#include "MapiRecipient.h"

void MapiRecipient::AddDisplayName (char const * displayName)
{
	auto_ptr<MapiProp> name (new DisplayName (displayName));
	_props.push_back (name);
}

void MapiRecipient::SerializeProps (MapiBufferSerializer<SPropValue> & out) const
{
	for (int i = 0; i < _props.size (); i++)
	{
		_props [i]->Serialize (out);
	}
}
