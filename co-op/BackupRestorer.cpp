//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include "precompiled.h"
#include "BackupRestorer.h"
#include "Catalog.h"
#include "ProjectMarker.h"
#include "Prompter.h"
#include "OutputSink.h"
#include "SelectRootPathDlg.h"
#include "ProjectRecoveryDlg.h"
#include "BackupSummaryDlg.h"
#include "ProjectProxy.h"
#include "ProjectData.h"
#include "ProjectPathClassifier.h"

#include <Ctrl/ProgressMeter.h>
#include <File/File.h>

BackupRestorer::BackupRestorer (Model & model)
	: _model (model),
	  _projectCount (0)
{
	for (ProjectSeq seq (_model.GetCatalog ()); !seq.AtEnd (); seq.Advance ())
		_projectCount++;
}

void BackupRestorer::RestoreRootPaths (Progress::Meter & meter)
{
	Project::RootPathVerifier pathVerifier (_model.GetCatalog ());
	while (!pathVerifier.Verify (meter))
	{
		pathVerifier.PromptForNew ();
	}
}

void BackupRestorer::RequestVerification (Progress::Meter & meter)
{
	meter.SetActivity ("Requesting project verification.");
	meter.SetRange (0, _projectCount, 1);
	bool alwaysSendRequestToAdmin = false;
	for (ProjectSeq seq (_model.GetCatalog ()); !seq.AtEnd (); seq.Advance ())
	{
		bool success = true;
		int projectId = seq.GetProjectId ();
		RecoveryMarker recoveryMarker (_model.GetCatalog (), projectId);
		if (recoveryMarker.Exists ())
			continue;	// Verification already requested

		SetCurrentActivity (projectId, meter);
		try
		{
			Project::Proxy projectProxy;
			projectProxy.Visit (projectId, _model.GetCatalog ());
			std::string caption ("In order to restore project '");
			caption += projectProxy.GetProjectName ();
			caption += "' (";
			caption += projectProxy.GetRootDir ();
			caption += ") from backup\n"
					   "Code Co-op needs to request verification from another project member.\n"
					   "Select this member from list.";
			Project::Db const & projectDb = projectProxy.GetProjectDb ();
			ProjectRecoveryData recoveryData (projectDb,
											  caption,
											  true);	// Restore from backup

			if (recoveryData.AmITheOnlyVotingMember ())
				continue;

			bool askUser = !alwaysSendRequestToAdmin;
			if (alwaysSendRequestToAdmin)
			{
				recoveryData.SetAlwaysSendToAdmin(true);
				if (projectDb.GetAdminId () == gidInvalid)
					askUser = true;	// Project doesn't have administrator - ask the user
				else if (projectDb.GetAdminId () == projectDb.GetMyId ())
					askUser = true;	// I'm project administrator - ask the user
			}

			if (askUser)
			{
				ProjectRecoveryRecipientsCtrl recoveryRecipientsCtrl (recoveryData);
				if (ThePrompter.GetData (recoveryRecipientsCtrl))
				{
					alwaysSendRequestToAdmin = recoveryData.IsAlwaysSendToAdmin ();
					UserId recipientId = recoveryData.GetSelectedRecipientId ();
					projectProxy.RequestVerification (recipientId);
				}
				else
				{
					_unverifiedProjectIds.push_back (projectId);
				}
			}
			else
			{
				Assert (alwaysSendRequestToAdmin);
				UserId adminId = projectDb.GetAdminId ();
				Assert (adminId != gidInvalid);
				projectProxy.RequestVerification (adminId);
			}
		}
		catch ( ... )
		{
			success = false;
		}
		if (!success)
			_unverifiedProjectIds.push_back (projectId);
		meter.StepAndCheck ();
	}
	// Create project repair list in the database folder
	RepairList repairList (_model.GetCatalog ());
	repairList.Save ();
	// Remove backup marker
	BackupMarker backupMarker;
	backupMarker.SetMarker (false);
}

void BackupRestorer::RepairProjects (Progress::Meter & meter)
{
	RepairList repairList;
	meter.SetActivity ("Repairing project files.");
	meter.SetRange (0, 2 * repairList.ProjectCount (), 1);
	std::vector<int> projectIds = repairList.GetProjectIds ();
	for (std::vector<int>::const_iterator iter = projectIds.begin (); iter != projectIds.end (); ++iter)
	{
		bool success = true;
		int projectId = *iter;
		// Project available for repair
		SetCurrentActivity (projectId, meter);
		meter.StepAndCheck ();
		try
		{
			Project::Proxy projectProxy;
			projectProxy.Visit (projectId, _model.GetCatalog ());
			meter.StepAndCheck ();
			projectProxy.RestoreProject ();
		}
		catch ( ... )
		{
			success = false;
		}

		// Always remove project from backup marker
		repairList.Remove (projectId);
		if (!success)
			_notRepairedProjectIds.push_back (projectId);
	}
}

void BackupRestorer::Summarize ()
{
	if (_unverifiedProjectIds.empty () && _notRepairedProjectIds.empty ())
	{
		TheOutput.Display ("Code Co-op projects have been successfuly restored from the backup archive.");
		return;
	}

	BackupSummary summary (_model.GetCatalog (), _unverifiedProjectIds, _notRepairedProjectIds);
	BackupSummaryCtrl dlgCtrl (summary);
	ThePrompter.GetData (dlgCtrl);
}

void BackupRestorer::SetCurrentActivity (int projectId, Progress::Meter & meter)
{
	ProjectSeq seq (_model.GetCatalog ());
	seq.SkipTo (projectId);
	Assert (!seq.AtEnd ());
	Project::Data projectData;
	seq.FillInProjectData (projectData);
	std::string activity (projectData.GetProjectName ());
	activity += " (ID: ";
	activity += ::ToString (projectId);
	activity += ")";
	meter.SetActivity (activity.c_str ());
}
