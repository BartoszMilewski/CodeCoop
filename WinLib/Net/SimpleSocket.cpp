// ---------------------------
// (c) Reliable Software, 2006
// ---------------------------
#include "WinLibBase.h"
#include "SimpleSocket.h"

SimpleSocket::SimpleSocket ()
:_sock (::socket(AF_INET, SOCK_STREAM, 0))
{
	if (_sock == INVALID_SOCKET)
		throw Win::SocketException ("Cannot open internet socket.");
}

SimpleSocket::~SimpleSocket ()
{
	::closesocket (_sock);
}

void SimpleSocket::Connect (std::string const & host, short port, int timeout)
{
	_host = host;
	_port = port;

	HostName info (host);
	sockaddr_in name;
	name.sin_family = AF_INET;
	name.sin_port = htons (port);
	name.sin_addr = info._addr;
	memset (&(name.sin_zero), 0, 8); 
	if (::connect (_sock, (sockaddr *)&name, sizeof (sockaddr)) != 0)
		throw Win::SocketException ("Socket connection failed.", host.c_str ());

	if (timeout > 0)
	{
		// set send/receive timeouts
		// SO_RCVTIMEO and SO_SNDTIMEO options are available in 
		// the Microsoft implementation of Windows Sockets 2
		// consciously ignoring errors during these calls
		::setsockopt (_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof (timeout));
		::setsockopt (_sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof (timeout));
	}
}

void SimpleSocket::Send (char const * buf, unsigned int len)
{
	if (::send (_sock, buf, len, 0) == SOCKET_ERROR)
	{
		throw Win::SocketException ("Socket send failed.");
	};
}

unsigned int SimpleSocket::Receive (char buf [], unsigned int size)
{
	int len = ::recv (_sock, buf, size, 0);
	if (len == SOCKET_ERROR)
	{
		throw Win::SocketException ("Socket receive failed");
	}
	else if (len == 0)
	{
		throw Win::SocketException ("Connection has been closed.");
	}
	return static_cast<unsigned int> (len);
}

HostName::HostName (std::string const & host)
{
	hostent *he = gethostbyname (host.c_str());
	if (he == 0)
	{
		throw Win::SocketException ("Cannot get host by name", host.c_str ());
	}
	char * address = he->h_addr_list [0];
	_addr = *((in_addr *) address);
}
