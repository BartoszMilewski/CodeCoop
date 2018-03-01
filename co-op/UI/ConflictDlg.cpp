//------------------------------------
//  (c) Reliable Software, 1998 - 2007
//------------------------------------

#include "precompiled.h"
#include "ConflictDlg.h"
#include "Conflict.h"
#include "Prompter.h"
#include "resource.h"

ScriptConflictCtrl::ScriptConflictCtrl (ConflictDetector & detector)
	: Dialog::ControlHandler (IDD_SCRIPT_CONFLICT),
	  _dlgData (detector)
{}

bool ScriptConflictCtrl::OnInitDialog () throw (Win::Exception)
{
	Assert (_dlgData.AreMyScriptsRejected () || _dlgData.AreMyLocalEditsInMergeConflict ());

	_conflictVersion.Init (GetWindow (), IDC_SCRIPT_CONFLICT_VERSION);
	_autoMerge.Init (GetWindow (), IDC_SCRIPT_CONFLICT_AUTOMERGE);
	_executeOnly.Init (GetWindow (), IDC_SCRIPT_CONFLICT_EXECUTEONLY);
	_postpone.Init (GetWindow (), IDC_SCRIPT_CONFLICT_POSTPONE);

	_conflictVersion.SetText (_dlgData.GetConflictVersion ().c_str ());
	if (_dlgData.IsAutoMerge ())
		_autoMerge.Check ();
	else
		_executeOnly.Check ();
	return true;
}

bool ScriptConflictCtrl::OnApply () throw ()
{
	if (_postpone.IsChecked ())
	{
		EndCancel ();
	}
	else
	{
		_dlgData.SetAutoMerge (_autoMerge.IsChecked ());
		EndOk ();
	}
	return true;
}

bool ScriptConflictDlg::Show (ConflictDetector & detector)
{
	if (!detector.IsConflict ())
		return true;
	ScriptConflictCtrl ctrl (detector);
    return ThePrompter.GetData (ctrl);
}
