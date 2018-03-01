//-----------------------------------------
//  Uncheckout.cpp
//  (c) Reliable Software, 2000 -- 2003
//-----------------------------------------

#include "precompiled.h"
#include "CmdArgs.h"
#include "SccErrorOut.h"
#include "SccProxy.h"
#include "SccOptions.h"
#include "CmdLineVersionLabel.h"

#include <Ex/WinEx.h>
#include <Ex/Error.h>
#include <Dbg/Assert.h>

#include <iostream>

class UncheckoutSwitch : public StandardSwitch
{
public:
	UncheckoutSwitch ()
	{
		SetAllInProject ();
	}
};

int main (int count, char * args [])
{
	int retCode = 0;
	try
	{
		UncheckoutSwitch validSwitch;
		CmdArgs cmdArgs (count, args, validSwitch);
		if (cmdArgs.IsHelp () || cmdArgs.IsEmpty ())
		{
			std::cout << CMDLINE_VERSION_LABEL << ".\n\n";
			std::cout << "Discard all changes in the project files and restore them to the original state.\n\n";
			std::cout << "uncheckout <options> <files>\n";
			std::cout << "options:\n";
			std::cout << "   -a -- uncheckout all project files\n";
		}
		else
		{
			CodeCoop::Proxy sccProxy (&SccErrorOut);
			Assert (cmdArgs.Size () != 0);
			CodeCoop::SccOptions options;
			if (cmdArgs.IsAllInProject ())
				options.SetAllInProject ();
			if (!sccProxy.UncheckOut (cmdArgs.Size (), cmdArgs.GetFilePaths (), options))
			{
				std::cerr << "uncheckout: Cannot uncheckout selected files." << std::endl;
				retCode = 1;
			}
		}
	}
	catch (Win::Exception e)
	{
		std::cerr << "uncheckout: " << e.GetMessage () << std::endl;
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
		std::cerr << "uncheckout: Unknown problem" << std::endl;
		retCode = 1;
	}
	return retCode;
}