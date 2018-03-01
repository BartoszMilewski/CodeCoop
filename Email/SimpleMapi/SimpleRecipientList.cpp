//----------------------------------
// (c) Reliable Software 1998 - 2005
//----------------------------------

#include "precompiled.h"
#include "SimpleRecipientList.h"
#include "AddrBook.h"

using namespace SimpleMapi;

RecipientList::RecipientList (std::vector<std::string> const & recipients, bool bccRecipients)
{
	std::vector<std::string>::const_iterator iter;
	for (iter = recipients.begin (); iter != recipients.end (); ++iter)
	{
		std::unique_ptr<Recipient> recip;
		if (bccRecipients)
			recip.reset (new BccRecipient);
		else
			recip.reset (new ToRecipient);
		recip->AddDisplayName (*iter);
		_recipients.push_back (std::move(recip));
	}
}

void RecipientList::Verify (AddressBook & addressBook)
{
	auto_array<MapiRecipDesc> resolvedRecipients (_recipients.size ());
	for (unsigned int i = 0; i < _recipients.size (); i++)
	{
		Recipient * recip = _recipients [i];
		ResolvedRecip resolvedRecip (addressBook, *recip);
		recip->AddAddress (resolvedRecip->lpszAddress);
		auto_array<unsigned char> entryId (resolvedRecip->ulEIDSize);
		memcpy (&entryId [0], resolvedRecip->lpEntryID, resolvedRecip->ulEIDSize);
		recip->AddEntryId (resolvedRecip->ulEIDSize, entryId);
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
