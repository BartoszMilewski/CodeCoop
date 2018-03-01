//----------------------------------
// (c) Reliable Software 1997 - 2009
//----------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "Model.h"
#include "Catalog.h"
#include "Registry.h"
#include "NewProj.h"
#include "ProjectBranchDlg.h"
#include "JoinProj.h"
#include "JoinProjectData.h"
#include "ProjectInviteData.h"
#include "ProjDefect.h"
#include "MemberInfo.h"
#include "ProjectMembers.h"
#include "FileList.h"
#include "FileTrans.h"
#include "License.h"
#include "OutputSink.h"
#include "AppInfo.h"
#include "MembershipChange.h"
#include "DispatcherScript.h"
#include "TransportHeader.h"
#include "ScriptHeader.h"
#include "TmpProjectArea.h"
#include "FileDataSequencer.h"
#include "PathSequencer.h"
#include "FeedbackMan.h"
#include "DispatcherProxy.h"
#include "PhysicalFile.h"
#include "SynchTrans.h"
#include "Transformer.h"
#include "VerificationReport.h"
#include "FolderEvent.h"
#include "ExportedHistoryTrailer.h"
#include "Messengers.h"
#include "CoopMemento.h"
#include "UserId.h"
#include "UserIdPack.h"
#include "ProjectOptionsEx.h"
#include "AdminElection.h"
#include "Workspace.h"
#include "Conflict.h"
#include "ProjectMarker.h"
#include "LicensePrompter.h"
#include "AckBox.h"
#include "ProjectBlueprint.h"
#include "JoinContext.h"
#include "ProjectChecker.h"

#include <Ctrl/ProgressMeter.h>
#include <Com/Shell.h>
#include <Com/ShellRequest.h>
#include <Ctrl/StatusBar.h>
#include <Ctrl/ProgressBar.h>
#include <File/Drives.h>
#include <File/SafePaths.h>

#include <TimeStamp.h>
#include <StringOp.h>

#include <iomanip>

void Model::InitPaths ()
{
	_dataBase.InitPaths (_pathFinder);
	_history.InitPaths (_pathFinder);
	_mailBox.InitPaths (_pathFinder);
}

void Model::SetCheckedOutMarker ()
{
	Assert (GetProjectId () != -1);

	CheckedOutFiles checkedOutFiles (_catalog, GetProjectId ());
	bool isMarkerChange = checkedOutFiles.Exists () &&  _checkInArea.IsEmpty () ||
						 !checkedOutFiles.Exists () && !_checkInArea.IsEmpty ();
	if (isMarkerChange)
	{
		checkedOutFiles.SetMarker (!_checkInArea.IsEmpty ());
		NotifyOtherCoops ();
	}
}

void Model::SetIncomingScriptsMarker (bool newMissing)
{
	dbg << "--> Model::SetIncomingScriptsMarker" << std::endl;
	dbg << "    New missing scripts: " << (newMissing ? "YES" : "NO") << std::endl;
	Assert (GetProjectId () != -1);

	IncomingScripts incomingScripts (_catalog, GetProjectId ());
	bool incomingScriptsPresent = _history.HasIncomingOrMissingScripts () ||
								  _mailBox.HasExecutableJoinRequest () ||
								  _mailBox.HasScriptsFromFuture () ||
								  _sidetrack.HasChunks () ||
								  !IsSynchAreaEmpty ();
	bool incomingScriptsMarkerChange = incomingScripts.Exists () && !incomingScriptsPresent ||
									  !incomingScripts.Exists () &&  incomingScriptsPresent;
	if (incomingScriptsMarkerChange)
	{
		dbg << "    Incoming script marker change -- incoming script present: " << (incomingScriptsPresent ? "YES" : "NO") << std::endl;
		incomingScripts.SetMarker (incomingScriptsPresent);
		
		NotifyOtherCoops ();
	}

	dbg << "    Has missing scripts: " << (_history.HasMissingScripts () ? "YES" : "NO") << std::endl;
	MissingScripts missingScripts (_catalog, GetProjectId ());
	missingScripts.SetMarker (_history.HasMissingScripts ());

	if (newMissing || incomingScriptsMarkerChange)
	{
		dbg << "    Notify Dispatcher" << std::endl;
		// Notify Dispatcher when new missing script detected or
		// when incoming scripts marker change
		StateChangeNotify stateChange (GetProjectId (), newMissing);
		Win::EnumProcess (DispatcherClassName, stateChange);
	}

	if (IsProjectReady ())
	{
		AwaitingFullSync fullSync (_catalog, GetProjectId ());
		if (fullSync.Exists ())
		{
			fullSync.SetMarker (false);
			NotifyOtherCoops ();
		}
	}

	dbg << "<-- Model::SetIncomingScriptsMarker" << std::endl;
}

void Model::NotifyOtherCoops (int localProjectId) const
{
	// Notify other Co-ops about this project state change
	if (localProjectId == -1)
		localProjectId = GetProjectId ();
	Assert (localProjectId != -1);
	StateChangeNotify stateChange (localProjectId);
	Win::EnumProcess (CoopClassName, stateChange);
}

void Model::BroadcastProjectChange (int localProjectId) const
{
	DispatcherProxy dispatcher;
	dispatcher.ProjectChange ();
	NotifyOtherCoops (localProjectId);
}

void Model::OnProjectStateChange ()
{
	_project.ExternalNotify ();
}

bool Model::TestConsistency (std::string const & sourcePath, Project::Data & projData)
{
	if (!_catalog.GetProjectData (sourcePath, projData))
	{
		// Project could not be found in the catalog.
		// Search by source path failed.
		return false;
	}
	return TestConsistency (projData);
}

bool Model::TestConsistency (Project::Data & projData)
{
	Assert (projData.IsValid ());
#if 0
	// Enable this code for those one-shot repair builds for a specific project
	if (!IsNocaseEqual (projData.GetProjectName (), "Go!Sync (Core)"))
		return false;
#endif
	char const * sourceFolder = projData.GetRootDir ();
	if (!File::Exists (sourceFolder))
	{
		// Project source tree doesn't exist.
		bool rootFolderCreated = true;
		try
		{
			_pathFinder.MaterializeFolderPath (sourceFolder, true);	// Quiet - don't ask the user
			std::string info ("Source tree of the project \"");
			info += projData.GetProjectName ();
			info += "\" was not found.\nCode Co-op has recreated the project's root folder:\n\n";
			info += sourceFolder;
			info += "\n\nRun Project Repair to recover project files.";
			TheOutput.Display (info.c_str ());
		}
		catch ( ... )
		{
			rootFolderCreated = false;
		}
		
		if (!rootFolderCreated)
		{
			// Project root folder cannot be created.
			FullPathSeq pathSeq (sourceFolder);
			bool inaccessiblePath = false;
			if (pathSeq.HasDrive ())
			{
				DriveInfo driveInfo (pathSeq.GetHead ());
				inaccessiblePath = driveInfo.IsRemote () ||
								   driveInfo.IsRemovable () ||
								   driveInfo.HasNoRoot () ||
								   driveInfo.IsCdRom ();
			}
			else if (pathSeq.IsUNC ())
			{
				inaccessiblePath = true;
			}

			if (inaccessiblePath)
			{
				_catalog.MarkProjectUnavailable (projData.GetProjectId ());
				return false;
			}
			else
			{
				throw Win::Exception ("The project root folder does not exist and cannot be created.", sourceFolder);
			}
		}
	}

	Assert (!_pathFinder.ProjectDir ().IsEqualDir (sourceFolder));

	// Try locking project
	PathFinder tmpPathFinder (_dataBase);
	tmpPathFinder.SetProjectDir (projData);
	if (tmpPathFinder.IsProjectLocked ())
	{
		std::string info ("Project ");
		info += projData.GetProjectName ();
		info += " (";
		info += sourceFolder;
		info += ")\nis currently in use and cannot be visited.";
		TheOutput.Display (info.c_str ());
		return false;
	}

	_userPermissions.TestProjectLockConsistency (tmpPathFinder);

	if (_catalog.IsProjectUnavailable (projData.GetProjectId ()))
		_catalog.MarkProjectAvailable (projData.GetProjectId ());
	return true;
}

// Notice: it is not a multithreaded lock.
// it only prevents same-thread recursion.
class RecursionLock
{
public:
	RecursionLock (int & counter)
		: _counter (counter)
	{
		_counter++;
	}
	~RecursionLock ()
	{
		_counter--;
	};
	bool IsBusy ()
	{
		return _counter > 1;
	}
private:
	int & _counter;
};

// Returns true if this project inbox folder change notification
bool Model::FolderChange (FilePath const & folder)
{
	RecursionLock lock (_changeCounter);
	if (lock.IsBusy ())
		throw Win::Exception (); // will retry later

	if (folder.IsEqualDir (_pathFinder.InBoxDir ()))
		return true;

	_directory.FolderChange (folder);
	_wikiDirectory.FolderChange (folder);
	return false;
}

void Model::XInitializeDatabaseAndHistory (std::string const & projectName,
										   MemberInfo const & userInfo,
										   Project::Options const & options,
										   bool isProjectCreated)
{
	// Set project name and create project root folder file data
	_dataBase.XSetProjectName (projectName);
	_dataBase.XCreateProjectRoot ();
	if (isProjectCreated)
		_dataBase.XAddMember (userInfo);	// Sets _myId

	// Set project options
	Project::Db & projectDb = _dataBase.XGetProjectDb ();
	projectDb.XInitProjectProperties (options);

	// Store in the history project creation marker
	StrTime timeStamp (CurrentTime ());
	std::string comment ("Project '");
	comment += projectName;
	if (isProjectCreated)
		comment += "' created ";
	else
		comment += "' joined ";
	comment += timeStamp.GetString ();
	ScriptHeader markerHdr (ScriptKindSetChange (), gidInvalid, projectName);
	markerHdr.SetScriptId (_dataBase.XMakeScriptId ());
	markerHdr.AddComment (comment);
	CommandList emptyCmdList;
	Assert ( isProjectCreated && !GlobalIdPack (markerHdr.ScriptId ()).IsFromJoiningUser () ||
			!isProjectCreated &&  GlobalIdPack (markerHdr.ScriptId ()).IsFromJoiningUser ());
	_history.XAddProjectCreationMarker (markerHdr, emptyCmdList);
	if (isProjectCreated)
	{
		AckBox fakeAckBox;
		// Store in the history empty initial file inventory script
		ScriptHeader inventoryHdr (ScriptKindSetChange (), gidInvalid, projectName);
		inventoryHdr.SetScriptId (_dataBase.XMakeScriptId ());
		inventoryHdr.AddComment ("File(s) added during project creation");
		_history.XGetLineages (inventoryHdr, UnitLineage::Empty);	// Don't add side lineages
		_history.XAddCheckinScript (inventoryHdr, emptyCmdList, fakeAckBox, true);	// Initial file inventory
		// Store membership update announcing project creator
		ScriptHeader membershipUpdateHdr (ScriptKindAddMember (), userInfo.Id (), projectName);
		membershipUpdateHdr.SetScriptId (_dataBase.XMakeScriptId ());
		membershipUpdateHdr.AddComment ("Project creator");
		CommandList cmdList;
		std::unique_ptr<ScriptCmd> cmd (new NewMemberCmd (userInfo));
		cmdList.push_back (std::move(cmd));
		_history.XAddCheckinScript (membershipUpdateHdr, cmdList, fakeAckBox);
	}
}

