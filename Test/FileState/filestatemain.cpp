// ---------------------------
// (c) Reliable Software, 2003
// ---------------------------
#include "precompiled.h"
#include "FileStates.h"
#include "SccErrorOut.h"
#include "SccProxyEx.h"
#include "CmdLineVersionLabel.h"

#include <Ex/WinEx.h>
#include <Ex/Error.h>

#include <iostream>

int main (int count, char * args [])
{
	int retCode = 0;
	try
	{
		FileStateMap TheStateMap;

		if (count != 3)
		{
			std::cout << CMDLINE_VERSION_LABEL << ".\n\n";
			std::cout << "Check file state.\n\n";
			std::cout << "filestate <file full path> <state string>\n";
			std::cout << "see filestate source code to find out possible values for <state string>\n";
			return 1;
		}
		else
		{
			unsigned long expectedState = -1;
			unsigned long expectedStateMask = 0;
			if (!TheStateMap.GetState (args [2], expectedState, expectedStateMask))
			{
				std::cerr << "Invalid expected file state: " << args [2] << std::endl;
				return 1;
			}
			Assert (expectedState != -1);

			char const * filePath = args [1];
			SccProxyEx exec (&SccErrorOut);
			unsigned long actualState = -1;
			if (exec.GetFileState (filePath, actualState))
			{
				Assert (actualState != -1);
				// only 13 (out of 15) bits says about persistent file state
				// bits 12 and 13 are volatile
				actualState = actualState & 0xe7ff;

				// some bits may have arbitrary values in a specific state
				// perform comparison only on significant bits
				if ((actualState | expectedStateMask) != (expectedState | expectedStateMask))
				{
					std::cout << filePath << " is not " << args [2]
					<< " (actual value: " << std::hex << actualState << ")" << std::endl;
					std::cerr << "File state mismatch!" << std::endl;
					retCode = 2;
				}
			}
			else
			{
				std::cerr << "Cannot check file state." << std::endl;
				retCode = 3;
			}
		}
	}
	catch (Win::Exception e)
	{
		std::cerr << "filestate: " << e.GetMessage () << std::endl;
		SysMsg msg (e.GetError ());
		if (msg)
			std::cerr << "System tells us: " << msg.Text ();
		std::string objectName (e.GetObjectName ());
		if (!objectName.empty ())
			std::cerr << "    " << objectName << std::endl;
		retCode = 2;
	}
	catch (...)
	{
		Win::ClearError ();
		std::cerr << "filestate: Unknown problem" << std::endl;
		retCode = 2;
	}
	return retCode;
}
