#if !defined (PROJECTBRANCHDLG_H)
#define PROJECTBRANCHDLG_H
//------------------------------------
//  (c) Reliable Software, 2005 - 2007
//------------------------------------

#include "ProjectBlueprint.h"
#include "NewProj.h"
#include "resource.h"

#include <Ctrl/PropertySheet.h>
#include <Win/Win.h>
#include <Com/Shell.h>
#include <Ctrl/Edit.h>
#include <Ctrl/Button.h>
#include <Ctrl/Static.h>

class Catalog;
class VersionInfo;
class MemberDescription;

namespace Win
{
	class CritSection;
}

namespace Project
{
	class Db;
}

class BranchProjectData : public Project::Blueprint
{
public:
	BranchProjectData::BranchProjectData (VersionInfo const & versionInfo,
										  MemberDescription const & myDescription,
										  Catalog & catalog,
										  bool isReceiver)
		: Project::Blueprint (catalog),
		  _versionInfo (versionInfo),
		  _myDescription (myDescription)
	{
		Project::Options & options = GetOptions ();
		options.SetBccRecipients (true);
		options.SetIsAdmin (true);
		options.SetCheckProjectName (true);
		options.SetIsReceiver (isReceiver);
	}

	GlobalId GetBranchScriptId () const;
	bool IsCurrentVersion () const;
	std::string GetVersionComment () const;
	std::string const & GetUserName () const;
	std::string const & GetUserComment () const;

private:
	VersionInfo const &			_versionInfo;
	MemberDescription const &	_myDescription;
};

class BranchGeneralPageHndlr : public PropPage::Handler
{
public:
	BranchGeneralPageHndlr (BranchProjectData & branchData)
		: PropPage::Handler (IDD_PROJECT_BRANCH_PAGE),
		  _branchData (branchData)
	{}

	void OnCancel (long & result) throw (Win::Exception);
	void OnApply (long & result) throw (Win::Exception);

	bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned id, unsigned notifyCode) throw ();

private:
	Win::StaticText		_versionFrame;
	Win::StaticText		_version;
	Win::Edit			_targetPath;
	Win::Button			_browseSource;
	Win::Edit			_projectName;
	Win::Edit			_userName;
	Win::Edit			_comment;
	BranchProjectData &	_branchData;
};

class ProjectBranchHndlrSet : public PropPage::HandlerSet
{
public:
	ProjectBranchHndlrSet (BranchProjectData & branchData);

	bool IsValidData () const { return _branchData.IsValid (); }

private:
	BranchProjectData &		_branchData;
	BranchGeneralPageHndlr	_generalPageHndlr;
	OptionsPageHndlr			_optionsPageHndlr;
};

#endif
