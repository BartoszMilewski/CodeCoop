// ---------------------------
// (c) Reliable Software, 2006
// ---------------------------
#include "WinLibBase.h"
#include "SecureSocket.h"
#include <Sys/SysVer.h>

bool IsFailure		  (SECURITY_STATUS s) { return s < 0;                      }
bool IsOK			  (SECURITY_STATUS s) { return s == SEC_E_OK;              }
bool IsContinueNeeded (SECURITY_STATUS s) { return s == SEC_I_CONTINUE_NEEDED; }

Schannel::Credentials::Credentials ()
{
	::ZeroMemory (&_schannelCred, sizeof (_schannelCred));
	_schannelCred.dwVersion  = SCHANNEL_CRED_VERSION;
	_schannelCred.dwFlags |= SCH_CRED_AUTO_CRED_VALIDATION | SCH_CRED_USE_DEFAULT_CREDS;
	//
	// Create an SSPI credential.
	//
	SECURITY_STATUS status = ::AcquireCredentialsHandle (
		NULL,                   // Name of principal    
		UNISP_NAME_A,           // Name of package
		SECPKG_CRED_OUTBOUND,   // Flags indicating use
		NULL,                   // Pointer to logon ID
		&_schannelCred,         // Package specific data
		NULL,                   // Pointer to GetKey() func
		NULL,                   // Value to pass to GetKey()
		&_packageCred,          // (out) Cred Handle
		NULL);                  // (out) Lifetime (optional)
	if (!IsOK (status))
		throw Win::SecurityException ("Cannot acquire security credentials.", status);
}

Schannel::Credentials::~Credentials ()
{
	::FreeCredentialsHandle (&_packageCred);
}

void Schannel::MessageLayout::Reset (CtxtHandle context)
{
	// Read stream encryption properties.
	SecPkgContext_StreamSizes sizes;

	SECURITY_STATUS status = ::QueryContextAttributes (
		&context,
		SECPKG_ATTR_STREAM_SIZES,
		&sizes);
	if(!IsOK (status))
		throw Win::SecurityException ("Cannot query security context attributes.", status);

	_hdrSize     = sizes.cbHeader;
	_dataSize    = sizes.cbMaximumMessage;
	_trailerSize = sizes.cbTrailer;
	_maxMsgSize  = _hdrSize + _dataSize + _trailerSize;
}

void Schannel::BufferedSocket::Clear ()
{
	ClearRawData ();
	ClearDecodedData ();
	ClearUnprocessedData ();
}

void Schannel::BufferedSocket::Reset (unsigned int size)
{
	unsigned int newSize = size;
	ClearRawData ();
	ClearDecodedData ();
	if (HasUnprocessedData ())
	{
		MoveUnprocessedToBeginning ();
		if (size < _unprocessedSize)
			newSize = _unprocessedSize;
	}
	_data.resize (newSize);
}

char const * Schannel::BufferedSocket::AcceptDecodedData (unsigned int & size)
{
	Assert (HasDecodedData ());
	// return decoded data in chunks with size 
	// not exceeding the max size requested by client
	size = std::min (size, _decodedSize);
	char const * decodedDataPtr = &_data [_decodedStart];
	_decodedStart += size;
	_decodedSize  -= size;
	return decodedDataPtr;
}

void Schannel::BufferedSocket::MoreRawData ()
{
	// can be called only after decoded data is accepted
	Assert (!HasDecodedData ());

	if (HasUnprocessedData ())
	{
		ClearRawData ();
		ClearDecodedData ();
		MoveUnprocessedToBeginning ();
		ConvertUnprocessedToRawData ();
	}
	else
	{
		Assert (_rawDataSize < _data.size ());
		_rawDataSize += _socket.Receive (&_data [0] + _rawDataSize, _data.size () - _rawDataSize);
	}
}

void Schannel::BufferedSocket::MoveUnprocessedToBeginning ()
{
	// move unprocessed to the beginning of the _data
	Assert (HasUnprocessedData ());
	if (_unprocessedStart != 0)
	{
		char const * unprocessedStartPtr = &_data [_unprocessedStart];
		char const * unprocessedEndPtr = unprocessedStartPtr + _unprocessedSize;
		std::copy (unprocessedStartPtr, unprocessedEndPtr, _data.begin ());
		_unprocessedStart = 0;
	}
}

