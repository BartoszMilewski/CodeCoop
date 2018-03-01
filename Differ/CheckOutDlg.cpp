//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------
#include "precompiled.h"
#include "CheckOutDlg.h"
#include "Resource.h"

CheckOutCtrl::CheckOutCtrl (bool & dontAsk)
	: Dialog::ControlHandler (IDD_CHECKOUT),
	 _dontAsk (dontAsk)
{}

bool CheckOutCtrl::OnInitDialog () throw (Win::Exception)
{
	_dontAskCheck.Init (GetWindow (), IDC_CHECK);
	return true;
}

bool CheckOutCtrl::OnApply () throw ()
{
	_dontAsk = _dontAskCheck.IsChecked ();
	EndOk ();
	return true;
}