void Model::NewProject (NewProjectData & projectData)
{
	// Update project creator description
	MemberDescription & creatorDescription = projectData.GetThisUser ();
	MemberId creatorUserIdString (UserId (0));
	creatorDescription.SetLicense (_userPermissions.GetNewProjectLicense ());
	creatorDescription.SetUserId (creatorUserIdString.Get ());

	Project::Options const & options = projectData.GetOptions ();
	StateAdmin admin;
	if (options.IsDistribution ())
	{
		admin.SetDistributor (true);
		admin.SetNoBranching (options.IsNoBranching ());
	}
	MemberInfo creatorInfo (UserId (0), admin, creatorDescription);

	Project::Data & project = projectData.GetProject ();
	Model remoteModel;
	remoteModel.CreateEmptyProject (project, creatorDescription);

	{
		// Transaction scope
		Transaction xact (remoteModel, remoteModel.GetPathFinder ());

		remoteModel.XInitializeDatabaseAndHistory (project.GetProjectName (),
												   creatorInfo,
												   options,
												   true);	// Creating new project

		xact.Commit ();
	}

	remoteModel.NotifyOtherCoops (project.GetProjectId ());
	remoteModel.NotifyDispatcher ();
}

void Model::NewProjectFromHistory (NewProjectData & projectData,
								   ExportedHistoryTrailer const & trailer,
								   std::string const & historyFilePath,
								   Progress::Meter & progressMeter)
{
	progressMeter.SetRange (0, trailer.GetScriptCount () + 17);
	progressMeter.SetActivity ("Creating empty project.");
	progressMeter.StepAndCheck ();

	UserId historyHighestUserId = trailer.GetHighestUserId ();
	UserId creatorUserId = historyHighestUserId + 1;

	// Update project creator description
	MemberDescription & creatorDescription = projectData.GetThisUser ();
	MemberId creatorUserIdString (creatorUserId);
	creatorDescription.SetLicense (_userPermissions.GetNewProjectLicense ());
	creatorDescription.SetUserId (creatorUserIdString.Get ());

	Project::Options const & options = projectData.GetOptions ();
	Assert (!options.IsDistribution ());
	StateAdmin admin;
	MemberInfo creatorInfo (creatorUserId, admin, creatorDescription);

	Project::Data & project = projectData.GetProject ();
	Model remoteModel;
	remoteModel.SetUIFeedback (_uiFeedback);
	remoteModel.CreateEmptyProject (project, creatorDescription);
	progressMeter.StepAndCheck ();

	{
		// History import transaction scope
		Transaction xact (remoteModel, remoteModel.GetPathFinder ());

		remoteModel.XInitializeDatabaseAndHistory (project.GetProjectName (),
												   creatorInfo,
												   options,
												   true);	// Creating new project
		progressMeter.StepAndCheck ();
		remoteModel.XDoImportHistory (trailer,
									  historyFilePath,
									  progressMeter,
									  true);	// New project from history

		xact.Commit ();
	}

	remoteModel.SetProjectId (project.GetProjectId ());
	remoteModel.NotifyOtherCoops (project.GetProjectId ());
	remoteModel.NotifyDispatcher ();

	// Recover project files
	progressMeter.SetActivity ("Creating project files.");
	GidList allProjectFiles;
	FileIndex const & fileIndex = remoteModel.GetFileIndex ();
	fileIndex.ListFolderContents (gidRoot, allProjectFiles, true);	// Recursive
	VerificationReport report;
	for (GidList::const_iterator iter = allProjectFiles.begin (); iter != allProjectFiles.end (); ++iter)
	{
		GlobalId gid = *iter;
		FileData const * fd = fileIndex.GetFileDataByGid (gid);
		FileState state = fd->GetState ();
		if (state.IsCheckedIn ())
		{
			if (fd->GetType ().IsFolder ())
				report.Remember (VerificationReport::MissingFolder, gid);
			else
				report.Remember (VerificationReport::Corrupted, gid);
		}
	}

	progressMeter.StepAndCheck ();
	remoteModel.RecoverFiles (report);
	progressMeter.StepAndCheck ();
}

void Model::MakeFileRequest (ShellMan::FileRequest & request)
{
	for (AllProjectFilesSequencer sequencer (_dataBase); !sequencer.AtEnd (); sequencer.Advance ())
	{
		FileData const * fileData = sequencer.GetFileData ();
		GlobalId gid = fileData->GetGlobalId ();
		char const * from = _pathFinder.GetFullPath (gid, Area::Project);
		request.AddFile (from);
	}
}

void Model::MakeHistoricalFileDeleteRequest (GidList const & files,
											 bool isMerge,
											 bool afterScriptChange,
											 bool useProjectRelativePath,
											 FilePath & targetRoot,
											 ShellMan::FileRequest & request)
{
	for (GidList::const_iterator iter = files.begin (); iter != files.end (); ++iter)
	{
		GlobalId gid = *iter;
		Restorer & restorer = isMerge ? _mergedFiles.GetRestorer (gid) :
										_historicalFiles.GetRestorer (gid);
		if (restorer.IsFolder ())
			continue;

		// Determine effective target name
		UniqueName effectiveTargetName;
		if (afterScriptChange)
		{
			if (restorer.DeletesItem ())
				effectiveTargetName = restorer.GetBeforeUniqueName ();
			else
				continue;
		}
		else
		{
			if (restorer.IsCreatedByBranch ())
				effectiveTargetName = restorer.GetAfterUniqueName ();
			else
				continue;
		}

		// Create relative target path
		Assert (effectiveTargetName.IsValid ());
		std::string tgtRelPath;
		if (useProjectRelativePath)
		{
			tgtRelPath = _pathFinder.GetRootRelativePath (effectiveTargetName);
		}
		else if (effectiveTargetName.IsNormalized ())
		{
			tgtRelPath = effectiveTargetName.GetName ();
		}
		else
		{
			PathSplitter splitter (effectiveTargetName.GetName ());
			tgtRelPath = splitter.GetFileName ();
			tgtRelPath += splitter.GetExtension ();
		}

		request.AddFile (targetRoot.GetFilePath (tgtRelPath));
	}
}

void Model::MakeTreeList (ShellMan::CopyRequest & request,
						  FileDataSequencer & sequencer,
						  FilePath const & target)
{
	for ( ; !sequencer.AtEnd (); sequencer.Advance ())
	{
		FileData const * fileData = sequencer.GetFileData ();
		GlobalId gid = fileData->GetGlobalId ();
		char const * from = _pathFinder.GetFullPath (gid, Area::Project);
		char const * projRelativePath = sequencer.GetProjectRelativePath ();
		char const * to = target.GetFilePath (projRelativePath);
		request.AddCopyRequest (from, to);
	}
}

void Model::XMakeTreeList (ShellMan::CopyRequest & request,
						   FileDataSequencer & sequencer,
						   TmpProjectArea & tmpArea,
						   FilePath const & target)
{
	for ( ; !sequencer.AtEnd (); sequencer.Advance ())
	{
		FileData const * fileData = sequencer.GetFileData ();
		GlobalId gid = fileData->GetGlobalId ();
		Area::Location areaFrom = tmpArea.IsReconstructed (gid) ? tmpArea.GetAreaId () : Area::Project;
		if (areaFrom == Area::Project)
		{
			FileState state = fileData->GetState ();
			if (!state.IsPresentIn (Area::Project))
			{
				Assert (state.IsRelevantIn (Area::Original));
				continue;
			}
		}
		char const * from = _pathFinder.XGetFullPath (gid, areaFrom);
		char const * projRelativePath = sequencer.GetProjectRelativePath ();
		char const * to = target.GetFilePath (projRelativePath);
		request.AddCopyRequest (from, to);
	}
}

// revisit: create path list of folders to be cleaned up
// check all places where File::CleanupTree is called
void Model::CleanupFolders (FilePath const & target, 
							FileDataSequencer & folderSeq,
							NewProjTransaction & projX) const
{
	// After aborted move, remove all empty project folders 
	// created in the target during copying
	for (; !folderSeq.AtEnd (); folderSeq.Advance ())
	{
		char const * projRelativePath = folderSeq.GetProjectRelativePath ();
		char const * targetPath = target.GetFilePath (projRelativePath);
		if (File::Exists (targetPath))
		{
			// add these directories to transaction
			projX.AddFolderPath (targetPath);
		}
	}
}

void Model::CopyProject (FilePath const & target, NewProjTransaction & projX)
{
	{
		TmpProjectArea emptyTmpArea;
		Transaction xact (*this, _pathFinder);
		// flags: make destination read-write: false
		// overwrite existing: true
		XCopyProject (target, emptyTmpArea, false, true, projX);
	}
	// Make all copied checked out files/folders read-write
	GidList checkedOut;
	_dataBase.ListCheckedOutFiles (checkedOut);
	for (FilteredSequencer seq (checkedOut, _dataBase); !seq.AtEnd (); seq.Advance ())
	{
		char const * projRelativePath = seq.GetProjectRelativePath ();
		char const * targetPath = target.GetFilePath (projRelativePath);
		File::MakeReadWrite (targetPath);
	}
}

// Returns true if copy operation succeded
void Model::XCopyProject (FilePath const & target,
						  TmpProjectArea & tmpArea,
						  bool makeDestinationReadWrite,
						  bool overwriteExisting,
						  NewProjTransaction & projX)
{
	// Make copy request -- make it quiet when in command line mode
	ShellMan::CopyRequest request (IsQuickVisit () && overwriteExisting);	// Completely silent operation
	if (overwriteExisting)
		request.OverwriteExisting ();
	XAllProjectFilesSequencer all (_dataBase);
	XMakeTreeList (request, all, tmpArea, target);
	XDoCopyProject (target, request, projX);
	// Check if all folders made to the target location.
	// Empty folders will not be created during file copy,
	// so we have to created them explicitly
	for (XAllProjectFoldersSequencer folderSeq (_dataBase); !folderSeq.AtEnd (); folderSeq.Advance ())
	{
		char const * projRelativePath = folderSeq.GetProjectRelativePath ();
		char const * targetPath = target.GetFilePath (projRelativePath);
		File::CreateFolder (targetPath);
	}

	if (makeDestinationReadWrite)
		request.MakeDestinationReadWrite ();
	else
		request.MakeDestinationReadOnly ();
}

void Model::XDoCopyProject (FilePath const & target,
						   ShellMan::CopyRequest & request,
						   NewProjTransaction & projX) const
{
	char const * targetFolder = target.GetDir ();
	try
	{
		// Copy project files to the target location
		std::string title ("Copying project files to '");
		title += targetFolder;
		title += '\'';
		request.DoCopy (TheAppInfo.GetWindow (), title.c_str ());
		// TESTING FAILURE: throw Win::Exception ();
	}
	catch (Win::Exception ex)
	{
		if (ex.GetMessage () == 0)
		{
			// User aborted copy operation
			projX.AddAbortRequest (request);
			XAllProjectFoldersSequencer folderSeq (_dataBase);
			CleanupFolders (targetFolder, folderSeq, projX);

			throw Win::Exception ();
		}
		else
		{
			throw;
		}
	}
	catch ( ... )
	{
		Win::ClearError ();
		throw Win::Exception ("Unknown exception during project copying.");
	}
}

class BranchTransactable : public Transactable
{
public:
	BranchTransactable (Model & model)
		: _model (model)
	{}

	void BeginTransaction ()  {}
	void CommitTransaction () throw () {}
	void AbortTransaction ()  {}
	void Clear () throw () {}
	void Serialize (Serializer& out) const { _model.Serialize (out); }
	void Deserialize (Deserializer& in, int version) {}
	bool IsSection () const { return _model.IsSection (); }
	int  SectionId () const { return _model.SectionId (); }
	int  VersionNo () const { return _model.VersionNo (); }

private:
	Model & _model;
};

