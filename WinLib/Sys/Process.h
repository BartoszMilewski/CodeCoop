#if !defined (PROCESS_H)
#define PROCESS_H
//----------------------------------
// (c) Reliable Software 2000 - 2007
//----------------------------------

#include <Sys/Synchro.h>

class Pipe;

namespace Win
{
	class Message;

	class ProcessProxy
	{
	public:
		ProcessProxy (char const * className, char const * windowName = 0);
		ProcessProxy (unsigned int threadId);
		ProcessProxy (Win::Dow::Handle hwnd = Win::Dow::Handle ())
			: _win (hwnd)
		{}

		void Init (HWND hwnd) { _win.Init (hwnd); }
		void Find (char const * className, char const * windowName = 0);

		Win::Dow::Handle GetWin () const { return _win; }

		bool PostMsg (int msg);
		bool PostMsg (Win::Message & msg);
		bool Kill (int timeout = 10000); // timeout 10 seconds

	private:
		static BOOL CALLBACK EnumCallback (HWND hwnd, LPARAM lParam);

	private:
		Win::Dow::Handle _win;
	};

	class ChildProcess
	{
	public:
		enum WaitStatus
		{
			processDied = WAIT_OBJECT_0,
			eventSignalled = WAIT_OBJECT_0 + 1,
			timedOut = WAIT_TIMEOUT
		};
	public:
		ChildProcess (std::string const & cmdLine, bool inheritParentHandles = false);
		ChildProcess ();
		~ChildProcess();

		operator HANDLE () { return _processInfo.hProcess; }
		void ShowMinimizedNotActive ();
		void ShowNormal ();
		void SetCurrentFolder (std::string const & curFolder) { _curFolder = curFolder; }
		void SetInheritParentHandles() { _inheritParentHandles = true; }
		void SetAppName (std::string const & appName) { _appName = appName; }
		void SetCmdLine(std::string const & cmdLine) { _cmdLine = cmdLine; }
		void SetSuspended() { _createSuspended = true; }
		void SetNoFeedbackCursor() { _startupInfo.dwFlags = STARTF_FORCEOFFFEEDBACK; }
		void RedirectOutput(Pipe const & pipe);

		bool Create (unsigned timeoutMilliSec = 1000); // 0 - don't wait at all
		void Resume ();
		bool WaitForDeath (unsigned int timeout = 5000) const;	// Default 5 sec. timeout
		WaitStatus WaitForDeath (Win::Event const & event, unsigned int timeout = 5000) const;
		bool IsAlive () const;
		void Terminate ();
		unsigned long GetExitCode () const;
		unsigned int GetThreadId () const { return _processInfo.dwThreadId; }
	private:
		void InitStartupInfo();
	private:
		std::string			_cmdLine;
		std::string			_appName;
		std::string			_curFolder;
		bool				_inheritParentHandles;
		bool				_createSuspended;
		PROCESS_INFORMATION	_processInfo;
		STARTUPINFO			_startupInfo; 	   
	};
}

#endif
