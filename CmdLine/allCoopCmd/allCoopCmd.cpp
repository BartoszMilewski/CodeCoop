//----------------------------------
// (c) Reliable Software 2006 - 2008
//----------------------------------

#include "precompiled.h"
#include "SccErrorOut.h"
#include "SccProxyEx.h"
#include "CmdLineVersionLabel.h"
#include "Catalog.h"

#include <Ex/WinEx.h>
#include <Ex/Error.h>
#include <Dbg/Assert.h>

#include <iostream>

int main (int count, char * args [])
{
	int retCode = 0;
	try
	{
		if (count == 1)
		{
			std::cerr << "allcoopcmd: missing Code Co-op command" << std::endl;
			return 1;
		}
		else
		{
			std::string const arg1 = args [1];
			if ((arg1.length () < 2) ||
				(arg1 [0] == '?') || 
				(arg1 [0] == '-' && arg1 [1] == '?') || 
				(arg1 [0] == '/' && arg1 [1] == '?'))
			{
				std::cout << CMDLINE_VERSION_LABEL << ".\n\n";
				std::cout << "Execute Code Co-op command in all local enlistments.\n\n";
				std::cout << "allcoopcmd command";
				return 0;
			}
		}

		std::string argList = args [1];
		for (int i = 2; i < count; ++i)
		{
			argList += ' ';
			argList += args [i];
		}

		SccProxyEx sccProxy (&SccErrorOut);
		Catalog cat;
		ProjectSeq seq (cat);
		std::cout << "Executing...\n";
		while (!seq.AtEnd ())
		{
			unsigned int projId = seq.GetProjectId ();
			if (!sccProxy.CoopCmd (projId, argList, false, true))// Skip GUI Co-op in the project
			{													 // Execute command without timeout
				std::cerr << "allcoopcmd: Cannot execute command " << argList 
					<< " in the " << seq.GetProjectName () 
					<< " project (id: " << projId << ")." << std::endl;
				retCode = 1;
			}
			seq.Advance ();
		}
	}
	catch (Win::Exception e)
	{
		std::cerr << "allcoopcmd: " << e.GetMessage () << std::endl;
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
		std::cerr << "allcoopcmd: Unknown problem" << std::endl;
		retCode = 1;
	}
	return retCode;
}
