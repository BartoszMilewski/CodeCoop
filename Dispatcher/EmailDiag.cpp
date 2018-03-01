//------------------------------------
//  (c) Reliable Software, 1999 - 2008
//------------------------------------

#include "precompiled.h"
#include "EmailDiag.h"
#include "EmailMan.h"
#include "DiagFeedback.h"
#include "Email.h"
#include "Registry.h"
#include "RegKeys.h"
#include "OutputSink.h"
#include "EmailRegistry.h"
#include "EmailMessage.h"

#include <Ex/WinEx.h>
#include <Ex/Error.h>
#include <StringOp.h>

Email::Diagnostics::Diagnostics (std::string const & myEmail,
								 DiagFeedback & feedback,
								 DiagnosticsProgress & progress)
	: _emailClientStatus (Email::NotTested),
	  _feedback (feedback),
	  _progress (progress),
	  _myEmail (myEmail)
{}

Email::Status Email::Diagnostics::Run (Email::Manager & emailMan)
{
	_feedback.Clear ();
	_progress.SetRange (0, 3, 1);
	Email::Status status = Email::NotTested;
	_emailClientStatus = Email::NotTested;
	try
	{
		// 1. check e-mail configuration 
		// e-mail should be valid if we are here (still we cannot assert on data read from registry).
		if (!emailMan.IsValid ())
			throw Win::InternalException ("Cannot run e-mail diagnostics. "
										  "The e-mail configuration is incorrect.");
		_progress.StepIt ();
		if (!CheckForCancel ())
			return Email::NotTested;

		// 2. outgoing e-mail first
		status = TestOutgoing (emailMan);

		_progress.StepIt ();
		if (!CheckForCancel ())
			return Email::NotTested;

		// 3. incoming e-mail
		Email::Status inStatus = TestIncoming (emailMan);
		if (inStatus != Email::Succeeded)
			status = inStatus;

		_progress.StepIt ();
		if (!CheckForCancel ())
			return Email::NotTested;
	}
	catch (Win::Exception e)
	{
		_feedback.Display (e.GetMessage ());
		_feedback.Display (e.GetObjectName ());
		if (e.GetError () != 0)
		{
			SysMsg sysMsg (e.GetError (), e.GetModuleHandle ());
			if (sysMsg.Text () != 0)
				_feedback.Display (sysMsg.Text ());
			std::string errMsg = "Error code: ";
			errMsg += ToHexString (e.GetError ());
			_feedback.Display (errMsg.c_str ());
		}
		status = Email::Failed;
		emailMan.ShutDown (); // ?
	}
	catch ( ... )
	{
		Win::ClearError ();
		_feedback.Display ("Unknown email test failure.");
		status = Email::Failed;
		emailMan.ShutDown (); // ?
	}

	_progress.Clear ();
	_feedback.Newline ();
	std::string info ("Email test: ");
	switch (status)
	{
	case Email::NotTested:
		info += "interrupted";
		break;
	case Email::Failed:
		info += "FAILED";
		break;
	case Email::Succeeded:
		info += "PASSED";
		break;
	}
	_feedback.Display (info.c_str ());
	return status;
}

Email::Status Email::Diagnostics::TestOutgoing (Email::Manager & emailMan)
{
	_feedback.Display ("Preparing to send a test message...");
	Email::Account const & account = emailMan.GetOutgoingAccount ();
	DisplayTechnologyInfo (account.GetImplementationId ());
	if (account.GetTechnology ().IsSmtp ())
	{
		SmtpAccount const & smtp = reinterpret_cast<SmtpAccount const &>(account);
		DisplayServerSettings (smtp.GetServer (), smtp.GetPort (), smtp.GetTimeout (), smtp.UseSSL ());
	}
	else
	{
		// using MAPI
		_feedback.Newline ();
		ExamineEmailClient ();
		if (_emailClientStatus != Email::Succeeded)
		{
			_feedback.Display ("The test cannot be continued.");
			return Email::Failed;
		}
	}

	_feedback.Display ("Establishing an e-mail session...");
	Mailer mailer (emailMan);
	OutgoingMessage msg;
	msg.SetSubject ("Code Co-op Test Message");
	msg.SetText ("This is a test message sent by Code Co-op Dispatcher"
				 "\r\nas a part of testing your e-mail."
				 "\r\n\r\nYou can delete this message."
				 "\r\n\r\nReliable Software Team");

	std::vector<std::string> recipVector;
	std::string info ("The test message will be sent to ");
	info += _myEmail;
	_feedback.Display (info.c_str ());
	recipVector.push_back (_myEmail);
	_feedback.Display ("Sending the test message...");
	mailer.Send (msg, recipVector);

	_feedback.Newline ();
	_feedback.Display ("The test message was sent successfully.");
	return Email::Succeeded;
}

