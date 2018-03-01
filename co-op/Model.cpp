//----------------------------------
// (c) Reliable Software 1997 - 2009
//----------------------------------
#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "Model.h"
#include "NewFile.h"
#include "Transformer.h"
#include "FileList.h"
#include "FileTrans.h"
#include "Visitor.h"
#include "DirTraversal.h"
#include "PhysicalFile.h"
#include "AppInfo.h"
#include "RecordSets.h"
#include "OutputSink.h"
#include "CoopMemento.h"
#include "ProjectData.h"
#include "ScriptHeader.h"
#include "ScriptCommandList.h"
#include "ScriptRecipients.h"
#include "TransportHeader.h"
#include "DispatcherScript.h"
#include "MultiLine.h"
#include "FeedbackMan.h"
#include "FileStateList.h"
#include "FileTyper.h"
#include "CmdLineSelection.h"
#include "IpcExchange.h"
#include "Workspace.h"
#include "ScriptProps.h"
#include "FileProps.h"
#include "FileChanges.h"
#include "Table.h"
#include "Registry.h"
#include "ProgramOptions.h"
#include "ToolOptions.h"
#include "AltDiffer.h"
#include "CmdLineMaker.h"
#include "MemoryLog.h"
#include "BuildOptions.h"
#include "Diagnostics.h"
#include "DispatcherProxy.h"
#include "HistoryRange.h"
#include "EmailRegistry.h"
#include "EmailConfig.h"
#include "RegKeys.h"
#include "PathRegistry.h"
#include "MergerProxy.h"
#include "EmailConfig.h"
#include "VersionInfo.h"
#include "AckBox.h"
#include "ProjectMarker.h"
#include "FileCopyist.h"
#include "FileDataSequencer.h"
#include "LineBuf.h"
#include "ScriptBasket.h"

#include <File/Dir.h>
#include <File/WildCard.h>
#include <Com/Shell.h>
#include <Com/ShellRequest.h>
#include <Com/DragDrop.h>
#include <Ex/WinEx.h>
#include <Sys/Clipboard.h>
#include <Sys/Process.h>
#include <Dbg/Out.h>
#include <Ex/Error.h>
#include <Sys/SysVer.h>
#include <XML/XmlTree.h>
#include <StringOp.h>

#include <fstream>

//
// TableProvider
//

std::unique_ptr<RecordSet> Model::Query (Table::Id tableId, Restriction const & restrict)
{
	if (tableId == Table::projectTableId)
	{
		if (IsProjectCatalogEmpty ())
		{
			_dummy.Init ("There are no projects on this computer. Select \"New Project\" or \"Join Project\" from the Project menu to create or join one");
			return std::unique_ptr<RecordSet> (new EmptyRecordSet (_dummy));
		}
		else
		{
			return std::unique_ptr<RecordSet> (new ProjectRecordSet (_project, restrict));
		}
	}
	else if (IsProjectReady ())
	{
		switch (tableId)
		{
		case Table::WikiDirectoryId:
			return std::unique_ptr<RecordSet> (new WikiRecordSet (_wikiDirectory));
		case Table::folderTableId:
			return std::unique_ptr<RecordSet> (new FolderRecordSet (_directory, restrict));
		case Table::checkInTableId:
			return std::unique_ptr<RecordSet> (new CheckInRecordSet (_checkInArea, restrict));
		case Table::mailboxTableId:
			return std::unique_ptr<RecordSet> (new MailBoxRecordSet (_mailBox, restrict));
		case Table::synchTableId:
			return std::unique_ptr<RecordSet> (new SynchRecordSet (_synchArea, restrict));
		case Table::historyTableId:
			return std::unique_ptr<RecordSet> (new HistoryRecordSet (_history, restrict));
		case Table::scriptDetailsTableId:
			return std::unique_ptr<RecordSet> (new ScriptDetailsRecordSet (_historicalFiles, restrict));
		case Table::mergeDetailsTableId:
			return std::unique_ptr<RecordSet> (new MergeDetailsRecordSet (_mergedFiles, restrict));
		case Table::emptyTableId:
			Assert (IsQuickVisit ());
			_dummy.Init ("Executing external command -- please wait");
			return std::unique_ptr<RecordSet> (new EmptyRecordSet (_dummy));
		default:
			Assert (!"Query -- Unknown table name");
			return std::unique_ptr<RecordSet>();
		}
	}

	Assert (!IsProjectReady ());
	if (IsQuickVisit ())
	{
		_dummy.Init ("Executing external command -- please wait");
	}
	else if (_pathFinder.IsInProject ())
	{
		// In the project awaiting full sync
		if (tableId == Table::mailboxTableId)
			return std::unique_ptr<RecordSet> (new MailBoxRecordSet (_mailBox, restrict));

		if (_dataBase.GetMyId () == gidInvalid || !_history.HasNextScript ())
		{
			_dummy.Init ("Wait for the full sync script from the project administrator.");
		}
		else
		{
			Assert (!_history.IsFullSyncExecuted () && _history.HasNextScript ());
			_dummy.Init ("Execute the full sync script received from the project administrator. Switch to the mailbox view and select \"Sync Next Script\" from the Selection menu.");
		}
	}
	else
	{
		// Not in any project
		if (IsProjectCatalogEmpty ())
			_dummy.Init ("There are no projects on this computer. Select \"New Project\" or \"Join Project\" from the Project menu to create or join one");
		else
			_dummy.Init ("No project is currently open. Select \"Visit\" from the Project menu to work with an existing project (or \"New Project\" to create one)");
	}
	return std::unique_ptr<RecordSet> (new EmptyRecordSet (_dummy));
}

bool Model::SupportsHierarchy (Table::Id tableId) const
{
	return tableId == Table::folderTableId && _pathFinder.IsInProject () && IsProjectReady ();
}

std::string Model::QueryCaption (Table::Id tableId, Restriction const & restrict) const
{
	switch (tableId)
	{
	case Table::projectTableId:
		return _project.GetCaption (restrict);
	case Table::folderTableId:
		return _directory.GetCaption (restrict);
	case Table::checkInTableId:
		return _checkInArea.GetCaption (restrict);
	case Table::mailboxTableId:
		return _mailBox.GetCaption (restrict);
	case Table::synchTableId:
		return _synchArea.GetCaption (restrict);
	case Table::historyTableId:
		return _history.GetCaption (restrict);
	case Table::scriptDetailsTableId:
		return _historicalFiles.GetCaption (restrict);
	case Table::mergeDetailsTableId:
		return _mergedFiles.GetCaption (restrict);
	case Table::WikiDirectoryId:
		return _wikiDirectory.GetCaption (restrict);
	case Table::emptyTableId:
		return _dummy.GetCaption (restrict);
	default:
		Assert (!"QueryCaption -- Unknown table name");
		return 0;
	}
}

bool Model::IsEmpty (Table::Id tableId) const
{
	switch (tableId)
	{
	case Table::checkInTableId:
		return _checkInArea.IsEmpty ();
	case Table::mailboxTableId:
		return !_history.HasIncomingOrMissingScripts () &&
			   !_mailBox.HasScripts () &&
			   !_mailBox.HasExecutableJoinRequest ();
	case Table::synchTableId:
		return _synchArea.IsEmpty ();
	case Table::projectTableId:
		return _project.IsEmpty ();
	case Table::scriptDetailsTableId:
		return _historicalFiles.IsEmpty ();
	case Table::mergeDetailsTableId:
		return _mergedFiles.IsEmpty ();
	case Table::emptyTableId:
		return true;
	}
	return false; 
}

DegreeOfInterest Model::HowInteresting (Table::Id tableId) const
{
	if (tableId == Table::projectTableId)
	{
		return _project.HowInteresting ();
	}
	else if (IsProjectReady ())
	{
		switch (tableId)
		{
		case Table::checkInTableId:
			return _checkInArea.IsEmpty () ? NotInteresting : Interesting;
		case Table::mailboxTableId:
			if (_history.HasIncomingOrMissingScripts () || _mailBox.HasExecutableJoinRequest ())
				return VeryInteresting;
			else if (_mailBox.HasScripts ())
				return Interesting;
			else
				return NotInteresting;
		case Table::synchTableId:
			return _synchArea.IsEmpty () ? NotInteresting : VeryInteresting;
		}
	}
	return NotInteresting;
}

Table const & Model::GetTable (Table::Id tableId) const
{
	switch (tableId)
	{
	case Table::projectTableId:
		return _project;
	case Table::folderTableId:
		return _directory;
	case Table::checkInTableId:
		return _checkInArea;
	case Table::mailboxTableId:
		return _mailBox;
	case Table::synchTableId:
		return _synchArea;
	case Table::historyTableId:
		return _history;
	case Table::scriptDetailsTableId:
		return _historicalFiles;
	case Table::mergeDetailsTableId:
		return _mergedFiles;
	case Table::emptyTableId:
		return _dummy;
	}
	Assert (!"GetTable -- Unknown table name");
	return _dummy;
}

//
// Model
//

Model::Model (bool quickVisit, char const * catPath)
	: _catalog (false, catPath),
	  _activityLog (_catalog.GetLogsPath ()),
	  _needRefresh (false),
	  _pathFinder (_dataBase),
	  _watcher (new MultiFolderWatcher (std::vector<std::string> (), TheAppInfo.GetWindow ())),
      _directory (_dataBase, _checkedOutFiles, *_watcher),
      _checkInArea (_dataBase, _pathFinder, _catalog, _project, _checkedOutFiles),
	  _history (_dataBase.XGetProjectDb ()),
	  _sidetrack (_dataBase.XGetProjectDb (), _pathFinder),
	  _mailBox (_dataBase.XGetProjectDb (), _history, _sidetrack),
      _synchArea (_dataBase, _pathFinder),
	  _project (_catalog),
	  _mailer (_dataBase.XGetProjectDb (), _catalog),
	  _userPermissions (_catalog),
	  _changeCounter (0),
	  _uiFeedback (0),
	  _quickVisit (quickVisit),
	  _isConverted (false),
	  _wikiDirectory (*_watcher, _pathFinder, _catalog.GetWikiPath ().GetDir ()),
	  _historicalFiles (_history, _pathFinder, _catalog),
	  _mergedFiles (_history, _pathFinder, _catalog)
{
	//	make worker thread death synchronous so we can catch memory leaks
	_watcher.SetWaitForDeath (10*1000);

	AddTransactableMember (_mailBox);
	AddTransactableMember (_dataBase);
	AddTransactableMember (_synchArea);
	AddTransactableMember (_history);
	AddTransactableMember (_sidetrack);
	AddTransactableMember (_checkedOutFiles);

	_table.push_back (&_project);
	_table.push_back (&_directory);
	_table.push_back (&_checkInArea);
	_table.push_back (&_synchArea);
	_table.push_back (&_history);
}

bool Model::IsWikiFolder () const
{
	// Revisit: we might want to store wiki attribute in project DB
	if (IsInProject ())
	{
		return _directory.IsWiki ();
	}
	return false;
}

bool Model::IsProjectCatalogEmpty ()
{
	ProjectSeq seq (_catalog);
	return seq.AtEnd ();
}

std::unique_ptr<HistoryChecker> Model::GetHistoryChecker () const
{
	std::unique_ptr<HistoryChecker> checker (new HistoryChecker (_history));
	return checker;
}

void Model::RetrieveAdminData (std::string & projectName,
							   std::string & adminHubId, 
							   Transport & adminHubTransport)
{
	projectName = GetProjectName ();
	std::unique_ptr<MemberDescription> admin = RetrieveUserDescription (GetAdminId ());
	adminHubId = admin->GetHubId ();
	Catalog & cat = GetCatalog ();
	if (IsNocaseEqual (admin->GetHubId (), cat.GetHubId ()))
	{
		// Admin is in this cluster
		adminHubTransport = cat.GetHubRemoteActiveTransport ();
	}
	else
	{
		// Admin is remote
		adminHubTransport = cat.GetInterClusterTransport (admin->GetHubId ());
	}
}

std::unique_ptr<MemberDescription> Model::RetrieveUserDescription (UserId userId) const
{
	std::unique_ptr<MemberDescription> user;
	if (_dataBase.GetMyId () != gidInvalid)
	{
		// Display information stored in the project database
		user = _dataBase.RetrieveMemberDescription (userId);
	}
	else
	{
		// Display information stored in the registry
		user.reset (new CurrentMemberDescription);
	}
	return user;
}

void Model::RetrieveNextVersionInfo (VersionInfo & info) const
{
	Assert (_history.HasNextScript ());
	_history.RetrieveNextVersionInfo (info);
}

bool Model::RetrieveVersionInfo (GlobalId scriptId, VersionInfo & info, bool fromMailbox) const
{
	if (fromMailbox)
	{
		if (_mailBox.IsScriptFilePresent (scriptId))
			return _mailBox.RetrieveVersionInfo (scriptId, info);
		else if (!_history.RetrieveVersionInfo (scriptId, info))
			info.SetComment("Missing script: Comment not available");
		return true;
	}

	return _history.RetrieveVersionInfo (scriptId, info);
}

