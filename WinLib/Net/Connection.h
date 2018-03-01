#if !defined (CONNECTION_H)
#define CONNECTION_H
// ---------------------------
// (c) Reliable Software, 2006
// ---------------------------

class Socket;
class SimpleSocket;

class Connection
{
public:
	Connection (std::string const & server, short port, int timeout = 0);
	virtual ~Connection () {}

	// can be called only once
	void SwitchToSSL ();

	Socket & GetSocket () { return *_socket.get (); }

protected:
	// order matters!
	std::unique_ptr<SimpleSocket>	_rawSocket;
	std::unique_ptr<Socket>		_socket;
};

#endif
