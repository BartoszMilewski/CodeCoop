//------------------------------------
//  (c) Reliable Software, 2002
//------------------------------------
#include "precompiled.h"
#include "Ctrl.h"
#include "Resource/resource.h"
#include <Win/WinMain.h>
#include <Win/WinClass.h>
#include <Win/WinMaker.h>
#include <Win/MsgLoop.h>
#include <Ex/WinEx.h>
#include <Handlers.h>

void NewHandler ();
void UnexpectedHandler ();

Out::Sink TheOutput;

int Win::Main (Win::Instance hInst, char const * cmdParam, int cmdShow)
{
	TheOutput.Init ("UI Test");

    try
    {
        // Create top window class
		Win::Class::TopMaker topWinClass (ID_MAIN, hInst, ID_MAIN);
        topWinClass.Register ();

		// Create Window Controller
		UiCtrl ctrl;
		// Create top window (hidden)
		Win::TopMaker topWin ("Tree Test", ID_MAIN, hInst);
		Win::Dow::Handle appWin = topWin.Create (&ctrl);

		// Display top window
		appWin.Show (cmdShow);

		// The main message loop
		Win::MessagePrepro msgPrepro;
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

void NewHandler ()
{
	throw Win::Exception ("Internal error: Out of memory");
}

void UnexpectedHandler ()
{
	TheOutput.Display ("Unexpected system exception occurred during program execution.\n"
					   "The program will now exit.");
	terminate ();
}
