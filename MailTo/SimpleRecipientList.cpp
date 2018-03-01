//
// (c) Reliable Software 1998
//

#include "SimpleRecipientList.h"
#include "SimpleAddrBook.h"
#include "StrongArray.h"
#include <memory.h>

RecipientList::RecipientList (std::vector<std::string> const & recipients)
{
	typedef std::vector<std::string>::const_iterator ConstStringIterator;
	for (ConstStringIterator iter = recipients.begin (); iter != recipients.end (); ++iter)
	{
		std::auto_ptr<SimpleRecipient> recip (new SimpleRecipient);
		recip->AddDisplayName (iter->c_str ());
		_recipients.push_back (recip);
	}
}

void RecipientList::Verify (AddressBook & addressBook)
{
	auto_array<MapiRecipDesc> resolvedRecipients (_recipients.size ());
	for (unsigned i = 0; i < _recipients.size (); i++)
	{
		SimpleRecipient * recip = _recipients [i];
		// Revisit: Direct resource transfer
		MapiRecipDesc * resolvedRecip = addressBook.Lookup (recip->GetDisplayName ());
		recip->AddAddress (resolvedRecip->lpszAddress);
		auto_array<unsigned char> entryId (resolvedRecip->ulEIDSize);
		memcpy (&entryId [0], resolvedRecip->lpEntryID, resolvedRecip->ulEIDSize);
		recip->AddEntryId (resolvedRecip->ulEIDSize, entryId);
		// Revisit: Ask Addres Book to release memory allocated for resolved recipient
		addressBook.Release (resolvedRecip);
		// Copy resolved recipient to the Simple MAPI Address List
		resolvedRecipients [i].ulReserved = 0;
		resolvedRecipients [i].ulRecipClass = recip->ulRecipClass;
		resolvedRecipients [i].lpszName = recip->lpszName;
		resolvedRecipients [i].lpszAddress = recip->lpszAddress;
		resolvedRecipients [i].ulEIDSize = recip->ulEIDSize;
		resolvedRecipients [i].lpEntryID = recip->lpEntryID;
	}
	_addrList = resolvedRecipients;
}