void Schannel::BufferedSocket::SetDecodedData (char const * startPtr, unsigned int size)
{
	Assert (startPtr != 0 && size > 0);
	Assert (!HasDecodedData ());
	Assert (startPtr >= &_data [0]);
	Assert (startPtr + size <= &_data [0] + _rawDataSize);
	_decodedStart = startPtr - &_data [0];
	_decodedSize  = size;
}

void Schannel::BufferedSocket::SetUnprocessedData (char const * startPtr, unsigned int size)
{
	Assert (size != (unsigned int)(-1));
	Assert (startPtr != 0 && size > 0);
	Assert (!HasUnprocessedData ());
	Assert (startPtr >= &_data [0]);
	Assert (startPtr + size <= &_data [0] + _rawDataSize);
	_unprocessedStart = startPtr - &_data [0];
	_unprocessedSize  = size;
}

Schannel::SecurityContext::~SecurityContext ()
{
	::DeleteSecurityContext (&_context);
}

void Schannel::SecurityContext::Negotiate (
		std::string const & hostname,
		SimpleSocket & sendSock, 
		BufferedSocket & recvSock)
{
	_hostname = hostname;
	std::vector<char> token = Initialize (true); // first time
	sendSock.Send (&token [0], token.size ());
	ClientHandshakeLoop (sendSock, recvSock);
}

void Schannel::SecurityContext::Renegotiate (SimpleSocket & sendSock, BufferedSocket & recvSock)
{
	recvSock.Reset ();
	std::vector<char> token = Initialize (false); // not a first time 
	sendSock.Send (&token [0], token.size ());
	ClientHandshakeLoop (sendSock, recvSock);
}

void Schannel::SecurityContext::Shutdown (SimpleSocket & sendSock, BufferedSocket & recvSock)
{
	if (!SecIsValidHandle (&_context))
		return;

	// Notify schannel that we are about to close the connection.
	SecBufferDesc outBufferDesc;
	SecBuffer     outBuffers [1];
	unsigned long type        = SCHANNEL_SHUTDOWN;
	outBuffers [0].pvBuffer   = &type;
	outBuffers [0].BufferType = SECBUFFER_TOKEN;
	outBuffers [0].cbBuffer   = sizeof (type);
	outBufferDesc.cBuffers    = 1;
	outBufferDesc.pBuffers    = outBuffers;
	outBufferDesc.ulVersion   = SECBUFFER_VERSION;

	SECURITY_STATUS status = ::ApplyControlToken (&_context, &outBufferDesc);
	if (IsFailure (status)) 
		throw Win::SecurityException ("Cannot shutdown secure session.", status);

	std::vector<char> myToken;
	status = DoInitialize (&_context, 0, &_context, myToken);
	if (IsFailure (status)) 
		throw Win::SecurityException ("Cannot shutdown secure session.", status);

	// Send the close notify message to the server.
	if (!myToken.empty ())
	{
		sendSock.Send (&myToken [0], myToken.size ());
	}
}

std::vector<char> Schannel::SecurityContext::Initialize (bool isFirstTime)
{
	SecBuffer inBuffers [2];
	inBuffers [0].pvBuffer   = 0;
	inBuffers [0].cbBuffer   = 0;
	inBuffers [0].BufferType = SECBUFFER_TOKEN;
	inBuffers [1].pvBuffer   = NULL;
	inBuffers [1].cbBuffer   = 0;
	inBuffers [1].BufferType = SECBUFFER_EMPTY;

	SecBufferDesc inBufferDesc;
	inBufferDesc.cBuffers  = 2;
	inBufferDesc.pBuffers  = inBuffers;
	inBufferDesc.ulVersion = SECBUFFER_VERSION;

	CtxtHandle    * context     = NULL;
	SecBufferDesc * inBufferPtr = NULL;
	CtxtHandle    * newContext  = NULL;
	if (isFirstTime)
	{
		newContext  = &_context;
	}
	else
	{
		context     = &_context;
		inBufferPtr = &inBufferDesc;
	}

	std::vector<char> token;
	SECURITY_STATUS status = DoInitialize (context, inBufferPtr, newContext, token);
	if (!IsContinueNeeded (status))
		throw Win::SecurityException ("Cannot initialize security context.", status);

	return token;
}

