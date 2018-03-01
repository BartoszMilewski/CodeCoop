//----------------------------------
// (c) Reliable Software 2001 - 2006
// ---------------------------------

#include "precompiled.h"
#include "MapiMailer.h"
#include "EmailMessage.h"
#include "MapiRecipientList.h"
#include "MapiBuffer.h"
#include "MapiUser.h"
#include "MapiStatusTable.h"
#include <Mail/EmailAddress.h>

void Mapi::Mailer::Send (OutgoingMessage & msg, std::vector<std::string> const & addressVector)
{
	RecipientList recipients (addressVector, _addressBook, msg.UseBccRecipients ());
	_outbox.Submit (msg, recipients, false);	// Quiet
}

void Mapi::Mailer::Save (OutgoingMessage & msg, std::vector<std::string> const & addressVector)
{
	RecipientList recipients (addressVector, _addressBook, msg.UseBccRecipients ());
	_outbox.Save (msg, recipients);
}

void Mapi::Mailer::GetLoggedUser (std::string & name, std::string & emailAddr)
{
	CurrentUser user (_defaultDir);
	if (user.IsValid ())
	{
		user.GetIdentity (name, emailAddr);
		// Check email address returned by the current user identity, because
		// some e-mail systems may use non-standard e-mail address in the user identity.
		// Most notable example is Microsoft Exchange server, which stores there string
		// allowing server access.
	}

	if (!Email::IsValidAddress (emailAddr))
	{
		emailAddr.clear ();
		name.clear ();
		// Mapi session cannot identify logged user.
		// Look at Sent Items
		SentItems sentItems (_defaultDir.GetMsgStore ());
		if (sentItems.IsValid ())
		{
			sentItems.GetSender (name, emailAddr);
			if (!Email::IsValidAddress (emailAddr))
			{
				emailAddr.clear ();
			}
		}
	}
}


// Returns true if recipient was resolved by the address book
bool Mapi::Mailer::VerifyEmailAddress (std::string & emailAddr)
{
	if (emailAddr.empty ())
		return false;

	AddrList addrList (1);
	ToRecipient recipient (emailAddr);
	// Revisit: retrieve address from addrList and modify emailAddr
	addrList.AddRecipient (recipient);

	if (_addressBook.ResolveName (addrList))
	{
		AddrList::Sequencer seq (addrList);
		Assert (!seq.AtEnd ());
		char const * resolvedEmailAddress = seq.GetEmailAddr ();
		Assert (resolvedEmailAddress != 0);
		emailAddr.assign (resolvedEmailAddress);
		return true;
	}
	return false;
}
