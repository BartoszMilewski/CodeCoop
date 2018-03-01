//----------------------------------
// (c) Reliable Software 1996 - 2007
//----------------------------------

#include "precompiled.h"
#include "DiffMain.h"
#include "Global.h"
#include "PathRegistry.h"
#include "resource.h"
#include "CommandParams.h"
#include "OutputSink.h"
#include "DifferCtrl.h"
#include "OutputSink.h"
#include "EditParams.h"

#include <Win/WinMain.h>
#include <Ctrl/Splitter.h>
#include <Win/WinClass.h>
#include <Win/WinMaker.h>
#include <Win/MsgLoop.h>
#include <Ex/WinEx.h>

#include <new.h>
#include <exception>

Out::Sink TheOutput;

// unexpected exception handler that displays message for user
void UnexpectedHandler ();

int Win::Main (Win::Instance hInst, char const * cmdParam, int cmdShow)
{
	Win::ClearError ();
	set_unexpected (&UnexpectedHandler);

    // load common controls
    // Notice: link with comctl32.lib
    InitCommonControls ();

    try
    {
		TheOutput.Init ("Differ", Registry::GetLogsFolder ());
        // Create top window class
		Win::Class::TopMaker topWinClass (DifferClassName, hInst, ID_MAIN);
        topWinClass.Register ();
		// Is there a running instance of this program?
		Win::Dow::Handle winOther = topWinClass.GetRunningWindow ();

		// Revisit: move these two to editor definition files
        // Create child pane classes
		Win::Class::Maker marginClass (IDC_MARGIN, hInst, WndProcMargin);
		// Revisit: explicit use of Windows constant
        marginClass.SetBgSysColor (COLOR_SCROLLBAR);
        marginClass.SetResCursor (IDC_INV_POINTER);
        marginClass.Register ();

		Win::Class::Maker editPaneClass (IDC_EDITPANE , hInst);
		editPaneClass.Style () << Win::Class::Style::DblClicks;
        editPaneClass.SetSysCursor (IDC_IBEAM);
        editPaneClass.Register ();

		Win::Class::Maker editClass (IDC_EDIT, hInst);
		editClass.Register ();

		// List Window class
		Win::Class::Maker winClassText (ListWindowClassName, hInst);
		winClassText.Register ();

        // Create top window
        ResString caption (hInst, ID_CAPTION);
		Win::TopMaker topWinMaker (caption, DifferClassName, hInst);
		topWinMaker.Style () << Win::Style::ClipChildren;
		Win::MessagePrepro msgPrepro;
		CommandParams params (cmdParam);
		DifferCtrl differCtrl (hInst, msgPrepro, params.ReleaseTree ());
		differCtrl.SetPlacementParams (Win::ShowCmd (cmdShow), winOther != 0);

		Win::Dow::Handle topWin = topWinMaker.Create (&differCtrl, "Code Co-op Differ");
		topWin.Show (cmdShow);
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
