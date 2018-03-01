// -------------------------------
// (c) Reliable Software 2003 - 05
// -------------------------------
#include <WinLibBase.h>
#include "Pop3.h"
#include <Parse/BufferedStream.h>
#include <StringOp.h>

Pop3::SingleLineResponse::SingleLineResponse (std::string const & line)
{
	Init (line);
}

Pop3::SingleLineResponse::SingleLineResponse (Socket & socket)
{
	// Single line responses may be up to 512 characters long,
    // including the terminating CRLF, but
	// usually they contain less than 128 characters.
	Socket::Stream stream (socket, 128);
	LineBreaker response (stream);
	Init (response.Get ());
}

void Pop3::SingleLineResponse::Init (std::string const & line)
{
	unsigned int const len = line.length ();
	if (len > 0)
	{
		if (line [0] == '+')
		{
			// +OK
			if (len < 3 || line [1] != 'O' || line [2] != 'K')
			{
				_err = "Incorrect response";
				_err += line;
			}
			else
			{
				unsigned idx = line.find_first_not_of (" \t", 3);
				if (idx != std::string::npos)
					_line = line.substr (idx);
			}
		}
		else if (line [0] == '-')
		{
			// -ERR
			if (len < 4 || line [1] != 'E' || line [2] != 'R' || line [3] != 'R')
			{
				_err = "Incorrect response: ";
				_err += line;
			}
			else
			{
				unsigned idx = line.find_first_not_of (" \t", 4);
				if (idx != std::string::npos)
					_err = line.substr (idx);
				else
					_err = "Unspecified error";
			}
		}
		else
		{
			_err = "Unknown response: ";
			_err += line;
		}
	}
	else
		_err = "Empty response";
}

Pop3::MultiLineResponse::MultiLineResponse (Socket & socket, unsigned bufSize) 
	: _stream (socket, bufSize),
	  _lineBreaker (_stream),
	  _firstLine (_lineBreaker.Get ())
{
	if (_firstLine.IsError ())
	{
		_done = true;
	}
	else
	{
		Advance ();
	}
}

void Pop3::MultiLineResponse::Advance ()
{
	if (AtEnd ())
		throw Pop3::Exception ("Pop3: cannot interpret message.", _lineBreaker.Get ());

	_lineBreaker.Advance ();
	std::string const & currLine = _lineBreaker.Get ();

	if (!currLine.empty () && currLine [0] == '.')
	{
		if (currLine.length () == 1)
		{
			// end marker
			_done = true;
		}
		else
		{
			// Eat leading period, it's byte stuffing.
			_line = currLine.substr (1);
		}
	}
	else
		_line = currLine;
}

Pop3::Connection::Connection (
		std::string const & server, 
		short port,
		int timeout)
		: ::Connection (server, port, timeout)
{}

Pop3::Connection::~Connection ()
{
	try
	{
		if (_socket.get () == 0)
			_rawSocket->SendLine ("quit");
		else
			_socket->SendLine ("quit");
	}
	catch (...)
	{}
}

Pop3::Session::Session (
		Connection & connection, 
		std::string const & user, 
		std::string const & pass, 
		bool useSSL)
		: _connection (connection)
{
	if (useSSL)
	{
		connection.SwitchToSSL ();
	}

	Socket & socket = _connection.GetSocket ();

	Pop3::SingleLineResponse initialResponse (socket);
	if (initialResponse.IsError ())
		throw Pop3::Exception ("Pop3: cannot connect to server.", initialResponse.GetError ());

	// Log in:
	// username
	std::string userStr ("user ");
	userStr += user;
    socket.SendLine (userStr);
	Pop3::SingleLineResponse usercmdResponse (socket);
	if (usercmdResponse.IsError ())
		throw Pop3::Exception ("Pop3: login error.", usercmdResponse.GetError ());
	// password
	std::string passwordStr ("pass ");
	passwordStr += pass;
    socket.SendLine (passwordStr);
	Pop3::SingleLineResponse passcmdResponse (socket);
	if (passcmdResponse.IsError ())
		throw Pop3::Exception ("Pop3: login error.", passcmdResponse.GetError ());
}

Pop3::MessageRetriever::MessageRetriever (Pop3::Session & session)
: _socket (session.GetSocket ()),
  _cur (0),
  _count (0)
{
	_socket.SendLine ("stat");
	Pop3::SingleLineResponse response (_socket);
	if (response.IsError ())
		throw Pop3::Exception ("Pop3: cannot retrieve account statistics.", 
							   response.GetError ());

	_count = ToInt (response.Get ());

	Advance ();
}

LineSeq & Pop3::MessageRetriever::RetrieveHeader ()
{
	Assert (!AtEnd ());
	std::string topCmd = "top " + ToString (_cur) + " 0";
	_socket.SendLine (topCmd);

	_lines.reset (new Pop3::MultiLineResponse (_socket, 4 * 1024)); // 4k buffer
	if (_lines->IsError ())
		throw Pop3::Exception ("Pop3: cannot retrieve message header from server.", 
							   _lines->GetError ());

	return *_lines;
}

LineSeq & Pop3::MessageRetriever::RetrieveMessage ()
{
	Assert (!AtEnd ());
	std::string retrCmd = "retr " + ToString (_cur);
	_socket.SendLine (retrCmd);

	_lines.reset (new Pop3::MultiLineResponse (_socket, 100000)); // large buffer
	if (_lines->IsError ())
		throw Pop3::Exception ("Pop3: cannot retrieve message from server.", 
							   _lines->GetError ());

	return *_lines;
}

void Pop3::MessageRetriever::DeleteMsgFromServer ()
{
	Assert (!AtEnd ());
	std::string deleCmd = "dele " + ToString (_cur);
	_socket.SendLine (deleCmd);
	Pop3::SingleLineResponse response (_socket);
	if (response.IsError ())
		throw Pop3::Exception ("Pop3: cannot delete message from server.", 
								response.GetError ());
}

char Pop3::Input::Get ()
{
	// don't eat the delimiter, it may be the beginning of MIME boundary
	char c = _stream.LookAhead ();
	if (c == '-' || c == '.' || c == ' ')
		return '\0';
	_stream.GetChar (); // accept
	if (c == '\r' && _stream.LookAhead () == '\n')
	{
		_stream.GetChar (); // '\n'
		c = _stream.LookAhead ();
		if (c == '-' || c == '.' || c == ' ')
			return '\0';
		_stream.GetChar (); // accept
	}
	return c;
}
