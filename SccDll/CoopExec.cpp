//------------------------------------
//  (c) Reliable Software, 2000 - 2007
//------------------------------------

#include "precompiled.h"
#include "CoopExec.h"
#include "PathRegistry.h"
#include "Global.h"

#include <Sys/Process.h>
#include <File/Path.h>
#include <File/File.h>


CoopExec::CoopExec ()
{
	// Check registry key created during setup
	FilePath path;
	std::string pgmPath = Registry::GetProgramPath ();
	if (!pgmPath.empty ())
	{
		if (File::Exists (pgmPath.c_str ()))
		{
			path.Change (pgmPath);
		}
		else
		{
			throw Win::Exception ("Cannot locate Code Co-op.");
		}
	}
	_coopPath = path.GetFilePath ("Co-op.exe");
}

bool CoopExec::Start ()
{
	return Execute ("", true);	// Show normal
}

bool CoopExec::StartServer (std::string const & bufferName, unsigned int keepAliveTimeout, bool stayInProject)
{
	std::string cmdLine ("-NoGUI ");
	cmdLine += Conversation;
	cmdLine += ':';
	cmdLine += bufferName;
	cmdLine += ' ';
	cmdLine += KeepAliveTimeout;
	cmdLine += ":\"";
	cmdLine += ::ToString (keepAliveTimeout);
	cmdLine += "\" ";
	cmdLine += StayInProject;
	cmdLine += ":\"";
	if (stayInProject)
		cmdLine += "yes";
	else
		cmdLine += "no";
	cmdLine += "\"";
	return Execute (cmdLine, false);
}

bool CoopExec::Execute (std::string const & cmd, bool showNormal)
{
	std::string cmdLine;
	cmdLine += '"';
	cmdLine += _coopPath;
	cmdLine += "\" ";
	cmdLine += cmd;
	Win::ChildProcess coop (cmdLine.c_str ());
	if (showNormal)
		coop.ShowNormal ();
	else
		coop.ShowMinimizedNotActive ();
	return coop.Create ();
}
