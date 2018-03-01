//----------------------------------------------------
// Procedure.cpp
// (c) Reliable Software 2000 -- 2002
//----------------------------------------------------
#include <WinLibBase.h>
#include "Procedure.h"
#include <Win/Controller.h>

// Window procedure shared by all window controllers

LRESULT CALLBACK Win::Procedure (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
	Win::Dow::Handle win (hwnd);
    Controller * pCtrl = win.GetLong<Controller *> ();

	if (pCtrl)
	{
		if (pCtrl->Dispatch (message, wParam, lParam, result))
			return result;
	}
	else if (message == WM_NCCREATE)
	{
        CreateData const * creation = 
            reinterpret_cast<CreateData const *> (lParam);
		Win::Controller * ctrl = static_cast<Win::Controller *> (creation->GetCreationData ());
		Controller::Attach (win, ctrl);
		Assert (ctrl == win.GetController ());
		if (pCtrl->Dispatch (message, wParam, lParam, result))
			return result;
	}
    return ::DefWindowProc (hwnd, message, wParam, lParam);
}

// Window procedure shared by all subclassing controllers

LRESULT CALLBACK Win::SubProcedure (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
	Win::Dow::Handle win (hwnd);
    SubController * pCtrl = win.GetLong<SubController *> ();
	if (pCtrl && pCtrl->Dispatch (message, wParam, lParam, result))
		return result;
	// call previous procedure with previous controller
	return pCtrl->CallPrevProc (message, wParam, lParam);
}
