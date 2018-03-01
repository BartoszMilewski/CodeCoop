//----------------------------------
// (c) Reliable Software 2007-09
//----------------------------------

#include "precompiled.h"

#include "OutputSink.h"
#include "Controller.h"
#include "Resource.h"

#include <Win/WinMain.h>
#include <Win/WinClass.h>
#include <Win/WinMaker.h>
#include <Win/MsgLoop.h>
#include <Ctrl/Controls.h>
#include <Ex/WinEx.h>

static char const ExperimentCaption [] = "Code Co-op Experiment";

int Win::Main (Win::Instance hInst, char const * cmdParam, int cmdShow)
{
	Win::UseCommonControls ();
	int status = 0;
	try
	{
		TheOutput.Init (ExperimentCaption);
		Win::Class::TopMaker topWinClass (MainCtrl::CLASS_NAME, hInst, IDI_MAIN_ICON);
		// Is there a running instance of this program?
		Win::Dow::Handle winOther = topWinClass.GetRunningWindow ();
		topWinClass.Register ();

		// Create Window
		Win::TopMaker topWin (ExperimentCaption, MainCtrl::CLASS_NAME, hInst);
		Win::MessagePrepro msgPrepro;
		MainCtrl ctrl (msgPrepro);

		Win::Dow::Handle appWin = topWin.Create (&ctrl, ExperimentCaption);
		ctrl.SetWindowPlacement (static_cast<Win::ShowCmd>(cmdShow), winOther != 0);
		// The main message loop
		status = msgPrepro.Pump ();
	}
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
	}
	catch (char const * msg)
	{
		TheOutput.Display (msg, Out::Error);
	}
	catch (...)
	{
		Win::ClearError ();
		TheOutput.Display ("Internal error: Unknown exception", Out::Error);
	}
	return status;
}
