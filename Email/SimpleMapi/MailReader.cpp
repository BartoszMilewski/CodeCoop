//-----------------------------------------
// (c) Reliable Software 1998 -- 2004
//-----------------------------------------

#include "precompiled.h"
#include "MailReader.h"
#include "SimpleMapiEx.h"

#include <LightString.h>
#include <File/Path.h>
#include <File/File.h>
#include <File/SafePaths.h>
#include <Dbg/Assert.h>

using namespace SimpleMapi;

MailboxIterator::MailboxIterator (Session & session, bool unreadOnly)
	: _session (session),
	  _done (false),
	  _findFlags (	MAPI_GUARANTEE_FIFO	// Search in the order of time received
				  | MAPI_LONG_MSGID)	// Message ID can be long up to 512 chars
{
	if (unreadOnly)
		_findFlags |= MAPI_UNREAD_ONLY;		// Search only unread messages

	// GetFunction checks if the function has been found in the MAPI DLL
	_session.GetFunction ("MAPIFindNext", _findNext);
	_session.GetFunction ("MAPIReadMail", _read);
	_session.GetFunction ("MAPIDeleteMail", _delete);
	_session.GetFunction ("MAPIFreeBuffer", _free);
	_messageID [0] = '\0';
	// Find first message
	Advance ();
}

void MailboxIterator::Advance ()
{
	Assert (!_done);
	int retriesLeft = 2;
	ULONG rCode = SUCCESS_SUCCESS;
	do
	{
		Assert (retriesLeft > 0);
		rCode = _findNext (_session.GetHandle (),	// Explicit session handle
							0,						// Parent window handle or zero
							0,						// Zero specifies interpersonal messages
							_messageID,				// Seed message ID
							_findFlags,
							0,						// Reserved; must be zero
							_messageID);			// Buffer for found message ID
		switch (rCode)
		{
		case MAPI_E_NO_MESSAGES:
		case MAPI_E_INVALID_MESSAGE:
			// No messages
			_done = true;
			retriesLeft = 0;
			break;
		case MAPI_E_FAILURE:
			// Clear MAPI_UNREAD_ONLY flag, because some simple MAPI
			// implementations don't support restrictions on message tables
			// Lotus Notes doesn't support restrictions
			_findFlags &= ~MAPI_UNREAD_ONLY;
			--retriesLeft;
			break;
		case MAPI_E_NOT_SUPPORTED:
			// Clear MAPI_GUARANTEE_FIFO, because some simple MAPI
			// implementations cannot honor this request
			_findFlags &= ~MAPI_GUARANTEE_FIFO;
			--retriesLeft;
			break;
		case SUCCESS_SUCCESS:
			retriesLeft = 0;
			break;
		default:
			throw SimpleMapi::Exception ("MAPI -- Cannot browse incoming messages", rCode);
			break;
		}
	} while (retriesLeft > 0);

	if (!_done && rCode == SUCCESS_SUCCESS)
		RetrieveMessage ();
	else if ( rCode != MAPI_E_NO_MESSAGES && rCode != MAPI_E_INVALID_MESSAGE)
		throw SimpleMapi::Exception ("MAPI -- Cannot browse incoming messages", rCode);
}

void MailboxIterator::Seek (std::string const & msgId) throw ()
{
	strcpy (_messageID, msgId.c_str ());
}

void MailboxIterator::RetrieveMessage ()
{
	RetrievedMessage message (_free);
	ULONG rCode = _read (_session.GetHandle (),	// Explicit session handle
						0,						// Parent window handle or zero
						_messageID,				// Message ID
						MAPI_ENVELOPE_ONLY	|	// Read only the message header
						MAPI_PEEK,				// Do not mark message as read
						0,						// Reserved; must be zero
						message.GetAddr ());	// Message read

	if (rCode != SUCCESS_SUCCESS)
		throw SimpleMapi::Exception ("MAPI -- Cannot read mail message envelope", rCode);

	_msgSubject.clear ();
	if (message.IsRetrieved () && message->lpszSubject != 0)
	{
		// Message has subject
		_msgSubject.assign (message->lpszSubject);
	}
}

void MailboxIterator::RetrieveAttachements (SafePaths & attPaths)
{
	// Read the whole message but don't mark it as read
	RetrievedMessage message (_free);
	// Save attachements
	ULONG rCode = _read (_session.GetHandle (),	// Explicit session handle
						 0,						// Parent window handle or zero
						 _messageID,			// Message ID
						 MAPI_PEEK,				// Do not mark message as read
						 0,						// Reserved; must be zero
						 message.GetAddr ());	// Message read
	if (rCode != SUCCESS_SUCCESS)
		throw SimpleMapi::Exception ("MAPI -- Cannot read mail message", rCode);
	if (message.IsRetrieved () && message->lpFiles != 0)
	{
		for (unsigned int i = 0; i < message->nFileCount; i++)
		{
			char const * from = message->lpFiles [i].lpszPathName;
			if (from != 0)
				attPaths.Remember (from);
		}
	}
}

void MailboxIterator::GetSender (std::string & name, std::string & email) const
{
	// Read the envelope only and don't mark it as read
	RetrievedMessage message (_free);
	ULONG rCode = _read (_session.GetHandle (),	// Explicit session handle
						 0,						// Parent window handle or zero
						 _messageID,			// Message ID
					     MAPI_ENVELOPE_ONLY	|	// Read only the message header
					     MAPI_PEEK,				// Do not mark message as read
						 0,						// Reserved; must be zero
						 message.GetAddr ());	// Message read
	if (rCode != SUCCESS_SUCCESS)
		throw SimpleMapi::Exception ("MAPI -- Cannot read message envelope", rCode);
	if (message.IsRetrieved ())
	{
		// Save sender data
		if (message->lpOriginator != 0 && message->lpOriginator->lpszName != 0)
			name.assign (message->lpOriginator->lpszName);
		if (message->lpOriginator != 0 && message->lpOriginator->lpszAddress != 0)
			email.assign (message->lpOriginator->lpszAddress);
	}
}

// return true if client can continue iteration
bool MailboxIterator::DeleteMessage () throw ()
{
	{
		RetrievedMessage message (_free);
		// Read message to mark it as read -- ignore errors
		_read (_session.GetHandle (),	// Explicit session handle
			   0,						// Parent window handle or zero
			   _messageID,				// Message ID
			   MAPI_ENVELOPE_ONLY,		// Read only the message header and mark it as read
			   0,						// Reserved; must be zero
			   message.GetAddr ());		// Message read
	}
	_delete (_session.GetHandle (),		// Explicit session handle
			 0,							// Parent window handle or zero
			 _messageID,				// Message ID
			 0,							// Flags; must be zero
			 0);						// Reserved; must be zero
	// Message ID no longer valid
	_messageID [0] = '\0';
	return false;
}
