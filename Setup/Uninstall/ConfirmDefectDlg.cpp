//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include "precompiled.h"
#include "ConfirmDefectDlg.h"
#include "resource.h"

ConfirmDefectDlgCtrl::ConfirmDefectDlgCtrl ()
	: Dialog::ControlHandler (IDD_CONFIRM_DEFECT),
	  _doDefect (true)
{}


bool ConfirmDefectDlgCtrl::OnInitDialog () throw (Win::Exception)
{
	_dontDefect.Init (GetWindow (), ID_DONT_DEFECT);
	GetWindow ().CenterOverScreen ();
	return true;
}

bool ConfirmDefectDlgCtrl::OnDlgControl (unsigned id, unsigned notifyCode) throw (Win::Exception)
{
	if (id == ID_DONT_DEFECT && Win::SimpleControl::IsClicked (notifyCode))
	{
		_doDefect = false;
		EndOk ();
		return true;
	}
	return false;
}
