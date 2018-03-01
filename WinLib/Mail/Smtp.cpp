// ---------------------------
// (c) Reliable Software, 2005
// ---------------------------
#include <WinLibBase.h>
#include "Smtp.h"
#include <Mail/Base64.h>
#include <sstream>

Smtp::Response::Response (Socket & socket)
: _code (-1)
{
	// The general form of a reply is a numeric completion code
	// (indicating failure or success) usually followed by a text string.	
	// In a multiline reply all lines but the last start with a status code followed by a hyphen,
	// the last line starts with a status code followed by a space.
	Socket::Stream stream (socket, 256);
	if (stream.LookAhead() == '\0')
		throw Win::InternalException ("Smtp: Server broke the connection");
	LineBreaker lines (stream);
	for (;;)
	{
		std::string const & line = lines.Get ();
		std::istringstream istr (lines.Get ());
		int newCode = -1;
		istr >> std::dec >> newCode;
		if (_code != -1)
		{
			if (newCode != _code)
				throw Win::InternalException ("Smtp: Invalid server response.");
		}
		_code = newCode;
		char sep = istr.get ();
		if (sep != std::string::npos)
		{
			std::string cmt;
			std::getline (istr, cmt);
			_comment.append (cmt);
		}

		if (sep == ' ')
			break;

		_comment.append ("\r\n");
		lines.Advance ();
	}
}

Smtp::Connection::Connection (
		std::string const & server, 
		short port, 
		int timeout)
		: ::Connection (server, port, timeout)
{
}

Smtp::Connection::~Connection ()
{
	try
	{
		if (_socket.get () == 0)
			_rawSocket->SendLine ("quit");
		else
			_socket->SendLine ("quit");
	}
	catch (...)
	{
	}
}

Smtp::Session::Session (Connection & connection, std::string const & domain, bool useSSL)
: _connection (connection),
  _domain (domain)
{
	if (useSSL)
	{
		Socket & socket = _connection.GetSocket ();
		if (socket.GetHostPort () == 465)
		{
			_connection.SwitchToSSL ();
			ReceiveConnectResponse ();
		}
		else
		{
			ReceiveConnectResponse ();
			SayHello ();
			SendStartTlsCmd ();
			_connection.SwitchToSSL ();
		}
	}
	else
	{
		ReceiveConnectResponse ();
	}
	SayHello ();
}

void Smtp::Session::SayHello ()
{
	Socket & socket = _connection.GetSocket ();
	// ehlo <domain>
	std::string ehloCmd = "EHLO " + _domain;
	socket.SendLine (ehloCmd.c_str ());
	Smtp::Response ehloResponse (socket);
	if (ehloResponse.GetStatus () != Smtp::Response::Ok)
	{
		// helo <domain>
		std::string heloCmd = "HELO " + _domain;
		socket.SendLine (heloCmd.c_str ());
		Smtp::Response heloResponse (socket);
		if (heloResponse.GetStatus () != Smtp::Response::Ok)
			throw Win::InternalException ("SMTP error: cannot initiate session.", heloResponse.c_str ());
	}
}

void Smtp::Session::ReceiveConnectResponse ()
{
	Smtp::Response connectResponse (_connection.GetSocket ());
	if (connectResponse.GetStatus () != Smtp::Response::ConnectSucceeded)
		throw Win::InternalException ("SMTP error: connection failed.", connectResponse.c_str ());
}

void Smtp::Session::SendStartTlsCmd ()
{
	Socket & socket = _connection.GetSocket ();
	socket.SendLine ("STARTTLS");
	Smtp::Response starttlsResponse (socket);
	if (starttlsResponse.GetStatus () != Smtp::Response::ConnectSucceeded)
		throw Win::InternalException ("SMTP error: cannot initiate SSL session.", starttlsResponse.c_str ());
}

void Smtp::Session::Authenticate (std::string const & user, std::string const & password)
{
	Socket & socket = _connection.GetSocket ();
	std::string auth ("AUTH LOGIN");
	socket.SendLine (auth);
	Smtp::Response authResponse (socket);
	if (authResponse.GetStatus () != Smtp::Response::AuthChallenge)
		throw Win::InternalException ("SMTP error: authentication failed.", authResponse.c_str ());
	socket.SendLine (Base64::Encode (user));
	Smtp::Response userResponse (socket);
	if (userResponse.GetStatus () != Smtp::Response::AuthChallenge)
		throw Win::InternalException ("SMTP error: authentication failed on username.", userResponse.c_str ());
	socket.SendLine (Base64::Encode (password));
	Smtp::Response passResponse (socket);
	if (passResponse.GetStatus () != Smtp::Response::AuthSucceeded)
		throw Win::InternalException ("SMTP error: authentication failed on password.", passResponse.c_str ());
}	
