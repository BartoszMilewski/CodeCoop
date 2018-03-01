//------------------------------------
//  (c) Reliable Software, 1997 - 2008
//------------------------------------

#include "precompiled.h"
#include "ProjDefect.h"
#include "resource.h"

#include <Dbg/Assert.h>

#include <StringOp.h>

ProjDefectCtrl::ProjDefectCtrl (ProjDefectData * data)
	: Dialog::ControlHandler (IDD_PROJECT_DEFECT),
	  _dlgData (data)
{}

bool ProjDefectCtrl::OnInitDialog () throw (Win::Exception)
{
	Win::Dow::Handle dlgWin (GetWindow ());
	_deleteHistory.Init (dlgWin, IDC_PROJ_DEFECT_DELETE_HISTORY);
	_deleteFiles.Init (dlgWin, IDC_PROJ_DEFECT_DELETE_FILES);
	_deleteAll.Init (dlgWin, IDC_PROJ_DEFECT_DELETE_ALL);

    if (_dlgData->DontDelete ())
        _deleteHistory.Check ();
    else
        _deleteHistory.UnCheck ();
	if (_dlgData->DeleteProjectFiles ())
		_deleteFiles.Check ();
	else
		_deleteFiles.UnCheck ();
    if (_dlgData->DeleteWholeProjectTree ())
        _deleteAll.Check ();
    else
        _deleteAll.UnCheck ();

	std::string caption ("Defect From Project ");
	caption += _dlgData->GetProjectName ();
	dlgWin.SetText (caption.c_str ());
	return true;
}

bool ProjDefectCtrl::OnApply () throw ()
{
	if (_deleteHistory.IsChecked ())
	{
		_dlgData->SetDontDelete ();
	}
	else if (_deleteFiles.IsChecked ())
	{
		_dlgData->SetDeleteProjectFiles ();
	}
	else
	{
		Assert (_deleteAll.IsChecked ());
		_dlgData->SetDeleteWholeProjectTree ();
	}
	EndOk ();
    return true;
}

// command line
// -Project_Defect kind:"Nothing | Files | Tree" force :"yes | no"

bool ProjDefectCtrl::GetDataFrom (NamedValues const & source)
{
	std::string force = source.GetValue ("force");
	if (force == "yes")
		_dlgData->SetUnconditionalDefect ();

	std::string kind = source.GetValue ("kind");
	if (IsNocaseEqual (kind, "Nothing"))
		_dlgData->SetDontDelete ();
	else if (IsNocaseEqual (kind, "Files"))
		_dlgData->SetDeleteProjectFiles ();
	else if (IsNocaseEqual (kind, "Tree"))
		_dlgData->SetDeleteWholeProjectTree ();
	else
		return false;

	return true;
}
