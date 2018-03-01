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
			std::cout << "Display project version description." << std::endl;
			std::cout << "Project is identified either by file that belongs to it or project id." << std::endl << std::endl;
			std::cout << "report <options> <file> or" << std::endl;
			std::cout << "report <options> -p:<project id>" << std::endl;
			std::cout << "options:" << std::endl;
			std::cout << "   -v:current -- display current project version description" << std::endl;
			std::cout << "   -v:<version id> -- display this project version description" << std::endl;
			std::cout << "                      version id in format 'xx-xxx' where 'x' is a" << std::endl;
			std::cout << "                      hexadecimal digit or as regular hexadecimal number" << std::endl;
		}
		else
		{
			SccProxyEx sccProxy (&SccErrorOut);
			GlobalId versionGid = gidInvalid;	// Assume current project version
			if (cmdArgs.IsVersionId ())
			{
				std::string const & versionId = cmdArgs.GetVersionId ();
				if (versionId != "current")
				{
					GlobalIdPack pack (versionId.c_str ());
					versionGid = pack;
				}
			}
			std::string versionDescr;
			if (cmdArgs.Size () != 0)
			{
				// By file path or root project path
				char const ** paths = cmdArgs.GetFilePaths ();
				if (!sccProxy.Report (paths [0], versionGid, versionDescr))
				{
					std::cerr << "report: Cannot retrieve project version description." << std::endl;
					retCode = 1;
				}

			}
			else
			{
				// By project id
				Assert (cmdArgs.GetProjectId () != -1);
				if (!sccProxy.Report (cmdArgs.GetProjectId (), versionGid, versionDescr))
				{
					std::cerr << "report: Cannot retrieve project version description." << std::endl;
					retCode = 1;
				}
			}
			std::cout << versionDescr.c_str () << std::endl;
		}
	}
	catch (Win::Exception e)
	{
		std::cerr << "report: " << e.GetMessage () << std::endl;
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
		std::cerr << "report: unknown problem" << std::endl;
		retCode = 1;
	}
	return retCode;
}
