//-----------------------------------------
// (c) Reliable Software 2001 -- 2003
//-----------------------------------------

#include "precompiled.h"
#include "CmdArgs.h"
#include "SccErrorOut.h"
#include "SccProxyEx.h"
#include "SccOptions.h"
#include "GlobalId.h"
#include "CmdLineVersionLabel.h"

#include <Ex/WinEx.h>
#include <Ex/Error.h>
#include <Dbg/Assert.h>

#include <iostream>

int main (int count, char * args [])
{
	int retCode = 0;
	try
	{
		StandardSwitch validSwitch;
		validSwitch.SetVersion ();
		validSwitch.SetProject ();
		CmdArgs cmdArgs (count, args, validSwitch);
		if (cmdArgs.IsHelp () || cmdArgs.IsEmpty ())
		{
			std::cout << CMDLINE_VERSION_LABEL << ".\n\n";
			std::cout << "Display next version id to be used in the project during checkin\n";
			std::cout << "or the current project version (the id of the last accepted script).\n\n";
			std::cout << "versionid <options> <file> or\n";
			std::cout << "versionid <options> -p:<project id>\n";
			std::cout << "options:\n";
			std::cout << "   -v:current -- display current version id";
		}
		else
		{
			SccProxyEx sccProxy (&SccErrorOut);
			unsigned int versionId;
			CodeCoop::SccOptions options;
			if (cmdArgs.IsVersionId ())
			{
				std::string const & versionId = cmdArgs.GetVersionId ();
				if (versionId == "current")
				{
					options.SetCurrent ();
				}
				else
				{
					std::cout << "versionid: unrecognized version id '" << versionId << "'\n";;
					return 1;
				}
			}
			if (cmdArgs.Size () != 0)
			{
				// By file path or root project path
				char const ** paths = cmdArgs.GetFilePaths ();
				versionId = sccProxy.VersionId (paths [0], options);
			}
			else
			{
				// By project id
				Assert (cmdArgs.GetProjectId () != -1);
				versionId = sccProxy.VersionId (cmdArgs.GetProjectId (), options);
			}
			if (versionId == -1)
			{
				std::cerr << "versionid: Cannot get version id." << std::endl;
				retCode = 1;
			}
			else
			{
				GlobalIdPack pack (versionId);
				std::cout << pack.ToString ();
			}
		}
	}
	catch (Win::Exception e)
	{
		std::cerr << "versionid: " << e.GetMessage () << std::endl;
		SysMsg msg (e.GetError ());
		if (msg)
			std::cerr << "System tells us: " << msg.Text ();
		std::string objectName (e.GetObjectName ());
		if (!objectName.empty ())
			std::cerr << "    " << objectName << std::endl;
		retCode = 1;
	}
	catch (...)
	{
		Win::ClearError ();
		std::cerr << "versionid: Unknown problem" << std::endl;
		retCode = 1;
	}
	return retCode;
}