void Schannel::SecurityContext::ClientHandshakeLoop (SimpleSocket & sendSock, BufferedSocket & recvSock)
{
	// Loop until the handshake is finished or an error occurs.
	bool isDone = false;
	do 
	{
		std::vector<char> myToken;
		bool isIncompleteRemoteToken = false;
		do 
		{
			recvSock.MoreRawData ();
			try
			{
				isDone = ProcessRemoteToken (
								recvSock,
								myToken, 
								isIncompleteRemoteToken);
			}
			catch (Win::SecurityException)
			{
				if (IsErrorReportNeeded () && !myToken.empty ())
				{
					sendSock.Send (&myToken [0], myToken.size ());
				}
				throw;
			}
		} while (isIncompleteRemoteToken);
		
		recvSock.AcceptRawData ();
		
		if (!myToken.empty ())
		{
			sendSock.Send (&myToken [0], myToken.size ());
		}
	} while (!isDone);
}

// return true if the token was successfully processed, 
// otherwise if continuation is needed return false
bool Schannel::SecurityContext::ProcessRemoteToken (
		BufferedSocket & bufSock,
		std::vector<char> & myToken, 
		bool & isRemoteTokenIncomplete)
{
	myToken.clear ();
	isRemoteTokenIncomplete = false;

	// Set up the input buffers. Buffer 0 is used to pass in data
	// received from the remote party. Schannel will consume some or all
	// of this. Leftover data (if any) will be signaled in buffer 1, 
	// by giving it the SECBUFFER_EXTRA type.

	SecBufferDesc   inBufferDesc;
	SecBuffer       inBuffers [2];
	inBuffers[0].pvBuffer   = bufSock.GetRawData ();
	inBuffers[0].cbBuffer   = bufSock.GetRawDataSize ();
	inBuffers[0].BufferType = SECBUFFER_TOKEN;

	inBuffers[1].pvBuffer   = NULL;
	inBuffers[1].cbBuffer   = 0;
	inBuffers[1].BufferType = SECBUFFER_EMPTY;

	inBufferDesc.cBuffers   = 2;
	inBufferDesc.pBuffers   = inBuffers;
	inBufferDesc.ulVersion  = SECBUFFER_VERSION;

	SECURITY_STATUS status = DoInitialize (&_context, &inBufferDesc, NULL, myToken);

	// we need to read more data from the server and try again
	if (status == SEC_E_INCOMPLETE_MESSAGE)
	{
		isRemoteTokenIncomplete = true;
		return false;
	}

	if (IsFailure (status))
		throw Win::SecurityException ("Failed to negotiate secure connection.", status);

	// here we expect only two success codes out of the whole range 
	if (!IsOK (status) && !IsContinueNeeded (status))
		throw Win::SecurityException ("Failed to negotiate secure connection.", status);

	// either handshake completed successfully
	// or we need to continue exchanging token with remote party
	if (inBuffers [1].BufferType == SECBUFFER_EXTRA)
	{
		// the "extra" buffer contains data:
		// SEC_E_OK: this is encrypted application protocol layer stuff.
		// It needs to be saved. 
		// The application layer will later decrypt it with DecryptMessage.
		// SEC_I_CONTINUE_NEEDED: Copy any leftover data from the "extra" buffer and 
		// go around again.
		// Notice: inBuffers [1].pvBuffer is NULL. 
		// Use only the inBuffers [1].cbBuffer to calculate extra data position
		bufSock.SetUnprocessedData (
			bufSock.GetRawData () + bufSock.GetRawDataSize () - inBuffers [1].cbBuffer,
			inBuffers [1].cbBuffer);
	}

	return IsOK (status);
}

SECURITY_STATUS Schannel::SecurityContext::DoInitialize (
	CtxtHandle    * context,
	SecBufferDesc * inBufferPtr,
	CtxtHandle    * newContext,
	std::vector<char> & token)
{
	SecBufferDesc   outBuffer;
	SecBuffer       outBuffers [1];
	outBuffers [0].pvBuffer   = NULL;
	outBuffers [0].BufferType = SECBUFFER_TOKEN;
	outBuffers [0].cbBuffer   = 0;
	outBuffer.cBuffers        = 1;
	outBuffer.pBuffers        = outBuffers;
	outBuffer.ulVersion       = SECBUFFER_VERSION;

	unsigned long dwSSPIFlags = 
		ISC_REQ_SEQUENCE_DETECT   |
		ISC_REQ_REPLAY_DETECT     |
		ISC_REQ_CONFIDENTIALITY   |
		ISC_RET_EXTENDED_ERROR    |
		ISC_REQ_ALLOCATE_MEMORY   |
		ISC_REQ_STREAM;
	SECURITY_STATUS	status = ::InitializeSecurityContext (
		&_schannelCred.ToNative (),
		context,
		const_cast<char*>(_hostname.c_str ()),
		dwSSPIFlags,
		0,
		0,
		inBufferPtr,
		0,
		newContext,
		&outBuffer,
		&_connectionParams,
		NULL);

	if (outBuffers [0].cbBuffer != 0 && outBuffers [0].pvBuffer != 0)
	{
		token.assign (static_cast<char const *>(outBuffers [0].pvBuffer), 
					  static_cast<char const *>(outBuffers [0].pvBuffer) + outBuffers [0].cbBuffer);
		::FreeContextBuffer (outBuffers [0].pvBuffer);
	}
	return status;
}