Email::Status Email::Diagnostics::TestIncoming (Email::Manager & emailMan)
{
	_feedback.Newline ();
	std::string info ("Listing your e-mail inbox (first ");
	info += ToString (InboxMsgDisplayCount);
	info += " messages).";
	_feedback.Display (info.c_str ());
	Email::Account const * account = emailMan.FindIncomingAccount (std::string ());
	if (account == 0)
		throw Win::InternalException ("Dispatcher e-mail subsystem failure.\n"
									  "Please contact support@relisoft.com");

	DisplayTechnologyInfo (account->GetImplementationId ());
	if (account->GetTechnology ().IsPop3 ())
	{
		Pop3Account const * pop3 = reinterpret_cast<Pop3Account const *>(account);
		DisplayServerSettings (pop3->GetServer (), pop3->GetPort (), pop3->GetTimeout (), pop3->UseSSL ());
	}
	else
	{
		if (_emailClientStatus == Email::NotTested)
		{
			_feedback.Newline ();
			ExamineEmailClient ();
		}
		if (_emailClientStatus != Email::Succeeded)
		{
			_feedback.Display ("The test cannot be continued.");
			return Email::Failed;
		}
	}
	return ListInbox (emailMan);
}

Email::Status Email::Diagnostics::ListInbox (Email::Manager & emailMan)
{
	// only the default account
	InboxIterator reader (emailMan, std::string (), false); // all messages in the inbox
	_feedback.Newline ();
	unsigned inboxMsgCount = 0;
	for (; !reader.AtEnd () && (inboxMsgCount < InboxMsgDisplayCount); 
			reader.Advance ())
	{
		// Display subject
		std::string info ("    ");
		info += ToString (inboxMsgCount + 1);
		info += ". ";
		info += reader.GetSubject ();
		_feedback.Display (info.c_str ());
		if (!CheckForCancel ())
			return Email::NotTested;

		++inboxMsgCount;
	}
	if (!reader.AtEnd ())
	{
		for (; !reader.AtEnd () && (inboxMsgCount < MaxInboxMsgDisplayCount); 
				reader.Advance ())
		{
			if (!CheckForCancel ())
				return Email::NotTested;

			++inboxMsgCount;
		}
	}
	// Display message count
	if (!reader.AtEnd ())
	{
		std::string info ("    There are more than ");
		info += ToString (MaxInboxMsgDisplayCount);
		info += " messages.";
		_feedback.Display (info.c_str ());
	}
	else
	{
		std::string info ("    Message count: ");
		info += ToString (inboxMsgCount);
		_feedback.Display (info.c_str ());
	}
	_feedback.Newline ();
	_feedback.Display ("The inbox was listed successfully.");
	return Email::Succeeded;
}