void Model::NewBranch (BranchProjectData & projectData,
					   Progress::Meter & progressMeter,
					   NewProjTransaction & projX)
{
	Assert (!IsQuickVisit ());
	std::unique_ptr<MemberDescription> branchCreator 
		= _dataBase.RetrieveMemberDescription (_dataBase.GetMyId ());
	MemberDescription const & user = projectData.GetThisUser ();
	branchCreator->SetName (user.GetName ());
	branchCreator->SetHubId (user.GetHubId ());
	branchCreator->SetComment (user.GetComment ());
	if (_userPermissions.IsReceiver ())
		branchCreator->SetLicense (_userPermissions.GetBranchLicense ());
	// else use current branch creator license

	// Register branch project in the catalog
	Project::Data & project = projectData.GetProject ();
	UpdateCatalog (project, *branchCreator);

	// Create project infrastructure at branch location
	PathFinder tmpPathFinder (_dataBase);
	BuildProjectInfrastructure (project, tmpPathFinder);
	TmpProjectArea tmpArea;
	History::Range undoRange;
	CreateUndoRange (projectData.GetBranchScriptId (), undoRange);

	//--------------------------------------------------------------------------
	Transaction xact (*this, _pathFinder);

	// Convert project file database to the state required by the project branch
	XPrepareFileDb (undoRange, tmpArea, progressMeter);
	progressMeter.Close();
	Project::Db & projectDb = _dataBase.XGetProjectDb ();
	// Prepare project database
	Project::Options const & options = projectData.GetOptions ();
	projectDb.XPrepareForBranch (project.GetProjectName (), options);
	// Remove from history all versions stored after the selected one
	_history.XPrepareForBranch (*branchCreator, project.GetProjectName (), projectData.GetBranchScriptId (), options);
	// Remove checkout notifications from the project database
	_checkedOutFiles.XPrepareForBranch ();

	// Copy project files to the branch location
	// Flags: make destination read-write: false
	// overwrite existing true
	XCopyProject (project.GetRootPath (), tmpArea, false, true, projX);

	// Perform fake transaction at branch location that will save converted database
	BranchTransactable branchModel (*this);
	Transaction fakeXact (branchModel, tmpPathFinder);
	fakeXact.Commit ();

	// Copy user and script logs to the branch project database folder
	FilePath const & dataPath = project.GetDataPath ();
	_dataBase.CopyLog (dataPath);
	_history.CopyLog (dataPath);

	//--------------------------------------------------------------------------
	// DO NOT COMMIT! Transactions are to be aborted and project state restored
	tmpArea.Cleanup (_pathFinder);

	NotifyOtherCoops ();
	NotifyDispatcher ();
}

void Model::JoinProject (JoinProjectData & joinData)
{
	dbg << "--> Model::JoinProject " << (joinData.IsInvitation () ? "INVITATION" : "JOIN REQUEST") << std::endl;
	MemberDescription & joinee = joinData.GetThisUser ();
	joinee.SetLicense (_userPermissions.GetLicenseString ());
	if (joinData.IsInvitation ())
	{
		Assert (!joinee.GetUserId ().empty ());
	}
	else
	{
		Assert (joinee.GetUserId ().empty ());
		MemberId randomUserId;
		joinee.SetUserId (randomUserId.Get ());
	}

	StateVotingMember state;
	if (joinData.IsObserver () || !_userPermissions.CanCreateNewProject ())
	{
		state.MakeObserver ();
	}
	UserIdPack idPack (joinee.GetUserId ());
	MemberInfo joineeInfo (idPack.GetUserId (), state, joinee);
	Assert (joineeInfo.GetMostRecentScript () == gidInvalid && joineeInfo.GetPreHistoricScript () == gidInvalid);
	Project::Data & project = joinData.GetProject ();

	{
		Model remoteModel;
		remoteModel.CreateEmptyProject (project, joinee);

		Project::Options const & options = joinData.GetOptions ();
		Transaction xact (remoteModel, remoteModel.GetPathFinder ());
		remoteModel.XInitializeDatabaseAndHistory (project.GetProjectName (),
												   joineeInfo,
												   options,
												   false);	// Joining existing project
		xact.Commit ();
	}

	if (!joinData.HasFullSyncScript () && !joinData.IsInvitation ())
	{
		// We don't have the full sync script - request it by sending the join request script
		std::string comment ("User ");
		comment += joinee.GetName ();
		comment += " requests to join project ";
		comment += project.GetProjectName ();
		ScriptHeader hdr (ScriptKindJoinRequest (),
						  gidInvalid,
						  project.GetProjectName ());
		hdr.SetScriptId (_dataBase.GetRandomId ());
		hdr.AddComment (comment);
		CommandList cmdList;
		std::unique_ptr<ScriptCmd> cmd (new JoinRequestCmd (joineeInfo));
		cmdList.push_back (std::move(cmd));

		// Create project administrator description
		MemberDescription admin (joinData.GetAdminHubId (),
								joinData.GetAdminHubId (),
								std::string (),
								std::string (),
								"*"); // user id label

		DispatcherScript dispatcherScript;
		// Add the intra-cluster transport leading to this machine
		Transport myPublicInboxTransport = _catalog.GetActiveIntraClusterTransportToMe ();
		if (!myPublicInboxTransport.IsUnknown ())
		{
			std::unique_ptr<DispatcherCmd> addMember (
				new AddMemberCmd (joinee.GetHubId (),
								joinee.GetUserId (),
								myPublicInboxTransport));
			dispatcherScript.AddCmd (std::move(addMember));
		}

		// Add the inter-cluster transport leading to our hub
		std::string myHubId = _catalog.GetHubId ();
		Transport myInterClusterTransportToMe = _catalog.GetHubRemoteActiveTransport ();
		if (myInterClusterTransportToMe.IsUnknown ())
			myInterClusterTransportToMe.Init (myHubId);

		if (!myInterClusterTransportToMe.IsUnknown ())
		{
			std::unique_ptr<DispatcherCmd> addHub (
				new ReplaceRemoteTransportCmd (myHubId, myInterClusterTransportToMe));
			dispatcherScript.AddCmd (std::move(addHub));
		}
		
		_mailer.UnicastJoin (hdr, cmdList, joinee, admin, &dispatcherScript);
		AwaitingFullSync fullSyncMarker (_catalog, project.GetProjectId ());
		fullSyncMarker.SetMarker (true);
	}

	BroadcastProjectChange (project.GetProjectId ());
	dbg << "<-- Model::JoinProject" << std::endl;
}

class InvitationDataProcessor
{
public:
	InvitationDataProcessor (Project::InviteData const & inviteData)
		: _inviteData (inviteData)
	{}

	bool CopyFile (std::string const & fileName, Progress::Meter & meter);
	void PreserveFile (std::string const & fileName);
	void DisplayErrors (bool isInvitation) const;

private:
	enum Errors
	{
		FtpException,
		FtpCopyCanceled,
		LocalCopyFailed
	};

private:
	Project::InviteData const &	_inviteData;
	BitSet<Errors>				_errors;
	std::string					_exceptionMsg;
};

bool InvitationDataProcessor::CopyFile (std::string const & fileName, Progress::Meter & meter)
{
	// Copy 'fileName' from the temporary folder to the invitation request target
	TmpPath tmpFolder;
	if (_inviteData.IsStoreOnInternet ())
	{
		Ftp::SmartLogin const & login = _inviteData.GetFtpLogin ();
		Ftp::Site ftpSite (login, login.GetFolder ());
		if (!ftpSite.Upload (tmpFolder.GetFilePath (fileName),
							 fileName,
							 meter))
		{
			// FTP upload failed or was canceled.
			if (ftpSite.IsException ())
			{
				_errors.set (FtpException, true);
				_exceptionMsg = ftpSite.GetErrorMsg ();
			}
			else
				_errors.set (FtpCopyCanceled, true);
			return false;
		}
	}
	else
	{
		Assert (_inviteData.IsStoreOnMyComputer () || _inviteData.IsStoreOnLAN ());
		FilePath targetPath (_inviteData.GetLocalFolder ());
		try
		{
			File::Copy (tmpFolder.GetFilePath (fileName),
						targetPath.GetFilePath (fileName));
		}
		catch (Win::Exception ex)
		{
			_exceptionMsg = Out::Sink::FormatExceptionMsg (ex);
			_errors.set (LocalCopyFailed, true);
			return false;
		}
	}
	return true;
}

void InvitationDataProcessor::PreserveFile (std::string const & fileName)
{
	// Move 'fileName' from the temporary folder to the user desktop
	TmpPath tmpFolder;
	FilePath userDesktopPath;
	ShellMan::VirtualDesktopFolder userDesktop;
	userDesktop.GetPath (userDesktopPath);
	File::Move (tmpFolder.GetFilePath (fileName),
				userDesktopPath.GetFilePath (fileName));
}

void InvitationDataProcessor::DisplayErrors (bool isInvitation) const
{
	std::string info;
	if (_inviteData.IsStoreOnInternet ())
	{
		if (isInvitation)
			info = "Project invitation";
		else
			info = "Project history";
		info += " upload to the FTP site ";
		if (_errors.test (FtpException))
		{
			info += "failed.";
			info += "\n\n";
			info += _exceptionMsg;
		}
		else
		{
			Assert (_errors.test (FtpCopyCanceled));
			info += "was canceled.";
		}
	}
	else
	{
		info = _exceptionMsg;
	}

	info += "\nCode Co-op stored project ";
	if (isInvitation)
		info += "invitation";
	else
		info += "history";
	info += " on your desktop.\n\n";
	if (isInvitation)
	{
		info += "You should either deliver it to the recipient or remove him/her from this project.";
		info += "\n\nNote: If the recipient is:"
				"\n   1. An e-mail peer, they should copy the script to their public inbox (usually c:\\co-op\\PublicInbox)."
				"\n   2. A hub, same as above."
				"\n   3. A satellite, they should copy the script to their hub's public inbox.";
	}
	else
	{
		info += "\n\nNote: The recipient should copy the .his file to their disk and import it using History>Import menu.";
	}
	TheOutput.Display (info.c_str ());
}

// Returns true when invitation successfully dispatched
bool Model::InviteToProject (Project::InviteData & inviteData,
							 ProjectChecker & projectChecker,
							 Progress::Meter & meter)
{
	if (!RepairProject (projectChecker, "Invitation"))
		return false;

	Assert (_dataBase.IsProjectAdmin ());

	JoinContext context (_catalog, _activityLog, _userPermissions.GetState (), true); // invitation
	context.SetComputerName (inviteData.GetComputerName ());
	context.SetManualInvitationDispatch (inviteData.IsManualInvitationDispatch ());
	Member joineeMember;
	if (inviteData.IsObserver ())
	{
        joineeMember.SetState (StateObserver ());
	}
	else
	{
		StateVotingMember state;
		state.SetCheckoutNotification (inviteData.IsCheckoutNotification ());
		joineeMember.SetState (state);
	}
	MemberDescription joineeDescription;
	joineeDescription.SetName (inviteData.GetUserName ());
	joineeDescription.SetHubId (inviteData.GetEmailAddress ());
	MemberInfo joineeInfo (joineeMember, joineeDescription); 
	context.SetJoineeInfo (joineeInfo);


	// Project verification sucessfull
	AcceptJoinRequest (context);

	if (inviteData.IsManualInvitationDispatch ())
	{
		std::string invitationFileName = context.GetInvitationFileName ();
		// Copy invitation file as requested
		InvitationDataProcessor processor (inviteData);
		if (!processor.CopyFile (invitationFileName, meter))
		{
			processor.PreserveFile (invitationFileName);
			processor.DisplayErrors (true);	// Invitation
			return false;
		}
	}

	if (inviteData.IsTransferHistory ())
	{
		// Export project history to the temporary folder
		std::string historyFileName (GetProjectName ());
		historyFileName += ".his";
		File::LegalizeName (historyFileName);
		SafeTmpFile tmpFilePath (historyFileName);

		if (_history.Export (tmpFilePath.GetFilePath (), meter))
		{
			// History export successfull - copy history file as requested
			InvitationDataProcessor processor (inviteData);
			if (!processor.CopyFile (historyFileName, meter))
			{
				inviteData.SetTransferHistory (false);
				processor.DisplayErrors (false);	// History
			}
		}
		else
		{
			// History export cancelled by the user
			inviteData.SetTransferHistory (false);
		}
	}
	return true; // the invitation part was successful
}

