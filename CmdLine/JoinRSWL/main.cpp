//------------------------------------
//  (c) Reliable Software, 2005 - 2008
//------------------------------------
#include "precompiled.h"
#include "SccProxyEx.h"
#include "SccErrorOut.h"
#include "PathRegistry.h"
#include "Registry.h"
#include <Ex/Error.h>
#include <File/Path.h>

#include <iostream>

int main (int count, char * args [])
{
	int retCode = 0;
	try
	{
		std::cout << "Joining RSWL project\n";
		SccProxyEx sccProxy (&SccErrorOut);
// -Project_Join project:"Name" root:"Local\Path" recipient:"recipHubId" user:"My Name"
//      email:"myHubId" comment:"my comment" state:"observer"
//      autosynch:"yes" autofullsynch:"yes" keepcheckedout:"yes" script:"full\synch\path"

		// Locate the project root in the "projects\RSWL" folder of 
		// the same drive where catalog resides
		std::string catPath = Registry::GetCatalogPath ();
		if (catPath.empty ())
		{
			::MessageBox (0,
				"Cannot find existing Code Co-op installation on this computer.\n"
				"The request to join the RSWL project will be ignored.",
				"Join RSWL",
				MB_ICONERROR | MB_OK);
			return 1;
		}
		PathSplitter splitter (catPath);
		char const * drive = splitter.GetDrive ();
		Assert (drive != 0);
		FilePath root (drive);
		root.DirDown ("projects");
		root.DirDown ("RSWL");

		// check root status
		char const * paths [1];
		paths [0] = root.GetDir ();
		long status = 0;

		if (sccProxy.Status (1, paths, &status))
		{
			if (status != 0) // already controlled
			{
				::MessageBox (0,
					"You already have RSWL project.\nYour request will be ignored.",
					"Join RSWL",
					MB_ICONINFORMATION);
				return 1;
			}
		}
		else
		{
			// Call to co-op failed.
			// Ignore and go further.
		}

		// Wait for Dispatcher to finish configuration
		std::cout << "Waiting for Dispatcher to finish configuration";
		unsigned int trial = 0;
		for (; trial < 10; ++trial)
		{
			Win::Sleep (1000); // 1 sec
			std::cout << '.';
			if (!Registry::IsFirstRun ())
				break;
		}

		bool success = false;
		if (trial < 10)
		{
			// Dispatcher configured successfully
			// invoke co-op
			std::string cmdLine ("Project_Join project:\"RSWL\" root:\"");
			cmdLine += root.GetDir ();
			cmdLine += "\" recipient:\"RSWL@relisoft.com\"";
			
			if (sccProxy.CoopCmd (cmdLine))
			{
				if (sccProxy.Status (1, paths, &status))
				{
					if (status != 0) // controlled
						success = true;
				}
			}
		}

		if (success)
		{
			::MessageBox (0,
						  "Congratulations!\nYou have successfully joined the RSWL project.\n"
						  "The join request has been emailed to the project administrator.\n"
						  "Wait for the return email \"Code Co-op Sync...\" (it may take a day or two).\n"
						  "It will be picked up by the Code Co-op Dispatcher (its icon will change color).\n"
						  "When that happens, open Code Co-op and execute the full synch script.",
						  "Join RSWL",
						  MB_ICONINFORMATION | MB_OK);
		}
		else
		{
			::MessageBox (0,
						"Code Co-op was unable to join RSWL.\n"
						"The most likely reason is that it wasn't able to detect your email client.\n"
						"Try running the Code Co-op Dispatcher configuration wizard (right click on\n"
						"the dispatcher icon in your taskbar) and then run PostInstall.exe from\n"
						"the Code Co-op installation folder.",
						"Join RSWL",
						MB_ICONERROR | MB_OK);
			retCode = 1;
		}
	}
	catch (Win::Exception e)
	{
		std::string text (e.GetMessage ());
		SysMsg msg (e.GetError ());
		if (msg)
		{
			text += "\nSystem tells us: ";
			text += msg.Text ();
		}
		std::string objectName (e.GetObjectName ());
		if (!objectName.empty ())
		{
			text += "\n    ";
			text += objectName;
		}
		::MessageBox (0, text.c_str (), "Join RSWL", MB_ICONERROR | MB_OK);
		retCode = 1;
	}
	catch (...)
	{
		Win::ClearError ();
		::MessageBox (0, "Unknown error", "Join RSWL", MB_ICONERROR | MB_OK);
		retCode = 1;
	}
	return retCode;
}
