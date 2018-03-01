//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------

#include "precompiled.h"
#include "SelectProjectDlg.h"
#include "Catalog.h"
#include "OutputSink.h"

ProjectList::ProjectList (Catalog & catalog,
						  int thisProjectId,
						  HistoryChecker const & historyChecker)
	: _catalog (catalog),
	  _historyChecker (historyChecker),
	  _thisProjectId (thisProjectId),
	  _selectedProjectId (-1),
	  _forkScriptId (gidInvalid)
{
	for (ProjectSeq seq (_catalog); !seq.AtEnd (); seq.Advance ())
	{
		if (seq.GetProjectId () == _thisProjectId)
			continue;

		Project::Data data;
		seq.FillInProjectData (data);
		_projects.push_back (data);
	}
}

ProjectList::ProjectList (Catalog & catalog,
						  int thisProjectId,
						  HistoryChecker const & historyChecker,
						  std::string const & name)
	: _catalog (catalog),
	  _historyChecker (historyChecker),
	  _thisProjectId (thisProjectId),
	  _selectedProjectId (-1),
	  _forkScriptId (gidInvalid)
{
	for (ProjectSeq seq (_catalog); !seq.AtEnd (); seq.Advance ())
	{
		if (seq.GetProjectId () == _thisProjectId)
			continue;

		std::string catalogName = seq.GetProjectName ();
		if (IsNocaseEqual (catalogName, name))
		{
			Project::Data data;
			seq.FillInProjectData (data);
			_projects.push_back (data);
		}
	}
}

bool ProjectList::SelectProject (int projectId)
{
	_forkScriptId = _historyChecker.IsRelatedProject (projectId);
	if (_forkScriptId == gidInvalid)
	{
		_historyChecker.DisplayError (_catalog, _thisProjectId, projectId);
		return false;
	}

	_selectedProjectId = projectId;
	return true;
}

class ListViewUpdate
{
public:
	ListViewUpdate (Win::ReportListing & listing)
		: _listing (listing)
	{}

	void operator() (Project::Data const & project)
	{
		Win::ListView::Item item;
		item.SetText (project.GetProjectName ().c_str ());
		item.SetParam (project.GetProjectId ());
		int row = _listing.AppendItem (item);
		_listing.AddSubItem (project.GetRootPath ().GetDir (), row, colRootPath);
	}

private:
    enum
    {
        colName,
        colRootPath
    };

	Win::ReportListing &	_listing;
};

bool SelectProjectCtrl::OnInitDialog () throw (Win::Exception)
{
	_projectList.Init (GetWindow (), IDC_PROJECT_LIST);
    _projectList.AddProportionalColumn (28, "Name");
    _projectList.AddProportionalColumn (67, "Root Path");

	ListViewUpdate updateListView (_projectList);
    // Display project list
	std::for_each (_dlgData.begin (), _dlgData.end (), updateListView);
	return true;
}

bool SelectProjectCtrl::OnApply () throw ()
{
	int itemCount = _projectList.GetCount ();
	for (int item = 0; item < itemCount; ++item)
	{
		if (_projectList.IsSelected (item))
		{
			int projectId = _projectList.GetItemParam (item);
			if (!_dlgData.SelectProject (projectId))
				return false;

			break;
		}
	}
	if (_dlgData.GetSelectedProjectId () == -1)
	{
		TheOutput.Display ("Please, select a project.");
		return false;
	}
	EndOk ();
	return true;
}

// Windows WM_NOTIFY handlers

Notify::Handler * SelectProjectCtrl::GetNotifyHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ()
{
	if (Notify::ListViewHandler::IsHandlerFor (idFrom))
		return this;
	else
		return 0;
}

bool SelectProjectCtrl::OnDblClick () throw ()
{
	OnApply ();
	return true;
}

