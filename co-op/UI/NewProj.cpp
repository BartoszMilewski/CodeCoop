//------------------------------------
//  (c) Reliable Software, 1996 - 2008
//------------------------------------

#include "precompiled.h"
#include "NewProj.h"
#include "Catalog.h"
#include "OutputSink.h"
#include "MemberInfo.h"
#include "InputSource.h"
#include "BrowseForFolder.h"

//
// Project general page controller
//

bool GeneralPageHndlr::OnInitDialog () throw (Win::Exception)
{
	_sourcePath.Init (GetWindow (), IDC_NEW_PROJECT_SOURCE_EDIT);
	_browseSource.Init (GetWindow (), IDC_NEW_PROJECT_BROWSE_SOURCE);
	_projectName.Init (GetWindow (), IDC_NEW_PROJECT_NAME_EDIT);
	_userName.Init (GetWindow (), IDC_NEW_PROJECT_USER_NAME);
	_userPhone.Init (GetWindow (), IDC_NEW_PROJECT_USER_PHONE);

	// Limit the number of characters the user can type in the source path edit field
	_sourcePath.LimitText (FilePath::GetLenLimit ());

	CurrentMemberDescription description;
	_userName.SetString (description.GetName ().c_str ());
	_userPhone.SetString (description.GetComment ().c_str ());
	_sourcePath.SetString (_newProjectData.GetProject ().GetRootDir ());

	Project::Options const & options = _newProjectData.GetOptions ();
	if (options.IsNewFromHistory ())
	{
		Project::Data const & project = _newProjectData.GetProject ();
		_historicalProjectName = project.GetProjectName ();
	}
	return true;
}

bool GeneralPageHndlr::OnDlgControl (unsigned id, unsigned notifyCode) throw ()
{
	if (id == IDC_NEW_PROJECT_BROWSE_SOURCE && Win::SimpleControl::IsClicked (notifyCode))
	{
		std::string path;
		if (BrowseForProjectRoot (path, GetWindow ()))
			_sourcePath.SetString (path);
	}
	return true;
}

void GeneralPageHndlr::OnKillActive (long & result) throw (Win::Exception)
{
	// Read page controls
	result = 0;	// Assume everything is ok
	Project::Data & projectData = _newProjectData.GetProject ();
	projectData.SetRootPath (_sourcePath.GetString ());
	projectData.SetProjectName (_projectName.GetString ());

	MemberDescription & member = _newProjectData.GetThisUser ();
	member.SetName (_userName.GetString ());
	member.SetComment (_userPhone.GetString ());
}

void GeneralPageHndlr::OnCancel (long & result) throw (Win::Exception)
{
	_newProjectData.Clear ();
}

void GeneralPageHndlr::OnApply (long & result) throw (Win::Exception)
{
	result = 0;	// Assume everything is ok
	if (!_newProjectData.IsValid ())
	{
		_newProjectData.DisplayErrors (GetWindow ());
		result = 1;	// Don't close dialog
		return;
	}

	Project::Options & options = _newProjectData.GetOptions ();
	if (options.IsProjectAdmin () && options.IsAutoSynch () &&
		!options.IsAutoJoin () && !options.IsDistribution ())
	{
		Out::Answer userChoice = TheOutput.Prompt (
			"You are the Admin for this project and you have selected to\n"
			"automatically execute all incoming synchronization changes,\n"
			"but not to automatically accept join requests.\n\n"
			"You'll have to occasionally check for join request and execute them manually\n\n"
			"Do you want to continue with your current settings (automatic join request\n"
			"processing not selected)?",
			Out::PromptStyle (Out::YesNo, Out::No));
		if (userChoice == Out::No)
		{
			result = 1;	// Don't close dialog
			return;
		}
	}

	if (options.IsNewFromHistory ())
	{
		Project::Data const & project = _newProjectData.GetProject ();
		if (IsNocaseEqual (_historicalProjectName, project.GetProjectName ()))
		{
			Out::Answer userChoice = TheOutput.Prompt (
				"You are attempting to recreate the project under its original name.\n\n"
				"Make sure nobody is enlisted in the original project.\n"
				"Otherwise, chose a new name for the recreated project.\n\n"
				"Are you sure there is no existing enlistment?",
				Out::PromptStyle (Out::YesNo, Out::No));
			if (userChoice == Out::No)
				result = 1;	// Don't close dialog
		}
	}
}

//
// Program options page controller
//

