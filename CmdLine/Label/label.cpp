//----------------------------------
// (c) Reliable Software 2004 - 2008
//----------------------------------

#include "precompiled.h"
#include "CmdArgs.h"
#include "SccErrorOut.h"
#include "SccProxyEx.h"
#include "PathClassifier.h"
#include "CmdLineVersionLabel.h"

#include <Ex/WinEx.h>
#include <Ex/Error.h>

#include <iostream>

// This applet is named "versionlabel" instead of simple "label"
// because the name "label" is already used by DOS command "label"

int main (int count, char * args [])
{
	int retCode = 0;
	try
	{
		StandardSwitch validSwitch;
		validSwitch.SetProject ();
		validSwitch.SetComment ();
		CmdArgs cmdArgs (count, args, validSwitch);
		if (cmdArgs.IsHelp () || cmdArgs.GetComment ().empty ())
		{
			std::cout << CMDLINE_VERSION_LABEL << ".\n\n";
			std::cout << "Adds label in the project's history.\n\n";
			std::cout << "versionlabel <options> <project root folders>\n";
			std::cout << "options:\n";
			std::cout << "   -c:\"comment\" -- label comment\n";
			std::cout << "   -p:<project id> -- identifies project when root folder is not specified\n";
			std::cout << "   -? -- display help\n";
		}
		else
		{
			SccProxyEx sccProxy (&SccErrorOut);
			std::string labelCmd ("History_AddLabel comment:\"");
			labelCmd += cmdArgs.GetComment ();
			labelCmd += "\"";
			if (cmdArgs.Size () != 0)
			{
				// Project(s) identified by root path
				PathClassifier classifier (cmdArgs.Size (), cmdArgs.GetFilePaths ());
				std::vector<std::string> const & unrecognizedPaths = classifier.GetUnrecognizedPaths ();
				if (!unrecognizedPaths.empty ())
				{
					std::cerr << "label: Cannot locate Code Co-op project(s):" << std::endl;
					for (std::vector<std::string>::const_iterator iter = unrecognizedPaths.begin ();
						 iter != unrecognizedPaths.end ();
						 ++iter)
					{
						std::string const & path = *iter;
						std::cerr << "   " << path << std::endl;
					}
				}
				// Add labels in the known projects
				std::vector<int> const & projectIds = classifier.GetProjectIds ();
				for (std::vector<int>::const_iterator iter = projectIds.begin ();
					 iter != projectIds.end ();
					 ++iter)
				{

					if (!sccProxy.CoopCmd (*iter, labelCmd, false, false))// Skip GUI Co-op in the project
					{													  // Execute command with timeout
						std::cerr << "label: Cannot add label to the history of the project with id:" << *iter << std::endl;
						retCode = 1;
					}
				}
			}
			if (cmdArgs.GetProjectId () != -1)
			{
				// Project identified by project id
				if (!sccProxy.CoopCmd (cmdArgs.GetProjectId (), labelCmd, false, false))// Skip GUI Co-op in the project
				{																	    // Execute command with timeout
					std::cerr << "label: Cannot add label to the history of the project with id: " << cmdArgs.GetProjectId () << std::endl;
					retCode = 1;
				}
			}
		}
	}
	catch (Win::Exception e)
	{
		std::cerr << "label: " << e.GetMessage () << std::endl;
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
		std::cerr << "label: Unknown problem" << std::endl;
		retCode = 1;
	}
	return retCode;
}
