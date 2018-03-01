// (c) Reliable Software, 2003
#include <WinLibBase.h>
#include "Internet.h"

namespace Internet
{
	Exception::Exception (const char * msg, const char * obj)
		: Win::Exception (msg, obj, Win::GetError (), ::GetModuleHandle ("wininet.dll"))
	{}

	Exception::Exception (const char * msg, const char * obj, unsigned error)
		: Win::Exception (msg, obj, error, ::GetModuleHandle ("wininet.dll"))
	{}

	Access::Access (std::string const & agent)
		: _agent (agent),
			_access (INTERNET_OPEN_TYPE_PRECONFIG), 
			_flags (0), 
			_proxyBypass ("<local>")
	{}

	bool Access::AttemptConnect ()
	{
		return ::InternetAttemptConnect (0) == ERROR_SUCCESS;
	}

	bool Access::CheckConnection (char const * url, bool force)
	{
		BOOL result = ::InternetCheckConnection (url, force? FLAG_ICC_FORCE_CONNECTION: 0, 0);
		if (result == FALSE)
		{
			HRESULT hr = ::GetLastError();
			if (hr == ERROR_INTERNET_LOGIN_FAILURE) // Can't ping without login
				return true;
			return false;
		}
		return true;
	}

	Internet::AutoHandle Access::Open ()
	{
		Internet::AutoHandle h (::InternetOpen (_agent.c_str (), 
												_access, 
												_proxy.c_str (), 
												_proxyBypass.c_str (), 
												_flags));
		if (h.IsNull ())
			throw Internet::Exception ("Cannot open internet connection");
		if ((_flags & INTERNET_FLAG_ASYNC) != 0)
			::InternetSetStatusCallback (h.ToNative (), Callback::Function);
		return h;
	}

	void (CALLBACK Callback::Function)(HINTERNET hInternet,
				DWORD_PTR dwContext,
				DWORD dwInternetStatus,
				LPVOID lpvStatusInformation,
				DWORD dwStatusInformationLength)
	{
		Callback * call = reinterpret_cast<Callback *> (dwContext);
		INTERNET_ASYNC_RESULT * result = static_cast<INTERNET_ASYNC_RESULT *> (lpvStatusInformation);
		if (call == 0)
			return;

		switch (dwInternetStatus)
		{
		case INTERNET_STATUS_CLOSING_CONNECTION:
			// Closing the connection to the server. The lpvStatusInformation parameter is NULL. 
			break;
		case INTERNET_STATUS_CONNECTED_TO_SERVER:
			// Successfully connected to the socket address (SOCKADDR) pointed to by lpvStatusInformation. 
			break;
		case INTERNET_STATUS_CONNECTING_TO_SERVER:
			// Connecting to the socket address (SOCKADDR) pointed to by lpvStatusInformation. 
			break;
		case INTERNET_STATUS_CONNECTION_CLOSED:
			// Successfully closed the connection to the server. The lpvStatusInformation parameter is NULL. 
			break;
		case INTERNET_STATUS_CTL_RESPONSE_RECEIVED:
			// Not implemented. 
			break;
		case INTERNET_STATUS_DETECTING_PROXY:
			// Notifies the client application that a proxy has been detected. 
			break;
		case INTERNET_STATUS_HANDLE_CLOSING:
			// This handle value has been terminated. 
			break;
		case INTERNET_STATUS_HANDLE_CREATED:
			// Used by InternetConnect to indicate it has created the new handle. This lets the application call InternetCloseHandle from another thread, if the connect is taking too long. The lpvStatusInformation parameter contains the address of an INTERNET_ASYNC_RESULT structure. 
			call->OnHandleCreated (reinterpret_cast<HINTERNET> (result->dwResult), result->dwError);
			break;
		case INTERNET_STATUS_INTERMEDIATE_RESPONSE:
			// Received an intermediate (100 level) status code message from the server. 
			break;
		case INTERNET_STATUS_NAME_RESOLVED:
			// Successfully found the IP address of the name contained in lpvStatusInformation. 
			break;
		case INTERNET_STATUS_PREFETCH:
			// Not implemented. 
			break;
		case INTERNET_STATUS_RECEIVING_RESPONSE:
			// Waiting for the server to respond to a request. The lpvStatusInformation parameter is NULL. 
			break;
		case INTERNET_STATUS_REDIRECT:
			// An HTTP request is about to automatically redirect the request. 
			// The lpvStatusInformation parameter points to the new URL. 
			// At this point, the application can read any data returned by the server 
			// with the redirect response and can query the response headers. 
			// It can also cancel the operation by closing the handle. 
			// This callback is not made if the original request specified 
			// INTERNET_FLAG_NO_AUTO_REDIRECT. 
			break;
		case INTERNET_STATUS_REQUEST_COMPLETE:
			// An asynchronous operation has been completed. 
			// The lpvStatusInformation parameter contains the address of an 
			// INTERNET_ASYNC_RESULT structure. 
			call->OnRequestComplete (reinterpret_cast<HINTERNET> (result->dwResult), result->dwError);
			break;
		case INTERNET_STATUS_REQUEST_SENT:
			// Successfully sent the information request to the server. 
			// The lpvStatusInformation parameter points to a DWORD containing 
			// the number of bytes sent. 
			break;
		case INTERNET_STATUS_RESOLVING_NAME:
			// Looking up the IP address of the name contained in lpvStatusInformation. 
			break;
		case INTERNET_STATUS_RESPONSE_RECEIVED:
			// Successfully received a response from the server. 
			// The lpvStatusInformation parameter points to a DWORD containing 
			// the number of bytes received. 
			break;
		case INTERNET_STATUS_SENDING_REQUEST:
			// Sending the information request to the server. The lpvStatusInformation parameter 
			// is NULL. 
			//case INTERNET_STATUS_STATE_CHANGE:
			//Moved between a secure (HTTPS) and a nonsecure (HTTP) site. This can be one of the following values: 
			//	case INTERNET_STATE_CONNECTED
			//	Connected state (mutually exclusive with disconnected state). 
			//	case INTERNET_STATE_DISCONNECTED
			//	Disconnected state. No network connection could be established. 
			//	case INTERNET_STATE_DISCONNECTED_BY_USER
			//	Disconnected by user request. 
			//	case INTERNET_STATE_IDLE
			//	No network requests are being made by the Microsoft® Win32® Internet functions. 
			//	case INTERNET_STATE_BUSY
			//	Network requests are being made by the Win32 Internet functions. 
			break;
		case INTERNET_STATUS_USER_INPUT_REQUIRED:
			// The request requires user input to be completed.
			break;
		default:
			break;
		};
	}
}
