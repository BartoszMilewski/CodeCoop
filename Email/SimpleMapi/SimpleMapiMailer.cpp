//----------------------------------
// (c) Reliable Software 2001 - 2006
// ---------------------------------

#include "precompiled.h"
#include "SimpleMapiMailer.h"
#include "EmailMessage.h"
#include "SimpleRecipientList.h"
#include "SimpleMapi.h"
#include "SimpleMapiEx.h"
#include <Mail/EmailAddress.h>

SimpleMapi::Mailer::Mailer (Session & session)
	: _session (session),
	  _addressBook (_session),
	  _outbox (_session)
{}

void SimpleMapi::Mailer::Send (OutgoingMessage & msg, std::vector<std::string> const & addrVector)
{
	RecipientList recipients (addrVector, msg.UseBccRecipients ());
	recipients.Verify (_addressBook);
	_outbox.Submit (msg, recipients, false);
}

void SimpleMapi::Mailer::Save (OutgoingMessage & msg, std::vector<std::string> const & addrVector)
{
	RecipientList recipients (addrVector, msg.UseBccRecipients ());
	recipients.Verify (_addressBook);
	_outbox.Save (msg, recipients);
}

void SimpleMapi::Mailer::GetLoggedUser (std::string & name, std::string & emailAddr)
{
	// Create fake email message
	MapiRecipDesc recip;

	recip.ulReserved = 0;
	recip.ulRecipClass = MAPI_TO;
	recip.lpszName = "Anybody";
	recip.lpszAddress = "SMTP:Anybody@Anywhere.com";
	recip.ulEIDSize = 0;
	recip.lpEntryID = 0;

	MapiMessage mailMsg;

	mailMsg.ulReserved = 0;
	mailMsg.lpszSubject = "Test message";
	mailMsg.lpszNoteText = "Test message";
	mailMsg.lpszMessageType = 0;
	mailMsg.lpszDateReceived = 0;
	mailMsg.lpszConversationID = 0;
	mailMsg.flFlags = MAPI_UNREAD;
	mailMsg.lpOriginator = 0;
	mailMsg.nRecipCount = 1;
	mailMsg.lpRecips = &recip;
	mailMsg.nFileCount = 0;
	mailMsg.lpFiles = 0;

	MapiSaveMail save;
	_session.GetFunction ("MAPISaveMail", save);
	char messageId [514];
	messageId [0] = '\0';
	// Save fake email message
	ULONG rCode = save (_session.GetHandle (),	// Explicit session handle
						0,						// Parent window handle or zero
						&mailMsg,				// Message
						MAPI_LONG_MSGID,	 	// Message ID can be long up to 512 chars
						0,						// Reserved; must be zero
						messageId);				// Buffer for saved message ID
	if (rCode != SUCCESS_SUCCESS)
		return;

	// Now read fake email message and retrieve sender info -- this is logged user
	MapiReadMail read;
	_session.GetFunction ("MAPIReadMail", read);
	MapiDeleteMail _delete;
	_session.GetFunction ("MAPIDeleteMail", _delete);
	MapiFree free;
	_session.GetFunction ("MAPIFreeBuffer", free);
	// Read the envelope only
	RetrievedMessage message (free);
	rCode = read (_session.GetHandle (),	// Explicit session handle
				  0,						// Parent window handle or zero
				  messageId,				// Message ID
				  MAPI_ENVELOPE_ONLY,		// Read only the message header
				  0,						// Reserved; must be zero
				  message.GetAddr ());		// Message read
	if (rCode == SUCCESS_SUCCESS && message.IsRetrieved ())
	{
		// Save sender data
		if (message->lpOriginator != 0 && message->lpOriginator->lpszName != 0)
			name.assign (message->lpOriginator->lpszName);
		if (message->lpOriginator != 0 && message->lpOriginator->lpszAddress != 0)
		{
			emailAddr.assign (message->lpOriginator->lpszAddress);
			if (!Email::IsValidAddress (emailAddr))
			{
				emailAddr.clear ();
			}
		}
	}
	_delete (_session.GetHandle (),	// Explicit session handle
			 0,						// Parent window handle or zero
			 messageId,				// Message ID
			 0,						// Flags; must be zero
			 0);					// Reserved; must be zero
}

bool SimpleMapi::Mailer::VerifyEmailAddress (std::string & emailAddr)
{
	if (emailAddr.empty ())
		return false;
	else
		return _addressBook.VerifyRecipient (emailAddr);
}

