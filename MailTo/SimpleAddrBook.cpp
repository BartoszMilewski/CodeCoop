//
// (c) Reliable Software 1998
//

#include "SimpleAddrBook.h"
#include "SimpleSession.h"
#include "MapiEx.h"

AddressBook::AddressBook (Session & session)
	: _session (session)
{
	_lookup = reinterpret_cast<MapiLookup>(_session.GetFunction ("MAPIResolveName"));
	_free = reinterpret_cast<MapiFree>(_session.GetFunction ("MAPIFreeBuffer"));
}

MapiRecipDesc * AddressBook::Lookup (char const * recipName) const
{
	ULONG rCode;
	MapiRecipDesc * resolvedRecip;

	rCode = _lookup (_session.GetHandle (),	// Session handle
					 0,						// No window handle
					 const_cast<LPTSTR>(recipName),
											// Recipient's name
					 0,						// No flags
					 0,						// Reserved -- must be zero
					 &resolvedRecip);
	if (rCode != SUCCESS_SUCCESS)
		throw MapiException ("Cannot lookup recipient", rCode, recipName);
	// Revisit: direct resource transfer -- resolvedRecip points to the
	// memory allocated by the Simple MAPI and has to be released by AddressBook::Lookup
	// clients by calling AddressBook::Release ()
	return resolvedRecip;
}