std::unique_ptr<Memento> Model::CreateMemento ()
{
	return std::unique_ptr<CoopMemento>(new CoopMemento (GetProjectId ()));
}

bool Model::RevertTo (Memento const & memento)
{
	CoopMemento const & coopMemento = static_cast<CoopMemento const &>(memento);

	if (coopMemento.GetOldProjectId () != -1)
	{
		_project.Enter (coopMemento.GetOldProjectId ());
		_mergedFiles.SetThisProjectId (coopMemento.GetOldProjectId ());
		Project::Data projData;
		_catalog.GetProjectData (coopMemento.GetOldProjectId (), projData);
		ReadProjectDb (projData);
		return true;
	}
	else
	{
		// If no project to return to
		_project.Exit ();
	}
	return false;
}

std::unique_ptr<VerificationReport> Model::VerifyProject ()
{
	std::unique_ptr<VerificationReport> report;
	if (!IsQuickVisit ())
	{
		SimpleMeter meter (_uiFeedback);
	    meter.SetRange (0, 2 * _dataBase.FileCount ());
		report = VerifyProject (meter);
	}
	return report;
}

void Model::QuietVerifyProject ()
{
	SimpleMeter meter (_uiFeedback);
	(void) VerifyProject (meter);
}

// Returns true if project root folder contains at least one controlled file/folder
bool Model::QuickProjectVerify ()
{
	FileSeq fileSeq (_pathFinder.GetProjectDir ());
	if (fileSeq.AtEnd ())
		return false;	// Project root folder is empty

	for ( ; !fileSeq.AtEnd (); fileSeq.Advance ())
	{
		UniqueName uname (_dataBase.GetRootId (), fileSeq.GetName ());
		FileData const * fd = _dataBase.FindProjectFileByName (uname);
		if (fd != 0)
			return true;	// Found controlled file in the project root folder
	}

	// No controlled files/folders found in the project root folder
	return false;
}

std::unique_ptr<VerificationReport> Model::VerifyProject (Progress::Meter & meter)
{
	if (_dataBase.GetMyId () == gidInvalid)
		return std::unique_ptr<VerificationReport>();

	Progress::Meter blindMeter;
	// Verify Project Staging Area for detection of
	// any previous aborted transactions
	StagingCleanup stagingCleanup (_pathFinder);
	StagingVerifier stagingVerifier (_dataBase, _pathFinder, stagingCleanup.MustRedo ());
	Traversal stagingTraversal (_pathFinder, Area::Staging, stagingVerifier, blindMeter);
	stagingCleanup.Commit ();
	meter.StepIt ();

	std::string info ("Checking Files On Disk (");
	info += ::ToString (_dataBase.FileCount ());
	info += " files)";
	meter.SetActivity (info.c_str ());
	std::unique_ptr<VerificationReport> report = _dataBase.Verify (_pathFinder, meter);
	GlobalId rootId = _dataBase.GetRootId ();
	meter.StepIt ();
	// Verify Project Area
	meter.SetActivity ("Verifying Project State");
	ProjectVerifier projVerifier (_dataBase, _pathFinder, meter);
	Traversal projTraversal (_pathFinder.ProjectDir (), rootId, projVerifier, blindMeter);
	meter.StepIt ();
	// Verify Original Area
	OriginalVerifier orgVerifier (_dataBase, _pathFinder);
	Traversal orgTraversal (_pathFinder, Area::Original, orgVerifier, blindMeter);
	meter.StepIt ();
	OriginalBackupVerifier orgBackupVerifier (_dataBase, _pathFinder);
	Traversal orgBackupTraversal (_pathFinder, Area::OriginalBackup, orgBackupVerifier, blindMeter);
	meter.StepIt ();
	// Verify PreSynch Area
	PreSynchVerifier backupVerifier (_dataBase, _pathFinder);
	Traversal preSynchTraversal (_pathFinder, Area::PreSynch, backupVerifier, blindMeter);
	meter.StepIt ();
	// Verify Synch Area
	SynchVerifier synchVerifier (_dataBase, _pathFinder, _synchArea);
	Traversal synchTraversal (_pathFinder, Area::Synch, synchVerifier, blindMeter);
	meter.StepIt ();
	_synchArea.Verify (_pathFinder);
	meter.StepIt ();
	// Verify Reference Area
	ReferenceVerifier refVerifier (_dataBase, _pathFinder, _synchArea);
	Traversal refTraversal (_pathFinder, Area::Reference, refVerifier, blindMeter);
	meter.StepIt ();
	// Verify Temporary Area
	TemporaryVerifier tmpVerifier (_dataBase, _pathFinder);
	Traversal tmpTraversal (_pathFinder, Area::Temporary, tmpVerifier, blindMeter);
	meter.StepIt ();
	// Verify Compare Area
	CompareVerifier cmpVerifier (_dataBase, _pathFinder);
	Traversal cmpTraversal (_pathFinder, Area::Compare, cmpVerifier, blindMeter);
	meter.StepIt ();
	// Verify Code Co-op system files
	_history.VerifyLogs ();
	meter.StepIt ();
	_dataBase.VerifyLogs ();
	meter.StepIt ();
	return report;
}

void Model::RepairHistory (History::Verification what, Progress::Meter & meter)
{
	meter.SetRange (0, 3);
	meter.SetActivity ("Traversing history.");
	meter.StepIt ();
	{
		// Repair transaction scope
		Transaction xact (*this, _pathFinder);
		_history.XDoRepair (what);
		xact.Commit ();
	}

	if (what == History::Membership)
	{
		meter.SetActivity ("Re-executing membership change scripts.");
		meter.StepIt ();
		AckBox dummyAckBox;
		ExecuteForcedScripts (dummyAckBox, meter);
	}
}

void Model::RepairMembership ()
{
	Transaction xact (*this, _pathFinder);
	_history.XDoRepairMembership ();
	xact.Commit ();
}

void Model::GetDeadCheckoutNotifiers (GidList & members) const
{
	Project::Db const & projectDb = GetProjectDb ();
	GidList deadMembers;
	projectDb.GetDeadMemberList (deadMembers);
	GidList checkoutNotifiers;
	_checkedOutFiles.GetNotifyingMembers (checkoutNotifiers);

	for (GidList::const_iterator iter = deadMembers.begin (); iter != deadMembers.end (); ++iter)
	{
		if (_checkedOutFiles.IsCheckedOutBy (*iter))
			members.push_back(*iter);
	}

	for (GidList::const_iterator iter = checkoutNotifiers.begin (); iter != checkoutNotifiers.end (); ++iter)
	{
		if (!projectDb.IsProjectMember (*iter))
			members.push_back(*iter);
	}
}

// Returns true when there are no checkout notifications from dead or unknown project members
bool Model::VerifyCheckoutNotifications () const
{
	GidList deadNotifiers;
	GetDeadCheckoutNotifiers (deadNotifiers); 
	return deadNotifiers.empty ();
}

void Model::RepairCheckoutNotifications ()
{
	GidList deadNotifiers;
	GetDeadCheckoutNotifiers (deadNotifiers);
	Assert (!deadNotifiers.empty());
	GidList emptyFileList;
	// Remove checkout notifications from unknow/dead project members
	//--------------------------------------------------------------------------
	Transaction xact (*this, _pathFinder);
	for (GidList::const_iterator iter = deadNotifiers.begin (); iter != deadNotifiers.end (); ++iter)
	{
		_checkedOutFiles.XUpdate (emptyFileList, *iter);
	}
	xact.Commit ();
	//--------------------------------------------------------------------------
}

void Model::SetCopyright (std::string const & copyright)
{
	Transaction xact (*this, _pathFinder);
	_dataBase.XSetCopyright (copyright);
	xact.Commit ();
}

GlobalId Model::GetCurrentVersion () const
{
	return _history.MostRecentScriptId ();
}

GlobalId Model::GetSecondLatestVersion () const
{
	return _history.GetPredecessorId (GetCurrentVersion ());
}

void Model::EditorOptions (ToolOptions::Editor const & dlgData)
{
	Registry::UserDifferPrefs prefs;
	if (dlgData.UsesExternalEditor ())
	{
		prefs.ToggleAlternativeEditor (true);
		prefs.SetAlternativeEditor (dlgData.GetExternalEditorPath (),
			dlgData.GetExternalEditorCommand ());
	}
	else
	{
		prefs.ToggleAlternativeEditor (false);
	}
}

void Model::DifferOptions (ToolOptions::Differ const & dlgData)
{
	Registry::UserDifferPrefs prefs;
	if (dlgData.UsesBc ())
	{
		std::string differPath;
		if (dlgData.UsesOriginalBc ())
			differPath = dlgData.GetOriginalBcPath (); 
		else
			differPath = dlgData.GetOurBcPath ();
		prefs.SetAlternativeDiffer (true, differPath, BCDIFFER_CMDLINE, BCDIFFER_CMDLINE2, true);
	}
	else if (dlgData.UsesGuiffy ())
	{
		prefs.SetAlternativeDiffer (true, 
									dlgData.GetGuiffyPath (), 
									GUIFFYDIFFER_CMDLINE,
									GUIFFYDIFFER_CMDLINE2,
									false);
	}
	else
	{
		// Use built-in differ/editor
		prefs.ToggleAlternativeDiffer (false);
	}
}

void Model::MergerOptions (ToolOptions::Merger const & dlgData)
{
	Registry::UserDifferPrefs prefs;
	if (dlgData.UsesBc ())
	{
		std::string mergerPath;
		if (dlgData.UsesOriginalBc ())
			mergerPath = dlgData.GetOriginalBcPath (); 
		else
			mergerPath = dlgData.GetOurBcPath ();
		prefs.SetAlternativeMerger (true, mergerPath, BCMERGER_CMDLINE, true);
		prefs.SetAlternativeAutoMerger (mergerPath, BCAUTOMERGER_CMDLINE, true);
	}
	else if (dlgData.UsesGuiffy ())
	{
		::SetupGuiffyRegistry (true, dlgData.GetGuiffyPath ());
	}
	else
	{
		// Use built-in merger
		prefs.ToggleAlternativeMerger (false);
	}

}

void Model::ProgramOptions (ProgramOptions::Data const & dlgData)
{
	if (!dlgData.ChangesDetected ())
		return;

	ProgramOptions::ChunkSize const & chunkSizeOptions = dlgData.GetChunkSizeOptions ();
	if (chunkSizeOptions.ChangesDetected ())
	{
		Email::RegConfig email;
		email.SetMaxEmailSize (chunkSizeOptions.GetChunkSize ());
		// Notify Dispatcher about chunk size change
		DispatcherProxy dispatcher;
		dispatcher.ChangeChunkSize (chunkSizeOptions.GetChunkSize ());
	}

	ProgramOptions::Resend const & resendOptions = dlgData.GetResendOptions ();
	if (resendOptions.ChangesDetected ())
	{
		Registry::UserDispatcherPrefs dispatcherPrefs;
		dispatcherPrefs.SetAutoResend (resendOptions.GetDelay (), resendOptions.GetRepeatInterval ());
	}

	ProgramOptions::ScriptConflict const & scriptConflictOptions = dlgData.GetScriptConflictOptions ();
	if (scriptConflictOptions.ChangesDetected ())
	{
		Registry::SetQuietConflictOption (scriptConflictOptions.IsResolveQuietly ());
	}

	ProgramOptions::Update const updateOptions = dlgData.GetUpdateOptions ();
	if (updateOptions.ChangesDetected ())
	{
		Registry::UserDispatcherPrefs dispatcherPrefs;
		if (updateOptions.IsAutoUpdate ())
		{
			// Turn on auto update by setting next update check to now
			Date nextCheck;
			nextCheck.Now ();
			dispatcherPrefs.SetIsConfirmUpdate (!updateOptions.IsBackgroundDownload ());
			dispatcherPrefs.SetUpdateTime (nextCheck.Year (), nextCheck.Month (), nextCheck.Day ());
			dispatcherPrefs.SetUpdatePeriod (updateOptions.GetUpdateCheckPeriod ());
		}
		else
		{
			// Turn off auto update by setting next update check to zero
			dispatcherPrefs.SetUpdateTime (0, 0, 0);
		}
	}

	ProgramOptions::Invitations const & invitations = dlgData.GetInitationOptions ();
	if (invitations.ChangesDetected ())
	{
		Registry::SetAutoInvitationOptions (invitations.IsAutoInvitation (),
										   invitations.GetProjectFolder ());
	}
}

//
// File operations
//

