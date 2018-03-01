// ----------------------------------
// (c) Reliable Software, 2003 - 2005
// ----------------------------------

#include "precompiled.h"
#include "UpdateDlg.h"
#include "OutputSink.h"
#include "AppInfo.h"
#include "Validators.h"

#include <Com/Shell.h>

void OpenWebsite (char const * pageTitle, char const * link, Win::Dow::Handle win)
{
	int errCode = ShellMan::Open (TheAppInfo.GetWindow (), link);
	if (errCode != -1)
	{
		std::string msg = ShellMan::HtmlOpenError (errCode, pageTitle, link);
		TheOutput.Display (msg.c_str (), Out::Error, win);
	}
}

UpdateCtrl::UpdateCtrl (UpdateDlgData & data)
	: Dialog::ControlHandler (IDD_UPDATE),
	  _data (data)
{}

bool UpdateCtrl::OnInitDialog () throw (Win::Exception)
{
	_statusStr.Init (GetWindow (), IDC_STATUS);
	_releaseNotesButton.Init (GetWindow (), IDC_RELEASE_NOTES);
	_versionHeadline.Init (GetWindow (), IDC_DESCRIPTION);
	_remindMeLater.Init (GetWindow (), IDC_REMIND_LATER);
	_turnOnAutoDownload.Init (GetWindow (), IDC_TURN_ON_AUTO_DOWNLOAD);

	if (_data._isExeDownloaded)
		_statusStr.SetText ("A new version is downloaded and ready to install!");
	else
		_statusStr.SetText ("A new version is available for download!");

	_bulletinHeadline.Init (GetWindow (), IDC_BULLETIN_HEADLINE);
	_bulletinButton.Init (GetWindow (), IDC_BULLETIN_LINK);

	_versionHeadline.SetString (_data._versionHeadline.c_str ());

	if (_data._releaseNotesLink.empty ())
		_releaseNotesButton.Disable ();

	_bulletinHeadline.SetText (_data._bulletinHeadline.c_str ());

	if (_data._bulletinLink.empty ())
		_bulletinButton.Disable ();

	if (!_data._isAutoCheck)
		_remindMeLater.Hide ();

	if (_data._isAutoCheck && _data._isAutoDownload)
	{
		_turnOnAutoDownload.Check ();
		_turnOnAutoDownload.Disable ();
	}

	return true;
}

bool UpdateCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
    switch (ctrlId)
    {
	case IDC_REMIND_LATER: // ends the dialog
		_data._isRemindMeLater = true;
        EndOk ();
		return true;
	case IDC_BULLETIN_LINK:
		if (_bulletinButton.IsClicked (notifyCode))
			OpenWebsite ("Bulletin", _data._bulletinLink.c_str (), GetWindow ());
		break;
	case IDC_RELEASE_NOTES:
		if (_releaseNotesButton.IsClicked (notifyCode))
			OpenWebsite ("Release notes", _data._releaseNotesLink.c_str (), GetWindow ());
		break;
	};
    return false;
}

bool UpdateCtrl::OnApply () throw ()
{
	if (!_data._isAutoCheck || !_data._isAutoDownload)
	{
		_data._turnOnAutoDownload = _turnOnAutoDownload.IsChecked ();
	}
	EndOk ();
	return true;
}

UpToDateCtrl::UpToDateCtrl (UpToDateDlgData & data)
	: Dialog::ControlHandler (IDD_UP_TO_DATE),
	  _data (data)
{}

bool UpToDateCtrl::OnInitDialog () throw (Win::Exception)
{
	_status.Init (GetWindow (), IDC_STATUS);
	_bulletinHeadline.Init (GetWindow (), IDC_BULLETIN_HEADLINE);
	_bulletinButton.Init (GetWindow (), IDC_BULLETIN_LINK);

	_status.SetText (_data._status.c_str ());
	_bulletinHeadline.SetText (_data._bulletinHeadline.c_str ());
	if (_data._bulletinLink.empty ())
		_bulletinButton.Disable ();

	return true;
}

bool UpToDateCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
    if (ctrlId == IDC_BULLETIN_LINK &&
		Win::SimpleControl::IsClicked (notifyCode))
    {
		OpenWebsite ("Bulletin", _data._bulletinLink.c_str (), GetWindow ());
		return true;
	};
    return false;
}

bool UpToDateCtrl::OnApply () throw ()
{
	EndOk ();
	return true;
}

UpdateOptionsCtrl::UpdateOptionsCtrl (UpdateOptionsDlgData & data)
	: Dialog::ControlHandler (IDD_UPDATE_OPTIONS),
	  _data (data)
{}

bool UpdateOptionsCtrl::OnInitDialog () throw (Win::Exception)
{
	_autoCheck.Init (GetWindow (), IDC_AUTO_CHECK);
	_inBackground.Init (GetWindow (), IDC_IN_BACKGROUND);
	_period.Init (GetWindow (), IDC_PERIOD_EDIT);
	_periodSpin.Init (GetWindow (), IDC_PERIOD_SPIN);

	_periodSpin.SetRange (UpdatePeriodValidator::GetMin (), UpdatePeriodValidator::GetMax ());
	_period.LimitText (2);

	if (_data._isAutoCheck)
		_autoCheck.Check ();
	_periodSpin.SetPos (_data._period);
	SetAutoCheckControls (_data._isAutoCheck);

	return true;
}

bool UpdateOptionsCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
    if (ctrlId == IDC_AUTO_CHECK)
    {
		SetAutoCheckControls (_autoCheck.IsChecked ());
		return true;
	};
    return false;
}

bool UpdateOptionsCtrl::OnApply () throw ()
{
	bool isOk = true;
	_data._isAutoCheck = _autoCheck.IsChecked ();
	if (_data._isAutoCheck)
	{
		_data._isAutoDownload = _inBackground.IsChecked ();
		int newPeriod = 0;
		_period.GetInt (newPeriod);
		UpdatePeriodValidator period (newPeriod);
		if (period.IsValid ())
		{
			_data._period = newPeriod;
		}
		else
		{
			period.DisplayError ();
			isOk = false;
		}
	}
	
	if (isOk)
		EndOk ();

	return true;
}

void UpdateOptionsCtrl::SetAutoCheckControls (bool isAuto)
{
	if (isAuto)
	{
		_inBackground.Enable ();
		_period.Enable ();
		_periodSpin.Enable ();

		if (_data._isAutoDownload)
			_inBackground.Check ();
	}
	else
	{
		_inBackground.UnCheck ();
		_inBackground.Disable ();
		_period.Disable ();
		_periodSpin.Disable ();
	}
}
