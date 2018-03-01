// ---------------------------------
// (c) Reliable Software 2003 - 2006
// ---------------------------------
#include <WinLibBase.h>
#include "Socket.h"
#include <StringOp.h>

WinSocks::WinSocks ()
{
	WORD version = MAKEWORD(2,0);
	int err = ::WSAStartup (version, &_info);
	if (err != 0)
	{
		switch (err)
		{
		case WSASYSNOTREADY:
			throw Win::Exception ("Windows Sockets: Network subsystem is not ready for network communication");
		case WSAVERNOTSUPPORTED:
			throw Win::Exception ("Version 2.0 of Windows Sockets support is not provided by this particular Windows Sockets implementation");
		case WSAEINPROGRESS: 
			throw Win::Exception ("A blocking Windows Sockets 1.1 operation is in progress");
		case WSAEPROCLIM: 
			throw Win::Exception ("Limit on the number of tasks supported by the Windows Sockets implementation has been reached");
		case WSAEFAULT: 
			Assert (!"The lpWSAData is not a valid pointer");
		}
	}
}

void Socket::Stream::FillBuf ()
{
	_bufPos = 0;
	_bufEnd = _sock.Receive (GetBuf (), BufferSize ());
	if (_bufEnd == 0)
	{
		throw Win::SocketException ("Connection has been closed.");
	}
}

void Socket::SendLine ()
{
	Send ("\r\n", 2);
}

// Appends newline to str
void Socket::SendLine (std::string const & str)
{
	std::string line = str;
	line.append ("\r\n", 2);
	Send (line.c_str (), line.length ());
}