// Never called with merge conflict!
void Model::AddDifferPaths (XML::Tree & xmlTree, FileData const & fileData)
{
	GlobalId gid = fileData.GetGlobalId ();
	FileState state = fileData.GetState ();
	UniqueName const & officialName = fileData.GetUniqueName ();

	XML::Node * root = xmlTree.SetRoot ("diff"); // we can change it to "edit"
	int count = 0;
	bool isCurrent = false;
	if (state.IsPresentIn (Area::Project))
	{
		char const * filePath = _pathFinder.GetFullPath (gid, Area::Project);
		if (File::Exists (filePath))
		{
			isCurrent = true;
			XML::Node * fileNode = root->AddEmptyChild ("file");
			fileNode->AddTransformAttribute ("path", filePath);
			fileNode->AddAttribute ("role", "current");
			++count;
		}
	}
	
	if (state.IsRelevantIn (Area::PreSynch))
	{
		if (state.IsPresentIn (Area::PreSynch))
		{
			XML::Node * fileNode = root->AddEmptyChild ("file");
			char const * filePath = _pathFinder.GetFullPath (gid, Area::PreSynch);
			fileNode->AddTransformAttribute ("path", filePath);
			fileNode->AddAttribute ("role", "before");
			fileNode->AddAttribute ("edit", "no");
			std::string title = "[before] ";
			title += officialName.GetName ();
			fileNode->AddTransformAttribute ("display-path", title);
			++count;
		}

		if (state.IsPresentIn (Area::Synch) && !isCurrent)
		{
			XML::Node * fileNode = root->AddEmptyChild ("file");
			char const * filePath = _pathFinder.GetFullPath (gid, Area::Synch);
			fileNode->AddTransformAttribute ("path", filePath);
			fileNode->AddAttribute ("role", "after");
			fileNode->AddAttribute ("edit", "no");
			std::string title = "[after] ";
			title += officialName.GetName ();
			fileNode->AddTransformAttribute ("display-path", title);
			// already counted as project
		}
	}
	else if (state.IsPresentIn (Area::Original))
	{
		// simple check-out
		XML::Node * fileNode = root->AddEmptyChild ("file");
		char const * filePath = _pathFinder.GetFullPath (gid, Area::Original);
		fileNode->AddTransformAttribute ("path", filePath);
		fileNode->AddAttribute ("role", "before");
		fileNode->AddAttribute ("edit", "no");
		std::string title = "[before] ";
		title += officialName.GetName ();
		fileNode->AddTransformAttribute ("display-path", title);
		++count;
	}

	if (count == 1)
		root->SetName ("edit");
}

void Model::AddMergerPaths (XML::Tree & xmlTree,
						FileData const & fileData,
						std::string const & srcTitle, 
						std::string const & targetTitle,
						std::string const & ancestorTitle)
{
	XML::Node * root = xmlTree.SetRoot ("merge");
	FileState state = fileData.GetState ();
	GlobalId gid = fileData.GetGlobalId ();
	for (Area::Seq seq; !seq.AtEnd (); seq.Advance ())
	{
		Area::Location area = seq.GetArea ();
		char const * filePath = _pathFinder.GetFullPath (gid, area);
		XML::Node * child = 0;
		switch (area)
		{
		case Area::Project:
			// Special case: visual studio bug
			if ((state.IsPresentIn (Area::Project) && !File::Exists (filePath)) ||			// Project file not present on disk
				(!File::Exists (filePath) && !state.IsCoDelete () && !state.IsSoDelete ()))	// Project file not present on disk but not deleted 
			{
				std::string msg ("Code Co-op cannot find file:\n    ");
				msg += filePath;
				msg += "\nand it cannot show you edit changes.\n\n"
					"If you recently used Visual Studio to rename a\n"
					"Visual Studio solution, this problem happened because Visual Studio\n"
					"renamed this solution file without coordinating with Code Co-op.\n"
					"If this is the case, you'll find a workaround to this Visual Studio bug\n"
					"in the Code Co-op Help.  Look under 'Renaming Visual Studio solutions'\n"
					"in the Help Index.\n\n"
					"If you were not using Visual Studio, please run Project Repair";
				TheOutput.Display (msg.c_str ()); 
				throw Win::Exception ();
			}
			Assert ((state.IsPresentIn (area) && File::Exists (filePath)) ||
					(!state.IsPresentIn (area) &&
					 ((state.IsCoDelete () || state.IsSoDelete ()) && !File::Exists (filePath)) ||
					 (!state.IsCoDelete () && !state.IsSoDelete () && File::Exists (filePath))));
			child = root->AddEmptyChild ("file");
			child->AddAttribute ("role", "result");
			child->AddTransformAttribute ("path", filePath);
			break;
		case Area::Reference:	// Reference Area *.ref
			Assert ((state.IsPresentIn (area) && File::Exists (filePath)) ||
					(!state.IsPresentIn (area) && !File::Exists (filePath)));
			child = root->AddEmptyChild ("file");
			child->AddAttribute ("role", "reference");
			child->AddTransformAttribute ("path", filePath);
			child->AddTransformAttribute ("display-path", ancestorTitle);
			child->AddAttribute ("edit", "no");
			break;
		case Area::Synch:		// Synch Area *.syn
			Assert ((state.IsPresentIn (area) && File::Exists (filePath)) ||
					(!state.IsPresentIn (area) && !File::Exists (filePath)));
			child = root->AddEmptyChild ("file");
			child->AddAttribute ("role", "source");
			child->AddTransformAttribute ("path", filePath);
			child->AddTransformAttribute ("display-path", srcTitle);
			child->AddAttribute ("edit", "no");
			break;
		case Area::PreSynch:	// Project Backup Area used during script unpack *.bak
			Assert ((state.IsPresentIn (area) && File::Exists (filePath)) ||
					(!state.IsPresentIn (area) && !File::Exists (filePath)));
			child = root->AddEmptyChild ("file");
			child->AddAttribute ("role", "target");
			child->AddTransformAttribute ("path", filePath);
			child->AddTransformAttribute ("display-path", targetTitle);
			child->AddAttribute ("edit", "no");
			break;
		}
	}
}

// gid invalid -> use shell association
// gid + uname -> open file for editing
// gid alone   -> open file for diff (if there is anything to be diffed)
void Model::Open (GlobalId gid, UniqueName const * uname)
{
	if (gid != gidInvalid)
	{
		// Controlled file or folder 
		FileData const * fileData = _dataBase.GetFileDataByGid (gid);
		Assert (fileData != 0);
		FileType type = fileData->GetType ();
		if (type.IsFolder ())
		{
			OpenFolder (fileData);
		}
		else if (!type.IsRecoverable ())
		{
			TheOutput.Display ("Unrecoverable file -- contents cannot be displayed.");
		}
		else if (type.IsTextual ())
		{
			if (uname != 0)
			{
				// Open using editor (defaults to build-in Differ)
				XML::Tree xmlArgs;
				XML::Node * root = xmlArgs.SetRoot ("edit");
				XML::Node * child = root->AddEmptyChild ("file");
				child->AddTransformAttribute ("path", GetFullPath (*uname));
				child->AddAttribute ("role", "current");
				ExecuteEditor (xmlArgs);
			}
			else
			{
				// Open using Differ/Merger (defaults to build-in Differ)
				XML::Tree xmlTree;
				if (fileData->GetState ().IsMergeContent ())
				{
					AddMergerPaths (xmlTree, *fileData, "Script Changes", "Local Changes", "Common Ancestor");
					ExecuteMerger (xmlTree);
					if (fileData->GetState ().IsMergeConflict ())
					{
						// Assume that user resolved merge conflict
						Transaction xact (*this, _pathFinder);
						Transformer trans (_dataBase, gid);
						trans.SetMergeConflict (false);
						_synchArea.Notify (changeEdit, gid);
						xact.Commit ();
					}
				}
				else
				{
					AddDifferPaths (xmlTree, *fileData);
					ExecuteDiffer (xmlTree);
				}
			}
		}
		else
		{
			// Binary file
			FileState state = fileData->GetState ();
			if (!state.IsPresentIn (Area::Project))
			{
				TheOutput.Display ("Cannot display binary file deleted by incoming script.");
			}
			else
			{
				if (uname == 0 && !fileData->GetState ().IsMergeContent ())
				{
					// diffing
					ToolOptions::Differ diffOptions;
					if (diffOptions.UsesOriginalBc ()) // use original BC to differ
					{
						XML::Tree xmlTree;
						AddDifferPaths (xmlTree, *fileData);
						ExecuteDiffer (xmlTree);
						return;
					}
				}
				// open using shell associations
				char const * curPath = _pathFinder.GetFullPath (fileData->GetUniqueName ());
				ShellOpen (curPath);
			}
		}
	}
	else
	{
		// Non project file -- open using shell associations
		Assert (uname != 0);
		char const * curPath = _directory.GetFilePath (uname->GetName ().c_str ());
		ShellOpen (curPath);
	}
}

void Model::Reconstruct (Restorer & restorer)
{
	Assert (!restorer.IsReconstructed ());
	if (!restorer.IsFolder ())
	{
		//--------------------------------------------------------------------------
		TransactionFileList fileList;
		Transaction xact (*this, _pathFinder);

		XPrepareRevert (restorer, fileList);
		// Start with base history path
		XRevertPath (restorer.GetBasePath (), restorer.GetBaseArea (), fileList);
		// Files restored by the selections from the base path are still in the Reference Area
		// Revert the diff path
		XRevertPath (restorer.GetDiffPath (), restorer.GetDiffArea (), fileList);

		//--------------------------------------------------------------------------
		// DO NOT COMMIT! Transactions are to be aborted and project state restored
	}
	restorer.SetReconstructed (true);
}

void Model::OpenHistoryDiff (GidList const & files, bool isMerge)
{
	Assert (!files.empty ());
	for (GidList::const_iterator iter = files.begin (); iter != files.end (); ++iter)
	{
		GlobalId gid = *iter;
		Restorer & restorer = isMerge ? _mergedFiles.GetRestorer (gid) :
										_historicalFiles.GetRestorer (gid);

		if (restorer.IsAbsent ())
		{
			std::string info ("Nothing to show. The file\n\n");
			info += restorer.GetRootRelativePath ();
			info += "\n\ndid not exist before or after.";
			TheOutput.Display (info.c_str ());
			return;
		}

		if (!restorer.IsReconstructed ())
			Reconstruct (restorer);

		if (restorer.IsFolder ())
			return;

		if (restorer.IsTextual ())
		{
			// Revisit: there was a logic for setting "show before" flag
			XML::Tree xmlTree;
			restorer.CreateDifferArgs (xmlTree);
			ExecuteDiffer (xmlTree);
			return;
		}
		
		if (restorer.IsBinary ())
		{
			ToolOptions::Differ diffOptions;
			if (diffOptions.UsesOriginalBc ()) // use original BC to differ
			{
				XML::Tree xmlTree;
				restorer.CreateDifferArgs (xmlTree);
				ExecuteDiffer (xmlTree);
				return;
			}
		}

		std::string const & filePath = restorer.GetBinaryFilePath ();
		ShellOpen (filePath.c_str ());
	}
}

void Model::BuildHistoricalFileCopyist (FileCopyist & copyist,
										GidList const & files,
										bool useProjectRelativePath,
										bool verifyTarget,
										bool afterScriptChange,
										bool isMerge)
{
	GidList badRestoreFiles;
	for (GidList::const_iterator iter = files.begin (); iter != files.end (); ++iter)
	{
		GlobalId gid = *iter;
		Restorer & restorer = isMerge ? _mergedFiles.GetRestorer (gid) :
										_historicalFiles.GetRestorer (gid);
		if (restorer.IsFolder ())
			continue;

		if (!restorer.IsReconstructed ())
			Reconstruct (restorer);

		// Determine source area and target name
		Area::Location tmpAreaId;
		UniqueName effectiveTargetName;
		if (afterScriptChange)
		{
			if (restorer.DeletesItem ())
				continue;

			tmpAreaId = restorer.GetAfterArea ().GetAreaId ();
			effectiveTargetName = restorer.GetAfterUniqueName ();
		}
		else
		{
			if (restorer.IsCreatedByBranch ())
				continue;

			tmpAreaId = restorer.GetBeforeArea ().GetAreaId ();
			effectiveTargetName = restorer.GetBeforeUniqueName ();
		}

		// Create source full path and relative target path
		std::string srcPath = _pathFinder.GetFullPath (gid, tmpAreaId);
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

		if (verifyTarget)
		{
			FilePath targetFolder (copyist.GetTargetPath ());
			char const * path = targetFolder.GetFilePath (tgtRelPath);
			if (File::Exists (path))
			{
				char const * projectPath = _pathFinder.GetFullPath (gid, Area::Project);
				if (IsNocaseEqual (path, projectPath))
				{
					// Probable attempt to restore file - remember its gid for display purposes
					badRestoreFiles.push_back (gid);
					continue;
				}
				else if (copyist.IsOverwriteExisting ())
				{
					if (File::IsReadOnly (path))
					{
						std::string info ("Sorry, cannot overwrite because the following file:\n\n");
						info += path;
						info += "\n\nis read-only.";
						TheOutput.Display (info.c_str ());
						return;
					}
				}
				else
				{
					// Don't copy file already present at target
					continue;
				}
			}
		}

		copyist.RememberFile (srcPath, tgtRelPath);
	}

	if (badRestoreFiles.size () != 0)
	{
		// Tell the user how to restore files
		std::string info ("To restore the following ");
		info += (badRestoreFiles.size () > 1 ? "files" : "file");
		info += ":\n\n";
		for (unsigned i = 0; i < badRestoreFiles.size (); ++i)
		{
			if (i != 0)
				info += "\n";
			info += _pathFinder.GetRootRelativePath (badRestoreFiles [i]);
			if (i > 6)
			{
				info += "\n...";
				break;
			}
		}
		info += "\n\nselect them in the file view and execute Selection>Show History.\n"
			"Next, in the history view, select the version in question and execute Selection>Restore.";

		TheOutput.Display (info.c_str ());
	}
}

