//------------------------------------
//  (c) Reliable Software, 2005 - 2006
//------------------------------------

#include "precompiled.h"
#include "ProjectBranchDlg.h"
#include "Catalog.h"
#include "VersionInfo.h"
#include "MemberDescription.h"
#include "OutputSink.h"
#include "MemberInfo.h"
#include "InputSource.h"
#include "BrowseForFolder.h"

#include <TimeStamp.h>

GlobalId BranchProjectData::GetBranchScriptId () const
{
	return _versionInfo.GetVersionId ();
}

bool BranchProjectData::IsCurrentVersion () const
{
	return _versionInfo.IsCurrent ();
}

std::string BranchProjectData::GetVersionComment () const
{
	std::string comment;
	if (!_versionInfo.IsCurrent ())
	{
		comment = GlobalIdPack (_versionInfo.GetVersionId ()).ToBracketedString ();
		comment += " -- ";
	}
	comment += _versionInfo.GetComment ();
	comment += "\r\nFrom ";
	StrTime timeStamp (_versionInfo.GetTimeStamp ());
	comment += timeStamp.GetString ();
	if (_versionInfo.IsCurrent ())
	{
		comment += "\r\n\r\nHint: To branch any historical version, switch to history view, ";
		comment += "\r\nselect a script and use menu Selection>Branch.";
	}
	return comment;
}

std::string const & BranchProjectData::GetUserName () const
{
	return _myDescription.GetName ();
}

std::string const & BranchProjectData::GetUserComment () const
{
	return _myDescription.GetComment ();
}

//
// Project branch general page controller
//

bool BranchGeneralPageHndlr::OnInitDialog () throw (Win::Exception)
{
	_versionFrame.Init (GetWindow (), IDC_BRANCH_VERSION_FRAME);
	_version.Init (GetWindow (), IDC_BRANCH_VERSION);
	_targetPath.Init (GetWindow (), IDC_BRANCH_TARGET);
	_browseSource.Init (GetWindow (), IDC_BRANCH_TARGET_BROWSE);
	_projectName.Init (GetWindow (), IDC_BRANCH_PROJECT_NAME);
	_userName.Init (GetWindow (), IDC_BRANCH_PROJECT_USER);
	_comment.Init (GetWindow (), IDC_BRANCG_PROJECT_COMMENT);

	// Limit the number of characters the user can type in the source path edit field
	_targetPath.LimitText (FilePath::GetLenLimit ());
	if (_branchData.IsCurrentVersion ())
		_versionFrame.SetText ("Use project files and folders as of: ");

	_version.SetText (_branchData.GetVersionComment ());
	_userName.SetString (_branchData.GetUserName ());
	_comment.SetString (_branchData.GetUserComment ());
	_targetPath.SetString (_branchData.GetProject ().GetRootDir ());
	return true;
}

bool BranchGeneralPageHndlr::OnDlgControl (unsigned id, unsigned notifyCode) throw ()
{
	if (id == IDC_BRANCH_TARGET_BROWSE && Win::SimpleControl::IsClicked (notifyCode))
	{
		std::string path;
		if (BrowseForProjectRoot (path, GetWindow ()))
			_targetPath.SetString (path);
	}
	return true;
}

void BranchGeneralPageHndlr::OnCancel (long & result) throw (Win::Exception)
{
	_branchData.Clear ();
}

void BranchGeneralPageHndlr::OnApply (long & result) throw (Win::Exception)
{
	result = 0;	// Assume everything is ok
	Project::Data & projectData = _branchData.GetProject ();
	projectData.SetRootPath (_targetPath.GetString ());
	projectData.SetProjectName (_projectName.GetString ());

	MemberDescription & member = _branchData.GetThisUser ();
	member.SetName (_userName.GetString ());
	member.SetComment (_comment.GetString ());

	if (!_branchData.IsValid ())
	{
		_branchData.DisplayErrors (GetWindow ());
		result = 1;	// Don't close dialog
	}
}

//
// Project branch controller set
//

ProjectBranchHndlrSet::ProjectBranchHndlrSet (BranchProjectData & branchData)
	: PropPage::HandlerSet ("Create Project Branch"),
	  _branchData (branchData),
	  _generalPageHndlr (_branchData),
	  _optionsPageHndlr (_branchData)
{
	AddHandler (_generalPageHndlr, "General");
	AddHandler (_optionsPageHndlr, "Options");
}
