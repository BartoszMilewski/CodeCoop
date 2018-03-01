//-------------------------------------
// (c) Reliable Software 2000 -- 2004
//-------------------------------------

#include "precompiled.h"
#include "CmdArgs.h"
#include "SccErrorOut.h"
#include "SccProxyEx.h"
#include "SccOptions.h"
#include "CmdLineVersionLabel.h"

#include <Ex/WinEx.h>
#include <Ex/Error.h>
#include <Dbg/Assert.h>
#include <StringOp.h>

#include <iostream>

class EditMemberSwitch : public StandardSwitch
{
public:
	EditMemberSwitch ()
	{
		SetUserId    ();
		SetUserName  ();
		SetUserState ();
	}
};

int main (int count, char * args [])
{
	int retCode = 0;
	try
	{
		EditMemberSwitch validSwitch;
		CmdArgs cmdArgs (count, args, validSwitch);
		if (cmdArgs.IsHelp () || cmdArgs.IsEmpty ())
		{
			std::cout << CMDLINE_VERSION_LABEL << ".\n\n";
			std::cout << "Edit description or change state of a project member.\n\n";
			std::cout << "editmember <options> <project's root folder>\n";
			std::cout << "options:\n";
			std::cout << "   -u:\"member id\"\n";
			std::cout << "   -n:\"name\"\n";
			std::cout << "   -s:\"[voting, observer, remove, admin]\"\n";
			std::cout << "   -? -- display help\n";
		}
		else
		{
			SccProxyEx sccProxy (&SccErrorOut);
			Assert (cmdArgs.Size () != 0);
			// -Project_Members userId:"id" [name:"name" | state:"[voting/observer/remove/admin]]"
			if (cmdArgs.Size () != 0)
			{
				char const ** paths = cmdArgs.GetFilePaths ();
				if (paths == 0 || strlen (paths [0]) == 0)
				{
					std::cerr << "editmember: Missing project root path." << std::endl;
					retCode = 1;
				}
				else
				{
					std::string cmdLine ("Project_Members ");
					std::string const & argUserId    = cmdArgs.GetUserId ();
					std::string const & argUserName  = cmdArgs.GetUserName ();
					std::string const & argUserState = cmdArgs.GetUserState ();

					if (argUserId.empty ())
					{
						std::cerr << "editmember: Missing user id." << std::endl;
						retCode = 1;
					}
					else if (ToInt (argUserId) == 0 && (argUserId.length () != 1 || argUserId [0] != '0'))
					{
						std::cerr << "editmember: User id must be an integer value." << std::endl;
						retCode = 1;
					}
					else if (argUserName.empty () && argUserState.empty ())
					{
						std::cerr << "editmember: Missing user name and user state." << std::endl;
						retCode = 1;
					}
					else
					{
						cmdLine += "userid:";
						cmdLine += argUserId;
						cmdLine += " ";
						if (!argUserName.empty ())
						{
							cmdLine += "name:";
							cmdLine += argUserName;
							cmdLine += " ";
						}
						if (!argUserState.empty ())
						{
							cmdLine += "state:";
							cmdLine += argUserState;
						}
						if (!sccProxy.CoopCmd (paths [0], cmdLine))
						{
							std::cerr << "editmember: Cannot edit project member." << std::endl;
							retCode = 1;
						}
					}
				}
			}
		}
	}
	catch (Win::Exception e)
	{
		std::cerr << "editmember: " << e.GetMessage () << std::endl;
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
		std::cerr << "editmember: Unknown problem" << std::endl;
		retCode = 1;
	}
	return retCode;
}