void Model::BuildProjectFileCopyist (FileCopyist & copyist,
									 GlobalId scriptId,
									 TmpProjectArea & tmpArea,
									 bool includeLocalEdits,
									 Progress::Meter & meter)
{
	History::Range undoRange;
	CreateUndoRange (scriptId, undoRange);

	//--------------------------------------------------------------------------
	Transaction xact (*this, _pathFinder);

	XPrepareFileDb (undoRange, tmpArea, meter, includeLocalEdits);

	// For every project file store in the copyist its source and target path.
	for (XAllProjectFilesSequencer sequencer (_dataBase); !sequencer.AtEnd (); sequencer.Advance ())
	{
		FileData const * fileData = sequencer.GetFileData ();
		GlobalId gid = fileData->GetGlobalId ();
		bool isReconstructed = tmpArea.IsReconstructed (gid);
		Area::Location areaFrom;
		if (isReconstructed)
		{
			areaFrom = tmpArea.GetAreaId ();
		}
		else
		{
			FileState state = fileData->GetState ();
			if (!state.IsPresentIn (Area::Project))
			{
				Assert (state.IsRelevantIn (Area::Original));
				continue;
			}
			areaFrom = Area::Project;
		}

		// Create source full path and relative target path
		std::string srcPath = _pathFinder.GetFullPath (gid, areaFrom);
		std::string tgtRelPath = sequencer.GetProjectRelativePath ();
		copyist.RememberFile (srcPath, tgtRelPath);
	}

	// Remember all project folders (some of which maybe empty and will not be created during file copy)
	for (XAllProjectFoldersSequencer folderSeq (_dataBase); !folderSeq.AtEnd (); folderSeq.Advance ())
	{
		copyist.RememberSourceFolder (folderSeq.GetProjectRelativePath ());
	}

	//--------------------------------------------------------------------------
	// DO NOT COMMIT! Transactions are to be aborted and project state restored
}

void Model::OpenFolder (FileData const * fileData)
{
	FileState state = fileData->GetState ();
	char const * name = fileData->GetName ().c_str ();
	char * msg;
	if (state.IsRelevantIn (Area::Synch))
	{
		// Synch changes
		if (state.IsRelevantIn (Area::Reference))
		{
			if (state.IsPresentIn (Area::Synch))
			{
				msg = "This folder has been restored by this sync.";
			}
			else
			{
				msg = "This folder will be deleted by this sync.";
			}
		}
		else
		{
			// There was nothing undone
			if (state.IsPresentIn (Area::Synch))
			{
				msg = "This is a new folder proposed by this sync.";
			}
			else
			{
				msg = "This folder will be deleted by this sync.";
			}
		}
	}
	else if (state.IsRelevantIn (Area::Original))
	{
		// Local edit changes
		if (state.IsPresentIn (Area::Original))
		{
			if (state.IsPresentIn (Area::Project))
			{
				msg = "This folder is to be checked in.";
			}
			else
			{
				msg = "This folder is to be deleted.";
			}
		}
		else
		{
			if (state.IsPresentIn (Area::Project))
			{
				msg = "New folder.";
			}
			else
			{
				msg = "This folder will be deleted.";
			}
		}
	}
	else
	{
		msg = "Cannot open folder.";
	}
	TheOutput.Display (msg);
}

FileData const * Model::GetFileData (std::string const & fullPath) const
{
	PathParser pathParser (_directory);
	UniqueName const * uname = pathParser.Convert (fullPath.c_str ());
	if (uname == 0)
		return 0;
	return _dataBase.FindProjectFileByName (*uname);
}

GlobalId Model::GetGlobalId(std::string const & fullPath) const
{
	FileData const * fileData = GetFileData(fullPath);
	if (fileData == 0)
		return gidInvalid;
	return fileData->GetGlobalId();
}

void Model::ShellOpen (char const * filePath)
{
	int errCode = ShellMan::Open (TheAppInfo.GetWindow (), filePath);
	if (errCode != -1)
	{
		std::string info;
		if (errCode == ShellMan::NoFile)
		{
			info += "The Shell returned error: File not found\n";
			info += filePath;
		}
		else if (errCode == ShellMan::NoAssoc)
		{
			info += "This file has no application associated with it\n";
			info += filePath;
		}
		else if (errCode == ShellMan::NoResource || errCode == ShellMan::NoMemory)
		{
			info == "Cannot open file due to low system resources\n"
					"Close some windows to free system resources";
		}
		else
		{
			SysMsg errInfo (errCode);
			info += "Problem opening file ";
			info += filePath;
			info += ".\nSystem tells us:\n";
			info += errInfo.Text ();
		}
		TheOutput.Display (info.c_str (), Out::Error);
	}
}

void Model::ShellSearch (char const * filePath)
{
	int errCode = ShellMan::Search (TheAppInfo.GetWindow (), filePath);
	if (errCode != -1)
	{
		std::string info;
		if (errCode == ShellMan::NoFile)
		{
			info += "Folder not found\n";
			info += filePath;
		}
		else if (errCode == ShellMan::NoResource || errCode == ShellMan::NoMemory)
		{
			info == "Cannot start Shell Search Utility due to low system resources\n"
				"Close some windows to free system resources";
		}
		else
		{
			SysMsg errInfo (errCode);
			info += "Problem searching folder ";
			info += filePath;
			info += ".\nSystem tells us:\n";
			info += errInfo.Text ();
		}
		TheOutput.Display (info.c_str (), Out::Error);
	}
}

void Model::ShellExecute (char const * progPath, char const * arguments)
{
	int errCode = ShellMan::Execute (TheAppInfo.GetWindow (), progPath, arguments);
	if (errCode != -1)
	{
		std::string info;
		if (errCode == ShellMan::NoFile)
		{
			info += "File not found.\n";
			info += progPath;
		}
		else if (errCode == ShellMan::NoResource || errCode == ShellMan::NoMemory)
		{
			info += "Cannot execute program:\n";
			info += progPath;
			info += "\ndue to low system resources. "
					"Close some windows to free system resources.";
		}
		else
		{
			SysMsg errInfo (errCode);
			info += "Problem executing program ";
			info += progPath;
			info += ".\nSystem tells us:\n";
			info += errInfo.Text ();
		}
		TheOutput.Display (info.c_str (), Out::Error);
	}
}

void Model::GetRenameInfo (std::string & info, UniqueName const & oldName, UniqueName const & newName)
{
	if (oldName.GetParentId () == newName.GetParentId ())
	{
		info += "File has been renamed from ";
		info += oldName.GetName ();
		info += " to ";
		info += newName.GetName ();
	}
	else
	{
		info += "File has been moved:\n";
		char const * oldFullPath = _pathFinder.GetFullPath (oldName);
		info += "Old location: ";
		info += _directory.GetRootRelativePath (oldFullPath);
		char const * newFullPath = _pathFinder.GetFullPath (newName);
		info += "\nNew location: ";
		info += _directory.GetRootRelativePath (newFullPath);
	}
}

void Model::GetChangeTypeInfo (std::string & info, FileType const & oldType, FileType const & newType)
{
	info += "File type been changed from ";
	info += oldType.GetName ();
	info += " to ";
	info += newType.GetName ();	
}

bool Model::ChangeDirectory (std::string const & relPath)
{
	return _directory.Change (relPath);
}

void Model::GotoRoot ()
{
	_directory.GotoRoot ();
}

bool Model::FolderUp ()
{
	_directory.Up ();
	return _directory.IsRootFolder ();
}

bool Model::NewFile (NewFileData & dlgData)
{
	FilePath targetDir (dlgData.GetTargetFolder ());
	std::string newFilePath = targetDir.GetFilePath (dlgData.GetName ());
	if (!File::Exists (newFilePath.c_str ()))
	{
		if (!File::Exists (dlgData.GetTargetFolder ()))
			File::CreateNewFolder (dlgData.GetTargetFolder ());
		// Create new file on disk
		FileSerializer out;
		out.Create (newFilePath.c_str ());
		dlgData.Serialize (out);
	}

	bool success = true;
	if (dlgData.DoAdd ())
	{
		PathParser pathParser (_directory);
		Workspace::Selection selection (newFilePath, pathParser);
		if (selection.empty ())
			return false;
		selection.SetType (dlgData.GetType ());
		dbg << "Model::New File initial selection" << std::endl;
		dbg << selection;
		success = Execute (selection);
	}

	if (success && dlgData.DoOpen ())
	{
		Open (newFilePath);
	}
	return success;
}

void Model::Open (std::string const & fullPath)
{
	PathParser pathParser (_directory);
	UniqueName const * uname = pathParser.Convert (fullPath.c_str ());
	if (uname == 0)
		return;
	FileData const * fileData = _dataBase.FindProjectFileByName (*uname);
	Assert (fileData != 0);
	GlobalId gid = fileData->GetGlobalId ();
	Open (gid, uname);
}

void Model::DeleteFile (std::string const & fullPath)
{
	FileData const * fileData = GetFileData (fullPath);
	if (fileData == 0)
		File::Delete (fullPath);
	else
	{
		GidList gidList;
		gidList.push_back (fileData->GetGlobalId ());
		DeleteFile (gidList, true);
	}
}

bool Model::VerifyNewFolderName (std::string const & name)
{
	if (name.empty ())
	{
		TheOutput.Display ("You have to provide a name for a new folder.");
		return false;
	}
	if (!FilePath::IsValid (name))
	{
		TheOutput.Display ("The new folder name contains illegal characters.");
		return false;
	}
	return true;
}

bool Model::NewFolder (std::string const & folderName)
{
	if (!VerifyNewFolderName (folderName))
		return false;

	std::string newFolderPath (_directory.GetFilePath (folderName));
	if (File::Exists (newFolderPath.c_str ()))
	{
		std::string info ("Cannot create ");
		info += folderName;
		info += ". A folder/file with this name already exists.";
		TheOutput.Display (info.c_str ());
		return false;
	}

	PathParser pathParser (_directory);
	Workspace::Selection selection (newFolderPath, pathParser);
	selection.SetType (FolderType ());
	dbg << "Model::New Folder initial selection" << std::endl;
	dbg << selection;
	return Execute (selection);
}

void Model::GetState (FileStateList & list)
{
	// Return file state
	PathParser parser (_directory);
	for (FileStateList::Iterator iter (list); !iter.AtEnd (); iter.Advance ())
	{
		char const * path = iter.GetPath ();
		UniqueName const * uname = parser.Convert (path);
		if (uname != 0)
		{
			Assert (uname->IsValid ());
			FileData const * fd = _dataBase.FindProjectFileByName (*uname);
			if (fd != 0)
			{
				FileState fileState = fd->GetState ();
				fileState.SetCheckedOutByOthers (_checkedOutFiles.IsCheckedOut (fd->GetGlobalId ()));
				iter.SetState (fileState.GetValue ());
			}
		}
	}
}

void Model::GetVersionId (FileStateList & list, bool isCurrent) const
{
	unsigned int versionId;
	if (isCurrent)
	{
		// Return current version id
		versionId = _history.MostRecentScriptId ();
	}
	else
	{
		// Return next version id
		versionId = _dataBase.GetNextScriptId ();
	}
	for (FileStateList::Iterator iter (list); !iter.AtEnd (); iter.Advance ())
	{
		iter.SetState (versionId);
	}
}

std::string Model::GetVersionDescription (GlobalId versionGid) const
{
	std::string versionDescr = _history.RetrieveVersionDescription (versionGid);
	return versionDescr;
}

bool Model::CanRestoreFile (GlobalId gid) const
{
	// User can restore file for the purpose of time travel in the history
	// if the reference version of the file can be overwritten at will.
	// When the file takes part in the script conflict resolution or
	// is synced out then its reference copy cannot be touched and time travel
	// in the history for such a file is temporarily blocked.
	FileData const * fileData = _dataBase.FindByGid (gid);
	if (fileData != 0)
	{
		FileState state = fileData->GetState ();
		return !state.IsRelevantIn (Area::Reference) &&	// Not restored and
			   !state.IsRelevantIn (Area::Synch);		// Not synced out
	}

	return true;
}

