//
// (c) Reliable Software 1998
//

#include "CmcRecipientList.h"
#include "CmcAddrBook.h"
#include "CmcEx.h"

RecipientList::RecipientList (std::vector<std::string> const & recipients)
{
	for (ConstStringIterator iter = recipients.begin (); iter != recipients.end (); iter++)
	{
		auto_ptr<CmcRecipient> recip = new CmcRecipient ();
		recip->AddDisplayName (iter->GetPtr ());
		_recipients.push_back (recip);
	}
}

void RecipientList::Verify (AddressBook & addressBook)
{
	auto_array<CMC_recipient> resolvedRecipients (_recipients.size ());
	for (int i = 0; i < _recipients.size (); i++)
	{
		CmcRecipient * recip = _recipients [i];
		// Revisit: Direct resource transfer
		CMC_recipient * resolvedRecip = addressBook.Lookup (_recipients [i]);
		recip->AddAddress (resolvedRecip->address);
		// Revisit: Ask Addres Book to release memory allocated for resolved recipient
		addressBook.Release (resolvedRecip);
		// Copy resolved recipient to the CMC Address List
		resolvedRecipients [i].name = recip->name;
		resolvedRecipients [i].name_type = recip->name_type;
		resolvedRecipients [i].address = recip->address;
		resolvedRecipients [i].role = recip->role;
		resolvedRecipients [i].recip_flags = recip->recip_flags;
		resolvedRecipients [i].recip_extensions = recip->recip_extensions;
	}
	// Mark te last recipient
	resolvedRecipients [i - 1].recip_flags = CMC_RECIP_LAST_ELEMENT;
	_cmcAddrList = resolvedRecipients;
}