Schannel::OutgoingMessage::OutgoingMessage (
	char const * buf, 
	unsigned int len, 
	Schannel::MessageLayout const & msgLayout)
{
	Assert (len <= msgLayout.GetDataSize ());

	_sendBuf.resize (msgLayout.GetHdrSize () + len + msgLayout.GetTrailerSize ());

	std::copy (buf, buf + len, &_sendBuf [0] + msgLayout.GetHdrSize ());

	_buffers [0].pvBuffer     = &_sendBuf [0];
	_buffers [0].cbBuffer     = msgLayout.GetHdrSize ();
	_buffers [0].BufferType   = SECBUFFER_STREAM_HEADER;

	_buffers [1].pvBuffer     = &_sendBuf [0] + msgLayout.GetHdrSize ();
	_buffers [1].cbBuffer     = len;
	_buffers [1].BufferType   = SECBUFFER_DATA;

	_buffers [2].pvBuffer     = &_sendBuf [0] + msgLayout.GetHdrSize () + len;
	_buffers [2].cbBuffer     = msgLayout.GetTrailerSize ();
	_buffers [2].BufferType   = SECBUFFER_STREAM_TRAILER;

	_buffers [3].BufferType   = SECBUFFER_EMPTY;

	_msg.ulVersion = SECBUFFER_VERSION;
	_msg.cBuffers  = 4;
	_msg.pBuffers  = _buffers;

}

void Schannel::OutgoingMessage::Encrypt (CtxtHandle context)
{
	// Encrypt
	SECURITY_STATUS status = ::EncryptMessage (&context, 0, &_msg, 0);
	if(IsFailure (status))
		throw Win::SecurityException ("Failed to encrypt outgoing message.", status);

	_msgSize = _buffers [0].cbBuffer + _buffers [1].cbBuffer + _buffers [2].cbBuffer;
}

Schannel::Codec::~Codec ()
{
	try
	{
		_context.Shutdown (_sendSock, _recvSock);
	}
	catch (...)
	{}
}

void Schannel::Codec::Connect (std::string const & host, short port, int timeout)
{
	_sendSock.Connect (host, port, timeout);
}

void Schannel::Codec::Negotiate () 
{ 
	_context.Negotiate (_sendSock.GetHostName (), _sendSock, _recvSock); 
	_msgLayout.Reset (_context.ToNative ());
	_recvSock.Reset (_msgLayout.GetMaxMsgSize ());
}

void Schannel::Codec::Renegotiate () 
{ 
	_context.Renegotiate (_sendSock, _recvSock); 
	_msgLayout.Reset (_context.ToNative ());
	_recvSock.Reset (_msgLayout.GetMaxMsgSize ());
}

void Schannel::Codec::Send (char const * buf, unsigned int len)
{
	Schannel::OutgoingMessage msg (buf, len, _msgLayout);
	msg.Encrypt (_context.ToNative ());
	_sendSock.Send (msg.GetMsgBegin (), msg.GetMsgSize ());
}

char const * Schannel::Codec::Receive (unsigned int & size) 
// size: in: max size expected by client, out: received size
{
	if (!_recvSock.HasDecodedData ())
	{
		bool isDone = false;
		do
		{
			_recvSock.MoreRawData ();
			isDone = DecryptReceivedData ();
		}while (!isDone);

		_recvSock.AcceptRawData ();
	}

	return _recvSock.AcceptDecodedData (size);
}