void Model::PreDeleteFile (GidList const & controlledItems, GidList & controlledFolders)
{
	Assert (controlledFolders.empty ());

	GidList checkedOutFiles;
	for (GidList::const_iterator iter = controlledItems.begin (); iter != controlledItems.end (); ++iter)
	{
		GlobalId gid = *iter;
		FileData const * fd = _dataBase.GetFileDataByGid (gid);
		if (fd->GetType ().IsFolder ())
		{
			controlledFolders.push_back (gid);
			// Recursively list folder contents
			GidList contents;
			_dataBase.ListFolderContents (gid, contents);
			for (GidList::const_iterator iter1 = contents.begin (); iter1 != contents.end (); ++iter1)
			{
				GlobalId subGid = *iter1;
				FileData const * subFileData = _dataBase.GetFileDataByGid (subGid);
				FileState state = subFileData->GetState ();
				if (!subFileData->GetType ().IsFolder () && state.IsRelevantIn (Area::Original))
				{
					// Checked out file
					checkedOutFiles.push_back (subGid);
				}
			}
		}
		else if (fd->GetState ().IsRelevantIn (Area::Original))
		{
			Assert (!fd->GetType ().IsFolder ());
			checkedOutFiles.push_back (gid);
		}
	}

	if (checkedOutFiles.empty ())
		return;

	// Place in the recycle bin all checked out files,
	// so we preserve local edits in case user changes
	// his mind. Uncheck-out will bring back original file
	// version to the project. Edits before delete can be
	// found in the recycle bin.
	ShellMan::FileRequest checkedOutDelReq;
	checkedOutDelReq.MakeItQuiet ();

	GidList existingFiles; // Checked out files that exist on disk
	for (GidList::const_iterator iter = checkedOutFiles.begin (); iter != checkedOutFiles.end (); ++iter)
	{
		GlobalId gid = *iter;
		FileData const * fileData = _dataBase.GetFileDataByGid (gid);
		FileState state = fileData->GetState ();
		char const * fullPath = _pathFinder.GetFullPath (gid, Area::Project);
		Assert (!fileData->GetType ().IsFolder ());
		if (!state.IsPresentIn (Area::Project) || !File::Exists (fullPath))
			continue; // deleted from project or from disk

		if (state.IsPresentIn (Area::Original))
		{
			checkedOutDelReq.AddFile (fullPath);
			existingFiles.push_back (gid);
		}
	}

	// Under transaction copy existing checked out files to the staging area
	// and delete them from the project area using Shell -- this will
	// place them in the recycle bin.  Committing transaction will bring back
	// checked out to the project area, so we can continue with uncheckout.
	if (!existingFiles.empty ())
	{
		// Transaction scope
		TransactionFileList fileList;
		FileTransaction xact (*this, _pathFinder, fileList);
		for (GidList::const_iterator iter = existingFiles.begin (); iter != existingFiles.end (); ++iter)
		{
			GlobalId gid = *iter;
			XPhysicalFile file (gid, _pathFinder, fileList);
			Transformer trans (_dataBase, gid);
			trans.CopyToProject (Area::Project, file, _pathFinder, fileList);
		}
		checkedOutDelReq.DoDelete (TheAppInfo.GetWindow (), true);	// No exceptions, please
		xact.Commit ();
	}
	// Uncheck-out
	Uncheckout (checkedOutFiles, true);	// Quiet
}

void Model::PostDeleteFolder (GidList const & controlledFolders)
{
	if (controlledFolders.empty ())
		return;

	// Check if deleted trees are empty. If not then ask the user.
	ShellMan::FileRequest folderDeleteRequest;
	folderDeleteRequest.MakeItQuiet ();
	for (GidList::const_iterator iter = controlledFolders.begin ();
		iter != controlledFolders.end ();
		++iter)
	{
		char const * folderPath = _pathFinder.GetFullPath (*iter, Area::Project);
		if (!File::IsTreeEmpty (folderPath))
		{
			if (IsQuickVisit ())
				continue;	// Don't ask and don't delete

			std::string info ("Deleted folder:\n\n");
			info += folderPath;
			info += "\n\nis not empty. Do you want to remove it from disk?";
			Out::Answer userChoice = TheOutput.Prompt (info.c_str (),
													   Out::PromptStyle (Out::YesNo,
													   Out::No,
													   Out::Question),
													   TheAppInfo.GetWindow ());
			if (userChoice == Out::No)
				continue;	// Don't delete folder
		}

		folderDeleteRequest.AddFile (folderPath);
	}

	folderDeleteRequest.DoDelete (TheAppInfo.GetWindow ());
}

bool Model::DeleteFile (GidList const & controlledItems, bool doDelete)
{
	Assert (!controlledItems.empty ());
	GidList controlledFolders;
	if (doDelete)
		PreDeleteFile (controlledItems, controlledFolders);

	// Delete/Remove project files
	Workspace::Selection selection (controlledItems, _dataBase, doDelete ? Workspace::Delete : Workspace::Remove);
	dbg << "Model::DeleteFile -- initial selection:" << std::endl;
	dbg << selection;
	bool result = Execute (selection);
	if (doDelete)
		PostDeleteFolder (controlledFolders);

	return result;
}

void Model::DeleteScript (SelectionSeq & seq)
{
	Assert (!IsQuickVisit ());
	GidList scriptsToBeDeleted;
	for ( ; !seq.AtEnd (); seq.Advance ())
	{
		GlobalId scriptId = seq.GetGlobalId ();
		if (_mailBox.IsScriptFilePresent (scriptId))
		{
			if (_mailBox.IsJoinRequestScript (scriptId))
			{
				// Do you want to delete?
				Out::Answer userChoice = TheOutput.Prompt (
					"You are deleting a join request.\n\nAre you sure?\n\n"
					"If so, you should still inform this person not to wait\n"
					"for a full sync and to defect from the (empty) project.",
					Out::PromptStyle (Out::YesNo,
									  Out::No,
									  Out::Question),
					TheAppInfo.GetWindow ());
				if (userChoice == Out::No)
					continue;
			}
			scriptsToBeDeleted.push_back (scriptId);
		}
		else if (_history.CanDelete (scriptId))
		{
			if (_history.IsMissing (scriptId))
			{
				if (_history.IsAtTreeTop (scriptId))
				{
					std::string whyCannotDelete;
					if (_history.CanDeleteMissing (scriptId, whyCannotDelete))
					{
						Out::Answer userChoice = TheOutput.Prompt ("You are deleting a missing script. "
							"Are you sure that nobody in the project has received this script?\n\n"
							"If you're not sure, press NO.",
							Out::PromptStyle (Out::YesNo,
							Out::No,
							Out::Warning),
							TheAppInfo.GetWindow ());
						if (userChoice == Out::No)
							continue;
					}
					else
					{
						TheOutput.Display (whyCannotDelete.c_str ());
						continue;
					}
				}
				else
				{
					TheOutput.Display ("Missing script can be removed from Inbox only if it's not followed by another script.");
					continue;
				}
			}
			scriptsToBeDeleted.push_back (scriptId);
		}
	}

	if (scriptsToBeDeleted.empty ())
		return;

	{
		// Transaction scope
		TransactionFileList fileList;
		FileTransaction xact (*this, _pathFinder, fileList);
		for (GidList::const_iterator iter = scriptsToBeDeleted.begin (); iter != scriptsToBeDeleted.end (); ++iter)
		{
			GlobalId scriptId = *iter;
			_mailBox.XDeleteScript (scriptId);
		}
		xact.Commit ();
	}
	ProcessMail (true); // force processing even if mailbox empty
}

void Model::RemoveFile (GidList const & corruptedFile)
{
	// Revisit: use Workspace::Selection
	TransactionFileList fileList;
	FileTransaction xact (*this, _pathFinder, fileList);
	for (GidList::const_iterator iter = corruptedFile.begin ();
		 iter != corruptedFile.end ();
		 ++iter)
	{
		// Removed corrupted files have to be checked in.
	    // If they are not checked in they already have passed
		// checksum verification, so they cannot be corrupted.
		Transformer trans (_dataBase, *iter);
		Assert (trans.GetState ().IsCheckedIn ());
		trans.DeleteFile (_pathFinder, fileList, false);
		_checkInArea.Notify (changeAdd, *iter);
		_directory.Notify (changeRemove, *iter);
	}
	xact.Commit ();
}

bool Model::VerifyNewFileName (std::string const & name)
{
	if (name.empty ())
	{
		TheOutput.Display ("You cannot rename because the new name is empty.");
		return false;
	}
	if (!File::IsValidName (name.c_str ()))
	{
		TheOutput.Display ("You cannot rename because the new name contains illegal characters.");
		return false;
	}
	return true;
}

bool Model::RenameFile (GlobalId renamedGid, UniqueName const & newUname)
{
	if (!VerifyNewFileName (newUname.GetName ()))
		return false;	// Not a valid file name

	bool cannotRename = false;
	FileData const * newNameFileData = _dataBase.FindProjectFileByName (newUname);
	if (newNameFileData == 0)
	{
		// File/folder under new name not recorded in the project database.
		// Check disk,
		char const * tgtPath = _pathFinder.GetFullPath (newUname);
		cannotRename = File::Exists (tgtPath);
	}
	else if (newNameFileData->GetGlobalId () != renamedGid)
	{
		// Another project file/folder uses the new name.
		if (newNameFileData->GetState ().IsPresentIn (Area::Project))
		{
			cannotRename = true;
		}
		else
		{
			// The other file is deleted/removed from the project.
			// Check disk.
			char const * tgtPath = _pathFinder.GetFullPath (newUname);
			cannotRename = File::Exists (tgtPath);
		}
	}
	else
	{
		UniqueName const & currentUName = newNameFileData->GetUniqueName ();
		if (currentUName.GetName () == newUname.GetName ())
		{
			// Names are identical (case-sensitive) - nothing to do
			Assert (newNameFileData->GetUniqueName ().IsEqual (newUname));
			return false;
		}
	}

	if (cannotRename)
	{
		FileData const * renamedFileData = _dataBase.GetFileDataByGid (renamedGid);
		bool isFolderOnDisk;
		if (newNameFileData != 0)
		{
			isFolderOnDisk = newNameFileData->GetType ().IsFolder ();
		}
		else
		{
			char const * tgtPath = _pathFinder.GetFullPath (newUname);
			isFolderOnDisk = File::IsFolder (tgtPath);
		}
		std::string info ("Cannot rename ");
		info += (renamedFileData->GetType ().IsFolder () ? "folder" : "file");
		info += " from '";
		info += renamedFileData->GetName ();
		info += "' to '";
		info += newUname.GetName ();
		info += "',\nbecause ";
		info += (isFolderOnDisk ? "folder" : "file");
		info += " with the same name already exists on disk.";
		TheOutput.Display (info.c_str ());
		return false;
	}

	GidList files;
	files.push_back (renamedGid);
	Workspace::Selection selection (files, _dataBase, newUname);
	dbg << "Model::RenameFile -- initial selection:" << std::endl;
	dbg << selection;
	return Execute (selection);
}

void Model::RenameFile (GlobalId renamedGid, std::string const & newName)
{
	FileData const * fd = _dataBase.GetFileDataByGid (renamedGid);
	UniqueName newUname (fd->GetUniqueName ().GetParentId (), newName);
	Assert (!newUname.IsStrictlyEqual (fd->GetUniqueName ()));
	RenameFile (renamedGid, newUname);
}

void Model::ChangeFileType (GidList const & files, FileType newType)
{
	Workspace::Selection selection (files, _dataBase, Workspace::Edit);
	selection.SetType (newType);
	dbg << "Model::ChangeFileType -- initial selection:" << std::endl;
	dbg << selection;
	Execute (selection, false);	// Don't extend selection
}

bool Model::IsClipboardContentsEqual (Win::FileDropHandle const & fileDrop) const
{
	Win::FileDropHandle::Sequencer sysClipboardSeq (fileDrop);
	if (sysClipboardSeq.GetCount () != _fileClipboard.size ())
		return false;
	std::set<std::string, NocaseLess> coopClipboardContents;
	for (unsigned int i = 0; i < _fileClipboard.size (); ++i)
	{
		std::string const & path = _fileClipboard [i]->Path ();
		coopClipboardContents.insert (path);
	}
	for ( ; !sysClipboardSeq.AtEnd (); sysClipboardSeq.Advance ())
	{
		char const * path = sysClipboardSeq.GetFilePath ();
		if (coopClipboardContents.find (path) == coopClipboardContents.end ())
			return false;
	}
	return true;
}

bool Model::RangeHasMissingPredecessors (bool isMerge) const
{
	if (isMerge)
		return _mergedFiles.RangeHasMissingPredecessors	();
	else
		return _historicalFiles.RangeHasMissingPredecessors ();
}

bool Model::Cut (SelectionSeq & seq)
{
	bool done = false;
	std::vector<std::string> files;
	// Place files in the Co-op clipboard
	_fileClipboard.clear ();
	for ( ; !seq.AtEnd (); seq.Advance ())
	{
		done = true;
		GlobalId gid = seq.GetGlobalId ();
		char const * filePath = _directory.GetFilePath (seq.GetName ());
		std::unique_ptr<FileTag> fileTag (new FileTag (gid, filePath));
		_fileClipboard.push_back (std::move(fileTag));
		files.push_back (filePath);
	}
	// Place files in the Windows clipboard
	Clipboard sysClipboard (TheAppInfo.GetWindow ());
	sysClipboard.PutFileDrop (files);
	return done;
}

bool Model::Copy (SelectionSeq & seq)
{
	std::vector<std::string> files;
	// Prepare path list
	_fileClipboard.clear ();
	for ( ; !seq.AtEnd (); seq.Advance ())
	{
		char const * filePath = _directory.GetFilePath (seq.GetName ());
		files.push_back (filePath);
	}
	// Place files in the Windows clipboard
	Clipboard sysClipboard (TheAppInfo.GetWindow ());
	sysClipboard.PutFileDrop (files);
	return true;
}

