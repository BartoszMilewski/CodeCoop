//----------------------------------
// (c) Reliable Software 2001 - 2008
//----------------------------------

#include "precompiled.h"
#include "CmdArgs.h"
#include "SccErrorOut.h"
#include "SccProxyEx.h"
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
		validSwitch.SetCommand ();
		validSwitch.SetProject ();

		CmdArgs cmdArgs (count, args, validSwitch, true);	// Accept paths only
		if (cmdArgs.IsHelp () || cmdArgs.IsEmpty ())
		{
			std::cout << CMDLINE_VERSION_LABEL << ".\n\n";
			std::cout << "Execute Code Co-op command in the specified project.\n\n";
			std::cout << "coopcmd <project root path> | -p:<project id> -c:\"command(s)\"";
		}
		else
		{
			if (cmdArgs.IsCommand ())
			{
				SccProxyEx sccProxy (&SccErrorOut);
				int const projId = cmdArgs.GetProjectId ();
				if (projId == -1)
				{
					char const ** paths = cmdArgs.GetFilePaths ();
					if (paths [0] != 0)
					{
						if (!sccProxy.CoopCmd (paths [0], cmdArgs.GetCommand ()))
						{
							std::cerr << "coopcmd: Cannot execute command " << cmdArgs.GetCommand ().c_str () << std::endl;
							retCode = 1;
						}
					}
					else
					{
						std::cerr << "coopcmd: missing project root folder or project id" << std::endl;
						retCode = 1;
					}
				}
				else
				{
					// by project id
					if (!sccProxy.CoopCmd (projId, cmdArgs.GetCommand (), false, true))// Skip GUI Co-op in the project
					{																   // Execute command without timeout
						std::cerr << "coopcmd: Cannot execute command " << cmdArgs.GetCommand ().c_str () << std::endl;
						retCode = 1;
					}
				}
			}
			else
			{
				std::cerr << "coopcmd: missing Code Co-op command" << std::endl;
				retCode = 1;
			}
		}
	}
	catch (Win::Exception e)
	{
		std::cerr << "coopcmd: " << e.GetMessage () << std::endl;
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
		std::cerr << "coopcmd: Unknown problem" << std::endl;
		retCode = 1;
	}
	return retCode;
}
