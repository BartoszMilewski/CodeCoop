//
// (c) Reliable Software 1998 -- 2004
//

#include "precompiled.h"
#include "AddrBook.h"
#include "SimpleSession.h"
#include "SimpleMapiEx.h"

using namespace SimpleMapi;

AddressBook::AddressBook (Session & session)
	: _session (session)
{
	_session.GetFunction ("MAPIResolveName", _lookup);
	_session.GetFunction ("MAPIFreeBuffer", _free);
}

// Returns true if recipient was resolved by the address book
bool AddressBook::VerifyRecipient (std::string & emailAddr) const
{
	ULONG rCode;
	MapiRecipDesc * resolvedRecip;

	rCode = _lookup (_session.GetHandle (),	// Session handle
					 0,						// No window handle
					 const_cast<LPTSTR>(emailAddr.c_str ()),
											// Recipient's name
					 0,						// No flags
					 0,						// Reserved -- must be zero
					 &resolvedRecip);
	emailAddr = resolvedRecip->lpszAddress;
	_free (resolvedRecip);
	return rCode == SUCCESS_SUCCESS;
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
	{
		if (rCode == MAPI_E_AMBIGUOUS_RECIPIENT)
		{
			// Let the user select the right recipient
			rCode = _lookup (_session.GetHandle (),	// Session handle
							 0,						// No window handle
							 const_cast<LPTSTR>(recipName),
													// Recipient's name
							 MAPI_DIALOG,			// A dialog box should be displayed for name resolution
							 0,						// Reserved -- must be zero
							 &resolvedRecip);
			if (rCode == MAPI_E_USER_ABORT)
				throw Win::Exception ("Recipient selection aborted by the user");
		}
		if (rCode != SUCCESS_SUCCESS)
			throw SimpleMapi::Exception ("MAPI -- Cannot lookup recipient in the address book", rCode, recipName);
	}
	return resolvedRecip;
}
