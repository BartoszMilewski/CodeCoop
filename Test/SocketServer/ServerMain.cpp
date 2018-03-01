//-----------------------------------
// (c) Reliable Software 2008
//-----------------------------------

#include "precompiled.h"

#include <File/File.h>
#include <File/Path.h>
#include <Ex/Error.h>
#include <Ex/Winex.h>
#include <StringOp.h>
#include <Ctrl/Output.h>
#include <Net/Socket.h>

#include <ws2tcpip.h>

#include <iostream>

class LocalHost
{
public:
	LocalHost ()
	{
		_name.reserve (256);
		if (gethostname (writable_string(_name), _name.capacity ()) == SOCKET_ERROR)
		{
			throw Win::SocketException ("Cannot get local host name");
		}
	}

	std::string const & Name () const { return _name; }

private:
	std::string	_name;
};

class SocketAddress
{
	friend class ListeningSocket;
public:
	SocketAddress(sockaddr * addr, size_t len)
		: _addr(addr), _len(len)
	{}
private:
	sockaddr const * _addr;
	int _len;
};

class ServerAddress
{
public:
	ServerAddress(std::string const & serverName, std::string const & port)
		: _serverAddress(0)
	{
		struct addrinfo hints;

		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_flags = AI_PASSIVE;
		// Resolve the server address and port
		if (getaddrinfo(serverName.c_str (), port.c_str (), &hints, &_serverAddress) != 0)
			throw Win::SocketException ("Cannot resolve server address", serverName.c_str ());
	}
	~ServerAddress()
	{
		if (_serverAddress != 0)
			freeaddrinfo(_serverAddress);
	}
	int Family() const
	{
		return _serverAddress->ai_family;
	}
	int SockType() const
	{
		return _serverAddress->ai_socktype;
	}
	int Protocol() const
	{
		return _serverAddress->ai_protocol;
	}
	SocketAddress SockAddr()
	{
		return SocketAddress(_serverAddress->ai_addr, _serverAddress->ai_addrlen);
	}
private:
	struct addrinfo *	_serverAddress;
};

class ListeningSocket
{
public:
	ListeningSocket (std::string const & serverName, std::string const & port);
	~ListeningSocket ()
	{
		if (_socket != INVALID_SOCKET)
			::closesocket (_socket);
	}

	SOCKET AcceptConnection ();
private:
	void Bind(SocketAddress addr)
	{
		if (bind (_socket, addr._addr, static_cast<int>(addr._len)) == SOCKET_ERROR)
			throw Win::SocketException ("Listening socket setup failed");
	}
private:
	SOCKET				_socket;
};


ListeningSocket::ListeningSocket (std::string const & serverName, std::string const & port)
	: _socket (INVALID_SOCKET)
{
	ServerAddress serverAddress(serverName, port);
	// Create a SOCKET for connecting to server
	_socket = socket(serverAddress.Family(), serverAddress.SockType(), serverAddress.Protocol());
	if (_socket == INVALID_SOCKET)
		throw Win::SocketException ("Listening socket creation failed");

	// Setup the TCP listening socket
	Bind(serverAddress.SockAddr());
	if (listen(_socket, SOMAXCONN) == SOCKET_ERROR)
		throw Win::SocketException ("Listening failed");
}


SOCKET ListeningSocket::AcceptConnection ()
{
	SOCKET clientSocket = accept(_socket, NULL, NULL);
	if (clientSocket == INVALID_SOCKET)
		throw Win::SocketException ("Accepting client request failed");

	return clientSocket;
}

int main (int argc, char * argv [])
{
	try
	{
		WinSocks winSockUser;

		LocalHost localHost;
		ListeningSocket serverSocket (localHost.Name (), "27015");
		std::cout << "Waiting for a client connection" << std::endl;
		SOCKET clientSocket = serverSocket.AcceptConnection ();
		std::cout << "Accept a client socket" << std::endl;
		std::string recvBuf;
		recvBuf.reserve (256);
		// Receive until the peer closes the connection
		int iResult = 0;
		do
		{
			int iResult = recv(clientSocket, writable_string (recvBuf), recvBuf.capacity (), 0);
			if ( iResult > 0 )
				std::cout << "From client: '" << recvBuf << "'" << std::endl;
			else if ( iResult == 0 )
				std::cout << "Connection closed" << std::endl;
			else
				throw Win::SocketException ("received failed");

		} while( iResult > 0 );
		closesocket(clientSocket);
	}
	catch (Win::SocketException ex)
	{
		std::cerr << "Socket Exception: " << Out::Sink::FormatExceptionMsg (ex) << std::endl;
	}
	catch (Win::Exception ex)
	{
		std::cerr << "Win::Exception: " << Out::Sink::FormatExceptionMsg (ex) << std::endl;
	}
	catch ( ... )
	{
		std::cerr << "Unknow exception" << std::endl;
	}
	return 1;
}
