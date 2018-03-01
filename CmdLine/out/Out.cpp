//-----------------------------------------
// (c) Reliable Software 1998 -- 2003
//-----------------------------------------

#include "precompiled.h"
#include "CmdArgs.h"
#include "SccErrorOut.h"
#include "SccProxy.h"
#include "SccOptions.h"
#include "CmdLineVersionLabel.h"

#include <Ex/WinEx.h>
#include <Ex/Error.h>

#include <iostream>

class OutSwitch : public StandardSwitch
{
public:
	OutSwitch ()
	{
		SetAllInProject ();
		SetRecursive ();
	}
};

// out [patterns]
int main (int count, char * args [])
{
	int retCode = 0;
	try
	{
		OutSwitch validSwitch;
		CmdArgs cmdArgs (count, args, validSwitch);
		if (cmdArgs.IsHelp () || cmdArgs.IsEmpty ())
		{
			std::cout << CMDLINE_VERSION_LABEL << ".\n\n";
			std::cout << "Prepare project files for edits.\n\n"; 
			std::cout << "checkout <options> <files>\n";
			std::cout << "options:\n";
			std::cout << "   -a -- checkout all project files\n";
			std::cout << "   -r -- recursive checkout\n";
			std::cout << "   -? -- display help\n";
		}
		else
		{
			CodeCoop::Proxy sccProxy (&SccErrorOut);
			if (cmdArgs.Size () != 0)
			{
				// Perform regular checkout or checkout all project files
				CodeCoop::SccOptions options;
				if (cmdArgs.IsAllInProject ())
					options.SetAllInProject ();
				if (!sccProxy.CheckOut (cmdArgs.Size (), cmdArgs.GetFilePaths (), options))
				{
					std::cerr << "checkout: Cannot checkout selected files." << std::endl;
					retCode = 1;
				}
			}
			if (cmdArgs.DeepSize () != 0)
			{
				// Perform recursive checkout
				CodeCoop::SccOptions options;
				options.SetRecursive ();
				if (!sccProxy.CheckOut (cmdArgs.DeepSize (), cmdArgs.GetDeepPaths (), options))
				{
					std::cerr << "checkout: cannot recursively checkout files." << std::endl;
					retCode = 1;
				}
			}
		}
	}
	catch (Win::Exception e)
	{
		std::cerr << "checkout: " << e.GetMessage () << std::endl;
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
		std::cerr << "checkout: Unknown problem" << std::endl;
		retCode = 1;
	}
	return retCode;
}
