// ---------------------------------
// (c) Reliable Software 2003 - 2006
// ---------------------------------
#include "precompiled.h"
#include "WinOut.h"
#include "AccountDlg.h"
#include "RegFunc.h"
#include "MailToDlg.h"
#include "EmailMessage.h"
#include "ScriptSubject.h"
#include "SmtpMailer.h"
#include "TestGlobal.h"
#include "EmailConfig.h"
#include "EmailAccount.h"

#include <Mail/HeaderSeq.h>
#include <Mail/Pop3.h>
#include <Win/Dialog.h>
#include <Net/Socket.h>
#include <Sys/Crypto.h>
#include <Mail/Base64.h>
#include <Mail/Mime.h>
#include <Mail/Smtp.h>
#include <StringOp.h>
#include <File/FileIo.h>
#include <File/Path.h>
#include <File/Dir.h>
#include <File/SafePaths.h>
#include <File/SerialUnVector.h>
#include <Sys/PackedTime.h>
#include <File/MemFile.h>
#include <Mail/MsgTree.h>
#include <Mail/MsgParser.h>
#include <Mail/Pop3Message.h>
#include <Mail/SmtpMessage.h>
#include <Mail/SmtpUtils.h>

namespace Pop3
{
	// Returns lines until end marker found (a period on a line by itself)
	// Ignores byte-stuffing leading periods
	// After every line, the stream is positioned at the beginning of next line
	// (it can be used to continue parsing)
	class LineSeq: public ::LineSeq
	{
	public:
		LineSeq (BufferedStream & stream) 
		: _stream (stream)
		{
			Advance ();
		}
		void Advance ()
		{
			Assert (!AtEnd ());
			_line.clear ();
			char c = _stream.GetChar ();
			if (c == '.')
			{
				// Eat leading period, it's either byte stuffing or end marker.
				c = _stream.GetChar ();
				// Is it end marker?
				if (c == '\r' && _stream.LookAhead () == '\n')
				{
					// eat LF
					_stream.GetChar ();
					_done = true;
					return;
				}
			}

			for (;;)
			{
				// End of line?
				if (c == '\r' && _stream.LookAhead () == '\n')
				{
					// eat LF
					_stream.GetChar ();
					break;
				}
				_line += c;
				c = _stream.GetChar ();
			}
		}
	private:
		BufferedStream & _stream;
	};
}

void DumpFile (std::string const & fileName, WinOut & out)
{
 	if (!File::Exists (fileName.c_str ()))
	{
		out.PutBoldLine ("File to be dumped does not exist");
		out.PutBoldLine (fileName.c_str ());
		out.PutBoldLine ("Please provide this file for tests");
		return;
	}
	FileIo file (fileName, File::ReadOnlyMode ());
	unsigned long len;
	do
	{
		len = 32;
		unsigned char buf [32];
		file.FillBuf (buf, len);
		std::string outstr;
		for (unsigned long i = 0; i < len; ++i)
		{
			outstr += ToString (static_cast<unsigned> (buf [i]));
			outstr += ", ";
		}
		out.PutLine (outstr.c_str ());
	} while (len != 0);
}

void PrintHeader (HeaderSeq & hdr, WinOut & out)
{
	std::string line (hdr.GetName ());
	line += ": ";
	line += hdr.GetValue ();
	line += "; ";
	line.append (hdr.GetComment ());
	out.PutLine (line.c_str ());
}

// Stream an email message file
class MsgFileStream: public BufferedStream
{
public:
	MsgFileStream (FileIo & fileIo, unsigned bufSize)
		: BufferedStream (bufSize), _fileIo (fileIo)
	{}
protected:
	void FillBuf ();

	FileIo & _fileIo;
};

void MsgFileStream::FillBuf ()
{
	_bufPos = 0;
	unsigned long bytes = BufferSize ();
	_fileIo.FillBuf (GetBuf (), bytes);
	if (bytes == 0)
	{
		Win::ClearError ();
		_buf [0] = '.';
		_buf [1] = '\r';
		_buf [2] = '\n';
		bytes = 3;
	}
	_bufEnd = bytes;
}

void MimeHeaders (LineSeq & seq, MIME::Headers & headers, WinOut & out);
void DecodeBase64 (BufferedStream & stream, std::string const & fileName);

std::string GetBoundary (MIME::Headers const & headers)
{
	MIME::Headers::Attributes const & typeAttr = headers.GetTypeAttributes ();
	MIME::Headers::Attributes::const_iterator result = typeAttr.find ("boundary");
	if (result == typeAttr.end ())
		throw Win::InternalException ("POP3: Corrupted message headers. No boundary defined.");
	
	std::string boundary = result->second;					
	boundary.insert (0, "--");
	return boundary;
}

