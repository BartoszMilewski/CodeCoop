#if !defined (SECURESOCKET_H)
#define SECURESOCKET_H
// ---------------------------
// (c) Reliable Software, 2006
// ---------------------------

// Secure Sockets

#define SECURITY_WIN32
#include <sspi.h>
#include <security.h>
#include <schannel.h>

#include "SimpleSocket.h"

namespace Win
{
	class SecurityException : public Win::Exception
	{
	public:
		SecurityException (char const * msg, unsigned long err)
			: Win::Exception (msg, 0, err)
		{}

		bool IsRenegotiate () const { return _err == SEC_I_RENEGOTIATE; }
	};
}

namespace Schannel
{
	class Credentials
	{
	public:
		Credentials ();
		~Credentials ();

		CredHandle ToNative () const { return _packageCred; }

	private:
		SCHANNEL_CRED	_schannelCred;
		CredHandle		_packageCred;
	};

	class MessageLayout
	{
	public:
		MessageLayout ()
			: _hdrSize (0),
			  _dataSize (0),
			  _trailerSize (0),
			  _maxMsgSize (0)
		{}
		void Reset (CtxtHandle context);

		unsigned int GetHdrSize     () const { return _hdrSize;     }
		unsigned int GetDataSize    () const { return _dataSize;    }
		unsigned int GetTrailerSize () const { return _trailerSize; }
		unsigned int GetMaxMsgSize  () const { return _maxMsgSize;  }
	private:
		unsigned int	_hdrSize;
		unsigned int	_dataSize;
		unsigned int	_trailerSize;
		unsigned int    _maxMsgSize;
	};

	class BufferedSocket
	{
		static unsigned int const InitialSize = 0x04000;
	public:
		BufferedSocket (SimpleSocket & socket)
			: _socket (socket), _decodedStart (0), _unprocessedStart (0)
		{
			Clear ();
			Reset ();
		}
		void Reset (unsigned int size = InitialSize);
		void Clear ();

		void MoreRawData ();
		char * GetRawData () { return &_data [0]; }
		unsigned int GetRawDataSize () const { return _rawDataSize; }
		void AcceptRawData () {	ClearRawData (); }

		bool HasDecodedData () const { return _decodedSize > 0; }
		char const * AcceptDecodedData (unsigned int & dataSize);
		void SetDecodedData (char const * start, unsigned int size);

		void SetUnprocessedData (char const * start, unsigned int size);
	private:
		void MoveUnprocessedToBeginning ();
		void ConvertUnprocessedToRawData ()
		{
			Assert (HasUnprocessedData ());
			Assert (_unprocessedStart == 0);
			_rawDataSize = _unprocessedSize;
			ClearUnprocessedData (); 
		}
		bool HasUnprocessedData () const { return _unprocessedSize > 0; }
		void ClearUnprocessedData ()
		{
			_unprocessedSize = 0;
		}
		void ClearRawData ()
		{
			_rawDataSize = 0;
		}
		void ClearDecodedData ()
		{
			_decodedSize = 0;
		}
	private:
		SimpleSocket	  & _socket;
		std::vector<char>	_data;
		unsigned int		_rawDataSize;	// size of data read from socket
		unsigned int		_decodedStart;
		unsigned int		_decodedSize;		
		unsigned int		_unprocessedStart;
		unsigned int		_unprocessedSize;
	};

	class SecurityContext
	{
	public:
		SecurityContext::SecurityContext ()
			: _connectionParams (0)
		{
			SecInvalidateHandle (&_context);
		}

		~SecurityContext ();

		CtxtHandle ToNative () const { return _context; }

		void Negotiate   (std::string const & hostname, SimpleSocket & sendSock, BufferedSocket & recvSock);
		void Renegotiate (SimpleSocket & sendSock, BufferedSocket & recvSock);
		void Shutdown    (SimpleSocket & sendSock, BufferedSocket & recvSock);
	private:
		std::vector<char> Initialize (bool isFirstTime);
		SECURITY_STATUS DoInitialize (	
				CtxtHandle    * context,
				SecBufferDesc * inBufferPtr,
				CtxtHandle    * newContext,
				std::vector<char> & token);
		void ClientHandshakeLoop (SimpleSocket & sendSock, BufferedSocket & recvSock);
		bool ProcessRemoteToken (
				BufferedSocket & bufSock,
				std::vector<char> & myToken, 
				bool & isRemoteTokenIncomplete);
		bool IsErrorReportNeeded () const 
		{ 
			return (_connectionParams & ISC_RET_EXTENDED_ERROR) == ISC_RET_EXTENDED_ERROR; 
		}
	private:
		Schannel::Credentials	_schannelCred;
		CtxtHandle				_context;
		std::string				_hostname;
		unsigned long			_connectionParams;
	};

	class Codec
	{
	public:
		Codec (SimpleSocket & sendSock)
			: _sendSock (sendSock),
			  _recvSock (_sendSock)
		{}
		~Codec ();
		void Connect (std::string const & host, short port, int timeout = 0);
		void Negotiate   ();
		void Renegotiate ();
		unsigned int GetMaxMsgSize () const { return _msgLayout.GetMaxMsgSize (); }

		void Send (char const * buf, unsigned int len);
		char const * Receive (unsigned int & size); // size: in: max size expected by client, out: received size
	private:
		void Reset ();
		bool DecryptReceivedData ();
	private:
		SimpleSocket			  & _sendSock;
		Schannel::BufferedSocket	_recvSock;
		Schannel::SecurityContext	_context;
		Schannel::MessageLayout		_msgLayout;
	};

	class OutgoingMessage
	{
	public:
		OutgoingMessage (char const * buf, unsigned int len, Schannel::MessageLayout const & msgLayout);
		
		void Encrypt (CtxtHandle context);

		char const * GetMsgBegin () const { return &_sendBuf [0]; }
		unsigned int GetMsgSize () const { return _msgSize; }
	private:
		std::vector<char>				_sendBuf;
		SecBuffer						_buffers [4];
		SecBufferDesc					_msg;
		unsigned int					_msgSize;
	};
}

class SecureSocket : public Socket
{
public:
	static bool IsSupported ();
public:
	SecureSocket (SimpleSocket & s);
	void Connect (std::string const & host, short port, int timeout = 0);
	void Send (char const * buf, unsigned int len);
	unsigned int Receive (char buf [], unsigned int size);

	std::string const & GetHostName () const { return _socket.GetHostName (); }
	short GetHostPort () const { return _socket.GetHostPort (); }

private:
	SimpleSocket	 & _socket;
	Schannel::Codec	   _schannelCodec;
};

#endif
