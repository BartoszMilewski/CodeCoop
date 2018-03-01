// ---------------------------
// (c) Reliable Software, 2005
// ---------------------------
#include "precompiled.h"
#include "SmtpMailer.h"
#include "EmailMessage.h"
#include <Mail/Base64.h>
#include <Mail/SmtpMessage.h>
#include <Mail/SmtpUtils.h>
#include <Mail/MsgTree.h>

Smtp::Mailer::Mailer (
		std::string const & server, 
		std::string const & user, 
		std::string const & password,
		std::string const & senderAddress,
		std::string const & domain)
	: _connection (server, 25, false),
	  _session (_connection, domain, false), // don't use SSL
	  _senderAddress (senderAddress)
{
	// this step is optional
	_session.Authenticate (user, password);
}

void Smtp::Mailer::Send (
	OutgoingMessage const & msg, 
	std::vector<std::string> const & addrVector, 
	std::string const & toFieldLabel)
{
	// To: field
	std::string toField;
	if (!toFieldLabel.empty ())
	{
//		toField = toFieldLabel + ":;";
	}

	Socket & socket = _connection.GetSocket ();
	// ================
	// SMTP: MESSAGE ENVELOPE
	// Sender
	std::string mailFromCmd = "MAIL FROM: " + ToBracketedString (_senderAddress);
    socket.SendLine (mailFromCmd.c_str ());
	Smtp::Response mailfromResponse (socket);
	if (mailfromResponse.GetStatus () != Smtp::Response::Ok)
		throw Win::Exception ("SMTP error: cannot initiate send transaction.", mailfromResponse.c_str ());

	// SMTP: MESSAGE ENVELOPE
	// Recipient(s)
	// Revisit: a list of recipients cannot be arbitrary long!
	// Servers are obliged to handle at least 100 recipients
	for (unsigned int i = 0; i < addrVector.size (); ++i)
	{
		// RCPT TO:
		std::string bracketedAddr = ToBracketedString (addrVector [i]);
		std::string rcptToCmd = "RCPT TO: " + bracketedAddr;
		socket.SendLine (rcptToCmd.c_str ());
		Smtp::Response rcpttoResponse (socket);
		if (rcpttoResponse.GetStatus () != Smtp::Response::Ok)
			throw Win::Exception ("SMTP error: server refused email address of recipient.", rcpttoResponse.c_str ());
	}

	// ===============
	// SMTP: MESSAGE CONTENT
    socket.SendLine ("DATA");
	Smtp::Response dataResponse (socket);
	if (dataResponse.GetStatus () != Smtp::Response::DataCmdOk)
		throw Win::Exception ("SMTP error: server refused accepting message.", dataResponse.c_str ());

	Smtp::Message smtpMsg ("Bartosz Milewski", _senderAddress, msg.GetSubject ());
	if (!msg.UseBccRecipients ())
		smtpMsg.SetToField (addrVector);

	if (msg.GetAttachmentCount () > 0)
	{
		// Body
		std::auto_ptr<MultipartMixedPart> body (new MultipartMixedPart ());

		// Text part
		std::auto_ptr<MessagePart> plainText (new PlainTextPart (msg.GetText ()));
		body->AddPart (plainText);
		
		// Binary parts
		for (OutgoingMessage::AttPathIter attIter = msg.AttPathBegin ();
			attIter != msg.AttPathEnd ();
			++attIter)
		{
			std::auto_ptr<MessagePart> attachment (
				new ApplicationOctetStreamPart (*attIter));
			body->AddPart (attachment);
		}

		smtpMsg.AddBody (std::auto_ptr<MessagePart>(body));
	}
	else
	{
		std::auto_ptr<MessagePart> plainText (new PlainTextPart (msg.GetText ()));
		smtpMsg.AddBody (plainText);
	}

	smtpMsg.Send (socket);

	Smtp::Response dataendResponse (socket);
	if (dataendResponse.GetStatus () != Smtp::Response::Ok)
		throw Win::Exception ("SMTP error: server did not accept message.", dataendResponse.c_str ());
}
