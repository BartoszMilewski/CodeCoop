//-----------------------------------------
// (c) Reliable Software 2001 -- 2004
//-----------------------------------------

#include "precompiled.h"

#include "CmdArgs.h"
#include "Catalog.h"
#include "ProjectMarker.h"
#include "CmdLineVersionLabel.h"

#include <File/Path.h>
#include <File/Dir.h>
#include <Ex/WinEx.h>
#include <Ex/Error.h>

#include <iostream>

class ProjectStatusSwitch : public StandardSwitch
{
public:
	ProjectStatusSwitch ()
	{
		SetAllInProject ();
	}
};

int main (int count, char * args [])
{
	int retCode = 0;
	try
	{
		ProjectStatusSwitch validSwitch;
		CmdArgs cmdArgs (count, args, validSwitch);
		if (cmdArgs.IsHelp ())
		{
			std::cout << CMDLINE_VERSION_LABEL << ".\n\n";
			std::cout << "List projects with checked out files and/or incoming scripts." << std::endl;
			std::cout << "options:\n";
			std::cout << "   -a -- list all projects\n";
			std::cout << "   -? -- display help\n";
		}
		else
		{
			Catalog catalog;
			for (ProjectSeq seq (catalog); !seq.AtEnd (); seq.Advance ())
			{
				// Count checked out files
				unsigned int checkoutCount = 0;
				FilePath databasePath (seq.GetProjectDataPath ());
				if (File::Exists (databasePath.GetDir ()))
				{
					FileSeq seq (databasePath.GetFilePath ("*.og1"));
					while (!seq.AtEnd ())
					{
						checkoutCount++;
						seq.Advance ();
					}
					if (checkoutCount == 0)
					{
						FileSeq seq (databasePath.GetFilePath ("*.og2"));
						while (!seq.AtEnd ())
						{
							checkoutCount++;
							seq.Advance ();
						}
					}
				}
				// Check if unpacked scripts present
				IncomingScripts incomingScript (catalog, seq.GetProjectId ());
				if (checkoutCount != 0 || incomingScript.Exists () || cmdArgs.IsAllInProject ())
				{
					FilePath sourcePath (seq.GetProjectSourcePath ());
					std::cout << seq.GetProjectName () << " (" << sourcePath.GetDir () << ")";
					if (checkoutCount != 0 || incomingScript.Exists ())
					{
						std::cout << " -- ";
						if (checkoutCount != 0)
							std::cout << checkoutCount << " checked out file(s); ";
						if (incomingScript.Exists ())
							std::cout << "incoming script(s)";
					}
					std::cout << std::endl;
				}
			}
		}
	}
	catch (Win::Exception e)
	{
		std::cerr << "projectstatus: " << e.GetMessage () << std::endl;
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
		std::cerr << "projectstatus: unknown problem" << std::endl;
		retCode = 1;
	}
	return retCode;
}
