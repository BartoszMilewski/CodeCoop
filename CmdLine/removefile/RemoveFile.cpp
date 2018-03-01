//------------------------------------
//  (c) Reliable Software, 2000 - 2006
//------------------------------------

#include "precompiled.h"
#include "CmdArgs.h"
#include "SccErrorOut.h"
#include "SccProxy.h"
#include "SccOptions.h"
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
		validSwitch.SetDeleteFromDisk ();
		CmdArgs cmdArgs (count, args, validSwitch);
		if (cmdArgs.IsHelp () || cmdArgs.IsEmpty ())
		{
			std::cout << CMDLINE_VERSION_LABEL << ".\n\n";
			std::cout << "Remove files from the Code Co-op project.\n\n";
			std::cout << "removefile <options> <files>\n";
			std::cout << "options:\n";
			std::cout << "	 -d -- delete from disk\n";
			std::cout << "   -? -- display help\n";
		}
		else
		{
			CodeCoop::Proxy sccProxy (&SccErrorOut);
			CodeCoop::SccOptions options;
			if (!cmdArgs.IsDeleteFromDisk ())
				options.SetDontDeleteFromDisk ();
			if (!sccProxy.RemoveFile (cmdArgs.Size (), cmdArgs.GetFilePaths (), options))
			{
				std::cerr << "removefile: Cannot remove selected files." << std::endl;
				retCode = 1;
			}
		}
	}
	catch (Win::Exception e)
	{
		std::cerr << "removefile: " << e.GetMessage () << std::endl;
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
		std::cerr << "removefile: Unknown problem" << std::endl;
	}
	return retCode;
}