bool OptionsPageHndlr::OnInitDialog () throw (Win::Exception)
{
	Win::Dow::Handle dlgWin (GetWindow ());
	_wiki.Init (dlgWin, IDC_PROJ_OPTIONS_WIKI);
	_autoSynch.Init (dlgWin, IDC_PROJ_OPTIONS_AUTO_SYNCH);
	_autoJoin.Init (dlgWin, IDC_PROJ_OPTIONS_AUTO_JOIN);
	_keepCheckedOut.Init (dlgWin, IDC_PROJ_OPTIONS_KEEP_CHECKED_OUT);
	_distributor.Init (dlgWin, IDC_PROJ_OPTIONS_DISTRIBUTOR);
	_noBranching.Init (dlgWin, IDC_PROJ_OPTIONS_DISALLOW_BRANCHING);
	_singleRecipient.Init (dlgWin, IDC_PROJ_OPTIONS_SINGLE_TO_RECIPIENT);
	_allBccRecipients.Init (dlgWin, IDC_PROJ_OPTIONS_ALL_BCC_RECIPIENTS);

	_wiki.UnCheck ();
	_autoSynch.UnCheck ();
	_autoJoin.UnCheck ();
	_keepCheckedOut.UnCheck ();
	_distributor.UnCheck ();
	_noBranching.UnCheck ();
	_noBranching.Disable ();
	_singleRecipient.Disable ();
	_allBccRecipients.Check ();
	_allBccRecipients.Disable ();
	Project::Options const & options = _projectData.GetOptions ();
	if (options.IsNewFromHistory ())
	{
		_wiki.Disable ();
		_distributor.Disable ();
	}
	return true;
}

bool OptionsPageHndlr::OnDlgControl (unsigned id, unsigned notifyCode) throw ()
{
	if (id == IDC_PROJ_OPTIONS_DISTRIBUTOR && Win::SimpleControl::IsClicked (notifyCode))
	{
		Project::Options const & options = _projectData.GetOptions ();
		if (_distributor.IsChecked ())
		{
			_noBranching.Enable ();
			_noBranching.UnCheck ();
			_autoJoin.UnCheck ();
			_autoJoin.Disable ();
			_singleRecipient.Enable ();
			_allBccRecipients.Enable ();
			if (options.UseBccRecipients ())
				_allBccRecipients.Check ();
			else
				_singleRecipient.Check ();
		}
		else
		{
			_noBranching.UnCheck ();
			_noBranching.Disable ();
			_singleRecipient.Disable ();
			_allBccRecipients.Disable ();
			_autoJoin.Enable ();
		}
		return true;
	}
    return false;
}

void OptionsPageHndlr::OnKillActive (long & result) throw (Win::Exception)
{
	// Read page controls
	result = 0;	// Assume everything is ok
	Project::Options & options = _projectData.GetOptions ();
	options.SetWiki (_wiki.IsChecked ());
	options.SetAutoSynch (_autoSynch.IsChecked ());
	options.SetAutoJoin (_autoJoin.IsChecked ());
	options.SetKeepCheckedOut (_keepCheckedOut.IsChecked ());
	options.SetDistribution (_distributor.IsChecked ());
	options.SetNoBranching (_noBranching.IsChecked ());
	options.SetBccRecipients (_allBccRecipients.IsChecked ());
}

void OptionsPageHndlr::OnCancel (long & result) throw (Win::Exception)
{
	_projectData.Clear ();
}

void OptionsPageHndlr::OnApply (long & result) throw (Win::Exception)
{
	// Validation already done by general page
	result = 0;	// Assume everything is ok
}

//
// New project controller set
//

NewProjectHndlrSet::NewProjectHndlrSet (NewProjectData & projectData)
	: PropPage::HandlerSet ("Create New Project"),
	  _newProjectData (projectData),
	  _generalPageHndlr (_newProjectData),
	  _optionsPageHndlr (_newProjectData)
{
	AddHandler (_generalPageHndlr, "General");
	AddHandler (_optionsPageHndlr, "Options");
}

// command line
// -Project_New project:"Name" root:"Local\Path" recipient:"recipHubId" user:"My Name"
//      email:"myHubId" comment:"my comment" state:"observer"
//      autosynch:"yes" autofullsynch:"yes" keepcheckedout:"yes"

bool NewProjectHndlrSet::GetDataFrom (NamedValues const & source)
{
	_newProjectData.ReadNamedValues (source);
#if 0
	dbg << "Names values: " << _newProjectData.GetNamedValues () << std::endl;
#endif
	return _newProjectData.IsValid ();
}
