// ----------------------------------
// (c) Reliable Software, 2000 - 2008
// ----------------------------------

#include "precompiled.h"
#include "MoveProj.h"
#include "Catalog.h"
#include "OutputSink.h"
#include "resource.h"
#include "BrowseForFolder.h"

#include <Com/Shell.h>
#include <Ctrl/Output.h>

MoveProjectCtrl::MoveProjectCtrl (MoveProjectData & moveData)
	: Dialog::ControlHandler (IDD_MOVE_PROJECT),
	  _moveData (moveData)
{}

// command line
// -project_move -target:"path" -virtual:"yes"
// if -virtual is set to "yes", the project tree is not copied,
// only the path in the database is updated

bool MoveProjectCtrl::GetDataFrom (NamedValues const & source)
{
	std::string path = source.GetValue ("target");
	if (path.empty ())
		return false;

	_moveData.GetProject ().SetRootPath (path);

	std::string isVirtual = source.GetValue ("virtual");
	_moveData.SetVirtualMove (IsNocaseEqual (isVirtual, "yes"));

	_moveData.SetProjectFilesOnly (false);

	return _moveData.IsRootPathOk ();
}

bool MoveProjectCtrl::OnInitDialog () throw (Win::Exception) 
{
    _newSrc.Init (GetWindow (), IDC_NEW_SRC);
    _browse.Init (GetWindow (), IDC_BROWSE);
	_moveFiles.Init (GetWindow (), IDC_PROJ_MOVE_FILES);
	_moveAll.Init (GetWindow (), IDC_PROJ_MOVE_ALL);
	if (_moveData.MoveProjectFilesOnly ())
		_moveFiles.Check ();
	else
		_moveAll.Check ();
	return true;
}

bool MoveProjectCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	if (ctrlId == IDC_BROWSE && Win::SimpleControl::IsClicked (notifyCode))
	{
		std::string path;
		if (BrowseForLocalFolder (path,
								  GetWindow (),
								  "Select folder (existing or not) for the new project root."))
		{
			_newSrc.SetString (path);
		}
		return true;
	};
	return false;
}

bool MoveProjectCtrl::OnApply () throw ()
{
	_moveData.GetProject ().SetRootPath (_newSrc.GetString ());
	_moveData.SetProjectFilesOnly (_moveFiles.IsChecked ());
	if (_moveData.IsRootPathOk ())
		EndOk ();
	else
		_moveData.DisplayErrors (GetWindow ());
	return true;
}
