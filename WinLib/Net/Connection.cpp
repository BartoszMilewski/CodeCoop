// ---------------------------
// (c) Reliable Software, 2006
// ---------------------------
#include <WinLibBase.h>
#include <Net/Connection.h>
#include <Net/SimpleSocket.h>
#include <Net/SecureSocket.h>

Connection::Connection (
			  std::string const & server, 
			  short port, 
			  int timeout)
{
	_socket.reset (new SimpleSocket ());
	_socket->Connect (server, port, timeout);
}

void Connection::SwitchToSSL ()
{
	Assert (_rawSocket.get () == 0);
	Assert (_socket.get    () != 0);

	_rawSocket.reset (dynamic_cast<SimpleSocket*>(_socket.release ()));

	Assert (_rawSocket.get () != 0);
	Assert (_socket.get    () == 0);

	_socket.reset (new SecureSocket (*_rawSocket.get ()));
}
