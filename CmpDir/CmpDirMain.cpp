//------------------------------------
//  (c) Reliable Software, 2002
//------------------------------------
#include "precompiled.h"
#include "Ctrl.h"
#include "CmpDirReg.h"
#include "OutSink.h"
#include "PathRegistry.h"
#include "Resource/resource.h"
#include <Win/WinMain.h>
#include <Win/WinClass.h>
#include <Win/WinMaker.h>
#include <Win/MsgLoop.h>
#include <Ex/WinEx.h>
#include <Handlers.h>

void UnexpectedHandler ();

Out::Sink TheOutput;

int Win::Main (Win::Instance hInst, char const * cmdParam, int cmdShow)
{
	UnexpectedHandlerSwitch unexpHandler (UnexpectedHandler);

    try
    {
		TheOutput.Init ("Directory Differ", Registry::GetLogsFolder ());
        // Create top window class
		Win::Class::TopMaker topWinClass (ID_MAIN, hInst, ID_MAIN);
        topWinClass.Register ();

		// Create Window Controller (needs access to MessagePrepro)
		Win::MessagePrepro msgPrepro;
		CmpDirCtrl ctrl (cmdParam, msgPrepro);

		// Create top window (hidden)
		ResString caption (hInst, ID_CAPTION);
		Win::TopMaker topWin (caption, ID_MAIN, hInst);
		Win::Dow::Handle appWin = topWin.Create (&ctrl);

		// Display top window
		Win::Placement placement (appWin);
		{
			// read from registry
			Registry::CmpDirUser userWin ("Window");
			RegKey::ReadWinPlacement (placement, userWin.Key ());
		}
		placement.CombineShowCmd (cmdShow);
		appWin.SetPlacement (placement);

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
