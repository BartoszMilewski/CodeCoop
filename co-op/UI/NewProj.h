#if !defined NEWPROJ_H
#define NEWPROJ_H
//------------------------------------
//  (c) Reliable Software, 1996 - 2008
//------------------------------------

#include "ProjectBlueprint.h"
#include "resource.h"

#include <Ctrl/PropertySheet.h>
#include <Win/Win.h>
#include <Com/Shell.h>
#include <Ctrl/Edit.h>
#include <Ctrl/Button.h>
#include <Ctrl/Static.h>

class Catalog;

namespace Win
{
	class CritSection;
}

namespace Project
{
	class Db;
}

class NewProjectData : public Project::Blueprint
{
public:
	NewProjectData::NewProjectData (Catalog & catalog)
		: Project::Blueprint (catalog)
	{
		Project::Options & options = GetOptions ();
		options.SetBccRecipients (true);
		options.SetCheckProjectName (true);
		options.SetIsAdmin (true);
	}
};

class GeneralPageHndlr : public PropPage::Handler
{
public:
	GeneralPageHndlr (NewProjectData & projectData)
		: PropPage::Handler (IDD_NEW_PROJECT_PAGE),
		  _newProjectData (projectData)
	{}

	void OnKillActive (long & result) throw (Win::Exception);
	void OnCancel (long & result) throw (Win::Exception);
	void OnApply (long & result) throw (Win::Exception);

	bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned id, unsigned notifyCode) throw ();

private:
    Win::Edit			_sourcePath;
    Win::Button			_browseSource;
    Win::Edit			_projectName;
    Win::Edit			_userName;
    Win::Edit			_userHubId;
    Win::Edit			_userPhone;
	NewProjectData &	_newProjectData;
	std::string			_historicalProjectName;
};

class OptionsPageHndlr : public PropPage::Handler
{
public:
	OptionsPageHndlr (Project::Blueprint & projectData)
		: PropPage::Handler (IDD_NEW_PROJECT_OPTIONS_PAGE),
		  _projectData (projectData)
	{}

	void OnKillActive (long & result) throw (Win::Exception);
	void OnCancel (long & result) throw (Win::Exception);
	void OnApply (long & result) throw (Win::Exception);

	bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned id, unsigned notifyCode) throw ();

private:
	Win::CheckBox		_wiki;
	Win::CheckBox		_autoSynch;
	Win::CheckBox		_autoJoin;
	Win::CheckBox		_keepCheckedOut;
	Win::CheckBox		_distributor;
	Win::CheckBox		_noBranching;
	Win::RadioButton	_singleRecipient;
	Win::RadioButton	_allBccRecipients;
	Project::Blueprint &_projectData;
};


class NewProjectHndlrSet : public PropPage::HandlerSet
{
public:
	NewProjectHndlrSet (NewProjectData & projectData);

	bool IsValidData () const { return _newProjectData.IsValid (); }

	bool GetDataFrom (NamedValues const & source);

private:
	NewProjectData &	_newProjectData;
	GeneralPageHndlr	_generalPageHndlr;
	OptionsPageHndlr	_optionsPageHndlr;
};

#endif
