//------------------------------------
//  (c) Reliable Software, 1997 - 2007
//------------------------------------

#include "precompiled.h"
#include "AboutDlg.h"
#include "MemberDescription.h"
#include "License.h"
#include "AppInfo.h"
#include "AppHelp.h"
#include "OutputSink.h"
#include "BuildOptions.h"

#include <Com/Shell.h>
#include <Sys/WinString.h>
#include <Ctrl/Output.h>
#include <StringOp.h>

HelpAboutData::HelpAboutData (MemberDescription const * currentUser,
							  std::string const & userLicense,
							  std::string const & projectName,
							  char const * projectPath)
	: _userName (currentUser->GetName ()),
	  _userEmail (currentUser->GetHubId ()),
	  _userPhone (currentUser->GetComment ()),
	  _userLicense (userLicense),
	  _projectName (projectName),
	  _projectPath (projectPath)
{}

bool HelpAboutCtrl::OnInitDialog () throw (Win::Exception)
{
	Win::StaticText stc (GetWindow (), IDC_ABOUT);
	stc.SetText ("This is " COOP_PRODUCT_NAME " v. " COOP_PRODUCT_VERSION ".\nServer-less Version Control System\nby Reliable Software");

	_copyright.Init (GetWindow (), IDC_COPYRIGHT);
	_userName.Init (GetWindow (), IDC_USER_NAME);
	_userEmail.Init (GetWindow (), IDC_USER_EMAIL);
	_userPhone.Init (GetWindow (), IDC_USER_PHONE);
	_userLicense.Init (GetWindow (), IDC_USER_LICENSE);
	_projectName.Init (GetWindow (), IDC_PROJECT_NAME);
	_projectPath.Init (GetWindow (), IDC_PROJECT_PATH);

	_copyright.SetText (COPYRIGHT);
	_userName.SetText (_dlgData->GetUser ());
	_userEmail.SetText (_dlgData->GetUserEmail ());
	_userPhone.SetText (_dlgData->GetUserPhone ());
	_userLicense.SetText (_dlgData->GetUserLicense ());
	_projectName.SetText (_dlgData->GetProjectName ());
	_projectPath.SetText (_dlgData->GetProjectPath ());
	return true;
}

// OnDlgControl
bool HelpAboutCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	Win::Dow::Handle appWnd = TheAppInfo.GetWindow ();
	if (ctrlId == IDC_BUTTON_WWW)
	{
		AppHelp::Display (AppHelp::SupportTopic, "support", appWnd);
		return true;
	}
	else if (ctrlId == IDC_BUTTON_LICENSE)
	{
		LicenseDlgData licDlgData (appWnd.GetInstance ());
		LicenseDlgCtrl licCtrl (&licDlgData);
		Dialog::Modal license (GetWindow (), licCtrl);
		return true;
	}
    return false;
}

bool HelpAboutCtrl::OnApply () throw ()
{
	EndOk ();
	return true;
}

LicenseDlgData::LicenseDlgData (Win::Instance inst)
{
	Resource<char const> license (inst, ID_LICENSE_TEXT, "TEXT");
	if (license.IsOk ())
		_text = license.Lock ();
	else
		throw Win::Exception ("Internal Error: cannot find license text");
}

bool LicenseDlgCtrl::OnInitDialog () throw (Win::Exception)
{
	_licenseText.Init (GetWindow (), IDC_LICENSE_AGREEMENT_TEXT);
	
	GetWindow ().CenterOverScreen ();
	_licenseText.SetFocus ();
	_licenseText.SetString (_dlgData->GetText ());
	return true;
}

bool LicenseDlgCtrl::OnApply ()
{
	EndOk ();
	return true;
}
