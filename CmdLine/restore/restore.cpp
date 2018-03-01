//----------------------------------
// (c) Reliable Software 2000 - 2008
//----------------------------------

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
			std::cout << "Restore a project version.\n\n";
			std::cout << "Project is identified by file that belongs to it." << std::endl << std::endl;
			std::cout << "restore [<options>] <file>" << std::endl;
			std::cout << "options:" << std::endl;
			std::cout << "   -v:<version id> -- restore project state after this version" << std::endl;
			std::cout << "                      version id in format 'xx-xxx' where 'x' is a" << std::endl;
			std::cout << "                      hexadecimal digit or as regular hexadecimal number" << std::endl;
		}
		else
		{
			SccProxyEx sccProxy (&SccErrorOut);
			GlobalId restoredVersionGid = gidInvalid;
			if (cmdArgs.IsVersionId ())
			{
				std::string const & argVersionId = cmdArgs.GetVersionId ();
				if (argVersionId != "previous")
				{
					GlobalIdPack pack (argVersionId.c_str ());
					restoredVersionGid = pack;
				}
			}
			// -Selection_Revert version:"'previous' | <script id>"
			if (cmdArgs.Size () != 0)
			{
				char const ** paths = cmdArgs.GetFilePaths ();
				if (paths == 0 || strlen (paths [0]) == 0)
				{
					std::cerr << "restore: Missing project file path." << std::endl;
					retCode = 1;
				}
				else
				{
					std::string cmdLine ("RestoreVersion version:\"");
					if (restoredVersionGid == gidInvalid)
					{
						cmdLine += "previous\"";
					}
					else
					{
						GlobalIdPack pack (restoredVersionGid);
						cmdLine += pack.ToString ();
						cmdLine += '"';
					}

					if (!sccProxy.CoopCmd (paths [0], cmdLine))
					{
						std::cerr << "restore: Cannot restore project version." << std::endl;
						retCode = 1;
					}
				}
			}
		}
	}
	catch (Win::Exception e)
	{
		std::cerr << "restore: " << e.GetMessage () << std::endl;
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
		std::cerr << "restore: Unknown problem" << std::endl;
		retCode = 1;
	}
	return retCode;
}