std::string GetAttName (MIME::Headers const & headers)
{
	MIME::Headers::Attributes const & dispositionAttr = headers.GetDispositionAttributes ();
	MIME::Headers::Attributes::const_iterator result = dispositionAttr.find ("filename");
	std::string attFilename;
	if (result == dispositionAttr.end ())
	{
		MIME::Headers::Attributes const & typeAttr = headers.GetTypeAttributes ();
		MIME::Headers::Attributes::const_iterator result2 = typeAttr.find ("name");
		if (result2 == typeAttr.end ())
		{
			throw Win::InternalException ("POP3: Unrecognized message syntax. "
				"No name defined for an attachment.");
		}
		else
		{
			attFilename = result2->second;
		}
	}
	else
	{
		attFilename = result->second;
	}
	if (attFilename.empty ())
		throw Win::InternalException ("POP3: Unrecognized message syntax. "
		"Attachment name cannot be empty.");

	return attFilename;
}

void DecodeMultiPart (BufferedStream & stream, std::string const & boundary, WinOut & out)
{
	Pop3::LineSeq seq (stream);
	unsigned boundaryLen = boundary.size ();
	while (!seq.AtEnd ())
	{
		std::string const line = seq.Get ();
		unsigned lineLen = line.size ();
		if (line.compare (0, boundaryLen, boundary) == 0)
		{
			// Multi-part MIME boundary found
			if (lineLen >= boundaryLen + 2 && line [boundaryLen] == '-' && line [boundaryLen + 1] == '-')
			{
				// Closing boundary
				out.PutBoldLine ("BOUNDARY END");
				seq.Advance ();
				break;
			}
			else
			{
				out.PutBoldLine ("BOUNDARY");
				seq.Advance ();
				// Parse MIME headers
				MIME::Headers headers;
				MimeHeaders (seq, headers, out);

				if (headers.IsMultiPart ())
				{
					// Recurse
					DecodeMultiPart (stream, GetBoundary (headers), out);
				}
				else if (headers.IsApplication () 
					&& headers.IsOctetStream () 
					&& headers.IsBase64 ())
				{
					std::string name = GetAttName (headers);
					if (name.empty ())
					{
						out.PutBoldLine ("Attachment without a name!");
					}
					else
					{
						std::string fileName ("decoded-");
						fileName += name;
						std::string msg ("Decoding attachment and saving it as: ");
						msg += fileName;
						out.PutBoldLine (msg.c_str ());
						DecodeBase64 (stream, fileName);
						DumpFile (fileName, out);
						out.PutLine ("----original----");
						DumpFile ("Scripts.znc", out);
					}
				}
			}
		}
		else
		{
			// Not a MIME boundary
			// out.PutLine (seq.Get ().c_str ());
			seq.Advance ();
		}
	}
}

void DecodeBase64 (BufferedStream & stream, std::string const & fileName)
{
	Base64::Decoder decoder;
	Pop3::Input input (stream);
	unmovable_vector<char> dest;
	SerialUnVector<char>::Output output (dest);
	decoder.Decode (input, output);
	FileIo outFile (fileName, File::OpenAlwaysMode ());
	unmovable_vector<char>::const_iterator it = dest.begin ();
	unmovable_vector<char>::const_iterator end = dest.end ();
	while (it != end)
	{
		unsigned char buf [128];
		unsigned long i = 0;
		do
		{
			buf [i] = *it;
			++i;
			++it;
		} while (it != end && i < 128);

		outFile.Write (buf, i);
	}
}

void MimeHeaders (LineSeq & seq, MIME::Headers & headers, WinOut & out)
{
	for (HeaderSeq hdr (seq); !hdr.AtEnd (); hdr.Advance ())
	{
		if (hdr.IsName ("Subject"))
		{
			out.PutLine (hdr.GetValue ().c_str ());
		}
		else if (hdr.IsName ("MIME-Version"))
		{
			out.PutLine (hdr.GetValue ().c_str ());
		}
		else if (hdr.IsName ("Content-Type"))
		{
			out.PutLine (hdr.GetValue ().c_str ());
			headers.SetType (hdr.GetValue (), hdr.GetComment ());
		}
		else if (hdr.IsName ("Content-Transfer-Encoding"))
		{
			out.PutLine (hdr.GetValue ().c_str ());
			headers.SetEncoding (hdr.GetValue ());
		}
		else if (hdr.IsName ("Content-Disposition"))
		{
			headers.SetDisposition (hdr.GetValue (), hdr.GetComment ());
		}
		else
		{
			out.PutBoldLine (hdr.GetName ().c_str ());
		}
	}
}

