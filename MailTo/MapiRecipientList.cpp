//
// (c) Reliable Software 1998
//

#include "MapiRecipientList.h"
#include "MapiAddrBook.h"
#include "MapiEx.h"

RecipientList::RecipientList (std::vector<std::string> const & recipients)
{
	for (ConstStringIterator iter = recipients.begin (); iter != recipients.end (); iter++)
	{
		auto_ptr<MapiRecipient> recip (new MapiRecipient ());
		recip->AddDisplayName (iter->GetPtr ());
		_recipients.push_back (recip);
	}
}

void RecipientList::Verify (AddressBook & addressBook)
{
	MapiAddrList addrList (_recipients.size ());
	auto_vector<MapiRecipient>::iterator iter
	for ( iter = _recipients.begin (); iter != _recipients.end (); iter++)
	{
		addrList.AddRecipient (iter->GetReference ());
	}
	addrList.Dump ("Address list before ResolveNames");
	HRESULT hRes = addressBook->ResolveName (0,	// [in] Handle of the parent window for a dialog box that is shown,
											// if necessary, to prompt the user to resolve ambiguity.
									MAPI_DIALOG,
											// Allows the display of a dialog box to prompt the user for additional
											// name resolution information. If this flag is not set,
											// no dialog box is displayed.
									"Test", // [in] Pointer to text for the title of the control in the dialog box that prompts
											// the user to enter a recipient. The title varies depending on the type of
											// recipient. This parameter can be NULL.
									addrList.GetBuf ());
											// [in-out] Pointer to an ADRLIST structure containing the list of
											// recipient names to be resolved.
	if (FAILED (hRes))
		throw MapiException ("Cannot resolve recipient names", hRes);
	addrList.Dump ("Address list after ResolveNames");
	_mapiAddrList = addrList;
}
