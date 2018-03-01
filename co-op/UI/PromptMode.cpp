//------------------------------------
//  (c) Reliable Software, 2004 - 2005
//------------------------------------

#include "precompiled.h"
#include "PromptMode.h"
#include "Registry.h"
#include "AppInfo.h"
#include "Prompter.h"
#include "resource.h"

#include <Sys/WinString.h>

PromptModeData::PromptModeData (unsigned int promptId)
	: _id (promptId)
{
	Registry::UserPreferences prefs;
	_isOn = prefs.CanAsk (_id);
	if (_isOn)
	{
		_prompt = ResString (TheAppInfo.GetWindow (), _id);
	}
}

void PromptModeData::TurnOffPrompt ()
{
	_isOn = false;
	Registry::UserPreferences prefs;
	prefs.TurnOffPrompt (_id);
}

PromptModeCtrl::PromptModeCtrl (PromptModeData * data)
	: Dialog::ControlHandler (IDD_PROMPT_MODE),
	  _dlgData (data)
{}

bool PromptModeCtrl::OnInitDialog () throw (Win::Exception)
{
    _dontShowAgain.Init (GetWindow (), IDC_PROMPT_MODE_DONOT_SHOW);
	_prompt.Init (GetWindow (), IDC_PROMPT_MODE_TEXT);
	_promptImage.Init (GetWindow (), IDC_PROMPT_ICON);
	
	_prompt.SetText (_dlgData->GetPrompt ().c_str ());
	_promptImage.SetIcon (_sysIcon.Load (IDI_QUESTION));
	return true;
}

bool PromptModeCtrl::OnApply () throw ()
{
	if (_dontShowAgain.IsChecked ())
		_dlgData->TurnOffPrompt ();
	EndOk ();
    return true;
}

// "No" button is actually a Cancel button (IDCANCEL), only caption is changed
// To make this work correctly the X button in the dialog's System menu is not displayed.
bool PromptModeCtrl::OnCancel () throw ()
{
	if (_dontShowAgain.IsChecked ())
		_dlgData->TurnOffPrompt ();
	EndCancel ();
	return true;
}

bool PromptMode::Prompt ()
{
	PromptModeCtrl ctrl (&_dlgData);
    return ThePrompter.GetData (ctrl);
}
