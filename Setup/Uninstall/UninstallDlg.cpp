//------------------------------------
//  (c) Reliable Software, 1997 - 2005
//------------------------------------

#include "precompiled.h"
#include "UninstallDlg.h"
#include "resource.h"

UninstallDlgCtrl::UninstallDlgCtrl ()
	: Dialog::ControlHandler (IDD_UNINSTALL)
{}


bool UninstallDlgCtrl::OnInitDialog () throw (Win::Exception)
{
	_uninstall.Init (GetWindow (), IDOK);
	_cancel.Init (GetWindow (), IDCANCEL);
	GetWindow ().CenterOverScreen ();
	return true;
}

bool UninstallDlgCtrl::OnApply () throw ()
{
	EndOk ();
	return true;
}
