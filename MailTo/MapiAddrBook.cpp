//
// (c) Reliable Software 1998
//

#include "MapiAddrBook.h"
#include "MapiSession.h"
#include "MapiEx.h"

#include <mapix.h>

AddressBook::AddressBook (Session & session)
{
	HRESULT hRes = session->OpenAddressBook (0,		// [in] Handle of the parent window for the common address
													// dialog box and other related displays.
											 0,		// [in] Pointer to the interface identifier (IID) representing the interface to be
													// used to access the address book. Passing NULL results in a pointer
													// to the address book's standard interface, or IAddrBook, being returned.
											 AB_NO_DIALOG,
													// Suppresses the display of dialog boxes.
											 &_p);	// [out] Pointer to a pointer to the address book.
	if (FAILED (hRes))
		throw MapiException ("Cannot open address book", hRes);
}