bool Model::Paste (DirectoryListing & nonProjectFiles)
{
	bool done = false;
	if (_fileClipboard.size () == 0)
		return done;

	FilePath currentFolder (_directory.GetCurrentPath ());
	// Remove recursive paths and split paths between project and non-project files
	for (unsigned int i = 0; i < _fileClipboard.size (); i++)
	{
		FileTag const * fileTag = _fileClipboard [i];
		Assert (fileTag != 0);
		if (currentFolder.HasPrefix (fileTag->Path ()))
		{
			// Remove recursive path
			_fileClipboard.erase (i);
			--i;
			continue;
		}

		if (fileTag->Gid () == gidInvalid)
			nonProjectFiles.InitWithPath (fileTag->Path ());
	}

	if (_fileClipboard.size () == nonProjectFiles.size ())
		return done;

	Workspace::Selection selection (_fileClipboard, _dataBase, _directory);
	dbg << "Model::Paste -- initial selection:" << std::endl;
	dbg << selection;
	done = Execute (selection);
	_fileClipboard.clear ();
	return done;
}

void Model::ReSendScript (GlobalId scriptId, ScriptRecipientsData const & dlgData)
{
	AddresseeList addresseeList;
	dlgData.GetSelection (addresseeList);
	ScriptHeader hdr;
	CommandList cmdList;
	if (_history.RetrieveScript (scriptId, hdr, cmdList, dlgData.GetUnitType ()))
	{
		std::unique_ptr<CheckOut::List> notification = GetCheckoutNotification ();
		_mailer.Multicast (hdr, cmdList, addresseeList, notification.get ());
	}
}

void Model::RequestResend (GlobalId scriptId, ScriptRecipientsData const & dlgData)
{
	UserIdList addresseeList;
	ScriptBasket scriptBasket;
	dlgData.GetSelection (addresseeList);
	{
		Transaction xact (*this, _pathFinder);
		_sidetrack.XRequestResend(scriptId, addresseeList, scriptBasket);
		xact.Commit ();
	}
	std::unique_ptr<CheckOut::List> notification = GetCheckoutNotification ();
	scriptBasket.SendScripts(_mailer, notification.get(), _dataBase.GetProjectDb());
}

std::unique_ptr<FileTag> Model::MakeFileTag (GlobalId gid) const
{
	Project::Path project (_dataBase);
	std::string fileInfo (project.MakePath (gid));
	fileInfo += ' ';
	GlobalIdPack pack (gid);
	fileInfo += pack.ToBracketedString ();
	std::unique_ptr<FileTag> tag (new FileTag (gid, fileInfo.c_str ()));
	return tag;
}

void Model::FindAllByName (std::string const & fileInfo, auto_vector<FileTag> & foundFiles) const
{
	PathSplitter keyName (fileInfo);
	std::string fileName (keyName.GetFileName ());
	fileName += keyName.GetExtension ();
	FileMatcher matcher (fileName.c_str ());
	if (!matcher.IsOk ())
		return;

	GidList fileList;
	_dataBase.FindAllByName (matcher, fileList);
	if (fileList.size () == 0)
		return;

	if (keyName.HasOnlyFileName ())
	{
		// User specified only file name
		for (GidList::const_iterator iter = fileList.begin (); 
			iter != fileList.end (); 
			++iter)
		{
			std::unique_ptr<FileTag> file = MakeFileTag (*iter);
			foundFiles.push_back (std::move(file));
		}
	}
	else
	{
		// User specified file path -- eliminate files found on paths
		// not including the specified one
		FilePath patternDir (keyName.GetDir ());
		Project::Path project (_dataBase);
		for (GidList::const_iterator iter = fileList.begin (); 
			iter != fileList.end (); 
			++iter)
		{
			char const * filePath = project.MakePath (*iter);
			PathSplitter splitFile (filePath);
			FilePath fileDir (splitFile.GetDir ());
			if (fileDir.HasSuffix (patternDir))
			{
				std::unique_ptr<FileTag> file = MakeFileTag (*iter);
				foundFiles.push_back (std::move(file));
			}
		}
	}
}

void Model::ExpandSubtrees (auto_vector<FileTag> & fileList) const
{
	GidList gidList;
	auto_vector<FileTag>::iterator it = fileList.begin ();
	while (it != fileList.end ())
	{
		GlobalId gid = (*it)->Gid ();
		FileData const * data = _dataBase.FindByGid (gid);
		if (data && data->GetType ().IsFolder ())
			_dataBase.FindAllDescendants (gid, gidList);
		++it;
	}

	for (GidList::const_iterator iter = gidList.begin (); iter != gidList.end (); ++iter)
	{
		std::unique_ptr<FileTag> file = MakeFileTag (*iter);
		fileList.push_back (std::move(file));
	}
}

void Model::FindAllByGid (GidList const & gids, auto_vector<FileTag> & foundFiles) const
{
	for (GidList::const_iterator iter = gids.begin (); iter != gids.end (); ++iter)
	{
		GlobalId gid = *iter;
		FileData const * fd = _dataBase.FindByGid (gid);
		if (fd != 0)
		{
			std::unique_ptr<FileTag> file = MakeFileTag (gid);
			foundFiles.push_back (std::move(file));
		}
	}
}

void Model::FindAllByComment (std::string const & keyword,
							  GidList & scripts,
							  auto_vector<FileTag> & foundFiles) const
{
	GidSet files;
	_history.FindAllByComment (keyword, scripts, files);
	for (GidSet::const_iterator iter = files.begin (); iter != files.end (); ++iter)
	{
		std::unique_ptr<FileTag> file = MakeFileTag (*iter);
		foundFiles.push_back(std::move(file));
	}
}

bool Model::IsProjectFile (GlobalId gid) const
{
	FileData const * fileData = _dataBase.GetFileDataByGid (gid);
	Assert (fileData != 0);
	return !fileData->GetState ().IsNone ();
}

bool Model::IsTextual (GlobalId gid) const
{
	FileData const * fileData = _dataBase.GetFileDataByGid (gid);
	Assert (fileData != 0);
	return fileData->GetType ().IsTextual ();
}

bool Model::IsFolder (GlobalId gid) const
{
	FileData const * fileData = _dataBase.GetFileDataByGid (gid);
	Assert (fileData != 0);
	return fileData->GetType ().IsFolder ();
}

char const * Model::GetFullPath (GlobalId gid) 
{
	FileData const * fileData = _dataBase.GetFileDataByGid (gid);
	Assert (fileData != 0);
    UniqueName const & uName = fileData->GetUniqueName ();
	return _pathFinder.GetFullPath (uName);
}

std::string Model::GetName (GlobalId gid) const
{
	FileData const * fileData = _dataBase.GetFileDataByGid (gid);
	Assert (fileData != 0);
	return std::string (fileData->GetName ());
}

bool Model::IsMerge () const
{
	for (SynchAreaSeq seq (_synchArea); !seq.AtEnd (); seq.Advance ())
	{
		GlobalId gid = seq.GetGlobalId ();
		FileData const * fileData = _dataBase.GetFileDataByGid (gid);
		FileState state = fileData->GetState ();
		Assert (state.IsRelevantIn (Area::Synch));
		if (state.IsMerge ())
			return true;
	}
	return false;
}

char const * Model::GetCurrentPath (GlobalId gid, bool preserveLocalEdits)
{
	FileData const * fileData = _dataBase.GetFileDataByGid (gid);
	Assert (fileData != 0);
	Area::Location area;
	FileState state = fileData->GetState ();
	Assert (!state.IsNone ());
	if (state.IsRelevantIn (Area::Original))
	{
		// Checked-out
		if (state.IsPresentIn (Area::Original))
		{
			if (preserveLocalEdits)
				area = Area::Project;
			else
				area = Area::Original;
		}
		else
			area = Area::Project;	// New file
	}
	else if (state.IsRelevantIn (Area::Synch))
	{
		// Synch-out
		if (state.IsPresentIn (Area::Synch))
			area = Area::Synch;
		else
			area = Area::PreSynch;	// Deleted by synch
	}
	else
	{
		area = Area::Project;
	}
	return _pathFinder.GetFullPath (gid, area);
}

bool Model::IsArchive () const
{
	return _history.IsArchive ();
}

void Model::Archive (GlobalId scriptId)
{
	if (_history.CanArchive (scriptId))
	{
		Transaction xact (*this, _pathFinder);
		_history.XArchive (scriptId);
		xact.Commit ();
	}
	else
	{
		GlobalIdPack pack (scriptId);
		std::string info ("The script ");
		info += pack.ToBracketedString ();
		info +=	" cannot be used as archive marker, because\n"
				"there are older unconfirmed or missing scripts in the history.";
		TheOutput.Display (info.c_str ());
	}
}

void Model::UnArchive ()
{
	Transaction xact (*this, _pathFinder);
	_history.XUnArchive ();
	xact.Commit ();
}

std::unique_ptr<ScriptProps> Model::GetScriptProps (GlobalId scriptGid) const
{
	if (_mailBox.IsScriptFilePresent (scriptGid))
	{
		// Script not unpacked -- present in the mailbox
		std::string const & scriptPath = _mailBox.GetScriptPath (scriptGid);
		return std::unique_ptr<ScriptProps> (new ScriptProps (GetProjectDb (), _pathFinder, scriptPath));
	}
	else if (_sidetrack.IsMissing (scriptGid))
	{
		return std::unique_ptr<ScriptProps> (new ScriptProps (GetProjectDb (), _sidetrack, scriptGid));
	}
	else
	{
		// Script unpacked -- present in the history
		return GetScriptProps (scriptGid, 0);	// No file filter
	}
}

std::unique_ptr<ScriptProps> Model::GetScriptProps (GlobalId scriptGid, FileFilter const * filter) const
{
	std::unique_ptr<ScriptProps> props (new ScriptProps (GetProjectDb (), _pathFinder));	
	if (_history.RetrieveScript (scriptGid, filter, *props))
	{
		return props;
	}
	return std::unique_ptr<ScriptProps>();
}

std::unique_ptr<FileProps> Model::GetFileProps (GlobalId fileGid) const
{
	FileData const * fd = _dataBase.GetFileDataByGid (fileGid);
	Project::Db const & projectDb = GetProjectDb ();
	std::unique_ptr<FileProps> props (new FileProps (*fd, projectDb));
	props->AddCheckoutNotifications (projectDb, _checkedOutFiles);
	return props;
}

void Model::VerifyTreeContents (GlobalId folder, VerificationReport & report, bool recursive)
{
	GidList folderContents;
	_dataBase.ListProjectFiles (folder, folderContents);
	for (GidList::const_iterator iter = folderContents.begin ();
	     iter != folderContents.end ();
		 ++iter)
	{
		FileData const * fileData = _dataBase.GetFileDataByGid (*iter);
		if (!fileData->GetState ().IsCheckedIn ())
			continue;// Skip checked out files/folders

		if (fileData->GetType ().IsFolder ())
		{
			if (recursive)
			{
				// Verify folder contents
				VerifyTreeContents (*iter, report, recursive);
			}
		}
		else
		{
			PhysicalFile file (*fileData, _pathFinder);
			if (fileData->GetCheckSum () != file.GetCheckSum (Area::Project))
			{
				// Remember corrupted file gid
				report.Remember (VerificationReport::Corrupted, *iter);
			}
		}
	}
}

std::unique_ptr<VerificationReport> Model::VerifyChecksum (GidList const & files, bool recursive)
{
	SimpleMeter meter (_uiFeedback);
	meter.SetRange (0, files.size () + 1);
	std::unique_ptr<VerificationReport> report (new VerificationReport ());
	for (GidList::const_iterator iter = files.begin (); iter != files.end (); ++iter)
    {
		meter.StepIt ();
        GlobalId gid = *iter;
		Assert (gid != gidInvalid);
		FileData const * fileData = _dataBase.GetFileDataByGid (gid);
		FileState state = fileData->GetState ();
		if (state.IsRelevantIn (Area::Original) || state.IsRelevantIn (Area::Synch))
			continue;// Skip checked out or synched out files/folders

		FileType type = fileData->GetType ();
		std::string activity ("Verifying ");
		std::string path = _pathFinder.GetRootRelativePath (gid);
		if (type.IsFolder () || type.IsRoot ())
		{
			// Verify folder contents
			activity += "folder contents: ";
			activity += path;
			meter.SetActivity (activity);
			VerifyTreeContents (gid, *report, recursive);
		}
		else
		{
			activity += "file checksum: ";
			activity += path;
			meter.SetActivity (activity);
			Assert (!type.IsFolder () && !type.IsRoot ());
			PhysicalFile file (*fileData, _pathFinder);
			if (fileData->GetCheckSum () != file.GetCheckSum (Area::Project))
			{
				// Remember corrupted file gid
				dbg << "Checksum mismatch: " << fileData->GetName () << std::endl;
				dbg << "    is: " << file.GetCheckSum (Area::Project).GetSum ();
				dbg	<< ", " << file.GetCheckSum (Area::Project).GetCrc ();
				dbg	<< " should be: " << fileData->GetCheckSum ().GetSum ();
				dbg	<< ", " << fileData->GetCheckSum ().GetCrc () << std::endl;
				report->Remember (VerificationReport::Corrupted, gid);
			}
		}
    }
	return report;
}