int TestMessageDecoding (WinOut & out)
{
	FileSeq fseq ("*.eml");
	if (fseq.AtEnd ())
	{
		out.PutLine ("Cannot find email message, *.eml in:");
		CurrentFolder current;
		out.PutLine (current.GetDir ());
		out.PutLine ("You have to copy one there to run this test");
		return -1;
	}
	else
	{
		out.PutLine ("Decoding email message file:");
		out.PutLine (fseq.GetName ());
	}
	FileIo file (fseq.GetName (), File::ReadOnlyMode ());
	MsgFileStream stream (file, 1024);
	Pop3::LineSeq seq (stream);
	MIME::Headers headers;
	MimeHeaders (seq, headers, out);

	if (headers.IsMultiPart ())
	{
		DecodeMultiPart (stream, GetBoundary (headers), out);
	}
	else if (headers.IsApplication () 
		  && headers.IsOctetStream () 
		  && headers.IsBase64 ())
	{
		std::string name = GetAttName (headers);
		if (name.empty ())
		{
			out.PutBoldLine ("Attachment without a name!");
		}
		else
		{
			std::string fileName ("decoded-");
			fileName += name;
			std::string msg ("Decoding attachment and saving it as: ");
			msg += fileName;
			out.PutBoldLine (msg.c_str ());
			DecodeBase64 (stream, fileName);
			DumpFile (fileName, out);
			out.PutLine ("----original----");
			DumpFile ("Scripts.znc", out);
		}
	}

	return 0;
}

void TestEncryption (WinOut & output)
{
	Crypt::String crypt ("destroy before reading", "test");
	output.PutLine (crypt.GetPlainText ());
	unsigned size;
	unsigned char const * data = crypt.GetData (size);
	output.PutLine ("Encrypted");
	Crypt::String decrypt (data, size);
	output.PutLine ("Decrypted");
	output.PutLine (decrypt.GetPlainText ());
	Assert (strcmp (crypt.GetPlainText (), decrypt.GetPlainText ()) == 0);
}

void TestFullEncryption (WinOut & output)
{
#if 0
	std::string text ("This is a secret text!");
#else
	std::string path ("PopTest.cpp");
	MemFileReadOnly in (path);
	char const * textBuf = in.GetBuf ();
	std::string text;
	std::copy (textBuf, textBuf + in.GetBufSize (), std::back_inserter (text));
#endif
	std::string cypher;
	std::string password ("secret");
	{
		Crypt::Streamer streamer (password);
		output.PutLine (text.c_str ());
		streamer.SetInput (&text [0], text.size ());
		std::ostringstream out;
		streamer.Encrypt (out);
		cypher = out.str ();
	}
	output.PutLine (cypher.c_str ());
	std::string decoded;
	{
		Crypt::Streamer streamer (password);
		streamer.SetInput (&cypher [0], cypher.size ());
		std::ostringstream out;
		streamer.Decrypt (out);
		decoded = out.str ();
	}
	output.PutLine (decoded.c_str ());
	Assert (decoded.size () == in.GetBufSize ());
	Assert (decoded == text);
}

