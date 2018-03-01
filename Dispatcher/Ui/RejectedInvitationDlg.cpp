// ---------------------------
// (c) Reliable Software, 2006
// ---------------------------
#include "precompiled.h"
#include "RejectedInvitationDlg.h"

bool RejectedInvitationCtrl::OnInitDialog () throw (Win::Exception)
{
	_projectNameEdit.Init (GetWindow (), IDC_PROJECT_NAME);
	_userNameEdit.Init (GetWindow (), IDC_USER_NAME);
	_computerNameEdit.Init (GetWindow (), IDC_COMPUTER_NAME);
	_explanationEdit.Init (GetWindow (), IDC_EXPLANATION);

	_projectNameEdit.SetText (_projectName);
	_userNameEdit.SetText (_userName);
	_computerNameEdit.SetText (_computerName);
	_explanationEdit.SetText (_explanation);

	return true;
}