void Model::RetrieveProjectPaths (SelectionSeq & seq, std::vector<std::string> & paths)
{
	for ( ; !seq.AtEnd (); seq.Advance ())
	{
		GlobalId gid = seq.GetGlobalId ();
		char const * path;
		if (gid == gidInvalid)
		{
			// Not controlled file/folder
			char const * name = seq.GetName ();
			path = _directory.GetFilePath (name);
		}
		else
		{
			// Controlled file/folder
			path = _pathFinder.GetFullPath (gid, Area::Project);
		}
		paths.push_back (path);
	}
}

void Model::PreserveLocalEdits (TransactionFileList & fileList)
{
	GidList checkedOutFiles;
	_dataBase.ListCheckedOutFiles (checkedOutFiles);
	for (GidList::const_iterator iter = checkedOutFiles.begin (); iter != checkedOutFiles.end (); ++iter)
	{
		GlobalId gid = *iter;
		FileData const * fd = _dataBase.GetFileDataByGid(gid);
		if (fd->GetType().IsFolder() || fd->GetState().IsToBeDeleted())
			continue;
		std::string projectPath (_pathFinder.GetFullPath (gid, Area::Project));
		char const * targetPath = _pathFinder.GetFullPath (gid, Area::LocalEdits);
		fileList.RememberCreated (targetPath, false);	// Not a folder path
		File::Copy (projectPath.c_str (), targetPath);
	}
}

std::string Model::GetCheckinAreaCaption () const
{
	Restriction restrict;
	return _checkInArea.GetCaption (restrict);
}

void Model::SaveDiagnostics (DiagnosticsRequest const & request)
{
	dbg << "->SaveDiagnostics" << std::endl;

	std::stringstream oss;
	// save some basic version information

	XML::Tree diagTree;
	XML::Node * root = diagTree.SetRoot ("Diagnostics");

	// Windows
	oss << "==General information:" << std::endl;
	SystemVersion sysVer;
	oss << "*" << sysVer << std::endl;

	if (request.IsVersionDump ())
	{
		XML::Node * version = root->AddChild ("Version");
		XML::Node * system = version->AddEmptyChild ("System");
		system->AddAttribute ("major", sysVer.MajorVer ());
		system->AddAttribute ("minor", sysVer.MinorVer ());
		system->AddAttribute ("build", sysVer.BuildNum ());
		system->AddTransformAttribute ("servicePack", sysVer.CSDVersion ());

		// Code Co-op
		oss << "*" COOP_PRODUCT_NAME " Version " COOP_PRODUCT_VERSION << std::endl;
		XML::Node * coop = version->AddEmptyChild ("Coop");
		coop->AddAttribute ("version", COOP_PRODUCT_VERSION);
	}

	// Email
	dbg << "Email dump" << std::endl;
	DumpEmail (oss);

	// Code Co-op setup
	oss << std::endl << "==Code Co-op Setup" << std::endl;
	if (Registry::IsUserSetup ())
		oss << "*Single user setup" << std::endl;
	else
		oss << "*All users setup" << std::endl;
	oss << "*Program path: " << Registry::GetProgramPath () << std::endl;
	oss << "*Catalog path: " << Registry::GetCatalogPath () << std::endl;
	oss << "*Command line tools path:" << Registry::GetCmdLineToolsPath () << std::endl;
	Registry::DispatcherUserRoot dispatcher;
	if (dispatcher.Key ().IsValuePresent ("ConfigurationState"))
	{
		oss << "*Configuration state: " << dispatcher.Key ().GetStringVal ("ConfigurationState") << std::endl;
	}

	if (request.IsCatalogDump ())
	{
		dbg << "Catalog dump" << std::endl;
		// Save catalog information
		oss << std::endl << "==Project catalog:" << std::endl;
		_catalog.Dump (root, oss, true);	//	true for diagnostics
	}

	BackupMarker backupMarker;
	if (backupMarker.Exists ())
		oss << "==Backup marker detected." << std::endl;
	RepairList repairList;
	if (repairList.Exists ())
	{
		dbg << "Repair list exists" << std::endl;
		oss << "==Project repair list:" << std::endl;
		std::vector<int> projectIds = repairList.GetProjectIds ();
		for (std::vector<int>::const_iterator iter = projectIds.begin (); iter != projectIds.end (); ++iter)
			oss << '*' << *iter << std::endl;
	}

	if (IsInProject ())
	{
		dbg << "Project dump" << std::endl;
		// Save this project's diagnostic information
		DumpProject (root, oss, request);
	}

	// Append error log?  Might be very large...

	// Write diagnostics to file
	OutStream out (request.GetTargetFilePath (),
				   request.IsOverwriteExisting () ? std::ios_base::out : std::ios_base::app);
	Assert (out.is_open ());
	if (request.IsXMLDump ())
	{
		if (request.IsOverwriteExisting ())
		{
			// Creating xml tree from scratch
			diagTree.Write (out);
		}
		else
		{
			// Appending to the existing xml tree
			// Write all root node children.
			XML::Node const * root = diagTree.GetRoot ();
			for (XML::Node::ConstChildIter it = root->FirstChild (); it != root->LastChild (); ++it)
			{
				XML::Node * child = *it;
				child->Write (out, 2);	// Set indent level at 2
			}
		}
	}
	else
	{
		out << oss.str ();
		// Append activity log if present
		_activityLog.Dump (out);
	}
	dbg << "<-SaveDiagnostics" << std::endl;
}

void Model::DumpEmail (std::ostream & out)
{
	out << std::endl << "==Email" << std::endl;
	try
	{
		Registry::DefaultEmailClient defaultClient;
		out << "*Default e-mail client name: " << defaultClient.GetName () << std::endl;
	}
	catch (...)
	{
		out << "*Default e-mail client name: Cannot access registry" << std::endl;
	}

	Email::RegConfig emailConfig;
	emailConfig.Dump (out);


	out << std::endl << "==Obsolete Email Settings" << std::endl;
	Registry::DispatcherPrefs dispatcherPrefs;
	unsigned long value = 0;
	out << "*Dispatcher is using POP3: ";
	bool isUsingPop3 = false;
	if (dispatcherPrefs.Key ().GetValueLong ("Use Pop3", value))
	{
		isUsingPop3 = (value == 1);
		out << (isUsingPop3 ? "Yes" : "No") << std::endl;
	}
	else
		out << "undefined" << std::endl;
	out << "*Dispatcher is using SMTP: ";
	bool isUsingSmtp = false;
	if (dispatcherPrefs.Key ().GetValueLong ("Use Smtp", value))
	{
		isUsingSmtp = (value == 1);
		out << (isUsingSmtp ? "Yes" : "No") << std::endl;
	}
	else
		out << "undefined" << std::endl;
}

void Model::DumpProject (XML::Node * root, std::ostream & out, DiagnosticsRequest const & request) const
{
	XML::Node * enlistment = root->AddChild ("Enlistment");
	dbg << std::endl << "==Code Co-op tool options:" << std::endl;
	ToolOptions::Differ differ;
	dbg << differ << std::endl;
	ToolOptions::Merger merger;
	dbg << merger << std::endl;
	ToolOptions::Editor editor;
	dbg << editor << std::endl;
	ProgramOptions::Data options (const_cast<Catalog &>(_catalog));
	dbg << std::endl << "==Code Co-op options:" << std::endl << options << std::endl;

	Project::Db const & projectDb = _dataBase.GetProjectDb ();
	dbg << std::endl << "==Project " << projectDb.ProjectName () << " (local id = " << std::dec << _project.GetCurProjectId () << ")" << std::endl;
	enlistment->AddAttribute ("localId", _project.GetCurProjectId ());
	XML::Node * project = enlistment->AddEmptyChild ("Project");
	project->AddTransformAttribute ("name", projectDb.ProjectName ());
	dbg << std::endl << "===Options:" << std::endl;
	dbg << "*" << (projectDb.IsAutoSynch () ? "Auto-sync mode" : "Regular sync mode") << std::endl;
	dbg << "*" << (projectDb.IsAutoFullSynch () ? "Auto-full sync mode" : "Regular full sync mode") << std::endl;
	dbg << "*" << (projectDb.IsAutoJoin () ? "Auto-join mode" : "Regular join mode") << std::endl;
	dbg << "*" << (projectDb.UseBccRecipients () ? "Use BCC-recipients" : "Use TO-recipients") << std::endl;
	dbg << "*After check-in keep files " << (projectDb.IsKeepCheckedOut () ? "checked out" : "checked in") << std::endl;
	MemberState myState = projectDb.GetMemberState (projectDb.GetMyId ());
	dbg << (myState.IsCheckoutNotification () ? "*Notify" : "*Don't notify") << " other project members about files checked out by me" << std::endl;

	dbg << std::endl << "===Project markers:" << std::endl;

	CheckedOutFiles checkoutMarker (_catalog, GetProjectId ());
	if (checkoutMarker.Exists ())
		dbg << "*Check out marker detected." << std::endl;

	IncomingScripts incomingScripts (_catalog, GetProjectId ());
	if (incomingScripts.Exists ())
		dbg << "*Incoming scripts marker detected." << std::endl;

	MissingScripts missingScripts (_catalog, GetProjectId ());
	if (missingScripts.Exists ())
		dbg << "*Missing scripts marker detected." << std::endl;

	AwaitingFullSync fullSyncMarker (_catalog, GetProjectId ());
	if (fullSyncMarker.Exists ())
		dbg << "*Awaiting full sync marker detected." << std::endl;

	RecoveryMarker recoveryMarker (_catalog, GetProjectId ());
	if (recoveryMarker.Exists ())
		dbg << "*Project recovery marker detected." << std::endl;

	BlockedCheckinMarker blockedCheckinMarker (_catalog, GetProjectId ());
	if (blockedCheckinMarker.Exists ())
		dbg << "*Blocked check in marker detected." << std::endl;

	dbg << std::endl;

	if (request.IsMembershipDump ())
	{
		dbg << "Membership dump" << std::endl;
		XML::Node * membership = enlistment->AddChild ("Membership");
		std::vector<MemberInfo> memberList(projectDb.RetrieveHistoricalMemberList());
		dbg << "===Membership:" << std::endl;
		dbg << "*Current user id = 0x" << std::hex << projectDb.GetMyId () << std::endl << std::endl;
		membership->AddAttribute ("myself", ToHexString (projectDb.GetMyId ()));
		std::map<UserId, MemberInfo const *> sortedMembers;
		for (std::vector<MemberInfo>::const_iterator iter = memberList.begin ();
			iter != memberList.end ();
			++iter)
		{
			MemberInfo const & info = *iter;
			dbg << "    " << info.Id() << std::endl;
			sortedMembers[info.Id ()] = &info;
		}
		dbg << "|! Project user id |! Name |! State |! Hub id & user id string |! Pre-historic script |! Most recent script |! License |" << std::endl;
		for (std::map<UserId, MemberInfo const *>::const_iterator iter = sortedMembers.begin ();
			 iter != sortedMembers.end ();
			 ++iter)
		{
			dbg << "  * " << *iter->second << std::endl;
			dbg << *iter->second << std::endl;
			iter->second->Dump (membership);
		}
		dbg << "Ebd Membership dump" << std::endl;
	}

	if (request.IsHistoryDump ())
	{
		_mailBox.Dump (out);	// Dumps scripts from mailbox and unpacked scripts from history
		_history.DumpMembershipChanges (out);
		_history.DumpDisconnectedScripts (out);
		_sidetrack.Dump (out);
	}

	_history.DumpCmdLog (out);

	dbg << "*There are " << _dataBase.FileCount () << " files/folders recorded in the project database." << std::endl;

	GidList checkedOut;
	_dataBase.ListCheckedOutFiles (checkedOut);
	if (!checkedOut.empty ())
	{
		if (checkedOut.size () < 20)
		{
			dbg << std::endl << "===Checked out files:" << std::endl;
			for (GidList::const_iterator iter = checkedOut.begin ();
				iter != checkedOut.end ();
				++iter)
			
			{
				FileData const * fd = _dataBase.GetFileDataByGid (*iter);
				dbg << *fd;
			}
		}
		else
		{
			dbg << "*There are " << checkedOut.size () << " checked out files/folders." << std::endl;
		}
	}

	GidList synchedOut;
	_dataBase.ListSynchFiles (synchedOut);
	if (!synchedOut.empty ())
	{
		if (synchedOut.size () < 20)
		{
			dbg << "*Local merge files:" << std::endl;
			for (GidList::const_iterator iter = synchedOut.begin ();
				iter != synchedOut.end ();
				++iter)
			
			{
				FileData const * fd = _dataBase.GetFileDataByGid (*iter);
				dbg << *fd;
			}
		}
		else
		{
			dbg << "*There are " << synchedOut.size () << " files/folders in the local merge area." << std::endl;
		}
	}
}

