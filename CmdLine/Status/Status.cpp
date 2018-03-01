//----------------------------------
// (c) Reliable Software 2001 - 2007
//----------------------------------

#include "precompiled.h"
#include "CmdArgs.h"
#include "SccErrorOut.h"
#include "SccProxy.h"
#include "SccOptions.h"
#include "scc.h"
#include "CmdLineVersionLabel.h"

#include <Ex/WinEx.h>
#include <Ex/Error.h>
#include <Dbg/Assert.h>

#include <iostream>

static bool GetStatus (CmdArgs const & cmdArgs);

class StatusSwitch : public StandardSwitch
{
public:
	StatusSwitch ()
	{
		SetAllInProject ();
	}
};

// status [patterns]
int main (int count, char * args [])
{
	int retCode = 0;
	try
	{
		StatusSwitch validSwitch;
		CmdArgs cmdArgs (count, args, validSwitch);
		if (cmdArgs.IsHelp () || cmdArgs.IsEmpty ())
		{
			std::cout << CMDLINE_VERSION_LABEL << ".\n\n";
			std::cout << "Display files status in the Code Co-op project.\n\n";
			std::cout << "status <options> <files>\n";
			std::cout << "options:\n";
			std::cout << "   -a -- display status of all project files\n";
		}
		else
		{
			bool cmdResult;
			if (cmdArgs.IsAllInProject ())
			{
				char const ** path = cmdArgs.GetFilePaths ();
				Assert (cmdArgs.Size () >= 1);
				CmdArgs allProjectFiles (path [0]);
				cmdResult = GetStatus (allProjectFiles);
			}
			else if (cmdArgs.Size () != 0)
			{
				cmdResult = GetStatus (cmdArgs);
			}
			if (!cmdResult)
			{
				std::cerr << "status: Cannot get status of the selected files." << std::endl;
				retCode = 1;
			}
		}
	}
	catch (Win::Exception e)
	{
		std::cerr << "status: " << e.GetMessage () << std::endl;
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
		std::cerr << "status: Unknown problem" << std::endl;
		retCode = 1;
	}
	return retCode;
}

static bool GetStatus (CmdArgs const & cmdArgs)
{
	// Get file status
	char const ** path = cmdArgs.GetFilePaths ();
	unsigned int count = cmdArgs.Size ();
	std::vector<long> fileState (count, SCC_STATUS_NOTCONTROLLED);
	CodeCoop::Proxy sccProxy (&SccErrorOut);
	if (!sccProxy.Status (count, path, &fileState [0]))
		return false;

	for (unsigned int i = 0; i < count; ++i)
	{
		std::cout << path [i] << " - ";
		long state = fileState [i];
		if (state == SCC_STATUS_NOTCONTROLLED)
		{
			std::cout << "NOT a project file";
		}
		else
		{
			if ((state & SCC_STATUS_CHECKEDOUT) != 0)
			{
				if ((state & SCC_STATUS_OUTOTHER) != 0)
					std::cout << "checked out here and by some other project member(s)";
				else
					std::cout << "checked out";
			}
			else
			{
				if ((state & SCC_STATUS_OUTOTHER) != 0)
					std::cout << "checked in here and checked out by some other project member(s)";
				else
					std::cout << "checked in";
			}
		}
		std::cout << std::endl;
	}
	return true;
}
