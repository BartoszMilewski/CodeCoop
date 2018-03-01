//------------------------------------
//  (c) Reliable Software, 2000 - 2009
//------------------------------------

#include <WinLibBase.h>
#include "Log.h"

#include <File/Path.h>
#include <File/File.h>

using namespace Dbg;

#if defined DIAGNOSTIC
// global log object
Dbg::Log Dbg::TheLog;
#endif

Log::~Log ()
{
	Close ();
}

void Log::Close ()
{
	if (IsOn ())
	{
		_isOn = false;
		_out.close ();
	}
}

void Log::Open (char const * fileName, char const * path)
{
	try
	{
		if (IsOn ())
			Close ();
		std::string logName = FormatLogName (fileName);
		FilePath logPath (path);
		// use current directory if necessary
		if (logPath.IsDirStrEmpty () || !File::Exists (logPath.GetDir ()))
		{
			CurrentFolder curFolder;
			logPath.Change (curFolder);
		}
		_out.Open (logPath.GetFilePath (logName.c_str ()));
		_isOn = _out.good ();
	}
	catch (...)
	{
		_isOn = false;
		Win::ClearError ();
	}
}

void Log::Write (std::string const & component, char const * msg)
{
	if (!IsOn () && !_monitor.IsRunning ())
		return;

	try
	{
		Monitor::InfoMsg info (_monitor.GetSourceId (), msg);
		_monitor.Write (info);
		if (IsOn ())
			_out << info.c_str () << std::flush;
	}
	catch ( ... ) 
	{
		Win::ClearError ();
	}
}

std::string Log::FormatLogName (std::string const & name) const
{
	// Prepend thread id to the name
	unsigned long threadId = ::GetCurrentThreadId ();
	std::ostringstream logName;
	logName << std::hex << threadId << "-" << name;
	return logName.str ();
}
