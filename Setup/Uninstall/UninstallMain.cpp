//---------------------------------------------
// UninstallMain.cpp
// (c) Reliable Software 1996-2003
//---------------------------------------------

#include "precompiled.h"
#include "UninstallMain.h"
#include "resource.h"
#include "OutputSink.h"
#include "PathRegistry.h"
#include "UninstallCtrl.h"
#include "GlobalFileNames.h"
#include "FileOperation.h"

#include <Win/WinClass.h>
#include <Win/WinMaker.h>
#include <Win/MsgLoop.h>
#include <File/File.h>
#include <File/Path.h>
#include <Ex/WinEx.h>

#include <LightString.h>
#include <Handlers.h>

void UnexpectedHandler ();

class UninstallRemover
{
public:
	UninstallRemover (char const * orgHandle, char const * orgFilePath);
	void Finish ();

private:
	HANDLE		_orgUninstall;
	FilePath	_orgPath;
};

int UninstallMain (HINSTANCE hInst, char * cmdParam, int cmdShow)
{
	try
	{
		UnexpectedHandlerSwitch unexpHandler (UnexpectedHandler);

		TheOutput.Init ("Code Co-op Uninstaller", Registry::GetLogsFolder ());
		// Command line interpretation:
		// __argv [0] is always the command with which the program is invoked
		// 1. Full Uninstall: no additional arguments
		// 2. Uninstall of Temporary Update: __argv [1] == "-restore"
		// 3. Uninstall Clone: __argv [1] == original Uninstall handle
		//					   __argv [2] == installation folder
		if (__argc > 2)
		{
			// This is uninstall clone running -- finish uninstall procedure
			// by deleting original uninstall program and remove installation folder
			if (__argv [1] != 0 && __argv [2] != 0)
			{
				UninstallRemover uninstall (__argv [1], __argv [2]);
				uninstall.Finish ();
			}
			return 0;
		}
		
		// This is Original or Update uninstall running

		// Create top window class
		Win::Class::TopMaker topWinClass (IDS_MAIN, hInst, IDI_RS_ICON);
		// Is there a running instance of this program?
		Win::Dow::Handle hwndOther = topWinClass.GetRunningWindow ();
		if (hwndOther != 0)
		{
			hwndOther.SetForeground ();
			hwndOther.Restore ();
			return 0;
		}

		topWinClass.Register ();

		Win::MessagePrepro msgPrepro;
		UninstallController uninstallCtrl (&msgPrepro, __argc == 1);

		// Create top window
		Win::PopupMaker topWin (IDS_MAIN, hInst);
		topWin.Style () << Win::Style::AddSysMenu;
		Win::Dow::Handle appWin = topWin.Create (&uninstallCtrl, "Code Co-op Uninstaller");
		appWin.Display (cmdShow);
		// The main message loop
		return msgPrepro.Pump ();
	}
    catch (Win::Exception e)
    {
        TheOutput.Display (e);
    }
    catch (...)
    {
		Win::ClearError ();
        TheOutput.Display ("Internal error: Unknown exception", Out::Error);
    }
	return 0;
}

void UnexpectedHandler ()
{
	TheOutput.Display ("Unexpected system exception occurred during program execution.\n"
					   "The program will now exit.");
	terminate ();
}

UninstallRemover::UninstallRemover (char const * orgHandle, char const * orgPath)
	: _orgPath (orgPath)
{
	_orgUninstall = reinterpret_cast<HANDLE> (strtoul (orgHandle, 0, 0));
}

void UninstallRemover::Finish ()
{
	// Wait till original uninstall process ends
	::WaitForSingleObject (_orgUninstall, INFINITE);
	::CloseHandle (_orgUninstall);
	File::DeleteNoEx (_orgPath.GetFilePath (UninstallExeName));
	// If installation folder is empty remove it
	//  ::RemoveDirectory only succeeds on empty directories
	::RemoveDirectory (_orgPath.GetDir ());

	// If Company folder is empty remove it too
	_orgPath.DirUp();
	::RemoveDirectory (_orgPath.GetDir ());
}
