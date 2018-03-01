#if !defined (INTERNET_H)
#define INTERNET_H
// (c) Reliable Software, 2003
#include <wininet.h>
#include <Win/Handles.h>
namespace Internet
{
	enum Protocol { FTP, HTTP, GOPHER };
	class Exception: public Win::Exception
	{
	public:
		Exception (const char * msg, const char * obj = 0);
		Exception (const char * msg, const char * obj, unsigned error);
	};

	template<class BaseHandle>
	struct Disposal
	{
		static void Dispose (BaseHandle h) throw () 
		{
			::InternetCloseHandle (h.ToNative ());
		}
	};

	class Handle: public Win::Handle<HINTERNET>
	{
	public:
		Handle (HINTERNET h = 0): Win::Handle<HINTERNET> (h)
		{}
	};

	typedef Win::AutoHandle<Internet::Handle, Disposal<Internet::Handle> > AutoHandle;

	class Access
	{
	public:
		Access (std::string const & agent);
		static bool AttemptConnect ();
		static bool CheckConnection (char const * url, bool force = false);
		void UsePreconfigProxy () { _access = INTERNET_OPEN_TYPE_PRECONFIG; }
		void UsePreconfigProxyNoAuto () { _access = INTERNET_OPEN_TYPE_PRECONFIG_WITH_NO_AUTOPROXY; }
		void UseNoProxy () { _access = INTERNET_OPEN_TYPE_DIRECT; }
		void UseProxy (std::string const & proxy)
		{
			_proxy = proxy;
			_access = INTERNET_OPEN_TYPE_PROXY;
		}
		void SetProxyBypass (std::string const & bypass) { _proxyBypass = bypass; }
		void WorkOffline () { _flags |= INTERNET_FLAG_OFFLINE; }
		void WorkAsync () { _flags |= INTERNET_FLAG_ASYNC; }

		Internet::AutoHandle Open ();
	private:
		std::string _agent;
		std::string _proxy;
		unsigned	_access;
		unsigned	_flags;
		std::string _proxyBypass;
	};

	class Callback
	{
		friend class Access;
	public:
		virtual ~Callback () {}
		virtual void OnHandleCreated (Internet::Handle h, unsigned error) {}
		virtual void OnRequestComplete (Internet::Handle h, unsigned error) {}
	private:
		static void (CALLBACK Function)(HINTERNET hInternet,
				DWORD_PTR dwContext,
				DWORD dwInternetStatus,
				LPVOID lpvStatusInformation,
				DWORD dwStatusInformationLength);
	};
}

#endif
