//----------------------------------
// (c) Reliable Software 2007
//----------------------------------

#include "precompiled.h"

#include "OutputSink.h"
#include "WikiBrowser.h"
#include "Resource.h"

#include <Win/WinMain.h>
#include <Win/WinClass.h>
#include <Win/WinMaker.h>
#include <Win/MsgLoop.h>
#include <Ctrl/Controls.h>
#include <Ex/WinEx.h>

static char const WikiBrowserCaption [] = "Code Co-op Wiki Browser";

int Win::Main (Win::Instance hInst, char const * cmdParam, int cmdShow)
{
	Win::UseCommonControls ();
	int status = 0;
	try
	{
		TheOutput.Init (WikiBrowserCaption);
		Win::Class::TopMaker topWinClass (Wiki::BrowserCtrl::CLASS_NAME, hInst, IDI_WIKI_BROWSER);
		// Is there a running instance of this program?
		Win::Dow::Handle winOther = topWinClass.GetRunningWindow ();
		topWinClass.Register ();

		// Create Window
		Win::TopMaker topWin (WikiBrowserCaption, Wiki::BrowserCtrl::CLASS_NAME, hInst);
		Win::MessagePrepro msgPrepro;
		Wiki::BrowserCtrl wikiBrowser (msgPrepro);

		Win::Dow::Handle appWin = topWin.Create (&wikiBrowser, WikiBrowserCaption);
		wikiBrowser.SetWindowPlacement (static_cast<Win::ShowCmd>(cmdShow), winOther != 0);
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
