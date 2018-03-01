//----------------------------------
// (c) Reliable Software 1998 - 2007
//----------------------------------

#include "precompiled.h"
#include "LicenseDlg.h"
#include "License.h"
#include "AppInfo.h"
#include "OutputSink.h"
#include "Registry.h"
#include "DistributorInfo.h"
#include "Global.h"
#include "resource.h"

#include <Com/Shell.h>
#include <Sys/WinString.h>

LicenseData::LicenseData (bool isLicensed, std::string const & status)
	: _isLicensed (isLicensed),
	  _status (status)
{
	_nag = Registry::IsNagging ();
}

bool LicenseData::SetNewLicense (std::string const & licensee, std::string const & key)
{
	License license (licensee, key);
	if (license.IsCurrentVersion ())
	{
		_newLicense = license.GetLicenseString ();
		return true;
	}

	return false;
}

LicenseCtrl::LicenseCtrl (LicenseData * data)
	: Dialog::ControlHandler (IDD_LICENSE_CHECK),
	  _dlgData (data)
{}

bool LicenseCtrl::OnInitDialog () throw (Win::Exception)
{
	_purchase.Init (GetWindow (), IDC_LICENSE_PURCHASE);
	_register.Init (GetWindow (), IDC_LICENSE_REGISTER);
	_evaluate.Init (GetWindow (), IDC_LICENSE_KEEP_EVALUATING);
	_licensee.Init (GetWindow (), IDC_LICENSEE_EDIT);
	_key.Init (GetWindow (), IDC_KEY_EDIT);
	_status.Init (GetWindow (), IDC_LICENSEE_TEXT);
	_dontNag.Init (GetWindow (), IDC_LICENSE_DONT_NAG);

	if (_dlgData->IsLicensed ())
		_evaluate.SetText ("Cancel");

	_status.SetText (_dlgData->GetStatus ());
	if (_dlgData->IsNoNagging ())
		_dontNag.Check ();
	else
		_dontNag.UnCheck ();
	return true;
}

bool LicenseCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	switch (ctrlId)
	{
	case IDC_LICENSE_PURCHASE:
		{
			Win::Dow::Handle appWnd = TheAppInfo.GetWindow ();
			int errCode = ShellMan::Open (appWnd, PurchaseLink);
			if (errCode != -1)
			{
				std::string msg = ShellMan::HtmlOpenError (errCode, "license", PurchaseLink);
				TheOutput.Display (msg.c_str (), Out::Error, GetWindow ());
			}
			EndOk ();
		}
		return true;
	case IDC_LICENSE_REGISTER:
		OnApply ();
		return true;
	case IDC_LICENSE_KEEP_EVALUATING:
		OnCancel ();
		return true;
	}
    return false;
}

bool LicenseCtrl::OnApply () throw ()
{
	std::string licensee (_licensee.GetTrimmedString ());
	std::string key (_key.GetTrimmedString ());
	if (!licensee.empty () && !key.empty ())
	{
		if (_dlgData->SetNewLicense (licensee, key))
		{
			EndOk ();
		}
		else
		{
			std::string info;
			License license (licensee, key);
			if (license.IsValid ())
			{
				info = "Invalid license.\n\nYou have a license for version ";
				info += ToString (license.GetVersion ());
				info += ".x\nYou are using version ";
				info += ToString (TheCurrentMajorVersion);
				info += ".x of Code Co-op.\n\n";

			}
			else
			{
				info  = "This is not a valid license:\n\n";
				info += licensee;
				info += " (";
				info += key;
				info += ")";
			}
			TheOutput.Display (info.c_str (), Out::Information, GetWindow ());
		}
	}
	else
	{
		// User entered empty licensee and key -- do nothing
		OnCancel ();
	}
	return true;
}

bool LicenseCtrl::OnCancel () throw ()
{
	_dlgData->SetNagging (!_dontNag.IsChecked ());
	EndCancel ();
	return true;
}