int TestPOP (WinOut & out)
{
	std::string server, user, password;
	unsigned long options = 0;
	Pop3Data data;
	std::string name;
	Email::RegConfig emailCfg;
	Pop3Account pop3Account (emailCfg, name);
	server = pop3Account.GetServer();
	user = pop3Account.GetUser();
	password = pop3Account.GetPassword();
	name = pop3Account.GetName();
	data.Init (name, server, user, password, password);

	Pop3Ctrl ctrl (&data);
	Dialog::Modal dlg (0, ctrl);
	if (!dlg.IsOK ())
	{
		out.PutLine ("Test aborted");
		return -1;
	}
/*
	Registry::SetPOP3ServerInfo (
							data.GetName (),
							data.GetServer (),
							data.GetUser (),
							data.GetPassword (),
							options);
*/
	out.PutLine (data.GetName ().c_str ());
	out.PutLine (data.GetServer ().c_str ());
	out.PutLine (data.GetUser ().c_str ());
	out.PutLine (data.GetPassword ().c_str ());

	WinSocks socks;
	SimpleSocket socket;

	out.PutBoldLine ("Connecting to server");
	socket.Connect (data.GetServer (), 110);
	Pop3::SingleLineResponse connResponse (socket);
	if (connResponse.IsError ())
		throw Win::Exception ("Pop3 connecting error", connResponse.GetError ().c_str ());
	out.PutLine (connResponse.c_str ());

	std::string userStr ("user ");
	userStr += data.GetUser ();
	out.PutBoldLine (userStr.c_str ());
    socket.SendLine (userStr);
	Pop3::SingleLineResponse usercmdResponse (socket);
	if (usercmdResponse.IsError ())
		throw Win::Exception ("Pop3 login error", usercmdResponse.GetError ().c_str ());
	out.PutLine (usercmdResponse.c_str ());

	std::string passwordStr ("pass ");
	passwordStr += data.GetPassword ();
	out.PutBoldLine (passwordStr.c_str ());
    socket.SendLine (passwordStr);
	Pop3::SingleLineResponse passcmdResponse (socket);
	if (passcmdResponse.IsError ())
		throw Win::Exception ("Pop3 login error", passcmdResponse.GetError ().c_str ());
	out.PutLine (passcmdResponse.c_str ());

	// Status (how many messages)
	out.PutBoldLine ("stat");
    socket.SendLine ("stat");
	Pop3::SingleLineResponse statResponse (socket);
	if (statResponse.IsError ())
		throw Win::Exception ("Pop3 status error", statResponse.GetError ().c_str ());
	out.PutLine (statResponse.c_str ());

	unsigned msgCount = ToInt (statResponse.Get ());

	// Listing of messages
	out.PutBoldLine ("list");
    socket.SendLine ("list");

	Pop3::MultiLineResponse listResponse (socket, 1024);
	if (listResponse.IsError ())
		throw Win::Exception ("Pop3 listing error", listResponse.GetError ().c_str ());
	out.PutLine (listResponse.Get ().c_str ());
	listResponse.Advance ();
	out.PutLine ("--------------------------");
	while (!listResponse.AtEnd ())
	{
		out.PutLine (listResponse.Get ().c_str ());
		listResponse.Advance ();
	}
	out.PutLine ("--------------------------");

	if (msgCount > 0)
	{
		// List message
		//out.PutBoldLine ("list 1");
		//socket.SendLine ("list 1");
		//line = socket.ReceiveLine();
		//out.PutLine (line.c_str ());

		// Retrieve complete message
		out.PutBoldLine ("retr 1");
		socket.SendLine ("retr 1");

		Pop3::MultiLineResponse retrResponse (socket, 100000);
		if (retrResponse.IsError ())
			throw Win::Exception ("Pop3 retrieval error", retrResponse.GetError ().c_str ());
		out.PutLine (retrResponse.Get ().c_str ());
		retrResponse.Advance ();

		out.PutLine ("--------------------------");
		for (HeaderSeq hdr (retrResponse); !hdr.AtEnd (); hdr.Advance ())
		{
			PrintHeader (hdr, out);
		}
		out.PutLine ("--------------------------");
		while (!retrResponse.AtEnd ())
		{
			std::string const & line = retrResponse.Get ();
			out.PutLine (line.c_str ());
			retrResponse.Advance ();
		}

		out.PutLine ("--------------------------");

		//out.PutBoldLine ("top 1 0");
		//socket.SendLine ("top 1 0");
		//PrintLines (socket, out);
#if 0
		out.PutBoldLine ("dele 3");
		socket.SendLine ("dele 3");
		Pop3::SingleLineResponse deleResponse (socket);
		if (deleResponse.IsError ())
			throw Win::Exception ("Pop3 status error", deleResponse.GetError ().c_str ());
		out.PutLine (deleResponse.c_str ());
#endif
	}

	out.PutBoldLine ("quit");
    socket.SendLine ("quit");
	Pop3::SingleLineResponse quitResponse (socket);
	out.PutLine (quitResponse.c_str ());

	return 0;
}

int TestPopSsl (WinOut & out)
{
	char const * server = "blunkmicro.com";
	int port = 995;
	char const * user = "bartosz";
	char const * password = "aOLX3dQbxnoqTHL";
	WinSocks	socks; // Initialize socket system
	Pop3::Connection	connection (server, port);
	Pop3::Session		session (connection, user, password, true); // use SSL
	return 0;
}

