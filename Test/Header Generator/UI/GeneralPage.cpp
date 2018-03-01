// ---------------------------
// (c) Reliable Software, 2000
// ---------------------------

#include "GeneralPage.h"
#include "HeaderDetails.h"
#include "resource.h"

extern void GenerateScript (HeaderDetails const & details);

void GeneralHandler::OnApply (long & result) throw (Win::Exception) 
{
	result = PSNRET_NOERROR;
	GenerateScript (_ctrl.GetDetails ());
}

void GeneralHandler::OnReset () throw (Win::Exception)
{
	// Cancel button was pressed
	// The PSN_RESET notification is received by every page that was initialised
	// It notifies to ignore all changes since Apply button was last clicked
	_ctrl.GetDetails ()._wasCanceled = true;
}

void GeneralCtrl::OnInitDialog () throw (Win::Exception)
{
	_known.Init (GetWindow (), IDC_PROJECT_KNOWN);
	_projectList.Init (GetWindow (), IDC_PROJECT_COMBO);
	_unknown.Init (GetWindow (), IDC_PROJECT_UNKNOWN);
	_define.Init (GetWindow (), IDC_PROJECT_DEFINE);
	_projectName.Init (GetWindow (), IDC_PROJECT);
	_forward.Init (GetWindow (), IDC_FORWARD);
	_defect.Init (GetWindow (), IDC_DEFECT);
	_addendum.Init (GetWindow (), IDC_ADDENDUM);

	//_known.Check ();
	//_projectName.Disable ();
	_define.Check ();
}

bool GeneralCtrl::OnCommand (int ctrlId, int notifyCode) throw (Win::Exception)
{
	switch (ctrlId)
	{
	case IDC_PROJECT_KNOWN:
		_projectName.Disable ();
		_projectList.Enable ();
		return true;
	case IDC_PROJECT_UNKNOWN:
		_projectName.Disable ();
		_projectList.Disable ();
		return true;
	case IDC_PROJECT_DEFINE:
		_projectName.Enable ();
		_projectList.Disable ();
		return true;
	}
	return false;
}

void GeneralCtrl::RetrieveData ()
{
	if (_known.IsChecked ())
	{
		// Revisit:
	}
	else if (_unknown.IsChecked ())
	{
		// Revisit:
	}
	else
	{
		_details._projectName = _projectName.GetString ();
	}
	_details._toBeForwarded = _forward.IsChecked ();
	_details._isDefect = _defect.IsChecked ();
	_details._hasAddendums = _addendum.IsChecked ();
}
