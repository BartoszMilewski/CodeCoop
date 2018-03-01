// ------------------------------
// (c) Reliable Software, 2005-06
// ------------------------------

#include <precompiled.h>
#undef DBG_LOGGING
#define DBG_LOGGING false

#include "SmtpMailer.h"
#include "EmailMessage.h"
#include "BadEmailExpt.h"
#include "OutputSink.h"
#include "Registry.h"
#include <Mail/Smtp.h>
#include <Mail/SmtpMessage.h>

extern std::string ToBracketedString (std::string const & str);

SmtpMailer::SmtpMailer (
				Smtp::Session & session, 
				std::string const & user, 
				std::string const & senderName,
				std::string const & senderAddress)
	: _session (session),
	  _user (user),
	  _senderName (senderName),
	  _senderAddress (senderAddress)
{
	Registry::UserDispatcherPrefs prefs;
	_isLogging = prefs.IsEmailLogging ();
}

// Adds debug logging to any socket
class DbgSocket: public Socket
{
public:
	DbgSocket (Socket & socket)
		: _socket (socket)
	{}
	
	void Connect (std::string const & host, short port, int timeout = 0)
	{
		_socket.Connect (host, port, timeout);
	}

	unsigned int Receive (char buf [], unsigned int size)
	{
		return _socket.Receive (buf, size);
	}

	std::string const & GetHostName () const { return _socket.GetHostName (); }
	short GetHostPort () const { return _socket.GetHostPort (); }

	void Send (char const * buf, unsigned int len)
	{
		std::string str (buf, len);
		dbg << "<!" << str << "!>\n";
		_socket.Send (buf, len);
	}
private:
	Socket & _socket;
};

void SmtpMailer::Send (OutgoingMessage & msg, std::vector<std::string> const & addrVector)
{
	Socket & socket = _session.GetSocket ();
	// Use this for debugging
	// DbgSocket socket (_session.GetSocket ());
	// ==============================================
	// SMTP: MESSAGE ENVELOPE
	Smtp::MailTransaction transact (socket);
	// Sender
	std::string mailFromCmd = "MAIL FROM: " + ToBracketedString (_senderAddress);
	socket.SendLine (mailFromCmd.c_str ());
	Smtp::Response mailfromResponse (socket);
	if (mailfromResponse.GetStatus () != Smtp::Response::Ok)
		throw Win::InternalException ("SMTP error: cannot initiate send transaction.", 
									  mailfromResponse.c_str ());
	// Recipient(s)
	// Revisit: a list of recipients cannot be arbitrary long!
	// Servers are obliged to handle at least 100 recipients
	for (unsigned int i = 0; i < addrVector.size (); ++i)
	{
		std::string const & currAddress = addrVector [i];
		std::string bracketedAddr = ToBracketedString (currAddress);
		// RCPT TO:
		std::string rcptToCmd = "RCPT TO: " + bracketedAddr;
		socket.SendLine (rcptToCmd.c_str ());
		Smtp::Response rcpttoResponse (socket);
		int status = rcpttoResponse.GetStatus ();
		if (status != Smtp::Response::Ok)
		{
			if (status == Smtp::Response::TooManyRecipients ||
				status == Smtp::Response::TooMuchMailData)
			{
				throw Win::InternalException ("SMTP error: there are too many message recipients.", rcpttoResponse.c_str ());
			}
			else if (status == Smtp::Response::BadEmailAddress)
			{
				std::string info = "to: " + currAddress;
				info += "\n\n";
				info += rcpttoResponse.GetComment ();
				throw Email::BadEmailException ("Cannot e-mail a script", info.c_str (), currAddress);
			}
			else
			{
				throw Win::InternalException ("SMTP error: recipient not accepted by the server.", rcpttoResponse.c_str ());
			}
		}
	}

	// SMTP: MESSAGE CONTENT
	socket.SendLine ("DATA");
	Smtp::Response dataResponse (socket);
	if (dataResponse.GetStatus () != Smtp::Response::DataCmdOk)
		throw Win::InternalException ("SMTP error: server refused accepting message.", dataResponse.c_str ());

	Smtp::Message smtpMsg (_senderName, _senderAddress, msg.GetSubject ());
	if (!msg.UseBccRecipients ())
	{
		smtpMsg.SetToField (addrVector);
	}

	if (msg.GetAttachmentCount () > 0)
	{
		// Body
		std::unique_ptr<MultipartMixedPart> body (new MultipartMixedPart ());

		// Text part
		std::unique_ptr<MessagePart> plainText (new PlainTextPart (msg.GetText ()));
		body->AddPart (std::move(plainText));

		// Binary parts
		for (OutgoingMessage::AttPathIter attIter = msg.AttPathBegin ();
			attIter != msg.AttPathEnd ();
			++attIter)
		{
			std::string const & attPath = *attIter;
			std::unique_ptr<MessagePart> attachment (
				new ApplicationOctetStreamPart (attPath));
			body->AddPart (std::move(attachment));
			if (_isLogging)
			{
				// save script attachments for debugging purposes
				std::string attFilename = msg.GetSubject ();
				attFilename += ".znc";
				TheOutput.LogFile (attPath, attFilename, "SMTP Script Log");
			}
		}

		smtpMsg.AddBody (std::move(body));
	}
	else
	{
		std::unique_ptr<MessagePart> plainText (new PlainTextPart (msg.GetText ()));
		smtpMsg.AddBody (std::move(plainText));
	}

	smtpMsg.Send (socket);

	Smtp::Response dataendResponse (socket);
	if (dataendResponse.GetStatus () != Smtp::Response::Ok)
		throw Win::InternalException ("SMTP error: server did not accept message.", dataendResponse.c_str ());

	transact.Commit ();
	// ==============================================
	if (_isLogging)
	{
		// save script note for debugging purposes
		std::string scriptNote (msg.GetSubject ());
		scriptNote += "\n\t";
		scriptNote += msg.GetText ();
		scriptNote += "\n\t";
		for (unsigned int i = 0; i < addrVector.size (); ++i)
		{
			scriptNote += addrVector [i];
			scriptNote += "\n\t";
		}
		TheOutput.LogNote ("SmtpScripts.log", scriptNote);
	}
}

void SmtpMailer::Save (OutgoingMessage & msg, std::vector<std::string> const & addrVector)
{
	Send (msg, addrVector);
}

void SmtpMailer::GetLoggedUser (std::string & name, std::string & emailAddr)
{
	name = _user;
	emailAddr = _senderAddress;
}

bool SmtpMailer::VerifyEmailAddress (std::string & emailAddr)
{
	return true;
}
