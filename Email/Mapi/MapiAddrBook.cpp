//
// (c) Reliable Software 1998 -- 2002
//

#include "precompiled.h"
#include "MapiAddrBook.h"
#include "MapiDefDir.h"
#include "MapiAddrList.h"
#include "MapiGPF.h"

using namespace Mapi;

AddressBook::AddressBook (DefaultDirectory & defDir)
{
	defDir.OpenAddressBook (*this);
}

// Returns true is name was resolved without any problem
bool AddressBook::ResolveName (AddrList & addrList)
{
	HRESULT hRes;
	try
	{	
		hRes = _addrBook->ResolveName (0,// [in] Handle of the parent window for a dialog box that is shown,
											 // if necessary, to prompt the user to resolve ambiguity.
										   MAPI_DIALOG,
											 // Allows the display of a dialog box to prompt the user for additional
											 // name resolution information. If this flag is not set,
											 // no dialog box is displayed.
										   0,// [in] Pointer to text for the title of the control in the dialog box that prompts
											 // the user to enter a recipient. The title varies depending on the type of
											 // recipient. This parameter can be NULL.
										   addrList.GetBuf ());
											 // [in-out] Pointer to an ADRLIST structure containing the list of
											 // recipient names to be resolved.
	}
	catch (...)
	{
		Mapi::HandleGPF ("IAddrBook::ResolveName");
		throw;
	}
	if (FAILED (hRes))
		throw Mapi::Exception ("MAPI -- Cannot resolve recipient names", hRes);
	return hRes == S_OK;
}