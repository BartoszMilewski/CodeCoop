//----------------------------------
// (c) Reliable Software 1996 - 2007
//----------------------------------

#include "precompiled.h"
#include "SetupMain.h"
#include "resource.h"
#include "OutputSink.h"
#include "SetupCtrl.h"
#include "PathRegistry.h"
#include "SetupParams.h"
#include "GlobalFileNames.h"

#include <Win/WinClass.h>
#include <Win/WinMaker.h>
#include <Win/MsgLoop.h>
#include <File/Path.h>
#include <File/File.h>
#include <Graph/Brush.h>
#include <Ex/WinEx.h>

#include <Handlers.h>

void UnexpectedHandler ();

int SetupMain (HINSTANCE hInst, char * cmdParam, int cmdShow)
{
	UnexpectedHandlerSwitch unexpHandler (UnexpectedHandler);

	// Installer can be run in the following modes:
	// 1. Full Install
	// 2. Install of Temporary Update
	// 3. Install of Permanent Update
	// 4. Install of Command-Line Tools
	// These are distinguished by checking the marker files in 
	// the temporary folder created by zip to keep the installer package files
	SetupController::InstallMode installMode = SetupController::Full;
	if (File::Exists (TempVersionMarker))
	{
		installMode = SetupController::TemporaryUpdate;
	}
	else if (File::Exists (PermanentUpdateMarker))
	{
		installMode = SetupController::PermanentUpdate;
	}
	else if (File::Exists (CmdLineToolsMarker))
	{
		installMode = SetupController::CmdLineTools;
	}

	// splash.Load (hInst, MAKEINTRESOURCE (IDB_SPLASH));
	int status = 0;
    try
    {
		TheOutput.Init ("Code Co-op Setup", Registry::GetLogsFolder ());
        // Create top window class
		Win::Class::TopMaker topWinClass (ID_MAIN, hInst, ICON_SETUP);
        // Is there a running instance of this program?
        Win::Dow::Handle hwndOther = topWinClass.GetRunningWindow ();
        if (hwndOther != 0)
        {
            hwndOther.SetForeground ();
            hwndOther.Restore ();
            return 0;
        }

        // Create top window
		Win::PopupMaker topWin (ID_MAIN, hInst);
		//int width, height;
		//splash.GetSize (width, height);

		if (installMode == SetupController::Full)
		{
			// Full setup is running
			int width = 746;
			int height = 487;
			Win::Rect screenRect;
			Win::DesktopWindow desktop;
			desktop.GetWindowRect (screenRect);
			int x = (screenRect.Width () - width) / 2;
			int y = (screenRect.Height () - height) / 2;
			topWin.SetPosition (x, y, width, height);
			topWin.Style () << Win::Style::AddSysMenu;
		}
		topWinClass.Register ();

		Win::MessagePrepro msgPrepro;
		SetupController setupCtrl (msgPrepro, installMode);

		Win::Dow::Handle appWin = topWin.Create (&setupCtrl, setupCtrl.GetCaption ().c_str ());
		appWin.Display (cmdShow);
		TheOutput.Init (setupCtrl.GetCaption ().c_str (), Registry::GetLogsFolder ());
		// The main message loop
		status = msgPrepro.Pump ();
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
	return status;
}            

void UnexpectedHandler ()
{
	TheOutput.Display ("Unexpected system exception occurred during program execution.\n"
					   "The program will now exit.");
	terminate ();
}
