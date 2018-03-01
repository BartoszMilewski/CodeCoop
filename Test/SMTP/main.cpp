//---------------------------
// (c) Reliable Software 2009
//---------------------------
#include "precompiled.h"
#include <Net/Socket.h>
#include <Sys/Crypto.h>
#include <Mail/Smtp.h>
#include <Mail/SmtpMessage.h>
#include <Sys/PackedTime.h>
#include <Mail/Base64.h>
#include <Ctrl/Output.h>
#include <iostream>

#include <File/SerialUnVector.h>

class StdOutput
{
public:
	void PutBoldLine(std::string const & str)
	{
		std::cout << "=> \"";
		std::cout << str << "\"" << std::endl;
	}
	void PutLine(std::string const & str)
	{
		std::cout << str << std::endl;
	}
	void PutLine()
	{
		std::cout << std::endl;
	}
};

StdOutput out;

int main()
{
	try
	{
	std::string server = "smtp.live.com";
	std::string user = "MilewskiArt@live.com";
	std::string password = "cier5woki";
	std::string recipientEmail = "bartosz@relisoft.co";
	std::string senderAddress = "MilewskiArt@live.com";

	std::string userDomain  = "Basior"; // to be 100% compliant userDomain should be 
								  // my.symbolic.address.com (e.g. workstation123.icpnet.pl)
								  // still the standard says that servers should accept
								  // conversations without userdomain specified
	std::string subject     = "subject:hello from smtp world.";
	std::string from		= "from:<" + senderAddress + ">";
	std::string to			= "to:<" + recipientEmail + ">";
	std::string body		= "Hello! This is a test message sent using SMTP protocol.";

	WinSocks socks;
	SimpleSocket smplSocket;

	// connect
	out.PutBoldLine ("Connecting to SMTP server");
	smplSocket.Connect (server, 25);
	Smtp::Response connectResponse (smplSocket);
	out.PutLine (connectResponse.c_str ());
	if (connectResponse.GetStatus () != Smtp::Response::ConnectSucceeded)
		throw Win::Exception ("SMTP error: connection failed.", connectResponse.c_str ());

	out.PutBoldLine ("-- Trying ESMTP --");
	std::string ehloStr = "EHLO ";
	ehloStr += userDomain;
	out.PutBoldLine (ehloStr.c_str ());
	smplSocket.SendLine (ehloStr.c_str ());
	Smtp::Response ehloResponse (smplSocket);
	out.PutLine (ehloResponse.c_str ());
	if (ehloResponse.GetStatus () != Smtp::Response::Ok)
	{
		out.PutBoldLine ("-- Trying SMTP --");
		std::string heloStr = "HELO";
		heloStr += userDomain;
		out.PutBoldLine (heloStr.c_str ());
		smplSocket.SendLine (heloStr.c_str ());
		Smtp::Response heloResponse (smplSocket);
		out.PutLine (heloResponse.c_str ());
		if (heloResponse.GetStatus () != Smtp::Response::Ok)
			throw Win::Exception ("SMTP error: helo command.", heloResponse.c_str ());
	}

	std::string startTls = "STARTTLS";
	out.PutBoldLine (startTls.c_str());
	smplSocket.SendLine(startTls);
	Smtp::Response startTlsResponse (smplSocket);
	//unsigned tlsStatus = startTlsResponse.GetStatus();
	out.PutLine (startTlsResponse.c_str ());

	SecureSocket socket(smplSocket);

	// Re-start after STARTTLS

	out.PutBoldLine ("-- Ehlo in TLS --");
	out.PutBoldLine (ehloStr.c_str ());
	socket.SendLine (ehloStr.c_str ());
	Smtp::Response ehloResponse2 (socket);
	out.PutLine (ehloResponse2.c_str ());
	if (ehloResponse2.GetStatus () != Smtp::Response::Ok)
	{
		out.PutBoldLine ("-- Trying SMTP --");
		std::string heloStr = "HELO";
		heloStr += userDomain;
		out.PutBoldLine (heloStr.c_str ());
		socket.SendLine (heloStr.c_str ());
		Smtp::Response heloResponse (socket);
		out.PutLine (heloResponse.c_str ());
		if (heloResponse.GetStatus () != Smtp::Response::Ok)
			throw Win::Exception ("SMTP error: helo command.", heloResponse.c_str ());
	}

/*
	std::string encodedString = "TWlsZXdza2lBcnRAbGl2ZS5jb20=";
	Base64::SimpleInput input (encodedString);
	Base64::Decoder decoder;
	unmovable_vector<char> dest;
	SerialUnVector<char>::Output output (dest);
	decoder.Decode (input, output);
	std::string decodedStr(dest.begin(), dest.end());
*/

	// authenticate
	std::string auth ("AUTH LOGIN");
	out.PutBoldLine (auth.c_str ());
	socket.SendLine (auth);
	Smtp::Response authResponse (socket);
	out.PutLine (authResponse.c_str ());
	if (authResponse.GetStatus () != Smtp::Response::AuthChallenge)
		throw Win::Exception ("SMTP error: auth username.", authResponse.c_str ());
	out.PutBoldLine (Base64::Encode (user).c_str ());

	socket.SendLine (Base64::Encode (user));

	Smtp::Response userResponse (socket);
	unsigned rsp = userResponse.GetStatus();
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
	}
	catch (Win::Exception ex)
	{
		out.PutLine(Out::Sink::FormatExceptionMsg (ex));
	}
	return 0;
}

