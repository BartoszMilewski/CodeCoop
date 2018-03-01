//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include "WinLibBase.h"
#include "ProgressControlHandler.h"

Progress::CtrlHandler::CtrlHandler (Progress::MeterDialogData & data, int dlgId)
	: Dialog::ControlHandler (dlgId),
	  _progressTimer (Progress::CtrlHandler::TimerId),
	  _dlgData (data),
	  _isVisible (false)
{
}

Progress::CtrlHandler::~CtrlHandler ()
{
	_progressTimer.Kill ();
}

void Progress::CtrlHandler::Show ()
{
	Win::Dow::Handle dlgWin (GetWindow ());
	dlgWin.CenterOverOwner ();
	dlgWin.BringToTop ();
	dlgWin.Show ();
	dlgWin.SetFocus ();
	Refresh ();
	_isVisible = true;
}

void Progress::CtrlHandler::OnTimer ()
{
	if (_isVisible)
	{
		// Regular progress tick
		Refresh ();
	}
	else
	{
		// Startup timeout elapsed -- show progress meter
		Show ();
		_progressTimer.Set (Tick);
	}
}
