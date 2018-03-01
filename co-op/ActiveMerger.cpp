//------------------------------------
//  (c) Reliable Software, 2006 - 2007
//------------------------------------
#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "ActiveMerger.h"
#include "Global.h"
#include "AppInfo.h"

#include <Sys/Process.h>
#include <Ex/Error.h>

ActiveMerger::ActiveMerger (Win::Dow::Handle winNotify, 
							XML::Tree & xmlArgs, 
							std::string const & appPath,
							GlobalId gid)
	: _winNotify (winNotify),
	  _appPath (appPath),
	  _msgActiveMerger (UM_AUTO_MERGER_COMPLETED),
	  _mergedFileGid (gid)
{
	_xmlArgs.swap (xmlArgs);
}

void ActiveMerger::Run ()
{
	dbg << "--> ActiveMerger::Run - " << GlobalIdPack (_mergedFileGid) << std::endl;
	std::ostream out (&_sharedXmlBuf); // use buf as streambuf
	_xmlArgs.Write (out);
	if (out.fail ())
		throw Win::InternalException ("Differ argument string too long.");

	std::ostringstream cmdLine;
	cmdLine << " /xmlspec 0x" << std::hex << _sharedXmlBuf.GetHandle ();

	
	Win::ChildProcess process (cmdLine.str ().c_str (), true);	// Must inherit parent's handles
	process.SetAppName (_appPath);
	process.ShowMinimizedNotActive ();
	bool isSuccess = false;
	if (process.Create ())
	{
		Win::ChildProcess::WaitStatus status = process.WaitForDeath (_event, 30 * 1000);
		if (status != Win::ChildProcess::eventSignalled)
		{
			unsigned retCode = process.GetExitCode ();
			isSuccess = !process.IsAlive () && retCode == 0;
			dbg << "<-- ActiveMerger::Run - " 
				<< GlobalIdPack (_mergedFileGid) << (isSuccess ? " - no conflicts" : " - conflicts detected") << std::endl;
			dbg << "     Merger exit code: 0x" << std::hex << retCode << std::endl;
		}
		else
		{
			dbg << "<-- ActiveMerger::Run - " << GlobalIdPack (_mergedFileGid) << " - killed prematurely."  << std::endl;
		}
	}
	else
	{
		dbg << "<-- ActiveMerger::Run - " << GlobalIdPack (_mergedFileGid) << " - merger didn't start" << std::endl;
		dbg << "    Last system error: " << LastSysErr ().Text () << std::endl;
	}
	// Warning 64-bit
	_msgActiveMerger.SetLParam (reinterpret_cast<long> (this));
	_msgActiveMerger.SetWParam (isSuccess);
	_winNotify.PostMsg (_msgActiveMerger);
}

void ActiveMergerWatcher::Add (XML::Tree & xmlArgs, std::string const & appPath, GlobalId fileGid)
{
	std::unique_ptr<auto_active<ActiveMerger> > merger 
		(new auto_active<ActiveMerger>
			(new ActiveMerger (TheAppInfo.GetWindow (),
								xmlArgs,
								appPath,
								fileGid)));
	merger->SetWaitForDeath (1000);
	_mergers.push_back (std::move(merger));
}