void Model::CreateEmptyProject (Project::Data & projectData, 
								MemberDescription const & member)
{
	// Allocates project id
	UpdateCatalog (projectData, member);
	BuildProjectInfrastructure (projectData, _pathFinder);
	InitPaths ();
	_history.CreateLogFiles ();
}

void Model::ProjectOptions (Project::OptionsEx const & options)
{
	Project::Db const & projectDb = _dataBase.GetProjectDb ();
	bool autoSynchChange = projectDb.IsAutoSynch () != options.IsAutoSynch ();
	bool autoJoinChange = projectDb.IsAutoJoin () != options.IsAutoJoin ();
	bool keepCheckedOutChange = projectDb.IsKeepCheckedOut () != options.IsKeepCheckedOut ();
	bool allBccChange = projectDb.UseBccRecipients () != options.UseBccRecipients ();
	if (autoSynchChange || autoJoinChange || keepCheckedOutChange || allBccChange)
	{
		Transaction xact (*this, _pathFinder);
		Project::Db & projectDb = _dataBase.XGetProjectDb ();
		projectDb.XSetAutoSynch (options.IsAutoSynch ());
		projectDb.XSetAutoJoin (options.IsAutoJoin ());
		projectDb.XSetKeepCheckedOut (options.IsKeepCheckedOut ());
		projectDb.XSetBccRecipients (options.UseBccRecipients ());
		xact.Commit ();
	}

	MemberState thisUserState = projectDb.GetMemberState (projectDb.GetMyId ());
	bool isCheckoutNotificationChange =
		thisUserState.IsCheckoutNotification () != options.IsCheckoutNotification ();
	if (isCheckoutNotificationChange)
		SetCheckoutNotification (options.IsCheckoutNotification ());

	if (thisUserState.IsDistributor () != options.IsDistribution ())
	{
		Assert (projectDb.MemberCount () == 1 || thisUserState.IsDistributor ());
		Assert (thisUserState.IsAdmin ());
		thisUserState.SetDistributor (options.IsDistribution ());
		thisUserState.SetNoBranching (options.IsNoBranching ());
		ChangeMemberState (projectDb.GetMyId (),  thisUserState);
	}
	// Store auto-invitation data in the registry
	Registry::SetAutoInvitationOptions (options.IsAutoInvite (), options.GetAutoInviteProjectPath ());
}

void Model::ReadProjectDb (Project::Data & projData, bool blind)
{
	_pathFinder.SetProjectDir (projData);
	_pathFinder.LockProject ();
	InitPaths ();

	{	// Transaction scope
		ReadTransaction xact (*this, _pathFinder);
		Read (xact.GetDeserializer ());
		xact.Commit ();
	}

	_project.Enter (projData.GetProjectId ());
	_mergedFiles.SetThisProjectId (projData.GetProjectId ());
	_historicalFiles.SetThisProjectId (projData.GetProjectId ());
	_historicalFiles.SetTargetProject (projData.GetProjectId (), gidInvalid);
	_directory.SetProjectRoot (_pathFinder.GetProjectDir (),
							   _dataBase.GetRootId (),
							   _dataBase.ProjectName (),
							   !IsQuickVisit ());
	_userPermissions.RefreshProjectData (_dataBase.GetProjectDb (), _pathFinder);

	if (blind)
		return;

	_directory.WatchCurrentFolder ();
	// Verify the project
	if (_isConverted)
	{
		AckBox ackBox;
		Assert (IsProjectReady ());
		// Broadcast this user membership update -- this will create
		// the first entry in this user's membership change history
		// and notify other converted project members about this user
		// conversion.
		{
			Transaction xact (*this, _pathFinder);
			//--------------------------------------------------------------------
			XBroadcastConversionMembershipUpdate (ackBox);
			//--------------------------------------------------------------------
			xact.Commit ();
		}
		_isConverted = false;
		SendAcknowledgments (ackBox);
	}

	if (!IsQuickVisit ())
	{
		UserId myId = _dataBase.GetMyId ();
		if (myId != gidInvalid)
		{
			MemberState myState = _dataBase.GetMemberState (myId);
			if (!myState.IsVerified ())
				TheOutput.Display ("Incorrect member state. Please, contact support@relisoft.com");
		}

		_watcher->AddFolder (_pathFinder.InBoxDir ().GetDir (), false); // non-recursive
	}
}

void Model::LeaveProject (bool removeProjectLock)
{
	if (!_pathFinder.IsInProject ())
		return;

	_pendingAutoMergers.Clear ();
	_pathFinder.UnlockProject();
	if (removeProjectLock)
		_pathFinder.RemoveLock ();
	//if (!IsQuickVisit ())
		_watcher->StopWatching (_pathFinder.InBoxDir ().GetDir ());
	UpdateRecentProjects ();
	_fileClipboard.clear ();
	_mergedFiles.SetThisProjectId (-1);
	_historicalFiles.SetThisProjectId (-1);
	_project.Exit ();
	_history.LeaveProject ();
	_historicalFiles.Clear (true);	// Force
	_mergedFiles.Clear (true);	// Force
	_userPermissions.ClearProjectData ();
	// Must be last
	_pathFinder.Clear ();

//	ClearTransaction xact (*this);	//	future code will look like this
	Clear ();						//	currently modifies real data, not xdata
//	xact.Commit ();					//	future code will look like this
}

void Model::UpdateRecentProjects ()
{
	if (!IsQuickVisit ())
	{
		Registry::RecentProjectList recentProjects;
		recentProjects.Add (GetProjectId ());
	}
}

void Model::MoveProject (int projId, FilePath const & newPath)
{
	Assert (projId != -1);
	_catalog.MoveProjectTree (projId, newPath);
	// Revisit: stop watching current directory
	BroadcastProjectChange (projId);
}

// Returns true when I can defect from the project
bool Model::CanDefect (ProjDefectData & projectData)
{
	if (!IsProjectReady ())
		return true;	// The full synch script not received yet

	Project::Db const & projectDb = _dataBase.GetProjectDb ();
	bool lastProjectMember = (projectDb.MemberCount () == 1);
	if (projectDb.IsProjectAdmin () && !lastProjectMember)
	{
		// The project administrator has to elect the new admin or remove all
		// project members before defecting
		if (!IsQuickVisit ())
		{
			TheOutput.Display ("Using Project>Members dialog, select the new administrator\n"
							   "or remove all project members, before defecting from this project.");
			
		}
		return false;
	}

	if (!IsQuickVisit ())
	{
		// Warn user if deleting whole project tree and/or defecting user
		// is the last one in this project.
		bool deletingAllProjectFiles = (projectData.DeleteWholeProjectTree () || projectData.DeleteProjectFiles ());
		std::string warning;
		if (lastProjectMember)
		{
			warning += "You are the last member of the project '";
			warning += GetProjectName ();
			warning += "'.\n\nDefecting from it will completely remove project history";
			if (deletingAllProjectFiles)
			{
				warning += "\nand delete ";
				if (projectData.DeleteWholeProjectTree ())
					warning += "every file and folder";
				else
					warning += "all project files";
				warning += " starting with the project root folder:\n\n";
				warning += GetProjectDir ();
				warning += "\n\n(Files will be moved to your recycle bin.)";
			}
			else
			{
				warning += '.';
			}
		}
		else if (deletingAllProjectFiles)
		{
			warning += "You are defecting from the project '";
			warning += projectDb.ProjectName ();
			warning += "' and you are deleting\n";
			if (projectData.DeleteWholeProjectTree ())
				warning += "every file and folder";
			else
				warning += "all project files";
			warning += " starting at the project root folder:\n\n";
			warning += GetProjectDir ();
			warning += "\n\n(Files will be moved to your recycle bin.)";
		}
		if (!warning.empty ())
		{
			warning += "\nDo you want to continue?";
			Out::Answer userChoice = TheOutput.Prompt (warning.c_str (),
											Out::PromptStyle (Out::YesNo, Out::No, Out::Warning));

			if (userChoice == Out::No)
				return false;
		}
	}
	return true;
}

// Returns true when defect completed
bool Model::Defect (ProjDefectData & projectData)
{
	Assert (IsInProject () && _project.GetCurProjectId () != -1);
	if (!projectData.IsUnconditionalDefect () && !CanDefect (projectData))
		return false;

	std::string rootFolder (_pathFinder.GetProjectDir ());
	ShellMan::FileRequest request;
	MakeFileRequest (request);

	DoDefect (_project.GetCurProjectId ());

	// We have passed the point of no return -- ignore all exceptions
	try
	{
		if (projectData.DeleteProjectFiles ())
		{
			// Delete enlisted project files
			request.DoDelete (TheAppInfo.GetWindow ());
		}
		else if (projectData.DeleteWholeProjectTree ())
		{
			// Delete project root folder
			ShellMan::Delete (TheAppInfo.GetWindow (),
							  rootFolder.c_str (),
							  ShellMan::SilentAllowUndo); 
		}
		else
		{
			// Make files read-write
			request.MakeReadWrite ();
		}
	}
#if defined (NDEBUG)
	catch ( ... )
	{
		Win::ClearError ();
		// Ignore all exceptions -- there is no going back, so lets
		// pretend that everything went smoothly with project removal
	}
#else
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
	}
#endif
	return true;
}

void Model::DoDefect (int removedProjectId, bool forcedDefect)
{
	Project::Db const & projectDb = _dataBase.GetProjectDb ();
	UserId myUserId = projectDb.GetMyId ();
	bool lastProjectMember = (projectDb.MemberCount () == 1);

	// Under transaction inform Dispatcher and other project members about the defecting user
	{
		Transaction xact (*this, _pathFinder);
		XPublishDefectInfo (lastProjectMember, myUserId, removedProjectId);
		_dataBase.XDefect ();
		_history.XDefect ();
		xact.CommitAndDelete (_pathFinder);
	}

	// We have passed the point of no return -- ignore all exceptions
	try
	{
		std::string rootFolder (_pathFinder.GetProjectDir ());
		std::string dataBaseFolder (_pathFinder.GetSysPath ());
		FilePath inboxPath = _catalog.GetProjectInboxPath (removedProjectId);
		// Remove project from the catalog
		_catalog.ForgetProject (removedProjectId);
		LeaveProject (true);	// Remove project lock file

		if (!forcedDefect)
		{
			// Leave project tree
			CurrentFolder curFolder;
			FilePath path (rootFolder);
			path.DirUp ();
			curFolder.Set (path.GetDir ());
		}

		// Tell Dispatcher to reset project list
		BroadcastProjectChange (removedProjectId);

		// Remove database folders
		FolderRequest coopDbFolder (dataBaseFolder.c_str (), DeleteFolder);
		_folderChange.push_back (coopDbFolder);
		FolderRequest coopInboxFolder (inboxPath.GetDir (), DeleteFolder);
		_folderChange.push_back (coopInboxFolder);
	}
#if defined (NDEBUG)
	catch ( ... )
	{
		Win::ClearError ();
		// Ignore all exceptions -- there is no going back, so lets
		// pretend that everything went smoothly with project removal
	}
#else
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
	}
#endif
}

