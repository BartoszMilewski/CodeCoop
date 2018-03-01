//------------------------------------
//  (c) Reliable Software, 1998 - 2006
//------------------------------------

#include "precompiled.h"
#include "RevertDlg.h"
#include "VersionInfo.h"
#include "resource.h"

#include <TimeStamp.h>

RevertData::RevertData (VersionInfo const & info, bool fileRestore)
	: _fileRestore (fileRestore)
{
	GlobalIdPack pack (info.GetVersionId ());
	_version = pack.ToBracketedString ();
	_version += " -- ";
	_version += info.GetComment ();
	_version += "\r\nFrom ";
	StrTime timeStamp (info.GetTimeStamp ());
	_version += timeStamp.GetString ();
}

RevertCtrl::RevertCtrl (RevertData const & data)
	: Dialog::ControlHandler (IDD_REVERT_INFO),
	  _dlgData (data)
{}

bool RevertCtrl::OnInitDialog () throw (Win::Exception)
{
	Win::Dow::Handle dlgWin = GetWindow ();
	_versionFrame.Init (dlgWin, IDC_REVERT_INFO_VERSION_FRAME);
	_version.Init (dlgWin, IDC_REVERT_INFO_VERSION);
	_revertDetails.Init (dlgWin, IDC_REVERT_INFO_DETAILS);

	if (_dlgData.IsFileRestore ())
	{
		dlgWin.SetText ("File Restore Information");
		_versionFrame.SetText ("You are about to restore the selected file(s) to the version before the following script: ");
		_revertDetails.SetText ("Code Co-op marked all the scripts that will be searched for edit changes to the selected files.");
	}
	else
	{
		dlgWin.SetText ("Project Restore Information");
		_versionFrame.SetText ("You are about to restore the project state to the version before the following script: ");
		_revertDetails.SetText ("Code Co-op marked all scripts that will be undone, and listed all affected files.");
	}
	_version.SetString (_dlgData.GetRevertVersion ());
	return true;
}

bool RevertCtrl::OnApply () throw ()
{
	EndOk ();
	return true;
}
