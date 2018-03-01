//-------------------------------------
// (c) Reliable Software 2000 -- 2003
//-------------------------------------

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

class CheckinSwitch : public StandardSwitch
{
public:
	CheckinSwitch ()
	{
		SetComment ();
		SetAllInProject ();
		SetKeepCheckedOut ();
	}
};

int main (int count, char * args [])
{
	int retCode = 0;
	try
	{
		CheckinSwitch validSwitch;
		CmdArgs cmdArgs (count, args, validSwitch);
		if (cmdArgs.IsHelp () || cmdArgs.IsEmpty ())
		{
			std::cout << CMDLINE_VERSION_LABEL << ".\n\n";
			std::cout << "Check-in file changes into project.\n\n";
			std::cout << "checkin <options> <files>\n";
			std::cout << "options:\n";
			std::cout << "   -c:\"comment\" -- check-in comment\n";
			std::cout << "   -a -- all checked out files\n";
			std::cout << "   -o -- keep checked out\n";
			std::cout << "   -? -- display help\n";
		}
		else
		{
			CodeCoop::Proxy sccProxy (&SccErrorOut);
			Assert (cmdArgs.Size () != 0);
			char const * comment;
			if (cmdArgs.IsComment ())
				comment = cmdArgs.GetComment ().c_str ();
			else
				comment = "No comment";
			CodeCoop::SccOptions options;
			if (cmdArgs.IsAllInProject ())
				options.SetAllInProject ();
			if (cmdArgs.IsKeepCheckedOut ())
				options.SetKeepCheckedOut ();
			if (!sccProxy.CheckIn (cmdArgs.Size (), cmdArgs.GetFilePaths (), comment, options))
			{
				std::cerr << "checkin: Cannot checkin selected files." << std::endl;
				retCode = 1;
			}
		}
	}
	catch (Win::Exception e)
	{
		std::cerr << "checkin: " << e.GetMessage () << std::endl;
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
		std::cerr << "checkin: Unknown problem" << std::endl;
		retCode = 1;
	}
	return retCode;
}
