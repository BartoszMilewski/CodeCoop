// ---------------------------
// (c) Reliable Software, 2005
// ---------------------------
#include "precompiled.h"
#include "MailToDlg.h"

bool MailToCtrl::OnInitDialog () throw (Win::Exception)
{
	GetWindow ().CenterOverScreen ();
	_emailEdit.Init (GetWindow (), IDC_RCPT);
	_emailEdit.SetString (_emailAddr);
	return true;
}

bool MailToCtrl::OnApply () throw ()
{
	_emailAddr = _emailEdit.GetString ();
	if (_emailAddr.empty ())
		::MessageBox (GetWindow ().ToNative (), 
					"Please enter recipient's e-mail address.", 
					"Recipient", 
					MB_ICONERROR | MB_OK);
	else
		EndOk ();
	return true;
}