int TestSecureSmtp (WinOut & out)
{
	std::string server = "smtp.gmail.com";
	short port = 25; // 465;
	bool useSSL = true;
	std::string domain = "Code Co-op";

	bool needsAuth = true;
	std::string user = "bartoszorama@gmail.com";
	std::string pass = "Chleb1owy";

	std::string senderName = "Bartosz Milewski";
	std::string senderAddress = "bartoszorama@gmail.com";

	std::string recipientAddress = "bartosz@relisoft.com";

	WinSocks	socks; // Initialize socket system

	Smtp::Connection connection (server, port);
	Smtp::Session session (connection, domain, useSSL);
	if (needsAuth)
		session.Authenticate (user, pass);
	Socket & socket = session.GetSocket ();
	Smtp::MailTransaction transact (socket);

	std::string mailFromCmd = "MAIL FROM: ";
	mailFromCmd += ToBracketedString (senderAddress);
	socket.SendLine (mailFromCmd.c_str ());
	Smtp::Response mailfromResponse (socket);
	if (mailfromResponse.GetStatus () != Smtp::Response::Ok)
		throw Win::InternalException ("SMTP error: cannot initiate send transaction.", 
									  mailfromResponse.c_str ());

	std::string bracketedAddr = ToBracketedString (recipientAddress);
	// RCPT TO:
	std::string rcptToCmd = "RCPT TO: ";
	rcptToCmd += bracketedAddr;
	socket.SendLine (rcptToCmd.c_str ());
	Smtp::Response rcpttoResponse (socket);
	int status = rcpttoResponse.GetStatus ();
	if (status != Smtp::Response::Ok)
	{
		if (status == Smtp::Response::BadEmailAddress)
		{
			std::string info = "to: " + recipientAddress;
			info += "\n\n";
			info += rcpttoResponse.GetComment ();
			throw Win::InternalException ("Cannot e-mail a script", info.c_str ());
		}
		else
		{
			throw Win::InternalException ("SMTP error: recipient not accepted by the server.", rcpttoResponse.c_str ());
		}
	}

	// SMTP: MESSAGE CONTENT
	socket.SendLine ("DATA");
	Smtp::Response dataResponse (socket);
	if (dataResponse.GetStatus () != Smtp::Response::DataCmdOk)
		throw Win::InternalException ("SMTP error: server refused accepting message.", dataResponse.c_str ());

	Smtp::Message smtpMsg (senderName, senderAddress, "Test Message");

	std::auto_ptr<MessagePart> plainText (new PlainTextPart ("This is a test."));
	smtpMsg.AddBody (plainText);

	smtpMsg.Send (socket);

	Smtp::Response dataendResponse (socket);
	if (dataendResponse.GetStatus () != Smtp::Response::Ok)
		throw Win::InternalException ("SMTP error: server did not accept message.", dataendResponse.c_str ());

	transact.Commit ();
	return 0;
}

