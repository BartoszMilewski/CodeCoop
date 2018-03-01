//------------------------------------
//  (c) Reliable Software, 2005 - 2008
//------------------------------------

#include "precompiled.h"
#include "CmdArgs.h"
#include "CmdFlags.h"
#include "SccProxyEx.h"
#include "Catalog.h"
#include "CmdLineVersionLabel.h"
#include "GlobalMessages.h"

#include <Win/Message.h>
#include <Ex/WinEx.h>
#include <Ex/Error.h>
#include <Ctrl/Output.h>

#include <iostream>
#include <fstream>

class DefectFromAllSwitch : public StandardSwitch
{
public:
	DefectFromAllSwitch ()
	{
		SetNotifySink ();
		SetForce ();
	}
};

int main (int count, char * args [])
{
	int retCode = 0;
	Win::Dow::Handle notifySink;
	try
	{
		DefectFromAllSwitch validSwitch;
		CmdArgs cmdArgs (count, args, validSwitch);
		if (cmdArgs.IsHelp ())
		{
			std::cout << CMDLINE_VERSION_LABEL << ".\n\n";
			std::cout << "Defect safely (source code remains intact) from all projects on this computer.\n\n";
			std::cout << "options:\n";
			std::cout << "   -f -- force unsafe project defect";
			std::cout << "   -? -- display help\n";
		}
		else
		{
			// Create Co-op command line
			std::string cmd ("Project_Defect kind:\"Nothing\"");	// Defect and remove only project database
			if (cmdArgs.IsForce ())
				cmd += " force:\"yes\"";
			Win::UserMessage msg (UM_PROGRESS_TICK);
			if (cmdArgs.IsNotifySink ())
			{
				Win::Dow::Handle handle (reinterpret_cast<HWND>(cmdArgs.GetNotifySink ()));
				notifySink.Reset (handle.ToNative ());
			}
			else
			{
				Out::Sink output;
				output.Init ("Code Co-op", "Defect From All Projects");
				std::string msg ("You are defecting (source code remains intact) from all projects on this computer");
				if (cmdArgs.IsForce ())
					msg += "\nand forcing unsafe project defect (for example when you are the project administrator)";
				msg += ".\n\nAre you sure?";
				Out::Answer userChoice = output.Prompt (msg.c_str (),
														Out::PromptStyle (Out::YesNo, Out::No, Out::Question));
				if (userChoice == Out::No)
					return retCode;
			}

			SccProxyEx sccProxy;
			Catalog catalog;
			if (cmdArgs.IsNotifySink ())
				notifySink.PostMsg (msg);
			for (ProjectSeq seq (catalog); !seq.AtEnd (); seq.Advance ())
			{
				if (cmdArgs.IsNotifySink ())
					notifySink.PostMsg (msg);

				if (!sccProxy.CoopCmd (seq.GetProjectId (), cmd, false, true))// Skip GUI Co-op in the project
				{															  // Execute command without timeout
					std::string info (seq.GetProjectName ());
					info += " (";
					info += ToString (seq.GetProjectId ());
					info += ")";
					std::cerr << "defectfromall: Cannot defect from the project: " << info << std::endl;
					retCode = 1;
				}
			}
		}
	}
	catch (Win::Exception e)
	{
		std::cerr << "defectfromall: " << e.GetMessage () << std::endl;
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
		std::cerr << "defectfromall: Unknown problem\n";
		retCode = 1;
	}
	if (!notifySink.IsNull ())
	{
		Win::UserMessage msg (UM_TOOL_END);
		notifySink.PostMsg (msg);
	}
	return retCode;
}
