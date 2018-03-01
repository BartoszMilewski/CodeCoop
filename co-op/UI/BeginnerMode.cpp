//------------------------------------
//  (c) Reliable Software, 1998 - 2006
//------------------------------------

#include "precompiled.h"
#include "BeginnerMode.h"
#include "Prompter.h"
#include "AppInfo.h"
#include "Registry.h"
#include "resource.h"

#include <Sys/WinString.h>

BeginnerModeCtrl::BeginnerModeCtrl (BeginnerModeData * data)
	: Dialog::ControlHandler (IDD_BEGINNER_MODE),
	  _dlgData (data)
{}

bool BeginnerModeCtrl::OnInitDialog () throw (Win::Exception)
{
    _show.Init (GetWindow (), IDC_BEGINNER_MODE_DONOT_SHOW);
	_message.Init (GetWindow (), IDC_BEGINNER_MODE_TEXT);
	_image.Init (GetWindow (), IDC_BEGINNER_ICON);
	
	_message.SetText (_dlgData->GetMessage ());
	_image.SetIcon (_sysIcon.Load (IDI_INFORMATION));
	return true;
}

bool BeginnerModeCtrl::OnApply () throw ()
{
	_dlgData->SetMessageStatus (!_show.IsChecked ());
	EndOk ();
	return true;
}

BeginnerMode::BeginnerMode ()
    : _dlgData (),
	  _disabledCount (0)
{
	_isModeOn = Registry::IsBeginnerModeOn ();
	if (_isModeOn)
	{
		int count = Registry::CountDisabledBeginner (IDS_FIRST_HELP + 1, IDS_LAST_HELP);
		_isModeOn = count < IDS_LAST_HELP - (IDS_FIRST_HELP + 1);
	}
}

void BeginnerMode::SetStatus (bool on)
{
	_isModeOn = on;
	Registry::SetBeginnerMode (on);
	if (on)
		_disabledCount = 0;

}

void BeginnerMode::Display (int msgId, bool isForce)
{
	if (!_isModeOn && !isForce)
		return;

	if (Registry::IsBeginnerMessageOff (msgId))
		return;

	ResString message (TheAppInfo.GetWindow (), msgId);
	_dlgData.SetMessage (message);
	BeginnerModeCtrl ctrl (&_dlgData);
    ThePrompter.GetData (ctrl);
	// Check if this message was disabled
	if (!_dlgData.IsMessageOn ())
	{
		// Store message id in the registry, so next time it is not displayed
		Registry::BeginnerMessageOff (msgId);
		_disabledCount++;
		_isModeOn = _disabledCount < IDS_LAST_HELP - (IDS_FIRST_HELP + 1);
	}
}

