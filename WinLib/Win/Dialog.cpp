//------------------------------------
//  (c) Reliable Software, 1996 - 2005
//------------------------------------

#include <WinLibBase.h>
#include "Dialog.h"

#include <Ctrl/Output.h>
#include <Win/Help.h>

void Dialog::Handle::MapRectangle (Win::Rect & rect)
{
	::MapDialogRect (H (), &rect);
}

bool Dialog::ControlHandler::OnControl (unsigned ctrlId, 
										unsigned notifyCode) throw (Win::Exception)
{
	if (ctrlId == Out::OK)
	{
		// Dialog OK button clicked
		if (OnApply ())
			return true;
	}
	else if (ctrlId == Out::Cancel)
	{
		// Dialog Cancel button clicked
		if (OnCancel ())
			return true;
	}
	else if (ctrlId == Out::Help)
	{
		// Dialog Help button clicked
		if (OnHelp ())
			return true;
	}
	else
		return OnDlgControl (ctrlId, notifyCode);
	return false;
}

bool Dialog::ControlHandler::OnHelp () throw ()
{
	if (_helpEngine != 0 && GetId () != -1)
	{
		return _helpEngine->OnDialogHelp (GetId ());
	}
	return false;
}

BOOL CALLBACK Dialog::Procedure (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	Dialog::Handle win (hwnd);
	Dialog::Controller * ctrl = win.GetLong<Dialog::Controller *> ();
	LRESULT result = 0;
	try
	{
		if (Dialog::DispatchMsg (ctrl, win, message, wParam, lParam))
			return TRUE;
	}
	catch (Win::Exception e)
	{
		// display error
		Out::Sink::DisplayException (e, win);
		// exit dialog
		if (ctrl != 0)
			ctrl->EndCancel ();
	}
	return FALSE;
}

Notify::Handler * Dialog::Controller::GetNotifyHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ()
{
	if (_ctrlHandler)
		return _ctrlHandler->GetNotifyHandler (winFrom, idFrom);
	else
		return 0;
}

bool Dialog::ModelessController::OnActivate (bool isClickActivate, bool isMinimized, Win::Dow::Handle prevWnd) throw (Win::Exception) 
{
	_prepro.SetDialogFilter (GetWindow (), _accelWin, _accel);
	Dialog::ControlHandler * handler = GetDlgControlHandler ();
	if (handler)
		handler->OnActivate ();
	return false; 
}

bool Dialog::ModelessController::OnDeactivate (bool isMinimized, Win::Dow::Handle prevWnd) throw (Win::Exception)
{
	Dialog::ControlHandler * handler = GetDlgControlHandler ();
	if (handler)
		handler->OnDectivate ();
	//_prepro.ResetDialogFilter ();
	_prepro.ResetDialogAccel ();
	return false; 
}

bool Dialog::ModelessController::OnDestroy () throw ()
{
	Dialog::ControlHandler * handler = GetDlgControlHandler ();
	if (handler)
		handler->OnDestroy ();
	_prepro.ResetDialogFilter ();
	return false; 
}