void Model::XPublishDefectInfo (bool lastProjectMember, UserId myUserId, int curProjectId)
{
	AckBox fakeAckBox;
	std::unique_ptr<MemberDescription> me;
	if (myUserId != gidInvalid)
	{
		me = _dataBase.XRetrieveMemberDescription (myUserId);
	}
	else
	{
		// Defecting user didn't receive full sync -- create sender description from catalog data
		Assert (curProjectId != -1);
		me.reset (new MemberDescription (std::string (),
										 _catalog.GetHubId (),
										 std::string (),
										 std::string (),
										 _catalog.GetUserId (curProjectId)));
	}
	std::unique_ptr<DispatcherCmd> addressChange (new AddressChangeCmd (me->GetHubId (),
																	  me->GetUserId (),
																	  std::string (),
																	  std::string ()));
	if (lastProjectMember || myUserId == gidInvalid)
	{
		// Last project member or defecting user didn't receive full sync -- no need to
		// send out defect script, just notify Dispatcher
		// Create transport header -- note we don't add recipient list,
		// because Code Co-op doesn't know it. If there is a need for
		// a script Dispatcher will know who the recipients are.
		DispatcherScript dispatcherScript;
		dispatcherScript.AddCmd (std::move(addressChange));
		TransportHeader txHdr;
		txHdr.AddScriptId (_dataBase.GetRandomId ());
		std::string hubId (_catalog.GetHubId ());
		Address myAddress (hubId.empty () ? me->GetHubId () : hubId,
						   _dataBase.XProjectName (),
						   me->GetUserId ());
		txHdr.AddSender (myAddress);
		DispatcherProxy dispatcher;
		dispatcher.Notify (txHdr, dispatcherScript);
	}
	else
	{
		// Send out defect script, which will carry Dispatcher notification script
		Assert (myUserId != gidInvalid);
		MemberState myState = _dataBase.XGetMemberState (myUserId);
		MemberInfo thisUserUpdate (myUserId, StateDead (myState, true), *me);
		XCheckinMembershipChange (thisUserUpdate, fakeAckBox, std::move(addressChange));
	}
}

void Model::ForceDefect (Project::Data & projData)
{
	int projId = projData.GetProjectId ();
	if (projId != -1)
	{
		// Try to defect from the project.
		try
		{
			Model tmpModel (true);	// Quick visit - don't display any warnings
			FeedbackManager dummy;	// Null feedback manager
			tmpModel.SetUIFeedback (&dummy);
			tmpModel.ReadProjectDb (projData, true);	// blind
			tmpModel.DoDefect (projId, true);	// Forced defect
			return;
		}
		catch ( ... )
		{
			// Cannot defect
		}
	}

	std::string info ("Cannot defect from the project '");
	info += projData.GetProjectName ();
	info += "'\n";
	info += "Source path: ";
	info += projData.GetRootDir ();
	info += "\nbecause cannot read this project database.";
	info += "\n\nAsk the project administrator to remove you from this project.";
	TheOutput.Display (info.c_str ());

	// We could not perform the proper defect operation - remove project from the catalog
	if (projId != -1)
	{
		FilePath dataPath = _catalog.GetProjectDataPath (projId);
		FilePath inboxPath = _catalog.GetProjectInboxPath (projId);
		FolderRequest coopDbFolder (dataPath.GetDir (), DeleteFolder);
		_folderChange.push_back (coopDbFolder);
		FolderRequest coopInboxFolder (inboxPath.GetDir (), DeleteFolder);
		_folderChange.push_back (coopInboxFolder);
		_catalog.ForgetProject (projId);
		BroadcastProjectChange (projId);
	}
}

//
// Project repair methods
//

void Model::FindCorruptedFiles (VerificationReport & report, 
								bool & extraRepairNeeded,
								Progress::Meter & progressMeter)
{

	FilePath projectDatabasePath = _catalog.GetProjectDataPath (GetProjectId ());
	for (FileSeq seq (projectDatabasePath.GetFilePath ("*.out")); !seq.AtEnd (); seq.Advance ())
	{
		GlobalIdPack gidPack (seq.GetName ());
		report.Remember (VerificationReport::PreservedLocalEdits, gidPack);
	}

	extraRepairNeeded = false;
	Project::Path relativePath (_dataBase);
	GidSet allGids;
	progressMeter.SetRange (0, _dataBase.FileCount ());
	for (AllProjectFilesSequencer sequencer (_dataBase); !sequencer.AtEnd (); sequencer.Advance ())
	{
		FileData const * fileData = sequencer.GetFileData ();
		// Skip new and unrecoverable files
		if (fileData->GetState ().IsNew () || !fileData->GetType().IsRecoverable())
			continue;// Just added files don't have checksum set yet and don't require repair

		GlobalId gid = fileData->GetGlobalId ();
		GidSet::const_iterator iter = allGids.find (gid);
		if (iter != allGids.end ())
		{
			// Duplicate global id found -- really bad!
			dbg << "Duplicate global id found: " << *fileData << std::endl;
			GlobalIdPack pack (fileData->GetGlobalId ());
			std::string info (fileData->GetUniqueName ().GetName ());
			info += ' '; 
			info += pack.ToBracketedString ();
			Win::ClearError ();
			throw Win::Exception ("Corrupted database: duplicate global id found. "
				"Please contact support@relisoft.com", info.c_str ());
		}
		else
		{
			allGids.insert (gid);
		}

		UniqueName const & uname = fileData->GetUniqueName ();
		std::string const & name = uname.GetName ();
		if (strlen (name.c_str ()) == 0)
		{
#if defined (SPECIAL_BUILD)
			extraRepairNeeded = true;
			continue;
#else
			// File/folder without name found -- really bad!
			dbg << "File/folder without name found: " << *fileData << std::endl;
			GlobalIdPack pack (fileData->GetGlobalId ());
			std::string info (uname.GetName ());
			info += ' ';
			info += pack.ToBracketedString ();
			Win::ClearError ();
			throw Win::Exception ("Corrupted database: file without name found. "
				"Please contact support@relisoft.com", info.c_str ());
#endif
		}

		if (!uname.IsNormalized ())
		{
			dbg << "Illegal name found: " << *fileData << std::endl;
			report.Remember (VerificationReport::IllegalName, gid);
		}

		// Does the parent exist?
		GlobalId gidParent = uname.GetParentId ();
		Assert (gidParent != gidInvalid);
		FileData const * parentData = _dataBase.FindByGid (gidParent);
		if (parentData == 0)
		{
			dbg << "Orphan file found: " << *fileData << std::endl;
			GlobalIdPack pack (fileData->GetGlobalId ());
			std::string info (uname.GetName ());
			info += ' ';
			info += pack.ToBracketedString ();
			Win::ClearError ();
			throw Win::Exception ("Corrupted database: project file without parent folder.", 
									info.c_str ());
		}

		// Parent exists in the database -- check if the project file:
		// 1. exist on disk
		// 2. has right checksum
		// 3. is read-only
		// 4. its parent is under control.
		PhysicalFile file (*fileData, _pathFinder);
		progressMeter.SetActivity (relativePath.MakePath (gid));
		FileState state = fileData->GetState ();
		// Check checksum and file attributes

		Area::Location area = Area::LastArea;
		if (state.IsCheckedIn ())
		{
			area = Area::Project;
		}
		else
		{
			Assume (state.IsRelevantIn (Area::Original), ::ToHexString (state.GetValue ()).c_str ());
			if (state.IsRelevantIn (Area::Synch))
				area = Area::Synch;
			else
				area = Area::Original;
		}

		CheckSum actualChecksum = file.GetCheckSum (area);
		if (fileData->GetCheckSum () != actualChecksum)
		{
			// Remember corrupted file gid
			report.Remember (VerificationReport::Corrupted, gid);
			if (area == Area::Synch)
			{
				// will uncheckout first, before repairing
				report.Remember (VerificationReport::MissingCheckedout, gid);
			}

			// Create dummy file, so that File::Copy doesn't throw during repair
			if (!file.ExistsIn (area) && area != Area::Project)
				file.CreateEmptyIn (area);

			if (state.IsRelevantIn (Area::Original) 
				&& !report.IsPresent (VerificationReport::PreservedLocalEdits, gid))
			{
				file.CopyToLocalEdits ();
				report.Remember (VerificationReport::PreservedLocalEdits, gid);
			}

			continue;
		}

		// Checskusm is now okay

		if (state.IsCheckedIn ())
		{
			if (!file.IsReadOnlyInProject ())
			{
				// Remember files without read-only attribute
				report.Remember (VerificationReport::MissingReadOnlyAttribute, gid);
			}
		}
		else
		{
			Assert (!state.IsNew ());
			Assert (file.ExistsIn (Area::Original));

			// Very specific optimization
			if (state.IsPresentIn (Area::Project) 
				&& !file.ExistsIn (Area::Project) 
				&& !file.ExistsIn (Area::LocalEdits))
			{
				// Can un-checkout instead of repairing
				report.Remember (VerificationReport::MissingCheckedout, gid);
			}
		}

		if (parentData->GetState ().IsNone () &&
			!report.IsPresent (VerificationReport::AbsentFolder, gidParent))
		{
			// Remember not controlled parent folder
			report.Remember (VerificationReport::AbsentFolder, gidParent);
		}

		progressMeter.StepAndCheck ();
	}
}

// Returns true when checked in folder was not present on disk
bool Model::FindMissingFolders (VerificationReport & report, Progress::Meter & progressMeter)
{
	bool missingFromDiskFolderFound = false;
	Project::Path relativePath (_dataBase);
	progressMeter.SetRange (0, _dataBase.FileCount ());
	for (AllProjectFoldersSequencer sequencer (_dataBase); !sequencer.AtEnd (); sequencer.Advance ())
	{
		FileData const * folderData = sequencer.GetFileData ();
		if (!folderData->GetState ().IsCheckedIn ())
			continue;	// Skip checked out folder

		FileData const * parentData = _dataBase.GetParentFileData (folderData);
		// Parent folder exists in the database - check parent folder state
		FileState parentState = parentData->GetState ();
		if (parentState.IsNone ())
		{
			GlobalId parentGid = parentData->GetGlobalId ();
			if (!report.IsPresent (VerificationReport::AbsentFolder, parentGid))
			{
				// Remember not controlled parent folder
				report.Remember (VerificationReport::AbsentFolder, parentGid);
			}
		}
		else if (!parentState.IsCheckedIn ())
			continue;	// If parent folder is checked out - don't check disk

		GlobalId folderGid = folderData->GetGlobalId ();
		progressMeter.SetActivity (relativePath.MakePath (folderGid));
		char const * path = _pathFinder.GetFullPath (folderGid, Area::Project);
		Assert (folderData->GetState ().IsCheckedIn ());
		if (!File::Exists (path))
		{
			// Recreate checked in folder on disk
			missingFromDiskFolderFound = true;
			_pathFinder.MaterializeFolderPath (path, true); // Quiet - don't ask user any questions
		}
		if (!folderData->GetUniqueName ().IsNormalized ())
		{
			dbg << "Illegal name found: " << *folderData << std::endl;
			report.Remember (VerificationReport::IllegalName, folderGid);
		}
		progressMeter.StepAndCheck ();
	}
	return missingFromDiskFolderFound;
}

