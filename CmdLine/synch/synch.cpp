//------------------------------------
//  (c) Reliable Software, 2000 - 2008
//------------------------------------

#include "precompiled.h"
#include "CmdArgs.h"
#include "SccErrorOut.h"
#include "SccProxyEx.h"
#include "PathClassifier.h"
#include "CmdLineVersionLabel.h"

#include <Ex/WinEx.h>
#include <Ex/Error.h>

#include <iostream>

int main (int count, char * args [])
{
	int retCode = 0;
	try
	{
		StandardSwitch validSwitch;
		CmdArgs cmdArgs (count, args, validSwitch, true);	// Expecting only paths not file names
		if (cmdArgs.IsHelp () || cmdArgs.IsEmpty ())
		{
			std::cout << CMDLINE_VERSION_LABEL << ".\n\n";
			std::cout << "Unpack and accept all incomming scripts in the Code Co-op project.\n\n";
			std::cout << "synch <project root folders>\n";
		}
		else
		{
			SccProxyEx sccProxy (&SccErrorOut);
			PathClassifier classifier (cmdArgs.Size (), cmdArgs.GetFilePaths ());
			std::vector<std::string> const & unrecognizedPaths = classifier.GetUnrecognizedPaths ();
			if (!unrecognizedPaths.empty ())
			{
				std::cerr << "synch: Cannot locate Code Co-op project(s):" << std::endl;
				for (std::vector<std::string>::const_iterator iter = unrecognizedPaths.begin ();
						iter != unrecognizedPaths.end ();
						++iter)
				{
					std::string const & path = *iter;
					std::cerr << "   " << path << std::endl;
				}
			}
			// Unpack scripts in the known projects
			std::vector<int> const & projectIds = classifier.GetProjectIds ();
			for (std::vector<int>::const_iterator iter = projectIds.begin ();
				 iter != projectIds.end ();
				 ++iter)
			{

				if (!sccProxy.CoopCmd (*iter, "All_Synch", false, true))// Skip GUI Co-op in the project
				{														// Execute command without timeout
					std::cerr << "synch: Cannot synchronize the project with id: " << *iter << std::endl;
					retCode = 1;
				}
			}
		}
	}
	catch (Win::Exception e)
	{
		std::cerr << "synch: " << e.GetMessage () << std::endl;
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
		std::cerr << "synch: Unknown problem" << std::endl;
		retCode = 1;
	}
	return retCode;
}
