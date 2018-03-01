#if !defined (SELECTPROJECTDLG_H)
#define SELECTPROJECTDLG_H
//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------

#include "Resource.h"
#include "ProjectData.h"
#include "HistoryChecker.h"

#include <Win/Dialog.h>
#include <Ctrl/ListView.h>

class Catalog;

class ProjectList
{
public:
	ProjectList (Catalog & catalog, int thisProjectId, HistoryChecker const & historyChecker);
	ProjectList (Catalog & catalog, int thisProjectId, HistoryChecker const & historyChecker, std::string const & name);

	std::vector<Project::Data>::const_iterator begin () { return _projects.begin (); }
	std::vector<Project::Data>::const_iterator end () { return _projects.end (); }

	bool SelectProject (int projectId);
	int GetSelectedProjectId () const { return _selectedProjectId; }
	GlobalId GetForkId () const { return _forkScriptId; }

private:
	Catalog &					_catalog;
	HistoryChecker const &		_historyChecker;
	std::vector<Project::Data>	_projects;
	int							_thisProjectId;
	int							_selectedProjectId;
	GlobalId					_forkScriptId;
};

class SelectProjectCtrl : public Dialog::ControlHandler, public Notify::ListViewHandler
{
public:
	SelectProjectCtrl (ProjectList & data)
		: Dialog::ControlHandler (IDD_SELECT_PROJECT),
		  Notify::ListViewHandler (IDC_PROJECT_LIST),
		  _dlgData (data)
	{}

	bool OnInitDialog () throw (Win::Exception);
	bool OnApply () throw ();

	Notify::Handler * GetNotifyHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ();
	bool OnDblClick () throw ();

private:
	Win::ReportListing	_projectList;
	ProjectList &		_dlgData;
};

#endif
