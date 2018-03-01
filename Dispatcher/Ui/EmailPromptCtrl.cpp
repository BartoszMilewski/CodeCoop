//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include "precompiled.h"
#include "EmailPromptCtrl.h"
#include "Resource.h"
#include "OutputSink.h"

#include <Mail/EmailAddress.h>

EmailPromptCtrl::EmailPromptCtrl (std::string & dlgData)
	: Dialog::ControlHandler (IDD_EMAIL_PROMPT),
	  _dlgData (dlgData)
{}

bool EmailPromptCtrl::OnInitDialog () throw (Win::Exception)
{
	_email.Init (GetWindow (), IDC_EMAIL_EDIT);
	// Make sure that dialog is visible
	GetWindow ().SetForeground ();
	return true;
}

bool EmailPromptCtrl::OnApply () throw ()
{
	std::string emailAddress = _email.GetTrimmedString ();
	if (!Email::IsValidAddress (emailAddress))
	{
		TheOutput.Display ("Please, provide a valid e-mail address.");
	}
	else
	{
		_dlgData = emailAddress;
		EndOk ();
	}
	return true;
}