void Model::CheckConsistency (VerificationReport & report, Progress::Meter & progressMeter)
{
	progressMeter.SetRange (0, _dataBase.FileCount () + _synchArea.Count ());
	for (DataBase::FileIter iter = _dataBase.begin (); iter != _dataBase.end (); ++iter)
	{
		progressMeter.StepAndCheck ();
		FileData const * fd = *iter;
		FileState state = fd->GetState ();
		if (state.IsDirtyUncontrolled ())
			report.Remember (VerificationReport::DirtyUncontrolled, fd->GetGlobalId ());
	}

	for (SynchAreaSeq seq (_synchArea); !seq.AtEnd (); seq.Advance ())
	{
		progressMeter.StepAndCheck ();
		GlobalId gid = seq.GetGlobalId ();
		FileData const * fd = _dataBase.GetFileDataByGid (gid);
		FileState state = fd->GetState ();
		if (state.IsCheckedIn() || state.IsNone())
			report.Remember (VerificationReport::SyncAreaOrphan, gid);
	}
}

void Model::MakeReadOnly (VerificationReport::Sequencer notReadOnlyFiles)
{
	if (notReadOnlyFiles.AtEnd ())
		return;

	SimpleMeter meter (_uiFeedback);
	meter.SetActivity ("Resetting Read-Only attribute.");
	meter.SetRange (0, notReadOnlyFiles.Count ());
	Project::Path relativePath (_dataBase);
	do
	{
		FileData const * fileData = _dataBase.GetFileDataByGid (notReadOnlyFiles.Get ());
		PhysicalFile file (*fileData, _pathFinder);
		file.MakeReadOnlyInProject ();
		meter.StepIt ();
		notReadOnlyFiles.Advance ();
	} while (!notReadOnlyFiles.AtEnd ());
}

void Model::MakeUncontrolled (VerificationReport::Sequencer uncontrolledItems)
{
	if (uncontrolledItems.AtEnd ())
		return;

	SimpleMeter meter (_uiFeedback);
	meter.SetActivity ("Correcting file/folder state.");
	meter.SetRange (0, uncontrolledItems.Count ());

	//--------------------------------------------------------------------------
	Transaction xact (*this, _pathFinder);
	do
	{
		Transformer trans (_dataBase, uncontrolledItems.Get ());
		trans.MakeNotInProject ();
		meter.StepIt ();
		uncontrolledItems.Advance ();
	} while (!uncontrolledItems.AtEnd ());
	xact.Commit ();
	//--------------------------------------------------------------------------
}

void Model::CleanupSyncArea (VerificationReport::Sequencer syncAreaItems)
{
	if (syncAreaItems.AtEnd ())
		return;

	SimpleMeter meter (_uiFeedback);
	meter.SetActivity ("Accepting synchronization script.");
	meter.SetRange (0, syncAreaItems.Count ());

	//--------------------------------------------------------------------------
	TransactionFileList fileList;
	FileTransaction xact (*this, _pathFinder, fileList);

	do
	{
		GlobalId gid = syncAreaItems.Get ();
		Transformer trans (_dataBase, gid);
		trans.AcceptSynch (_pathFinder, fileList, _synchArea);
		meter.StepIt ();
		syncAreaItems.Advance ();
	} while (!syncAreaItems.AtEnd ());

	xact.Commit ();
	//--------------------------------------------------------------------------
}

void Model::RestoreLocalEdits (VerificationReport::Sequencer preservedLocalEdits)
{
	if (preservedLocalEdits.AtEnd ())
		return;

	SimpleMeter meter (_uiFeedback);
	meter.SetActivity ("Restoring local edits.");
	meter.SetRange (0, 2 * preservedLocalEdits.Count () + 1);
	// Make a copy of sequencer for the second iteration
	VerificationReport::Sequencer originalLocalEdits = preservedLocalEdits;
	GidList checkoutFiles;
	do
	{
		GlobalId gid = preservedLocalEdits.Get ();
		FileData const * fd = _dataBase.GetFileDataByGid (gid);
		FileState state = fd->GetState ();
		if (state.IsCheckedIn ())
			checkoutFiles.push_back (gid);
		preservedLocalEdits.Advance ();
		meter.StepIt ();
	} while (!preservedLocalEdits.AtEnd ());

	if (!checkoutFiles.empty ())
	{
		CheckOut (checkoutFiles, false, false);	// Don't include folders and not recursive
		meter.StepIt ();
	}

	do
	{
		GlobalId gid = originalLocalEdits.Get ();
		std::string source (_pathFinder.GetFullPath (gid, Area::LocalEdits));
		char const * target = _pathFinder.GetFullPath (gid, Area::Project);
		File::Copy (source.c_str (), target);
		File::Delete (source);
		originalLocalEdits.Advance ();
		meter.StepIt ();
	} while (!originalLocalEdits.AtEnd ());
}

void Model::CleanupLocalEdits ()
{
	// Delete all *.out files from the project database folder
	FilePath projectDatabasePath = _catalog.GetProjectDataPath (GetProjectId ());
	for (FileSeq seq (projectDatabasePath.GetFilePath ("*.out")); !seq.AtEnd (); seq.Advance ())
	{
		File::Delete (projectDatabasePath.GetFilePath (seq.GetName ()));
	}
}

void Model::MakeProjectFolder (VerificationReport::Sequencer absentFolders)
{
	if (absentFolders.AtEnd ())
		return;

	SimpleMeter meter (_uiFeedback);
	meter.SetActivity ("Setting controlled state for missing folders.");
	meter.SetRange (0, absentFolders.Count ());
	TransactionFileList fileList;
	//--------------------------------------------------------------------------
	SynchTransaction xact (*this, _pathFinder, _dataBase, fileList);
	do
	{
		Transformer trans (_dataBase, absentFolders.Get ());
		trans.MakeProjectFolder ();
		meter.StepIt ();
		absentFolders.Advance ();
	} while (!absentFolders.AtEnd ());
	xact.Commit ();
	//--------------------------------------------------------------------------
}

void Model::CorrectIllegalNames (VerificationReport & report)
{
	dbg << "--> Model::CorrectIllegalNames" << std::endl;
	VerificationReport::Sequencer illegalNames = report.GetSequencer (VerificationReport::IllegalName);
	if (illegalNames.AtEnd ())
	{
		dbg << "--> Model::CorrectIllegalNames - no illegal names." << std::endl;
		return;
	}

	Transaction xact (*this, _pathFinder);
	for (; !illegalNames.AtEnd (); illegalNames.Advance ())
	{
		GlobalId gid = illegalNames.Get ();
		FileData const * fd = _dataBase.GetFileDataByGid (gid);
		dbg << "Correcting name of: " << *fd;
		UniqueName const & illegalUname = fd->GetUniqueName ();
		FileState itemState = fd->GetState ();
		Assume (itemState.IsPresentIn (Area::Project), illegalUname.GetName ().c_str ());
		Assert (!illegalUname.IsNormalized ());

		// Construct the correct unique name for this item
		// Recycle absent folders
		// WARNING: this will stop working when folder move is implemented!
		UniqueName correctUname;
		GlobalId currentParentGid = illegalUname.GetParentId ();
		PartialPathSeq pathSeq (illegalUname.GetName ().c_str ());
		for ( ; !pathSeq.IsLastSegment (); pathSeq.Advance ())
		{
			UniqueName segmentUname (currentParentGid, pathSeq.GetSegment ());
			dbg << "Searching for the folder with unique name: " << pathSeq.GetSegment () << " - parent id: " << GlobalIdPack (currentParentGid) << std::endl;
			FileData const * parentFd = _dataBase.XFindAbsentFolderByName (segmentUname);
			if (parentFd == 0)
			{
				dbg << "Not found among non-project folders\n";
				parentFd = _dataBase.XFindProjectFileByName (segmentUname);
				if (parentFd == 0 || !parentFd->GetType().IsFolder())
				{
					throw Win::InternalException (
						"Cannot correct file/folder name, because parent folder "
						"is not recorded in the project database.\n"
						"Please contact support@relisoft.com",
						illegalUname.GetName ().c_str ());
				}
			}
			Assume (parentFd->GetType ().IsFolder (), parentFd->GetUniqueName ().GetName ().c_str ());
			currentParentGid = parentFd->GetGlobalId ();
			if (!parentFd->GetState ().IsPresentIn (Area::Project))
				report.Remember (VerificationReport::AbsentFolder, parentFd->GetGlobalId ());
		}
		correctUname.Init (currentParentGid, pathSeq.GetSegment ());
		dbg << "Corrected unique name: " << correctUname.GetName () << " - " << GlobalIdPack (gid).ToSquaredString () << "; parent: " << GlobalIdPack (correctUname.GetParentId ()).ToString () << std::endl;

		// Correct item's unique name in the file database
		FileData * fdEdit = _dataBase.XGetEdit (gid);
		fdEdit->SetName (correctUname);

		// Correct item unique name in the history
		_history.XCorrectItemUniqueName (gid, correctUname);
	}
	xact.Commit ();
	dbg << "<-- Model::CorrectIllegalNames" << std::endl;
}

void Model::ExtraRepair ()
{
	// a good place for extra repairing
}

// Returns true when complete repair of corrupted files (nothing is left)
bool Model::RepairFiles (VerificationReport::Sequencer corruptedFiles)
{
	if (corruptedFiles.AtEnd ())
		return true;

	SimpleMeter meter (_uiFeedback);
	GidSet unrecoverableFiles;
	Workspace::RepairHistorySelection selection (_history,
												 corruptedFiles,
												 unrecoverableFiles,
												 meter);
	if (!selection.empty())
	{
		//--------------------------------------------------------------------------
		TransactionFileList fileList;
		FileTransaction xact (*this, _pathFinder, fileList);
		XExecuteRepair (selection, fileList, unrecoverableFiles, meter);
		xact.Commit ();
		//--------------------------------------------------------------------------
	}

	if (!unrecoverableFiles.empty ())
	{
		std::string info("Project \"");
		info += GetProjectName();
		info += "\nThe following files cannot be repaired:\n\n";
		for (GidSet::const_iterator iter = unrecoverableFiles.begin ();
			 iter != unrecoverableFiles.end ();
			 ++iter)
		{
			FileData const * fd = _dataBase.GetFileDataByGid (*iter);
			info += GlobalIdPack (*iter).ToBracketedString ();
			info += " - ";
			info += fd->GetName ();
			info += "\n";
		}
		info += "\n\nPlease, contact support@relisoft.com";
		TheOutput.Display (info.c_str (), Out::Error);
		return false;
	}
	return true;
}

// Returns true when complete repair
// Don't change the order of repairs unless you know what you're doing
bool Model::RecoverFiles (VerificationReport & report)
{
	Assert (!report.IsEmpty ());
	VerificationReport::Sequencer missingNew = report.GetSequencer (VerificationReport::MissingNew);
	if (!missingNew.AtEnd ())
	{
		// Missing New files are removed from the project
		TransactionFileList fileList;
		//--------------------------------------------------------------------------
		FileTransaction xact (*this, _pathFinder, fileList);
		for (; !missingNew.AtEnd (); missingNew.Advance ())
		{
			Transformer trans (_dataBase, missingNew.Get ());
			trans.MakeNotInProject ();
		}
		xact.Commit ();
		//--------------------------------------------------------------------------
	}

	// Missing folders are created on disk
	for (VerificationReport::Sequencer missingFolders = report.GetSequencer (VerificationReport::MissingFolder);
		 !missingFolders.AtEnd ();
		 missingFolders.Advance ())
	{
		char const * path = _pathFinder.GetFullPath (missingFolders.Get (), Area::Project);
		File::MaterializePath (path, true);	// Don't ask questions
	}

	// Fix illegal names if present. Fixing illegal names may
	// add absent folders to the verification report.
	CorrectIllegalNames (report);

	// Change the state (from none to checked in) of absent project folders
	MakeProjectFolder (report.GetSequencer (VerificationReport::AbsentFolder));

	// Set missing read-only attribute
	MakeReadOnly (report.GetSequencer (VerificationReport::MissingReadOnlyAttribute));

	// The uncheckout missing must be done before RepairFiles 
	// (we don't want to repair files that are in the merge state
	VerificationReport::Sequencer missingCheckout = report.GetSequencer (VerificationReport::MissingCheckedout);
	if (!missingCheckout.AtEnd ())
	{
		// Un-checkout missing checked out files
		GidList files;
		do
		{
			files.push_back (missingCheckout.Get ());
			missingCheckout.Advance ();
		} while (!missingCheckout.AtEnd ());

		Uncheckout (files, true);	// Quiet -- don't ask are you sure question
	}

	// Make sure that uncontrolled items have correct state
	MakeUncontrolled (report.GetSequencer (VerificationReport::DirtyUncontrolled));

	// Cleanup Sync Area if necessary
	CleanupSyncArea (report.GetSequencer (VerificationReport::SyncAreaOrphan));

	// Repair corrupted files
	bool isCompleteRepair = RepairFiles (report.GetSequencer (VerificationReport::Corrupted));

	// Restore local edits if present
	RestoreLocalEdits (report.GetSequencer (VerificationReport::PreservedLocalEdits));

	// Just in case delete all *.out files from the project database folder
	CleanupLocalEdits ();

	SetIncomingScriptsMarker ();
	return isCompleteRepair;
}