void Email::Diagnostics::ExamineEmailClient ()
{
	_emailClientStatus = Email::Succeeded;
	Registry::DefaultEmailClient emailClient;
	std::string emailClientName = emailClient.GetName ();
	if (emailClientName.empty ())
	{
		_feedback.Display ("    Warning: No default MAPI client in the registry."
						   "\r\n    Use Windows Control Panel to set your"
						   "\r\n    default email program"
						   "\r\n    (see Internet Options>Programs>E-mail).");
	}
	else
	{
		std:: string info ("    Your default email client is: ");
		info += emailClientName;
		info += '.';
		_feedback.Display (info.c_str ());
		_feedback.Display ("    You can use Windows Control Panel to change it."
						   "\r\n    (see Internet Options>Programs>E-mail)\r\n");
		if (emailClientName == "Outlook Express")
		{
			_emailClientStatus = CheckIdentities ();
			if (_emailClientStatus == Email::Failed)
			{
				_feedback.Display ("    Different current and default identities"
								   "\r\n    prevent Code Co-op from retrieving e-mail"
								   "\r\n    messages from the Outlook Express inbox folder.");
			}
		}
	}
}

Email::Status Email::Diagnostics::CheckIdentities ()
{
	Registry::OutlookExpressIdentitiesCheck identitiesKey;
	if (!identitiesKey.Exists ())
		return Email::Succeeded;	// Outlook Express is not using identities

	Registry::OutlookExpressIdentities identities;
	RegKey::Seq keySequencer (identities.Key ());
	if (keySequencer.Count () <= 1)
		return Email::Succeeded;	// Just one identity defined -- cannot cause problems

	std::string currentIdentity = identities.GetCurrentIdentity ();
	std::string defaultIdentity = identities.GetDefaultIdentity ();
	if (currentIdentity.empty ())
	{
		// Current identity not set -- user didn't selected it yet.
		// Upon startup Outlook Express will ask the user to select identity.
		// The 'start as' identity will be suggested if present
		currentIdentity = identities.GetStartAsIdentity ();
		if (currentIdentity.empty ())
		{
			// There is no 'start as' identity defined, so the default identity will be suggested.
			currentIdentity = defaultIdentity;
		}
	}

	if (defaultIdentity != currentIdentity)
	{
		// We have a problem. When user starts Outlook Express he/she uses current identity, but
		// when Code Co-op via Simple Mapi browses Outlook Express message store, the default identity is used.
		// Since those identities are different Code Co-op is seeing different message store then the user.
		Registry::OutlookExpressIdentity defaultIdentityKey (defaultIdentity);
		std::string defaultUserName = defaultIdentityKey.GetUserName ();
		Registry::OutlookExpressIdentity currentIdentityKey (currentIdentity);
		std::string currentUserName = currentIdentityKey.GetUserName ();
		std::string info ("Your Outlook Express is configured to use multiple identities.\n"
						  "Outlook Express uses the identity '");
		info += currentUserName;
		info += "', however the default identity is set to '";
		info += defaultUserName;
		info += "'\n\n Code Co-op needs to change the default identity from '";
		info += defaultUserName;
		info += "' to '";
		info += currentUserName;
		info += "'.";
		Out::Answer userChoice = TheOutput.Prompt (info.c_str (), Out::PromptStyle (Out::OkCancel, Out::OK, Out::Question));
		if (userChoice == Out::Cancel)
			return Email::Failed;

		identities.SetDefaultIdentity (currentIdentity);
	}

	return Email::Succeeded;
}

void Email::Diagnostics::DisplayTechnologyInfo (std::string const & technology)
{
	std::string info ("Dispatcher is using ");
	info += technology;
	info += '.';
	_feedback.Display (info.c_str ());
}

void Email::Diagnostics::DisplayServerSettings (std::string const & server, 
												short port,
												int timeout,
												bool useSSL)
{
	std::string connInfo ("Connecting to ");
	connInfo += server;
	connInfo += " on port ";
	connInfo += ToString (port);
	if (useSSL)
		connInfo += " using SSL";
	connInfo +='.';
	_feedback.Display (connInfo.c_str ());

	std::string timeoutInfo ("Server timeout is set to ");
	timeoutInfo += ToString (timeout / 1000);
	timeoutInfo += " seconds.";
	_feedback.Display (timeoutInfo.c_str ());
}

bool Email::Diagnostics::CheckForCancel ()
{
	if (_progress.WasCanceled ())
	{
		_feedback.Display ("Test canceled by user.");
		_progress.Clear ();
		return false;
	}
	return true;
}