int TestSMTP (WinOut & out)
{
	// Follow SMTP Standard:
	// http://www.ietf.org/rfc/rfc2821.txt
	// and SMTP-AUTH SMTP Extension Standard
	// http://www.ietf.org/rfc/rfc2554.txt

	// parameters (same as in case of POP3)
	std::string server, user, password, senderAddress;
	unsigned long options = 0;
	SmtpData data;
	Email::RegConfig emailCfg;
	SmtpAccount smtpAccount (emailCfg);
	server = smtpAccount.GetServer();
	user = smtpAccount.GetUser();
	password = smtpAccount.GetPassword();
	senderAddress = smtpAccount.GetSenderAddress();
	// Revisit: add options
	data.Init (server, senderAddress, user, password, password, options == 1);
	SmtpCtrl ctrl (&data);
	Dialog::Modal dlg (0, ctrl);
	if (!dlg.IsOK ())
	{
		out.PutLine ("Test aborted");
		return -1;
	}
	server = data.GetServer ();
	senderAddress = data.GetEmail ();
	user = data.GetUser ();
	password = data.GetPassword ();
	//Registry::SetSMTPServerInfo (data.GetServer (), data.GetUser (), data.GetPassword (), data.GetEmail (), 
	//							 data.IsAuthenticate () ? 1 : 0);

	std::string recipientEmail (senderAddress);
	MailToCtrl mailToCtrl (recipientEmail);
	Dialog::Modal mailToDlg (0, mailToCtrl);
	if (!mailToDlg.IsOK ())
	{
		out.PutLine ("Test aborted");
		return -1;
	}

	std::string userDomain  = ""; // to be 100% compliant userDomain should be 
								  // my.symbolic.address.com (e.g. workstation123.icpnet.pl)
								  // still the standard says that servers should accept
								  // conversations without userdomain specified
	std::string subject     = "subject:hello from smtp world.";
	std::string from		= "from:<" + senderAddress + ">";
	std::string to			= "to:<" + recipientEmail + ">";
	std::string body		= "Hello! This is a test message sent using SMTP protocol.";

	WinSocks socks;
	SimpleSocket socket;

	// connect
	out.PutBoldLine ("Connecting to SMTP server");
	socket.Connect (server, 25);
	Smtp::Response connectResponse (socket);
	out.PutLine (connectResponse.c_str ());
	if (connectResponse.GetStatus () != Smtp::Response::ConnectSucceeded)
		throw Win::Exception ("SMTP error: connection failed.", connectResponse.c_str ());

	out.PutBoldLine ("-- Trying ESMTP --");
	std::string ehloStr = "EHLO";
	ehloStr += userDomain;
	out.PutBoldLine (ehloStr.c_str ());
	socket.SendLine (ehloStr.c_str ());
	Smtp::Response ehloResponse (socket);
	out.PutLine (ehloResponse.c_str ());
	if (ehloResponse.GetStatus () != Smtp::Response::Ok)
	{
		out.PutBoldLine ("-- Trying SMTP --");
		std::string heloStr = "HELO ";
		heloStr += userDomain;
		out.PutBoldLine (heloStr.c_str ());
		socket.SendLine (heloStr.c_str ());
		Smtp::Response heloResponse (socket);
		out.PutLine (heloResponse.c_str ());
		if (heloResponse.GetStatus () != Smtp::Response::Ok)
			throw Win::Exception ("SMTP error: helo command.", heloResponse.c_str ());
	}

	// authenticate
	std::string auth ("AUTH LOGIN ");
	out.PutBoldLine (auth.c_str ());
	socket.SendLine (auth);
	Smtp::Response authResponse (socket);
	out.PutLine (authResponse.c_str ());
	if (authResponse.GetStatus () != Smtp::Response::AuthChallenge)
		throw Win::Exception ("SMTP error: auth username.", authResponse.c_str ());
	out.PutBoldLine (Base64::Encode (user).c_str ());
	socket.SendLine (Base64::Encode (user));
	Smtp::Response userResponse (socket);
	out.PutLine (userResponse.c_str ());
	if (userResponse.GetStatus () != Smtp::Response::AuthChallenge)
		throw Win::Exception ("SMTP error: auth password.", userResponse.c_str ());
	out.PutBoldLine (Base64::Encode (password).c_str ());
	socket.SendLine (Base64::Encode (password));
	Smtp::Response passResponse (socket);
	out.PutLine (passResponse.c_str ());
		if (passResponse.GetStatus () != Smtp::Response::AuthSucceeded)
		throw Win::Exception ("SMTP error: authentication failed.", passResponse.c_str ());
	
	// ================
	// MESSAGE ENVELOPE
	// Sender
	std::string mailFrom = "MAIL FROM: <";
	mailFrom += senderAddress;
	mailFrom += ">";
	out.PutBoldLine (mailFrom.c_str ());
    socket.SendLine (mailFrom.c_str ());
	Smtp::Response mailfromResponse (socket);
	out.PutLine (mailfromResponse.c_str ());
	if (mailfromResponse.GetStatus () != Smtp::Response::Ok)
		throw Win::Exception ("SMTP error: mail from command.", mailfromResponse.c_str ());

	// ================
	// MESSAGE ENVELOPE
	// Recipient(s)

	// RCPT TO: 1
	std::string mailTo = "RCPT TO: <"; // may be repeated
	mailTo += recipientEmail;
	mailTo += ">";
	out.PutBoldLine (mailTo.c_str ());
    socket.SendLine (mailTo.c_str ());
	Smtp::Response mailtoResponse (socket);
	out.PutLine (mailtoResponse.c_str ());
	if (mailtoResponse.GetStatus () != Smtp::Response::Ok)
		throw Win::Exception ("SMTP error: rcpt to command.", mailtoResponse.c_str ());


	// RCPT TO: 2
	std::string mailTo2 = "RCPT TO: <piotr@trojanowski.a4.pl>";
	out.PutBoldLine (mailTo2.c_str ());
    socket.SendLine (mailTo2.c_str ());
	Smtp::Response mailto2Response (socket);
	out.PutLine (mailto2Response.c_str ());
	if (mailto2Response.GetStatus () != Smtp::Response::Ok)
		throw Win::Exception ("SMTP error: rcpt to command.", mailto2Response.c_str ());

	// RCPT TO: 3
	std::string mailTo3 = "RCPT TO: <bartosz@relisoft.com>";
	out.PutBoldLine (mailTo3.c_str ());
    socket.SendLine (mailTo3.c_str ());
	Smtp::Response mailto3Response (socket);
	out.PutLine (mailto3Response.c_str ());
	if (mailto3Response.GetStatus () != Smtp::Response::Ok)
		throw Win::Exception ("SMTP error: rcpt to command.", mailto3Response.c_str ());

	// RCPT TO: 4
	std::string mailTo4 = "RCPT TO: <coop@metasoft.internetdsl.pl>";
	out.PutBoldLine (mailTo4.c_str ());
    socket.SendLine (mailTo4.c_str ());
	Smtp::Response mailto4Response (socket);
	out.PutLine (mailto4Response.c_str ());
	if (mailto4Response.GetStatus () != Smtp::Response::Ok)
		throw Win::Exception ("SMTP error: rcpt to command.", mailto4Response.c_str ());

	// ============
	// MESSAGE DATA
	out.PutBoldLine ("DATA");
    socket.SendLine ("DATA");
	Smtp::Response dataResponse (socket);
	if (dataResponse.GetStatus () != Smtp::Response::DataCmdOk)
		throw Win::Exception ("SMTP error: server denied accepting data.", dataResponse.c_str ());
	out.PutLine (dataResponse.c_str ());

	// MESSAGE DATA
	// Header
	// DATE: required
	// Revisit: enter actual data
	std::string currentTime = "Date: ";
	CurrentPackedTime now;
	PackedTimeStr nowStr (now);
	currentTime += nowStr.ToString ();

	out.PutLine (currentTime.c_str ());
	socket.SendLine (currentTime.c_str ());

	// FROM: required
	out.PutBoldLine (from.c_str ());
	socket.SendLine (from);

	// TO: 
	out.PutBoldLine ("To:Co-op:;");
	socket.SendLine ("To:Co-op:;");

	// BCC:
	//out.PutBoldLine ("bcc:<bartosz@relisoft.com>,<coop@metasoft.internetdsl.pl>,<piotr@trojanowski.a4.pl>,<piotroj@poczta.onet.pl>,<piotr@relisoft.com>");
	//socket.SendLine ("bcc:<bartosz@relisoft.com>,<coop@metasoft.internetdsl.pl>,<piotr@trojanowski.a4.pl>,<piotroj@poczta.onet.pl>,<piotr@relisoft.com>");

	// SUBJECT:
	out.PutBoldLine (subject.c_str ());
	socket.SendLine (subject);

	// ============
	// MESSAGE BODY
	out.PutBoldLine ("");
	socket.SendLine ();
	out.PutBoldLine (body.c_str ());
	socket.SendLine (body);
	out.PutBoldLine (".");
    socket.SendLine (".");

	Smtp::Response dataendResponse (socket);
	out.PutLine (dataendResponse.c_str ());
	if (dataendResponse.GetStatus () != Smtp::Response::Ok)
		throw Win::Exception ("SMTP error: data block not accepted.", dataendResponse.c_str ());

	// end session
	out.PutBoldLine ("QUIT");
    socket.SendLine ("QUIT");
	Smtp::Response quitResponse (socket);
	out.PutLine (quitResponse.c_str ());

	return 0;
}