//
// Project members
//

void Model::ProjectMembers (ProjectMembersData const & projectMembers, bool & isRecoveryNeeded)
{
	Assert (isRecoveryNeeded == false);
	if (!projectMembers.ChangesDetected ())
		return;

	AckBox ackBox;
	bool const isProjectAdmin = _dataBase.IsProjectAdmin ();
	// Store membership changes in the database and send control script.
	for (ProjectMembersData::Sequencer seq (projectMembers); !seq.AtEnd (); seq.Advance ())
	{
		std::unique_ptr<DispatcherCmd> addressChange;
		MemberInfo const & newMemberInfo = seq.GetMemberInfo ();
		MemberState const newState = newMemberInfo.State ();
		bool const thisUserChange = newMemberInfo.Id () == _dataBase.GetMyId ();
		MemberState const currentState = _dataBase.GetMemberState (newMemberInfo.Id ());
		if (thisUserChange && currentState.IsObserver () && newState.IsVoting ())
		{
			// This user changes from observer to voting
			isRecoveryNeeded = true;
		}
	
		if (!isProjectAdmin && !thisUserChange && !newState.IsAdmin () && !currentState.IsAdmin ())
		{
			// Regular project member removes unverified project member -- don't send membership update
			Assert (!newState.IsVerified ());
			Assert (!currentState.IsVerified ());
			Assert (newState.IsDead () || newState.IsObserver ());
			Transaction xact (*this, _pathFinder);

			_dataBase.XChangeState (newMemberInfo.Id (), newState);
			_history.XRemoveFromAckList (newMemberInfo.Id (), newState.IsDead (), ackBox);

			xact.Commit ();
			continue;
		}

		Assert (isProjectAdmin ||
				thisUserChange ||
				(!newState.IsVerified () && newState.IsAdmin ()) ||
				(!currentState.IsVerified () && currentState.IsAdmin ()));
		std::unique_ptr<MemberDescription> curMemberDescription 
			= _dataBase.RetrieveMemberDescription (newMemberInfo.Id ());
		Assert (curMemberDescription->GetUserId () == newMemberInfo.Description ().GetUserId ());

		std::string const & newHubId = newMemberInfo.HubId ();
		std::string const & curHubId = curMemberDescription->GetHubId ();
		if (!IsNocaseEqual (newHubId, curHubId) || newState.IsDead ())
		{
			// Project member Hub's Email Address change or
			// member has been removed from the project.
			std::string const & userId = curMemberDescription->GetUserId ();
			if (newState.IsDead ())
			{
				addressChange.reset (new AddressChangeCmd (curHubId,
														   userId,
														   std::string (),
														   std::string ()));
			}
			else
			{
				// Hub's Email Address change
				if (thisUserChange)
				{
					// This user Hub's Email Address change
					// Update this user description in the registry
					Registry::StoreUserDescription (newMemberInfo.Description ());
					// Change Hub's Email Address key in the project description
					_catalog.RefreshProjMemberHubId (_project.GetCurProjectId (), newHubId);
				}
				else
				{
					Assert (isProjectAdmin);
				}
				addressChange.reset (new AddressChangeCmd (curHubId,
														   userId,
														   newHubId,
														   userId));
			}
			Assert (addressChange.get () != 0);
		}

		{
			// Transaction scope
			Transaction xact (*this, _pathFinder);

			XCheckinMembershipChange (newMemberInfo, ackBox, std::move(addressChange));
			if (isProjectAdmin && !thisUserChange)
			{
				// I'm the project admin and I'm changing some other project member.
				MemberState newState = newMemberInfo.State ();
				if (newState.IsDead () || newState.IsObserver ())
					_history.XRemoveFromAckList (newMemberInfo.Id (), newState.IsDead (), ackBox);
				if (newState.IsDead ())
				{
					_history.XCleanupMemberTree (newMemberInfo.Id ());
					GidList emptyList;
					_checkedOutFiles.XUpdate(emptyList, newMemberInfo.Id ());
				}
			}

			xact.Commit ();
		}
	}

	SendAcknowledgments (ackBox);
}

void Model::ProjectNewAdmin (UserId newAdmin)
{
	Assert (newAdmin != gidInvalid);
	UserId curAdmin = _dataBase.GetAdminId ();
	Assert (newAdmin != curAdmin);
	AckBox ackBox;

	{
		// Transaction scope
		Transaction xact (*this, _pathFinder);

		if (curAdmin != gidInvalid)
		{
			StateObserver observer (_dataBase.XGetMemberState (curAdmin));
			XCheckinStateChange (curAdmin, observer, ackBox);
		}

		StateAdmin admin (_dataBase.XGetMemberState (newAdmin));
		XCheckinStateChange (newAdmin, admin, ackBox);

		xact.Commit ();
	}

	SendAcknowledgments (ackBox);
}

bool Model::ExportHistory (std::string const & tgtFolder, Progress::Meter & progressMeter) const
{
	Assert (!IsQuickVisit ());
	return _history.Export (tgtFolder, progressMeter);
}

std::unique_ptr<ExportedHistoryTrailer> Model::RetrieveHistoryTrailer (std::string const & srcPath) const
{
	FileDeserializer in (srcPath);
	File::Size fileSize = in.GetSize ();
	// We start by reading the signature from the exported history file
	File::Offset startOffset = LargeInteger (fileSize.ToMath () - Serializer::SizeOfLong ());
	in.SetPosition (startOffset);
	unsigned long signature = in.GetLong ();
	SerFileOffset trailerStart;
	if (signature == 'EXHT')
	{
		// Old format:
		// long trailerStart, long signature
		startOffset = LargeInteger (fileSize.ToMath () - 2 * Serializer::SizeOfLong ());
		in.SetPosition (startOffset);
		trailerStart = File::Offset (in.GetLong (), 0);
	}
	else if (signature == 'EXHL')
	{
		// Large format:
		// long version, File::Offset trailerStart, long signature
		CountingSerializer count;
		trailerStart.Serialize (count);
		startOffset = LargeInteger (fileSize.ToMath () - count.GetSize () - 2 * Serializer::SizeOfLong ());
		in.SetPosition (startOffset);
		long version = in.GetLong ();
		trailerStart.Deserialize (in, version);
	}
	else
		throw Win::InternalException ("Cannot import history - corrupted file.", srcPath.c_str ());

	in.SetPosition (trailerStart);
	std::unique_ptr<ExportedHistoryTrailer> trailer (new ExportedHistoryTrailer (in));
	return trailer;
}

void Model::XDoImportHistory (ExportedHistoryTrailer const & trailer,
							  std::string const & historyFilePath,
							  Progress::Meter & meter,
							  bool isNewProjectFromHistory)
{
	GidSet affectedFileIds;
	_history.XImport (trailer,
					  historyFilePath,
					  affectedFileIds,
					  meter,
					  isNewProjectFromHistory);

	if (!affectedFileIds.empty ())
	{
		// Remove the files we know about
		_dataBase.XRemoveKnownFilesFrom (affectedFileIds);
		if (!affectedFileIds.empty ())
		{
			// Import the files we don't know about
			meter.SetActivity ("Preparing file database for import.");
			meter.StepAndCheck ();
			std::vector<FileData> fileData;
			_history.XRetrieveFileData (affectedFileIds, fileData);
			// Set appropriate file state
			for (std::vector<FileData>::iterator fileIter = fileData.begin ();
				 fileIter != fileData.end ();
				 ++fileIter)
			{
				FileState state;	// Default is none
				if (isNewProjectFromHistory && !fileIter->GetState ().IsNone ())
					state = StateCheckedIn ();	

				fileIter->SetState (state);
			}
			Assert (affectedFileIds.empty ());
			Assert (!fileData.empty ());
			meter.SetActivity ("Importing file data.");
			meter.StepAndCheck ();
			_dataBase.XImportFileData (fileData.begin (), fileData.end ());
			meter.StepAndCheck ();
		}
	}
}

void Model::ImportHistory (std::string const & srcPath, Progress::Meter & progressMeter)
{
	std::unique_ptr<ExportedHistoryTrailer> trailer = RetrieveHistoryTrailer (srcPath);
	progressMeter.SetRange (0, trailer->GetScriptCount () + 10);
	progressMeter.SetActivity ("Verifying exported history file.");
	progressMeter.StepAndCheck ();
	if (_history.VerifyImport (*trailer))
	{
		progressMeter.StepAndCheck ();

		Transaction xact (*this, _pathFinder);

		XDoImportHistory (*trailer, srcPath, progressMeter);

		xact.Commit ();
	}
}

void Model::XCheckinMembershipChange (MemberInfo const & updateInfo, 
									  AckBox & ackBox,
									  std::unique_ptr<DispatcherCmd> attachment)
{
	XMembershipChange membershipChange (updateInfo, _dataBase.XGetProjectDb ());
	if (membershipChange.ChangesDetected ())
	{
		membershipChange.UpdateProjectMembership ();
		std::unique_ptr<CheckOut::List> notification = XGetCheckoutNotification ();
		membershipChange.BuildScript (_history);
		membershipChange.StoreInHistory (_history, ackBox);
		membershipChange.Broadcast (_mailer, std::move(attachment), notification.get ());
	}
	if (updateInfo.Id () == _dataBase.XGetMyId ())
		_userPermissions.XRefreshProjectData (_dataBase.XGetProjectDb (), &_pathFinder);
}

void Model::XBroadcastConversionMembershipUpdate (AckBox & ackBox)
{
	bool doBroadcast = _dataBase.XScriptNeeded (true);	// Membership change
	Project::Db & projectDb = _dataBase.XGetProjectDb ();
	UserId userId = projectDb.XGetMyId ();
	GlobalId conversionScriptId = _dataBase.XMakeScriptId ();
	UserId adminId = projectDb.XGetAdminId ();
	// My current info contains correct most recent script id marker -- XMakeScriptId updated it
	std::unique_ptr<MemberInfo> myCurrentInfo = projectDb.XRetrieveMemberInfo (userId);
	Assert (myCurrentInfo.get () != 0);
	MemberState state = myCurrentInfo->State ();
	// After conversion I'm the verified project member
	state.SetVerified (true);
	if (adminId == gidInvalid && !state.IsObserver ())
	{
		// Project doesn't have administrator (at least to my best knowledge).
		// Since I'm not the project observer I'll make myself the project administrator.
		state.MakeAdmin ();
	}
	myCurrentInfo->SetState (state);
	projectDb.XChangeState (userId, state);
	MemberNameTag tag (myCurrentInfo->Name (), userId);
	MembershipUpdateComment comment (tag, "upgrades to Code Co-op version 4.5");
	ScriptHeader hdr (ScriptKindAddMember (), userId, projectDb.XProjectName ());
	hdr.SetScriptId (conversionScriptId);
	hdr.AddComment (comment);
	CommandList cmdList;
	std::unique_ptr<ScriptCmd> cmd (new NewMemberCmd (*myCurrentInfo));
	cmdList.push_back (std::move(cmd));
	// Don't include side lineages with conversion membership update.
	// The main script lineage contains only reference id set to gidInvalid
	// because the conversion membership update is the very first script in
	// the membership change history of that user.
	Lineage mainLineage;
	mainLineage.PushId (gidInvalid);
	hdr.AddMainLineage (mainLineage);
	if (doBroadcast)
		_mailer.XBroadcast (hdr, cmdList, 0); // no checkout notifications
	_history.XAddCheckinScript (hdr, cmdList, ackBox);
}

