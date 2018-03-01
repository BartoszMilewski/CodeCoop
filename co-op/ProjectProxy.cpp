//------------------------------------
//  (c) Reliable Software, 2007 - 2008
//------------------------------------

#include "precompiled.h"
#include "ProjectProxy.h"
#include "Catalog.h"
#include "ProjectData.h"
#include "Lineage.h"
#include "ProjectChecker.h"

#include <Ctrl/ProgressMeter.h>

void Project::Proxy::Visit (int projectId, Catalog & catalog)
{
	ProjectSeq projectSeq (catalog);
	Assert (!projectSeq.AtEnd ());
	projectSeq.SkipTo (projectId);
	Assert (!projectSeq.AtEnd ());
	Project::Data projectData;
	projectSeq.FillInProjectData (projectData);
	_model.ReadProjectDb (projectData, true);	// blind
}

void Project::Proxy::PreserveLocalEdits (TransactionFileList & fileList)
{
	_model.PreserveLocalEdits (fileList);
}

void Project::Proxy::RequestVerification (UserId recipientId)
{
	_model.RequestRecovery (recipientId, false);	// Always block check-in
}

void Project::Proxy::RestoreProject ()
{
	if (_model.QuickProjectVerify ())
		return;
	
	GidList allProjectFiles;
	PathFinder & pathFinder = _model.GetPathFinder ();
	FileIndex const & fileIndex = _model.GetFileIndex ();
	fileIndex.ListFolderContents (gidRoot, allProjectFiles, true); // Recursive
	VerificationReport report;
	for (GidList::const_iterator iter = allProjectFiles.begin (); iter != allProjectFiles.end (); ++iter)
	{
		GlobalId gid = *iter;
		FileData const * fd = fileIndex.GetFileDataByGid (gid);
		
		if (fd->GetType ().IsFolder ())
		{
			report.Remember (VerificationReport::MissingFolder, gid);
		}
		else
		{
			FileState state = fd->GetState ();
			if (state.IsRelevantIn (Area::Original))
			{
				char const * path = pathFinder.GetFullPath (*iter, Area::LocalEdits);
				if (File::Exists (path))
				{
    				report.Remember (VerificationReport::PreservedLocalEdits, gid);
    				continue;
				}
			}
		
			report.Remember (VerificationReport::Corrupted, gid);
		}
	}
	
	if (!report.IsEmpty ())
		_model.RecoverFiles (report);
}