int TestHTTP (WinOut & out)
{
	WinSocks socks;
	SimpleSocket socket;
	out.PutLine ("Connecting to www.google.com");
	socket.Connect ("www.google.com", 80);
    socket.SendLine ("GET / HTTP/1.0");
    socket.SendLine ("Host: www.google.com");
    socket.SendLine ();
	out.PutLine ("Recieving data through HTTP");
    for (;;)
	{
		SimpleSocket::Stream stream (socket, 1024);
		LineBreaker lines (stream);
		while (!lines.Get ().empty ())
		{
			out.PutLine (lines.Get ().c_str ());
			lines.Advance ();
		}
    }
	return 0;
}

void TestNamedPair (WinOut & out)
{
	{
		std::string str ("foo=bar");
		out.PutLine (str.c_str ());
		NamedPair<'='> np (str);
		out.PutBoldLine (np.GetName ().c_str ());
		out.PutBoldLine (np.GetValue ().c_str ());
	}
	{
		std::string str ("	 foo 	 = 	bar	  ");
		out.PutLine (str.c_str ());
		NamedPair<'='> np (str);
		out.PutBoldLine (np.GetName ().c_str ());
		out.PutBoldLine (np.GetValue ().c_str ());
	}
	{
		std::string str ("	 foo 	 = 	\"bar mleczny = bar mleczny\" ");
		out.PutLine (str.c_str ());
		NamedPair<'='> np (str);
		out.PutBoldLine (np.GetName ().c_str ());
		out.PutBoldLine (np.GetValue ().c_str ());
	}
	{
		std::string str ("	 foo 	 = 	\'bar mleczny = \"bar mleczny\"\'	;adkfsdfjsfjsdkfj");
		out.PutLine (str.c_str ());
		NamedPair<'=', ';', '\''> np;
		unsigned pos = np.Init (str);
		out.PutBoldLine (np.GetName ().c_str ());
		out.PutBoldLine (np.GetValue ().c_str ());
		out.PutBoldLine (str.substr (pos).c_str ());
	}
	{
		std::string str ("foo : bar mleczny				,baz=baz");
		out.PutLine (str.c_str ());
		NamedPair<':', ','> np;
		unsigned pos = np.Init (str);
		out.PutBoldLine (np.GetName ().c_str ());
		out.PutBoldLine (np.GetValue ().c_str ());
		out.PutBoldLine (str.substr (pos).c_str ());
	}
}

