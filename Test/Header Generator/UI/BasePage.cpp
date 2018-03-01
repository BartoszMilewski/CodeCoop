// ---------------------------
// (c) Reliable Software, 2000
// ---------------------------

#include "BasePage.h"

void BaseHandler::OnSetActive (long & result) throw (Win::Exception)
{
	// mark the page as 'changed' so that it receives PSN_APPLY and PSN_RESET
	// any time the user makes changes to any page in the sheet
	PropSheet_Changed (_ctrl.GetWindow ().GetParent (), _ctrl.GetWindow ().operator HWND ());
}

void BaseHandler::OnKillActive (long & result) throw (Win::Exception) 
{
	_ctrl.RetrieveData ();
	result = FALSE;
}