void Model::CreateRange (GidList const & selectedScriptIds,
						 GidSet const & selectedFiles,
						 bool extendedRange,
						 History::Range & range)
{
	if (selectedScriptIds.size () > 1 || extendedRange)
	{
		// More then one script selected or we have a request for the extended range
		if (selectedScriptIds.size () > 1)
		{
			_history.CreateRangeFromTwoScripts (selectedScriptIds.front (),
												selectedScriptIds.back (),
												selectedFiles,
												range);
			if (range.Size () != 0 &&
				selectedScriptIds.size () > 2 &&
				!range.IsSubsetOf (selectedScriptIds))
			{
				// Not all range ids are selected - the range is empty
				range.Clear ();
			}
		}
		else if (selectedScriptIds.size () != 0)
		{
			if (_history.IsRejected (selectedScriptIds.front ()))
			{
				GlobalId firstExecutedScriptId = _history.CreateRangeToFirstExecuted (selectedScriptIds.front (),
																					  selectedFiles,
																					  range);
				Assert (_history.IsBranchPoint (firstExecutedScriptId));
			}
			else
			{
				_history.CreateRangeFromCurrentVersion (selectedScriptIds.front (),
														selectedFiles,
														range);
			}
		}
	}
	else if (selectedScriptIds.size () != 0)
	{
		// Just one script selected and no request for extended range
		range.AddScriptId (selectedScriptIds.front ());
	}
}

bool Model::UpdateHistoricalFiles (History::Range const & range, GidSet const & selectedFiles, bool extendedRange)
{
	SimpleMeter meter (_uiFeedback);
	return _historicalFiles.ReloadScripts (range, selectedFiles, extendedRange, meter);
}

bool Model::UpdateMergedFiles (History::Range const & range, GidSet const & selectedFiles, bool extendedRange)
{
	SimpleMeter meter (_uiFeedback);
	return _mergedFiles.ReloadScripts (range, selectedFiles, extendedRange, meter);
}

void Model::RequestRecovery (UserId memberToAsk, bool dontBlockCheckin)
{
	if (memberToAsk != gidInvalid)
	{
		GidList knownDeadMembers;
		Project::Db const & projectDb = _dataBase.GetProjectDb ();
		projectDb.GetDeadMemberList (knownDeadMembers);
		_history.PruneDeadMemberList(knownDeadMembers);
		Lineage lineage;
		_history.GetCurrentLineage (lineage);

		ScriptHeader hdr (ScriptKindResendRequest (),
						  gidInvalid, // unit ID
						  projectDb.ProjectName ());
		CommandList cmdList;
		std::unique_ptr<ScriptCmd> cmd (new VerificationRequestCmd (lineage.GetReferenceId (),
																  knownDeadMembers));
		cmdList.push_back (std::move(cmd));

		std::unique_ptr<MemberDescription> recipient 
			(projectDb.RetrieveMemberDescription (memberToAsk));	
		Assert (recipient.get () != 0);
		std::string comment ("Requesting project verification from: ");
		comment += recipient->GetName ();
		hdr.AddComment (comment);

		//-------------------------------------------
		Transaction xact (*this, _pathFinder);
		hdr.SetScriptId (_dataBase.XMakeScriptId ());
		std::unique_ptr<CheckOut::List> notification = XGetCheckoutNotification ();
		_mailer.XUnicast (hdr, cmdList, *recipient, notification.get());
		xact.Commit ();
		//-------------------------------------------
	}
	BlockedCheckinMarker blockCheckin (_catalog, GetProjectId ());
	if (dontBlockCheckin)
		blockCheckin.SetMarker (false);	// Remove if exists
	else
		blockCheckin.SetMarker (true);	// Create if doesn't exists
	RecoveryMarker recovery (_catalog, GetProjectId ());
	recovery.SetMarker (memberToAsk != gidInvalid);
}

class BlameReport
{
public:
	void RecordCommand (FileCmd const & cmd, GlobalId scriptId);
	void Save (std::string const & filePath, History::Db const & history);

private:
	typedef std::pair<GlobalId, std::string> EditLine;

private:
	std::vector<EditLine>	_lines;
};

void BlameReport::RecordCommand (FileCmd const & cmd, GlobalId scriptId)
{
	if (cmd.GetType () == typeWholeFile)
	{
		Assert (_lines.size () == 0);
		WholeFileCmd const & newFileCmd = reinterpret_cast<WholeFileCmd const &>(cmd);
		File::Size size = newFileCmd.GetNewFileSize ();
		LogDeserializer in (newFileCmd.GetPath (), newFileCmd.GetOffset ());
		std::vector<char> buf (size.Low ());
		in.GetBytes (&buf [0], size.Low ());
		LineBuf lineBuf (&buf [0], size.Low ());
		for (unsigned i = 0; i < lineBuf.Count (); ++i)
		{
			Line const * fileLine = lineBuf.GetLine (i);
			std::string text (fileLine->Buf (), fileLine->Len ());
			_lines.push_back (std::make_pair(scriptId, text));
		}
	}
	else
	{
		Assert (cmd.GetType () == typeTextDiffFile);
		std::vector<EditLine> tmpLines (_lines.size (),
										std::make_pair(gidInvalid, ""));
		DiffCmd const & diffCmd = reinterpret_cast<DiffCmd const &>(cmd);
		LineArray const & newLines = diffCmd._newLines;
		unsigned clusterCount = diffCmd._clusters.size ();
		int countDelLines = 0;
		int countAddLines = 0;

		for (unsigned i = 0; i < clusterCount; ++i)
		{
			Cluster const & cluster = diffCmd._clusters [i];
			int oldL = cluster.OldLineNo ();
			int newL = cluster.NewLineNo ();
			int cluLen = cluster.Len ();
			
			if (oldL == -1)
			{
				// Add new lines
				std::vector<EditLine> newEditLines;
				for (int j = 0; j < cluLen; j++)
				{
					char const * lineBuf = newLines.GetLine (countAddLines);
					int len = newLines.GetLineLen (countAddLines);
					std::string text (lineBuf, len);
					newEditLines.push_back (std::make_pair(scriptId, text));
					countAddLines++;
				}
				std::vector<EditLine>::iterator insertPos = tmpLines.begin ();
				for (unsigned i = 0; i < tmpLines.size () && i != newL; ++i)
					++insertPos;

				tmpLines.insert (insertPos, newEditLines.begin (), newEditLines.end ());
			}
			else if (newL == -1)
			{
				// Delete lines
				for (int j = 0; j < cluLen; j++)
				{
					unsigned deletedLineNo = oldL + j;
					tmpLines [deletedLineNo].first = gidInvalid;
					countDelLines++;
				}
			}
			else
			{
				// Moved lines
				for (int j = 0; j < cluLen; j++)
				{
					unsigned sourceLineNo = oldL + j;
					unsigned targetLineNo = newL + j;
					tmpLines [targetLineNo] = _lines [sourceLineNo];
				}
			}
		}

		// Done with edits - remove lines with script id equal gidInvalid
		std::vector<EditLine>::iterator iter = tmpLines.begin ();
		while (iter != tmpLines.end ())
		{
			EditLine const & line = *iter;
			if (line.first == gidInvalid)
				iter = tmpLines.erase (iter);
			else
				++iter;
		}
		_lines.swap (tmpLines);
	}
}

void BlameReport::Save (std::string const & filePath, History::Db const & history)
{
	OutStream out (filePath);
	unsigned lineNo = 1;
	for (std::vector<EditLine>::const_iterator iter = _lines.begin ();
		 iter != _lines.end ();
		 ++iter)
	{
		EditLine const & line = *iter;
		GlobalIdPack pack (line.first);
		std::string authorName = history.GetSenderName (pack.GetUserId ());
		MemberNameTag authorTag (authorName, pack.GetUserId ());
		out << "| " << ::ToString (lineNo) << " | " << authorTag << " | " << pack << " | \n";
		std::string::size_type end = line.second.find_first_of ("\n\r");
		if (end == std::string::npos)
		{
			// No CRLF
			out << ' ' << line.second << '|' << std::endl;
		}
		else
		{
			// Strip CRLF
			out << ' ' << line.second.substr (0, end) << '|' << std::endl;
		}
		++lineNo;
	}
}

std::string Model::CreateBlameReport (GlobalId fileGid) const
{
	if (!IsTextual (fileGid))
	{
		TheOutput.Display ("Blame report can be generated only for the textual files.");
		return std::string ();
	}
	SimpleMeter meter (_uiFeedback);
	GidSet unrecoverableFiles;
	Workspace::BlameHistorySelection selection (_history, fileGid, meter);
	Assert (!selection.empty ());
	Workspace::Selection::Sequencer selectionSeq (selection);
	Assert (!selectionSeq.AtEnd ());
	Workspace::HistoryItem const & item = selectionSeq.GetHistoryItem ();
	meter.SetActivity ("Writing the blame report.");
	Workspace::HistoryItem::ForwardCmdSequencer commandSeq (item);
	meter.SetRange (1, commandSeq.Count ());
	GidList const & scriptIds = selection.GetScriptIds ();
	BlameReport blameReport;
	for (GidList::const_iterator scriptIter = scriptIds.begin ();
		 !commandSeq.AtEnd () && scriptIter != scriptIds.end ();
		 commandSeq.Advance (), ++scriptIter)
	{
		meter.StepIt ();
		FileCmd const & cmd = commandSeq.GetCmd ();
		blameReport.RecordCommand (cmd, *scriptIter);
	}
	TmpPath tmpPath;
	std::string filePath (tmpPath.GetFilePath ("BlameReport.wiki"));
	blameReport.Save (filePath, _history);
	return filePath;
}

//
// Transaction support
//

void Model::BeginTransaction ()
{
	TransactableContainer::BeginTransaction ();
	StartNotifying ();
}

void Model::PostCommitTransaction () throw ()
{
	CommitNotifications (IsQuickVisit());
}

void Model::AbortTransaction ()
{
	TransactableContainer::AbortTransaction ();
	AbortNotifying ();
}

void Model::Clear () throw ()
{
	_directory.Clear ();
	_wikiDirectory.Clear ();
	TransactableContainer::Clear ();
}

void Model::Serialize (Serializer& out) const
{
	_dataBase.Save (out);
	_synchArea.Save (out);
	_history.Save (out);
	_sidetrack.Save (out);
	_checkedOutFiles.Save (out);
}

void Model::Deserialize (Deserializer& in, int version)
{
	CheckVersion (version, VersionNo ());
	_dataBase.Read (in);
	_synchArea.Read (in);
	_history.Read (in);

	if (version <= 36)
	{
		if (_dataBase.XGetMyId () == gidInvalid)
		{
			Win::ClearError ();
			throw Win::Exception ("Cannot convert a project that is awaiting a full synch script.\n"
								  "You have to re-install version 4.2, unpack the full synch script\n"
								  "and then install version 4.5 again.\n"
								  "Sorry for the inconvenience!");
		}
		MemoryLog conversionLog;
		try
		{
			// Convert from version 4.2 to version 4.5
			conversionLog << "Converting project " << _dataBase.XProjectName () << " (" << _pathFinder.GetProjectDir () << ")" << std::endl;
			SimpleMeter meter (_uiFeedback);
			_dataBase.XSetOldestAndMostRecentScriptIds (_history, &meter, conversionLog);
			_history.XConvert (&meter, conversionLog);
			_isConverted = true;
		}
		catch (Win::Exception ex)
		{
			std::string exMsg (Out::Sink::FormatExceptionMsg (ex));
			conversionLog << exMsg << std::endl;
			char const * logPath = _pathFinder.GetSysFilePath ("ConversionLog.txt");
			conversionLog.Save (logPath);
		}
		catch ( ... )
		{
			conversionLog << "Unknown exception during project database conversion" << std::endl;
			char const * logPath = _pathFinder.GetSysFilePath ("ConversionLog.txt");
			conversionLog.Save (logPath);
		}
		if (!_isConverted)
		{
			Win::ClearError ();
			throw Win::InternalException ("Project conversion failed. Please, send the following log file to the support@relisoft.com",
									  _pathFinder.GetSysFilePath ("ConversionLog.txt"));
		}
	}
	if (version > 45)
		_sidetrack.Read (in);

	if (50 < version && version < 54)
	{
		// Cleanup after version 4.5
		_history.XMembershipTreeCleanup ();
	}

	_checkedOutFiles.Read (in);
}

void Model::StartNotifying ()
{
	for (TableIter seq = _table.begin (); seq != _table.end (); ++seq)
		(*seq)->StartNotifying ();
}

void Model::CommitNotifications (bool delayRefresh) throw ()
{
	try
	{
		for (TableIter seq = _table.begin (); seq != _table.end (); ++seq)
			(*seq)->CommitNotifications (delayRefresh);
	}
	catch (...)
	{
#if !defined NDEBUG
		TheOutput.Display ("Error during CommitNotifications");
#endif
		Win::ClearError ();
		_needRefresh = true;
	}
}

void Model::AbortNotifying ()
{
	for (TableIter seq = _table.begin (); seq != _table.end (); ++seq)
		(*seq)->AbortNotifying ();
}

bool Model::HasHub (std::string & computerName)
{
	return _catalog.HasHub (computerName);
}
