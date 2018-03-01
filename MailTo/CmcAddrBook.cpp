#if !defined (CMCADDRBOOK_CPP)
#define CMCADDRBOOK_CPP
//
// (c) Reliable Software 1998
//

#include "CmcAddrBook.h"
#include "CmcSession.h"
#include "CmcEx.h"

AddressBook::AddressBook (Session & session)
	: _session (session)
{
	_lookup = reinterpret_cast<CmcLookup>(_session.GetCmcFunction ("cmc_look_up"));
	_free = reinterpret_cast<CmcFree>(_session.GetCmcFunction ("cmc_free"));
}

CMC_recipient * AddressBook::Lookup (CMC_recipient const * recip) const
{
	CMC_return_code	rCode;
	CMC_uint32 count;
	CMC_recipient * resolvedRecip;

	rCode = _lookup (_session.GetId (),		// Session id
					 recip,					// Recipient to lookup
					 CMC_LOOKUP_RESOLVE_UI |// Resolve names using UI
					 CMC_ERROR_UI_ALLOWED,	// Display errors using UI
					 0,						// Default UI ID
					 &count,				// Numer of names found
					 &resolvedRecip,
					 0);					// No lookup extensions
	if (rCode != CMC_SUCCESS)
		throw CmcException ("Cannot lookup recipient", rCode);
	// Revisit: direct resource transfer -- resolvedRecip points to the
	// memory allocated by the CMC and has to be released by AddressBook::Lookup
	// clients by calling AddressBook::Release ()
	return resolvedRecip;
}

#endif