void SendTestMessage (WinOut & out,
					  std::vector<std::string> const & addrVector,
					  std::string const & server, 
					  std::string const & user, 
					  std::string const & password, 
					  std::string const & senderAddress,
					  std::string const & subject,
					  std::string const & text,
					  std::string const & att)
{
	std::string domain = Registry::GetComputerName ();
	if (domain.empty ())
		domain = "Code Co-op";

	out.PutLine ("Connecting to the SMTP server.");
	Smtp::Mailer mailer (server, user, password, senderAddress, domain);

	OutgoingMessage msg;
	msg.SetSubject (subject);
	msg.SetText (text);
	if (!att.empty ())
		msg.AddFileAttachment (att);

	out.PutLine ("Sending a test message.");
	//	mailer.Send (msg, addrVector, "Co-op Users in Project X");
	mailer.Send (msg, addrVector);
	out.PutLine ("The test message sent successfully.");
	out.PutLine ("Disconnecting from the SMTP server.");
}

void TestCoopScriptsSending (WinOut & out)
{
	out.PutBoldLine ("Test sending Co-op scripts.");

	// =====================
	// interaction with user
	std::string server, user, password, senderAddress;
	unsigned long options = 0;
	if (!GetSMTPInfoFromUser (out, server, user, password, senderAddress, options))
		return;

	// ================
	// message elements
	std::string subject     = "Code Co-op Sync:znc.snc::POP3 Test:0-0";
	std::string text		= "An smtp message with a textual part of body that is "
							  "longer than 78 characters. It should be broken into lines.";

	std::vector<std::string> addrVector;
	addrVector.push_back (senderAddress);
/*	addrVector.push_back ("piotroj@poczta.onet.pl");
	addrVector.push_back ("piotr@trojanowski.a4.pl");
	addrVector.push_back ("piotr@relisoft.com");
	addrVector.push_back ("trojanowski@a4.pl");
	addrVector.push_back ("bartosz@relisoft.com");
	addrVector.push_back ("coop@metasoft.internetdsl.pl");
*/
	// ================
	CurrentFolder currentFolder;
	std::string const attachment = currentFolder.GetFilePath ("Scripts.znc");

	SendTestMessage (out, addrVector, server, user, password, senderAddress, subject, text, attachment);
}

void TestCoopScriptsRetrieval (WinOut & out)
{
	out.PutBoldLine ("Test co-op scripts retrieval");
	std::string server, user, password;
	unsigned long options = 0;
	if (!GetPOP3InfoFromUser (out, server, user, password, options))
		return;

	WinSocks socks;
	out.PutLine ("Connecting to Pop3 server.");
	Pop3::Connection connection (server, 110, false);
	out.PutLine ("Logging in.");
	bool useSSL = false;
	Pop3::Session session (connection, user, password, useSSL);
	out.PutLine ("Iterating over all messages.");
	for (Pop3::MessageRetriever retriever (session); !retriever.AtEnd (); retriever.Advance ())
	{
		// first retrieve only message header
		Pop3::Message msgHdr; // sink for the parser
		Pop3::Parser parser;
		parser.Parse (retriever.RetrieveHeader (), msgHdr);

		Subject::Parser subject (msgHdr.GetSubject ());
		if (subject.IsScript ())
		{
			out.PutLine ("Found a script");
			// Save attachments in temporary area and initialize paths
			SafePaths attPaths;
			// retrieve the whole message from server
			Pop3::Message msg;
			parser.Parse (retriever.RetrieveMessage (), msg);
			TmpPath tmpFolder;
			msg.SaveAttachments (tmpFolder, attPaths);
			// retriever.DeleteMsgFromServer ();
		}
	}
	out.PutLine ("Disconnected.");
}

void TestMessageParsing (WinOut & out)
{
	out.PutBoldLine ("Test message building");
	const char MsgFullPath [] = "c:\\projects\\rebecca\\test\\pop3\\001 Co-op Sync Script.txt";

	FileIo msgFile (MsgFullPath, File::ReadOnlyMode ());
	MsgFileStream stream (msgFile, 1024);
	Pop3::LineSeq seq (stream);
	Pop3::Message msg;
	Pop3::Parser parser;
	parser.Parse (seq, msg);
	SafePaths attPaths;
	FilePath destFolder ("c:\\projects\\rebecca\\test\\pop3\\tmp");
	msg.SaveAttachments (destFolder, attPaths);
}

int RunTest (WinOut & out)
{
	out.PutLine ("Begin Test");
	try
	{
		// TestFullEncryption (out);
		// TestNamedPair (out);
		// TestHTTP (out);
		// TestPOP (out);
		TestPopSsl (out);
		// TestMessageDecoding (out);
		// TestSMTP (out);
		// TestCoopScriptsSending (out);
		// TestCoopScriptsRetrieval (out);
		// TestMessageParsing (out);
		// RunConfigTool (out);
		TestSecureSmtp (out);
	}
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
	}
	catch (...)
	{
		TheOutput.Display ("Unexpected error", Out::Error);
	}
	out.PutLine ("End Test");
	return 0;
}