void Model::UpdateEnlistmentAddress (std::string const & newHubId)
{
	Assert (!newHubId.empty ());
	UserId thisUserId = _dataBase.GetMyId ();
	std::unique_ptr<MemberDescription>
		oldDescription = _dataBase.RetrieveMemberDescription (thisUserId);
	if (newHubId == oldDescription->GetHubId ())
		return;

	MemberDescription newDescription (*oldDescription);
	newDescription.SetHubId (newHubId.c_str ());
	MemberState thisUserState = _dataBase.GetMemberState (thisUserId);
	MemberInfo newUserInfo (thisUserId, thisUserState, newDescription);
	AckBox ackBox;

	{
		// Transaction scope
		Transaction xact (*this, _pathFinder);

		// Add address change command to the script
		std::unique_ptr<DispatcherCmd> addendum (new AddressChangeCmd (oldDescription->GetHubId (),
																	oldDescription->GetUserId (),
																	newDescription.GetHubId (),
																	newDescription.GetUserId ()));
		XCheckinMembershipChange (newUserInfo, ackBox, std::move(addendum));

		xact.Commit ();
	}

	SendAcknowledgments (ackBox);
}

void Model::ListControlledFiles (GlobalId folderGid, NocaseSet & files)
{
	Assert (folderGid != gidInvalid);
	GidList contents;
	SimpleMeter meter (_uiFeedback);
	meter.SetActivity ("Building project file list");
	// Recursively list project folder contents
	_dataBase.ListFolderContents (folderGid, contents);
	meter.SetRange (0, contents.size (), 1);
	for (GidList::const_iterator iter = contents.begin (); iter != contents.end (); ++iter)
	{
		meter.StepIt ();
		GlobalId gid = *iter;
		char const * path = _pathFinder.GetFullPath (gid, Area::Project);
		files.insert (path);
	}
}

// Returns true when valid license provided
bool Model::PromptForLicense (LicensePrompter & prompter)
{
	License license (_userPermissions.GetLicenseString ());
	DistributorLicense distribLicense (_catalog);
	if (prompter.Query (license, _userPermissions.GetTrialDaysLeft (), distribLicense))
	{
		if (prompter.IsNewLicense ())
		{
			License currentLicense (_userPermissions.GetLicenseString ());
			License newLicense (prompter.GetNewLicense ());
			if (!newLicense.IsValid () || !newLicense.IsValidProduct ())
			{
				TheOutput.Display ("This license is not valid for the current product.");
				return false;
			}

			if (_userPermissions.IsBetterOrDifferentLicense (newLicense))
			{
				// More seats for the current licensee or
				// the new licensee
				if ((_userPermissions.IsChangedLicensee (newLicense) 
						|| _userPermissions.IsChangedProduct (newLicense))
					&& !_userPermissions.IsGlobalLicenseeEmpty ())
				{
					std::string info ("The new license has different licensee name "
									  "or product type than the current license.\n\n"
									  "New license: ");
					info += newLicense.GetDisplayString ();
					info += "\n\nCurrent license: ";
					info += currentLicense.GetDisplayString ();
					info += "\n\nAre you sure that you want to use the new license in all projects?";
					Out::Answer userChoice = TheOutput.Prompt (info.c_str (),
															   Out::PromptStyle (Out::YesNo,
																				 Out::No,
																				 Out::Question),
															   TheAppInfo.GetWindow ());
					if (userChoice == Out::No)
						return false;
				}

				// We have the new license -- enter it into the catalog
				_userPermissions.SetLicense (_catalog, newLicense);
				if (IsInProject () && IsProjectReady ())
				{
					// Enter it into the local project database and refresh user permissions
					ChangeUserLicense (prompter.GetNewLicense ());
					_userPermissions.RefreshProjectData (_dataBase.GetProjectDb (), _pathFinder);
				}
				return true;
			}
			else if (!currentLicense.IsEqual (prompter.GetNewLicense ()))
			{
				// Less seats for the current licensee
				Assert (!_userPermissions.IsBetterOrDifferentLicense (newLicense) &&
						!_userPermissions.IsChangedProduct (newLicense) &&
					    !_userPermissions.IsChangedLicensee (newLicense)); 
				std::string info ("You already have a better license "
								  "than the new one.\n\nCurrent license: ");
				info += currentLicense.GetDisplayString ();
				info += "\n\nNew license: ";
				info += newLicense.GetDisplayString ();
				info += "\n\nThe new license has been ignored.";
				TheOutput.Display (info.c_str ());
			}
		}
	}
	return false;
}

std::string Model::GetLicenseDisplayString () const
{
	if (_userPermissions.HasValidLicense ())
	{
		// Get license version and seats
		License license (_userPermissions.GetLicenseString ());
		if (license.IsValid ())
		{
			// The user has valid global license
			return license.GetDisplayString ();
		}
		else
		{
			// Display local project license
			License license (_userPermissions.GetProjectLicenseString ());
			return license.GetDisplayString ();
		}
	}
	else if (_userPermissions.IsTrial ())
	{
		std::string msg ("Evaluation copy -- ");
		msg += ToString (_userPermissions.GetTrialDaysLeft ());
		msg += " trial day(s) left.";
		return msg;
	}
	else
	{
		return "Expired evaluation copy.";
	}
}

void Model::VerifyLegalStatus ()
{
	if (!IsProjectReady ())
		return;

	// Check this user license
	if (_userPermissions.NeedsLocalLicenseUpdate ())
	{
		License globalLicense (_catalog);
		ChangeUserLicense (globalLicense.GetLicenseString ());
		_userPermissions.RefreshProjectData (_dataBase.GetProjectDb (), _pathFinder);
	}

	if (!IsQuickVisit () && _userPermissions.ForceToObserver ())
	{
		UserId thisUserId = _dataBase.GetMyId ();
		MemberState curState = _dataBase.GetMemberState (thisUserId);
		if (curState.IsVotingMember ())
		{
			StateObserver newState (curState);
			if (curState.IsAdmin ())
			{
				AdminElection election (_dataBase.GetProjectDb ());
				if (election.IsNewAdminElected ())
				{
					ProjectNewAdmin (election.GetNewAdmin ().Id ());
				}
				else
				{
					ChangeMemberState (thisUserId, newState);
				}
			}
			else
			{
				ChangeMemberState (thisUserId, newState);
			}

			std::string info ("Your Code Co-op trial period has just ended. ");
			if (_userPermissions.IsReceiver ())
			{
				info += "If the project administrator doesn't send you a\n"
					    "valid license you will be removed from the project ";
				info += _dataBase.ProjectName ();
			}
			else
			{
				info += "You must purchase a license to continue\n"
						"using the full-functionality after 31-days, otherwise you will become an Observer with\n"
						"limited functionality.";
			}
			TheOutput.Display (info.c_str ());
		}
	}
}

void Model::ChangeUserLicense (std::string const & newLicense)
{
	bool makeVotingMember = false;
	UserId thisUserId = _dataBase.GetMyId ();
	MemberState thisUserState = _dataBase.GetMemberState (thisUserId);
	if (!IsQuickVisit () && thisUserState.IsObserver ())
	{
		// The user is a project observer and entered license.
		// Ask if he/she wants to change state to voting member.
		std::string msg ("You are an \"observer\" in the project \"");
		msg += GetProjectName ();
		msg += "\"\n\n"
				"Do you want to change your status to \"voting member\"?\n\n"
				"(You can always change your status later, in the Project Members dialog.)";
		Out::Answer userChoice = TheOutput.Prompt (msg.c_str ());
		makeVotingMember = (userChoice == Out::Yes);
	}

	AckBox ackBox;

	{
		// Transaction scope
		Transaction xact (*this, _pathFinder);

		// Store New license in the database and in the control script
		std::unique_ptr<MemberDescription> thisUser = _dataBase.XRetrieveMemberDescription (thisUserId);
		MemberDescription newDescription (thisUser->GetName (),
										thisUser->GetHubId (),
										thisUser->GetComment (),
										newLicense,
										thisUser->GetUserId ());
		if (makeVotingMember)
			thisUserState.MakeVotingMember ();

		MemberInfo update (thisUserId, thisUserState, newDescription);
		XCheckinMembershipChange (update, ackBox);

		xact.Commit ();
	}

	SendAcknowledgments (ackBox);
}

void Model::PropagateLocalLicense ()
{
	_userPermissions.PropagateLocalLicense (_catalog);
}

void Model::XCheckinStateChange (UserId userId, MemberState newState, AckBox & ackBox)
{
	std::unique_ptr<MemberDescription> thisUser = _dataBase.XRetrieveMemberDescription (userId);
	MemberInfo update (userId, newState, *thisUser);
	XCheckinMembershipChange (update, ackBox);
}

void Model::ChangeMemberState (UserId userId, MemberState newState)
{
	Assert (userId != gidInvalid);
	AckBox ackBox;
	{
		Transaction xact (*this, _pathFinder);
		XCheckinStateChange (userId, newState, ackBox);
		xact.Commit ();
	}
	SendAcknowledgments (ackBox);
}

void Model::BuildProjectInfrastructure (Project::Data & projData, PathFinder & pathFinder)
{
	pathFinder.SetProjectDir (projData);
	pathFinder.CreateDirs ();
	_userPermissions.TestProjectLockConsistency (pathFinder);
	// Create New transaction switch file
	SwitchFile	newSwitch (pathFinder);
}

void Model::UpdateCatalog (Project::Data & projData,
							MemberDescription const & user)
{
	// assigns new project id
	_catalog.RememberNewProject (projData, user);
	Registry::StoreUserDescription (user);
}

void Model::NotifyDispatcher ()
{
	Assert (_dataBase.GetMyId () != gidInvalid);
	// Create Dispatcher script
	std::unique_ptr<MemberDescription> thisUser = _dataBase.RetrieveMemberDescription (_dataBase.GetMyId ());
	DispatcherScript dispatcherScript;
	Transport myPublicInboxTransport = _catalog.GetActiveIntraClusterTransportToMe ();
	std::unique_ptr<DispatcherCmd> addMember (new AddMemberCmd (thisUser->GetHubId(),
															  thisUser->GetUserId (),
															  myPublicInboxTransport));
	dispatcherScript.AddCmd (std::move(addMember));

	// Create transport header -- note we don't add recipient list,
	// because Code Co-op doesn't know it. If there is a need for
	// a script, Dispatcher will know who the recipients are.
	TransportHeader txHdr;
	txHdr.AddScriptId (_dataBase.GetRandomId ());
	Address sender (_catalog.GetHubId (), _dataBase.ProjectName (), "Dispatcher");
	txHdr.AddSender (sender);
	DispatcherProxy dispatcher;
	dispatcher.Notify (txHdr, dispatcherScript);
}
