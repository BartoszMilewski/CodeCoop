#if !defined (WINSOCKET_H)
#define WINSOCKET_H
// ---------------------------------
// (c) Reliable Software 2003 - 2006
// ---------------------------------
#include <Parse/BufferedStream.h>

// Main Windows Sockets header, WinSock2.h, already included through windows.h

// Initialize and deinitialize Windows Sockets Subsystem
class WinSocks
{
public:
	WinSocks ();
	~WinSocks ()
	{
		::WSACleanup ();
	}
private:
	WSADATA _info;
};

namespace Win
{
	class SocketException : public Win::Exception
	{
	public:
		SocketException (char const * msg, char const * objName = 0)
			: Win::Exception (msg, objName, ::WSAGetLastError ())
		{
			::WSASetLastError (0);
		}
		bool IsTimedOut () const { return _err == WSAETIMEDOUT; }
	};
}

class Socket
{
public:
	class Stream: public BufferedStream
	{
	public:
		Stream (Socket & sock, unsigned bufSize)
			: BufferedStream (bufSize), _sock (sock)
		{}
	protected:
		void FillBuf ();
	private:
		Socket	   &_sock;
	};

public:
	virtual ~Socket () {}

	virtual void Connect (std::string const & host, short port, int timeout = 0) = 0; // in milisec

	virtual void Send (char const * buf, unsigned int len) = 0;
	void SendLine ();
	// Appends newline to str
	void SendLine (std::string const & str);

	// Low level method
	virtual unsigned int Receive (char buf [], unsigned int size) = 0;

	virtual std::string const & GetHostName () const = 0;
	virtual short GetHostPort () const = 0;

protected:
	Socket () {}
private:
	// prevent value semantics
	Socket (Socket const &);
	Socket & operator = (Socket const &);
};

#endif
