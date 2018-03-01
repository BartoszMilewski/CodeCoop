//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include "precompiled.h"
#include "BackupSummaryDlg.h"
#include "Catalog.h"
#include "ProjectData.h"
#include "Resource.h"

#include <StringOp.h>

BackupSummary::BackupSummary (Catalog & catalog,
							  std::vector<int> const & unverifiedProjectIds,
							  std::vector<int> const & notRepairedProjectIds)
{
	for (std::vector<int>::const_iterator iter = unverifiedProjectIds.begin ();
		 iter != unverifiedProjectIds.end ();
		 ++iter)
	{
		int projectId = *iter;
		Project::Data projectData;
		catalog.GetProjectData (projectId, projectData);
		ProjectInfo info (projectData.GetProjectName (),
						  projectData.GetProjectIdString (),
						  projectData.GetRootDir ());
		_unverifiedProjects.push_back (info);
	}

	for (std::vector<int>::const_iterator iter = notRepairedProjectIds.begin ();
		 iter != notRepairedProjectIds.end ();
		 ++iter)
	{
		int projectId = *iter;
		Project::Data projectData;
		catalog.GetProjectData (projectId, projectData);
		ProjectInfo info (projectData.GetProjectName (),
						  projectData.GetProjectIdString (),
						  projectData.GetRootDir ());
		_notRepairedProjects.push_back (info);
	}
}

BackupSummaryCtrl::BackupSummaryCtrl (BackupSummary const & summary)
	: Dialog::ControlHandler (IDD_BACKUP_SUMMARY),
	  _summary (summary)
{}

bool BackupSummaryCtrl::OnInitDialog () throw (Win::Exception)
{
	_projectList.Init (GetWindow (), IDC_PROJECT_LIST);

	_projectList.AddProportionalColumn (25, "Project");
	_projectList.AddProportionalColumn (9, "Id", Win::Report::Center);
	_projectList.AddProportionalColumn (50, "Root Path");
	_projectList.AddProportionalColumn (15, "Problem", Win::Report::Center);

	for (BackupSummary::ProjectSequencer seq = _summary.GetUnverifiedProjects ();
		 !seq.AtEnd ();
		 seq.Advance ())
	{
		Win::ListView::Item item;
		item.SetText (seq.GetProjectName ());
		int row = _projectList.AppendItem (item);
		_projectList.AddSubItem (seq.GetProjectId (), row, 1);	// Id column
		_projectList.AddSubItem (seq.GetRootPath (), row, 2);	// Root path column
		_projectList.AddSubItem ("Unverified", row, 3);	// Problem column
	}


	for (BackupSummary::ProjectSequencer seq = _summary.GetUnrepairedProjects ();
		 !seq.AtEnd ();
		 seq.Advance ())
	{
		Win::ListView::Item item;
		item.SetText (seq.GetProjectName ());
		int row = _projectList.AppendItem (item);
		_projectList.AddSubItem (seq.GetProjectId (), row, 1);	// Id column
		_projectList.AddSubItem (seq.GetRootPath (), row, 2);	// Root path column
		_projectList.AddSubItem ("Not repaired", row, 3);	// Problem column
	}

	return true;
}