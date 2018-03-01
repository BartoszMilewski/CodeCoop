//------------------------------------------------
// DisplayWin.cpp
// (c) Reliable Software 2002
// -----------------------------------------------

#include "precompiled.h"
#include "DisplayWin.h"
#include "DisplayFrameCtrl.h"
#include "Global.h"

DisplayWindow::DisplayWindow (char const * caption, Win::Dow::Handle appWnd)
{
	// Create display frame
	Win::TopMaker frameMaker (caption, ListWindowClassName, appWnd.GetInstance ());
	std::unique_ptr<DisplayFrameController> ctrl (new DisplayFrameController ());
	ctrl->SaveUIPrefs ();
	_dumpWin = ctrl->GetRichDumpWindow ();
	_frame = frameMaker.Create (std::move(ctrl), caption);
}
