#if !defined (SERVICE_H)
#define SERVICE_H
//-----------------------------------
// (c) Reliable Software 2007
//-----------------------------------

#include "Winsvc.h"

#include <Win/Handles.h>

namespace Service
{
	unsigned const CustomCommandMin = 128;
	unsigned const CustomCommnadMax = 255;

	template<class BaseHandle>
	struct Disposal
	{
		static void Dispose (BaseHandle h) throw () 
		{
			::CloseServiceHandle (h.ToNative ());
		}
	};

	class Handle: public Win::Handle<SC_HANDLE>
	{
	public:
		Handle (SC_HANDLE h = 0): Win::Handle<SC_HANDLE> (h)
		{}
	};

	typedef Win::AutoHandle<Service::Handle, Disposal<Service::Handle> > AutoHandle;

	class Status : public SERVICE_STATUS
	{
	public:
		Status ()
		{
			dwServiceType = SERVICE_WIN32;
			dwCurrentState = SERVICE_START_PENDING;
			dwControlsAccepted = SERVICE_ACCEPT_STOP;
			dwWin32ExitCode = 0;
			dwServiceSpecificExitCode = 0;
			dwCheckPoint = 0;
			dwWaitHint = 0;
		}

		void SetRunning () { dwCurrentState = SERVICE_RUNNING; }
		void SetStopped () { dwCurrentState = SERVICE_STOPPED; }
		void SetPaused ()  { dwCurrentState = SERVICE_PAUSED;  }
	};

	class Installer
	{
	public:
		Installer (std::string const & serviceName,	// For identification in service database
				   std::string const & displayName,	// For display only
				   std::string const & exePath)
			: _serviceName (serviceName),
			  _displayName (displayName),
			  _exePath (exePath)
		{}

		void Install ();
		void Deinstall ();

	private:
		std::string	_serviceName;
		std::string	_displayName;
		std::string	_exePath;
	};

	class Base
	{
	public:
		struct Context
		{
			std::string	const *			_serviceName;
			Service::Base *				_thisService;
			SERVICE_STATUS_HANDLE		_statusHandle;	// Doesn't have to be closed!
		};

	protected:
		class Log
		{
		public:
			void SetPath (std::string const & path) { _path = path; }
			void Write (std::string const & msg);

		private:
			std::string	_path;
		};

	protected:
		Base (std::string const & name, std::string const & displayName);

	public:
		virtual void Run (std::vector<std::string> const & args) = 0;
		virtual void OnStop () = 0;
		virtual void OnPause () = 0;
		virtual void OnContinue (Service::Status & status) = 0;
		virtual void OnInterrogate (Service::Status & status) = 0;
		virtual void OnCustomCommand (unsigned command) = 0;

		void SetLogPath (std::string const & path) { _log.SetPath (path); }

		bool Initialize ();

	private:
		static void WINAPI ServiceMain (DWORD argc, LPTSTR * argv);
		static DWORD WINAPI ServiceControlHandlerEx (DWORD dwControl,
													 DWORD dwEventType,
													 LPVOID lpEventData,
													 LPVOID lpContext);

	private:
		static Service::Base::Context	_context;

	protected:
		Service::Base::Log				_log;
		std::string						_name;
		std::string						_displayName;
	};
}

extern char const * serviceName;

#endif