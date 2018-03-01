//----------------------------------
// (c) Reliable Software 2000 - 2007
//----------------------------------

#include <WinLibBase.h>

#include "Process.h"
#include "File/Pipe.h"

using namespace Win;

ProcessProxy::ProcessProxy (char const * className, char const * windowName)
{
	Find (className, windowName);
}

ProcessProxy::ProcessProxy (unsigned int threadId)
{
	::EnumThreadWindows (threadId, &EnumCallback, reinterpret_cast<LPARAM>(this));
}

void ProcessProxy::Find (char const * className, char const * windowName)
{
	_win.Reset (::FindWindow (className, windowName));
}

bool ProcessProxy::PostMsg (int msg)
{
	if (!_win.IsNull ())
	{
		return _win.PostMsg (msg);
	}
	return false;
}

bool ProcessProxy::PostMsg (Win::Message & msg)
{
	if (!_win.IsNull ())
	{
		return _win.PostMsg (msg);
	}
	return false;
}

// Returns true if the process died
bool ProcessProxy::Kill (int timeout)
{
	if (_win.IsNull ())
		return true;

	DWORD processId = 0;
	DWORD threadId = ::GetWindowThreadProcessId (_win.ToNative (), &processId);
	if (processId == 0)
		return true; // window is gone

	HANDLE processHandle = ::OpenProcess (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE,
										FALSE, 
										processId);
	if (processHandle == 0)
		return false; // can't open process->can't kill it
	PostMsg (WM_DESTROY);
	_win.Reset ();
	// Wait for process death 
	bool success = ::WaitForSingleObject (processHandle, timeout) == WAIT_OBJECT_0;
	::CloseHandle (processHandle);
	return success;
}

BOOL CALLBACK ProcessProxy::EnumCallback (HWND hwnd, LPARAM lParam)
{
	// Revisit: what if thread has multiple nonchild windows?
	Assert (lParam != 0);
	ProcessProxy * thisProcessProxy = reinterpret_cast<ProcessProxy *>(lParam);
	thisProcessProxy->Init (hwnd);
	return FALSE;
}

ChildProcess::ChildProcess (std::string const & cmdLine, bool inheritParentHandles)
	: _cmdLine (cmdLine),
	  _inheritParentHandles (inheritParentHandles),
	  _createSuspended(false)
{
	InitStartupInfo();
}

ChildProcess::ChildProcess ()
	: _inheritParentHandles (false),
	  _createSuspended(false)
{
	InitStartupInfo();
}

void ChildProcess::InitStartupInfo()
{
	memset (&_processInfo, 0, sizeof (PROCESS_INFORMATION));
	memset (&_startupInfo, 0, sizeof (STARTUPINFO));
	_startupInfo.hStdInput = INVALID_HANDLE_VALUE;
	_startupInfo.hStdOutput = INVALID_HANDLE_VALUE;
	_startupInfo.hStdError = INVALID_HANDLE_VALUE;
	_startupInfo.cb = sizeof (STARTUPINFO);
}

ChildProcess::~ChildProcess()
{
	if (_processInfo.hProcess != 0)
	{
		::CloseHandle(_processInfo.hProcess);
	}

	if (_processInfo.hThread != 0)
	{
		::CloseHandle(_processInfo.hThread);
	}
}

void ChildProcess::ShowMinimizedNotActive ()
{
	_startupInfo.dwFlags |= STARTF_USESHOWWINDOW;
	_startupInfo.wShowWindow = SW_SHOWMINNOACTIVE;
}

void ChildProcess::ShowNormal ()
{
	_startupInfo.dwFlags |= STARTF_USESHOWWINDOW;
	_startupInfo.wShowWindow = SW_SHOWNORMAL;
}

void ChildProcess::RedirectOutput(Pipe const & pipe)
{
	_startupInfo.dwFlags |= STARTF_USESTDHANDLES;
	_startupInfo.hStdOutput = pipe.ToNativeWrite();
	_inheritParentHandles = true;
}

bool ChildProcess::Create (unsigned timeoutMilliSec)
{
	char const * appName = _appName.empty () ? 0 : _appName.c_str ();
	char const * curFolder = _curFolder.empty () ? 0 : _curFolder.c_str ();
	char const * cmdLine = _cmdLine.empty () ? 0 : _cmdLine.c_str ();
	DWORD dwCreationFlags = NORMAL_PRIORITY_CLASS;
	if (_createSuspended)
	{
		dwCreationFlags |= CREATE_SUSPENDED;
	}
	BOOL result = ::CreateProcess (appName,	// pointer to name of executable module
								   const_cast<char *>(cmdLine),	// pointer to command line string
								   0,		// process security attributes
								   0,		// thread security attributes  
								   _inheritParentHandles ? TRUE : FALSE,// handle inheritance flag
								   dwCreationFlags,	// creation flags
								   0,		// pointer to New environment block
								   curFolder,// pointer to current directory name
								   &_startupInfo,
								   &_processInfo);
	if (result != FALSE)
	{
		// Wait till child process finishes its initialization or times out
		if (timeoutMilliSec == 0)
		{
			return true;
		}
		else
		{
			unsigned waitResult = ::WaitForInputIdle (_processInfo.hProcess, timeoutMilliSec);
			return (waitResult == 0 || waitResult == WAIT_TIMEOUT);
		}
	}
	else
		return false;
}

void ChildProcess::Resume ()
{
	::ResumeThread(_processInfo.hThread);
}

bool ChildProcess::WaitForDeath (unsigned int timeout) const
{
	DWORD result = ::WaitForSingleObject (_processInfo.hProcess, timeout);
	return result == WAIT_OBJECT_0;
}

ChildProcess::WaitStatus ChildProcess::WaitForDeath (Win::Event const & event, unsigned int timeout) const
{
    HANDLE objects [2];
	objects [0] = _processInfo.hProcess;
	objects [1] = event.ToNative ();
	DWORD waitStatus = ::WaitForMultipleObjects (2, objects, FALSE, timeout);
	return static_cast<WaitStatus> (waitStatus);
}

bool ChildProcess::IsAlive () const
{
	unsigned long exitCode = GetExitCode ();
	return exitCode == STILL_ACTIVE;
}

void ChildProcess::Terminate ()
{
	::TerminateProcess (_processInfo.hProcess, -1);
	_processInfo.hProcess = 0;
	_processInfo.hThread = 0;
	_processInfo.dwProcessId = 0;
	_processInfo.dwThreadId = 0;
}

unsigned long ChildProcess::GetExitCode () const
{
	unsigned long exitCode;
	::GetExitCodeProcess (_processInfo.hProcess, &exitCode);
	return exitCode;
}