// return true if decryption was finished successfully
bool Schannel::Codec::DecryptReceivedData ()
{
	SecBuffer inBuffers [4];
	inBuffers [0].pvBuffer   = _recvSock.GetRawData ();
	inBuffers [0].cbBuffer   = _recvSock.GetRawDataSize ();
	inBuffers [0].BufferType = SECBUFFER_DATA;
	inBuffers [1].BufferType = SECBUFFER_EMPTY;
	inBuffers [2].BufferType = SECBUFFER_EMPTY;
	inBuffers [3].BufferType = SECBUFFER_EMPTY;

	SecBufferDesc inBufferDesc;
	inBufferDesc.ulVersion = SECBUFFER_VERSION;
	inBufferDesc.cBuffers  = 4;
	inBufferDesc.pBuffers  = inBuffers;

	// Attempt to decrypt the data
	SECURITY_STATUS status = ::DecryptMessage (&_context.ToNative (), &inBufferDesc, 0, NULL);
	/*
	Revisit:
	Windows 2000 Professional:  Schannel returns SEC_E_OK instead of SEC_I_CONTEXT_EXPIRED. 
	If the length of the output buffer is zero and the first byte of the encrypted packet is 0x15, 
	the application can safely assume that the message was a close_notify message and 
	change the return value to SEC_I_CONTEXT_EXPIRED.
	*/
	if (IsOK (status))
	{
		// Locate data and (optional) extra buffers.
		SecBuffer *     dataBuffer = 0;
		SecBuffer *     extraBuffer = 0;
		for (int i = 1; i < 4; i++)
		{
			if (dataBuffer == NULL && inBuffers [i].BufferType == SECBUFFER_DATA)
				dataBuffer = &inBuffers [i];
			if (extraBuffer == NULL && inBuffers [i].BufferType == SECBUFFER_EXTRA)
				extraBuffer = &inBuffers [i];
		}
		if (dataBuffer)
		{
			_recvSock.SetDecodedData (
						static_cast<char const *>(dataBuffer->pvBuffer), 
						dataBuffer->cbBuffer);
		}
		if (extraBuffer)
		{
			_recvSock.SetUnprocessedData (
						static_cast<char const *>(extraBuffer->pvBuffer), 
						extraBuffer->cbBuffer);
		}

		if (dataBuffer == 0 && extraBuffer == 0)
		{
			// no decoded data, no unprocessed data
			_recvSock.Clear ();
		}
	}
	else if (status == SEC_E_INCOMPLETE_MESSAGE)
	{
		// The input buffer contains only a fragment of an
		// encrypted record. Loop around and read some more
		// data.
	}
	else
		throw Win::SecurityException ("Failed to decrypt incoming message.", status);

	return status == SEC_E_OK;
}

bool SecureSocket::IsSupported ()
{
	// EncryptMessage, DecryptMessage APIs supported on Windows 2000 and XP
	SystemVersion sysVer;
	if (sysVer.IsWin32Windows () ||					  // Win95, 98, Me
		sysVer.IsWinNT () && sysVer.MajorVer () == 4) // WinNT
	{
		return false;
	}
	return true;
}

SecureSocket::SecureSocket (SimpleSocket & s)
: _socket (s),
  _schannelCodec (_socket)
{
	if (!IsSupported ())
		throw Win::InternalException ("Secure internet connections (SSL) are not supported by the operating system.");

	_schannelCodec.Negotiate ();
}

void SecureSocket::Connect (const std::string & host, short port, int timeout)
{
	Assert (!"Should never be called.");
}

void SecureSocket::Send (char const * buf, unsigned int len)
{
	// split data to be sent into chunks with size 
	// not exceeding the max schannel message size
	unsigned int const maxMsgSize = _schannelCodec.GetMaxMsgSize ();
	unsigned int const chunkSize = std::min (maxMsgSize, len);
	unsigned int curr = 0;
	do
	{
		_schannelCodec.Send (buf + curr, std::min (chunkSize, len - curr));
		curr += chunkSize;
	}while (curr < len);
}

unsigned int SecureSocket::Receive (char buf [], unsigned int size)
{
	char const * startPos = 0;
	unsigned int sizeReceived = size;
	bool isSuccess = false;
	do 
	{
		try
		{
			startPos = _schannelCodec.Receive (sizeReceived);
			isSuccess = true;
		}
		catch (Win::SecurityException e)
		{
			if (!e.IsRenegotiate ())
				throw;
		}

		if (!isSuccess)
		{
			_schannelCodec.Renegotiate ();
		}

	} while (!isSuccess);

	std::copy (startPos, startPos + sizeReceived, buf);
	return sizeReceived;
}
