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
#include <Net/SimpleSocket.h>

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

int main (int argc, char * argv [])
{
	try
	{
		std::string msg ("Hello from socket client!");
		WinSocks winSockUser;

		LocalHost localHost;

		SimpleSocket serverSocket;
		serverSocket.Connect (localHost.Name (), 27015);
		serverSocket.Send (msg.c_str (), msg.length ());

		return 0;
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
