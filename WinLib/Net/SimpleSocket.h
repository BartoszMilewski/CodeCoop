#if !defined (SIMPLESOCKET_H)
#define SIMPLESOCKET_H
// ---------------------------
// (c) Reliable Software, 2006
// ---------------------------
#include "Socket.h"

// simple unsecured socket

class SimpleSocket : public Socket
{
public:
	SimpleSocket ();
	~SimpleSocket ();
	
	void Connect (std::string const & host, short port, int timeout = 0); // in milisec

	void Send (char const * buf, unsigned int len);
	unsigned int Receive (char buf [], unsigned int size);

	std::string const & GetHostName () const { return _host; }
	short GetHostPort () const { return _port; }

protected:
	SOCKET			_sock;
	std::string		_host;
	short			_port;
};


class HostName
{
	friend class SimpleSocket;
public:
	HostName (std::string const & host);
private:
	in_addr _addr;
};

#endif
