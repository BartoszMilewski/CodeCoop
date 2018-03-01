//----------------------------------
// (c) Reliable Software 2003 - 2008
//----------------------------------

#include "precompiled.h"
#include "CmdArgs.h"
#include "SccErrorOut.h"
#include "SccProxyEx.h"
#include "SccOptions.h"
#include "GlobalId.h"
#include "CmdLineVersionLabel.h"

#include <Ex/WinEx.h>
#include <Ex/Error.h>
#include <Dbg/Assert.h>

#include <iostream>

class ExportSwitch : public StandardSwitch
{
public:
	ExportSwitch ()
	{
		SetVersion ();
		SetProject ();
		SetOverwrite ();
		SetLocalEdits ();
	}
};

int main (int count, char * args [])
{
	int retCode = 0;
	try
	{
		ExportSwitch validSwitch;
		CmdArgs cmdArgs (count, args, validSwitch, true);	// Expecting only paths not file names
		if (cmdArgs.IsHelp () || cmdArgs.IsEmpty ())
		{
			std::cout << CMDLINE_VERSION_LABEL << ".\n\n";
			std::cout << "Export project files at specific version to the target folder." << std::endl;
			std::cout << "Project is identified by the local project id (as show in the project view)." << std::endl;
			std::cout << "Target folder cannot contain other Code Co-op project." << std::endl << std::endl;
			std::cout << "export <options> <target folder>" << std::endl;
			std::cout << "options:" << std::endl;
			std::cout << "   -p:<local project id> -- specifies project" << std::endl;
			std::cout << "   -v:current -- export current project version " << std::endl;
			std::cout << "   -v:<version id> -- export this project version" << std::endl;
			std::cout << "                      version id in format as shown in the history view" << std::endl;
			std::cout << "                      or as regular hexadecimal number" << std::endl;
			std::cout << "   -l - include local edits (valid only when exporting current project version)" << std::endl;
			std::cout << "   -o - overwrite existing files in the target folder" << std::endl;
			std::cout << "   -? - display help" << std::endl;
		}
		else
		{
			SccProxyEx sccProxy (&SccErrorOut);
			GlobalId versionGid = gidInvalid;	// Assume current project version
			if (cmdArgs.IsVersionId ())
			{
				std::string const & versionId = cmdArgs.GetVersionId ();
				if (versionId != "current")
				{
					GlobalIdPack pack (versionId.c_str ());
					versionGid = pack;
				}
			}
			int projectId = cmdArgs.GetProjectId ();
			if (projectId != -1)
			{
				// -Selection_VersionExport version:"'current' | <script id>" overwrite:"yes | no" localedits: "yes | no" target:"<export file path>"
				std::string cmdLine ("Selection_VersionExport version:\"");
				if (versionGid == gidInvalid)
				{
					cmdLine += "current\"";
				}
				else
				{
					GlobalIdPack pack (versionGid);
					cmdLine += pack.ToString ();
					cmdLine += '"';
				}

				cmdLine += " overwrite:\"";
				if (cmdArgs.IsOverwrite ())
					cmdLine += "yes\"";
				else
					cmdLine += "no\"";

				cmdLine += " localedits:\"";
				if (cmdArgs.IsLocalEdits () && versionGid == gidInvalid)
					cmdLine += "yes\"";
				else
					cmdLine += "no\"";

				cmdLine += " target:\"";
				char const ** paths = cmdArgs.GetFilePaths ();
				if (paths == 0 || strlen (paths [0]) == 0)
				{
					std::cerr << "export: Missing target folder." << std::endl;
					retCode = 1;
				}
				else
				{
					cmdLine += paths [0];
					cmdLine += '"';
					if (!sccProxy.CoopCmd (projectId, cmdLine, false, true))// Skip GUI Co-op in the project
					{														// Execute command without timeout
						std::cerr << "export: Cannot export project version." << std::endl;
						retCode = 1;
					}
				}
			}
			else
			{
				std::cerr << "export: Missing project id." << std::endl;
				retCode = 1;
			}
		}
	}
	catch (Win::Exception e)
	{
		std::cerr << "export: " << e.GetMessage () << std::endl;
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
		std::cerr << "export: unknown problem" << std::endl;
		retCode = 1;
	}
	return retCode;
}
