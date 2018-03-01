// ----------------------------------
// (c) Reliable Software, 2002 - 2006
// ----------------------------------

#include "precompiled.h"
#include "StandaloneJoin.h"
#include "MemberDescription.h"
#include "Catalog.h"
#include "OutputSink.h"
#include "BrowseForFolder.h"

StandaloneJoinHandlerSet::StandaloneJoinHandlerSet (
			JoinProjectData & joinData, 
			NocaseSet const & projects)
	: PropPage::HandlerSet ("Join Existing Project"),
	  _joinCtrl (joinData, projects, IDD_STANDALONE_JOIN),
	  _sourceCtrl (joinData, IDD_STANDALONE_JOIN_SOURCE),
	  _userCtrl (joinData, IDD_STANDALONE_JOIN_USER)
{
	AddHandler (_joinCtrl);
	AddHandler (_sourceCtrl);
	AddHandler (_userCtrl);
}

void StandaloneJoinCtrl::OnSetActive (long & result) throw (Win::Exception)
{
	if (_local.IsChecked ())
        SetButtons (PropPage::Wiz::Next);
	else
        SetButtons (PropPage::Wiz::Finish);

	result = 0;
}

bool StandaloneJoinCtrl::GoNext (long & result)
{
	Project::Data & project = _joinData.GetProject ();
	project.SetProjectName (_projects.RetrieveEditText ());
	_joinData.SetRemoteAdmin (false);
	result = IDD_STANDALONE_JOIN_SOURCE; 
	return true;
}

void StandaloneJoinCtrl::OnFinish (long & result) throw (Win::Exception)
{
	_joinData.SetRemoteAdmin (true);
	_joinData.SetConfigureFirst (true);
}

bool StandaloneJoinCtrl::OnInitDialog () throw (Win::Exception)
{
	_local.Init (GetWindow (), IDC_LOCAL);
	_nonLocal.Init (GetWindow (), IDC_NOT_LOCAL);
	_projects.Init (GetWindow (), IDC_LOCAL_PROJECTS);

	_nonLocal.Check ();
	_projects.Disable ();
	if (_projectList.empty ())
	{
		_local.Disable ();
	}
	else
	{
		for (NocaseSet::const_iterator it = _projectList.begin (); it != _projectList.end (); ++it)
		{
			_projects.AddToList (it->c_str ());
		}
		_projects.SetSelection (0);
	}

	return true;
}

bool StandaloneJoinCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	switch (ctrlId)
	{
	case IDC_LOCAL:
		_nonLocal.UnCheck ();
		_projects.Enable ();
        SetButtons (PropPage::Wiz::Next);
		break;
	case IDC_NOT_LOCAL:
		_local.UnCheck ();
		_projects.Disable ();
        SetButtons (PropPage::Wiz::Finish);
		break;
	default:
		return false;
	}
	return false;
}

void StandaloneJoinSourceCtrl::OnSetActive (long & result) throw (Win::Exception)
{
	SetButtons (PropPage::Wiz::NextBack);
	result = 0;
}

bool StandaloneJoinSourceCtrl::GoNext (long & result)
{
	Project::Data & project = _joinData.GetProject ();
	project.SetRootPath (_source.GetString ());
	if (_joinData.IsRootPathOk ())
	{
		result = IDD_STANDALONE_JOIN_USER;
		return true;
	}
	else
	{
		_joinData.DisplayErrors (GetWindow ());
		return false;
	}
}

bool StandaloneJoinSourceCtrl::OnInitDialog () throw (Win::Exception)
{
	_source.Init (GetWindow (), IDC_SOURCE_PATH);
	_browse.Init (GetWindow (), IDC_BROWSE);
	return true;
}

bool StandaloneJoinSourceCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	switch (ctrlId)
	{
	case IDC_BROWSE:
        {
			std::string folder;
			if (BrowseForProjectRoot (folder, GetWindow (), false))	// Joining
                _source.SetString (folder);
        }
        return true;
		break;
	default:
		return false;
	}
	return false;
}

void StandaloneJoinUserCtrl::OnSetActive (long & result) throw (Win::Exception)
{
	SetButtons (PropPage::Wiz::BackFinish);
	result = 0;
}

void StandaloneJoinUserCtrl::OnFinish (long & result) throw (Win::Exception)
{
	MemberDescription & joinee = _joinData.GetThisUser ();
	joinee.SetName (_name.GetString ());
	joinee.SetComment (_comment.GetString ());
	_joinData.SetObserver (_observer.IsChecked ());

	if (joinee.GetName ().empty ())
	{
		TheOutput.Display ("Please specify your name.", Out::Information, GetWindow ());
		result = -1;
	}
}

bool StandaloneJoinUserCtrl::OnInitDialog () throw (Win::Exception)
{
	_name.Init (GetWindow (), IDC_JOIN_PROJECT_USER_NAME);
	_comment.Init (GetWindow (), IDC_JOIN_PROJECT_USER_PHONE);
	_observer.Init (GetWindow (), IDC_JOIN_PROJECT_OBSERVER);

	CurrentMemberDescription description;
	_name.SetString (description.GetName ().c_str ());
	_comment.SetString (description.GetComment ().c_str ());

	if (_joinData.IsObserver ())
		_observer.Check ();
	if (_joinData.IsForceObserver ())
		_observer.Disable ();

	return true;
}
