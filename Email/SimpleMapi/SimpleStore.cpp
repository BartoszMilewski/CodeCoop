//
// (c) Reliable Software 1998 --2005
//

#include "precompiled.h"
#include "SimpleMapiMessage.h"
#include "SimpleStore.h"
#include "SimpleMapi.h"
#include "SimpleSession.h"
#include "SimpleRecipientList.h"
#include "EmailMessage.h"
#include "SimpleMapiEx.h"

#include <File/File.h>

using namespace SimpleMapi;

void Outbox::Submit (OutgoingMessage const & msg, RecipientList const & recipients, bool verbose)
{
	Message mailMsg (msg, recipients.GetMapiAddrList (), recipients.GetCount ());
	// Send message
	ULONG rCode;
	Send send;
	_session.GetFunction ("MAPISendMail", send);
	ULONG flags = verbose ? MAPI_DIALOG : 0; // If verbose request Simple MAPI send dialog for message late changes 
	rCode = send (_session.GetHandle (),
		0,
		mailMsg.ToNative (),
		flags,	
		0);
	if (rCode != SUCCESS_SUCCESS)
	{
		if (flags == MAPI_DIALOG && rCode == MAPI_E_USER_ABORT)
			throw Win::Exception ("Script sending aborted by the user");
		else
			throw SimpleMapi::Exception ("MAPI -- Cannot send message", rCode);
	}
}

void Outbox::Save (OutgoingMessage const & msg, RecipientList const & recipients)
{
	Message mailMsg (msg, recipients.GetMapiAddrList (), recipients.GetCount ());
	// Save message
	ULONG rCode;
	MapiSaveMail save;
	_session.GetFunction ("MAPISaveMail", save);
	char msgId [64];
	msgId [0] = '\0';
	rCode = save (_session.GetHandle (), 
		0, 
		mailMsg.ToNative (), 
		0, 
		0, 
		msgId);
	if (rCode != SUCCESS_SUCCESS)
	{
		throw SimpleMapi::Exception ("MAPI -- Cannot save message", rCode);
	}
}
