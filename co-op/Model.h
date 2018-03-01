#if !defined MODEL_H
#define MODEL_H
//------------------------------------
//	(c) Reliable Software, 1996 - 2009
//------------------------------------

#include "DataBase.h"
#include "Mailer.h"
#include "Transact.h"
#include "PathFind.h"
#include "Mailbox.h"
#include "CheckInArea.h"
#include "SynchArea.h"
#include "History.h"
#include "CheckoutNotifications.h"
#include "Table.h"
#include "Catalog.h"
#include "ProjectTable.h"
#include "FolderEvent.h"
#include "DummyTable.h"
#include "FileCtrlState.h"
#include "FileTag.h"
#include "Directory.h"
#include "FeedbackUser.h"
#include "Sidetrack.h"
#include "Permissions.h"
#include "ActivityLog.h"
#include "HistoricalFiles.h"
#include "MergeStatus.h"
#include "HistoryChecker.h"
#include "WikiDirectory.h"
#include "ActiveMerger.h"

#include <File/FolderWatcher.h>

#include <iosfwd>

class Addressee;
class SelectionSeq;
class WindowSeq;
class CheckInUI;
class NewFileData;
class NewFolderData;
class NewProjectData;
class BranchProjectData;
class JoinProjectData;
class ProjDefectData;
class ProjectMembersData;
class ScriptRecipientsData;
class FolderWatcher;
class MemberDescription;
class TransactionFileList;
class FileTyper;
class Memento;
class CommandList;
class TmpProjectArea;
namespace ShellMan
{
	class FileRequest;
	class CopyRequest;
}
namespace Progress
{
	class Meter;
}
class ScriptConflictDlg;
class FileDataSequencer;
class XConflict;
namespace ProgramOptions
{
	class Data;
}
namespace ToolOptions
{
	class Editor;
	class Differ;
	class Merger;
}
namespace Project
{
	class Db;
	class Data;
	class Options;
	class OptionsEx;
	class InviteData;
}
class VerificationReport;
class ProjectChecker;
class FileStateList;
class FileData;
class ScriptProps;
class FileProps;
class ConflictDetector;
class MembershipConflictDetector;
class DispatcherCmd;
class ControlCmd;
class CmdControlExec;
class VersionInfo;
class FullSynchData;
class FileSerializer;
class MemoryLog;
class NewProjTransaction;
class LicensePrompter;
class AckBox;
class DistributorJoinContext;
class JoinContext;
class DiagnosticsRequest;
class SaveFilesData;
class PhysicalFile;
class FileCopyist;

namespace Mailbox
{
	class ScriptInfo;
}
namespace Win
{
	class FileDropHandle;
}
namespace Workspace
{
	class Selection;
	class CheckinSelection;
	class ScriptSelection;
	class HistorySelection;
	class RepairHistorySelection;
}
namespace XML { class Node; }
namespace Project { class Proxy; }

class Model : public TransactableContainer, public TableProvider, public FeedbackUser
{
	friend class ProjectChecker;

public:
	Model (bool quickVisit = false, char const * catPath = 0);

	bool NeedRefresh () const { return _needRefresh; }
	void NoNeedRefresh () { _needRefresh = false; }

	FeedbackManager * SetUIFeedback (FeedbackManager * feedback)
	{
		FeedbackManager * prevFeedbackManager = _uiFeedback;
		_uiFeedback = feedback;
		return prevFeedbackManager;
	}
	FeedbackManager * GetProgressIndicator () const
	{
		return _uiFeedback;
	}

	bool VerifyHistory (History::Verification what) const { return _history.Verify (what); }
	void RepairHistory (History::Verification what, Progress::Meter & meter);
	bool VerifyMembership () const { return _history.VerifyMembership (); }
	void RepairMembership ();
	bool VerifyCheckoutNotifications () const;
	void RepairCheckoutNotifications ();

	GlobalId GetGlobalId(std::string const & fullPath) const;

	Directory & GetDirectory () { return _directory; }
	WikiDirectory & GetWikiDirectory () { return _wikiDirectory; }
	Catalog & GetCatalog () { return _catalog; }
	ActivityLog & GetActivityLog () { return _activityLog; }
	Project::Db const & GetProjectDb () const { return _dataBase.GetProjectDb (); }
	FileIndex const & GetFileIndex () const { return _dataBase; }
	PathFinder & GetPathFinder () { return _pathFinder; }
	ScriptDetails & GetHistoricalFiles () { return _historicalFiles; }
	MergeDetails & GetMergedFiles () { return _mergedFiles; }
	std::unique_ptr<HistoryChecker> GetHistoryChecker () const;

	char const * GetProjectDir () const { return _pathFinder.GetProjectDir (); }
	int GetProjectId () const { return 	_project.GetCurProjectId (); }
	void SetProjectId (int projectId) { _project.Enter (projectId); }
	void UpdateRecentProjects ();

	void OnProjectStateChange ();
	void OnAutoMergeCompleted (ActiveMerger const * merger, bool success);

	std::string const & GetCopyright () const { return _dataBase.GetCopyright (); }
	void SetCopyright (std::string const & copyright);

	std::string const & GetProjectName () const { return _dataBase.ProjectName (); }
	GlobalId GetCurrentVersion () const;
	GlobalId GetSecondLatestVersion () const;
	UserId GetAdminId () const { return _dataBase.GetAdminId (); }
	UserId GetMyId () const { return _dataBase.GetMyId (); }
	GlobalId CheckForkIds (GidList const & clientForkIds, bool deepForks, GidList & myYoungerForkIds)
	{
		return _history.CheckForkIds (clientForkIds, deepForks, myYoungerForkIds);
	}
	bool IsFromLocalBranch (GlobalId scriptId) const { return _history.IsRejected (scriptId); }
	bool IsProjectReady () const { return GetMyId () != gidInvalid && _history.IsFullSyncExecuted (); }
	bool IsInProject () const { return _pathFinder.IsInProject (); }
	bool IsWikiFolder () const;
	bool IsProjectCatalogEmpty ();
	bool CanRestoreFile (GlobalId gid) const;
	bool HasPendingMergeRequests () const { return !_pendingAutoMergers.IsEmpty (); }
	void RetrieveAdminData (std::string & projectName,
							std::string & adminHubId,
							Transport & adminHubTransport);
	std::unique_ptr<MemberDescription> RetrieveUserDescription (UserId userId) const;
	std::vector<MemberInfo> RetrieveRecipients () const 
	{ 
		return _dataBase.RetrieveBroadcastList (); 
	}
	void RetrieveNextVersionInfo (VersionInfo & info) const;
	bool RetrieveVersionInfo (GlobalId scriptId, VersionInfo & info, bool fromMailbox = false) const;
	std::unique_ptr<Memento> CreateMemento ();
	bool RevertTo (Memento const & mem);

	std::unique_ptr<VerificationReport> VerifyProject ();
	void QuietVerifyProject ();
	bool QuickProjectVerify ();

	void VerifyLegalStatus ();
	void LeaveProject (bool removeProjectLock = false);
	void ProcessMail (bool force = false);
	bool IsSynchAreaEmpty () const { return _synchArea.IsEmpty (); }
	bool IsMembershipUpToDate () const { return !_history.HasMembershipUpdatesFromFuture (); }
	bool IsMerge () const;
	bool IsCheckInAreaEmpty () const { return _checkInArea.IsEmpty (); }
	bool IsRootFolder () const { return _directory.IsRootFolder (); }
	bool IsClipboardEmpty () const { return _fileClipboard.size () == 0; }
	bool IsClipboardContentsEqual (Win::FileDropHandle const & fileDrop) const;
	bool RangeHasMissingPredecessors (bool isMerge) const;
	void ClearClipboard () { _fileClipboard.clear (); }
	bool CanCheckIn (bool & blocked) const;
	std::unique_ptr<VerificationReport> VerifyChecksum (GidList const & files, bool recursive);
	void VerifyTreeContents (GlobalId folder, VerificationReport & report, bool recursive);
	bool TestConsistency (std::string const & path, Project::Data & projData);
	bool TestConsistency (Project::Data & projData);
	bool HasFolderRequest () const { return !_folderChange.empty (); }
	std::vector<FolderRequest> & GetFolderRequests () { return _folderChange; }
	bool RetrieveFolderChange (std::string & path) { return _watcher->RetrieveChange (path); }
	bool IsAutoSynch () const { return _dataBase.IsAutoSynch (); }
	bool IsAutoFullSynch () const { return _dataBase.IsAutoFullSynch (); }
	bool IsAutoJoin () const { return _dataBase.IsAutoJoin (); }
	bool IsQuickVisit () const { return _quickVisit; }
	void SetQuickVisit (bool flag) { _quickVisit = flag; }
	void SetMergeTarget (int targetProjectId,
						 GlobalId forkScriptId = gidInvalid);
	void ClearMergeTarget (bool saveHints = false) { _mergedFiles.ClearTargetProject (saveHints); }
	void GetInterestingScripts (GidSet & interestingScripts) const;
	void PrepareMerge (Progress::Meter & progressMeter, HistoricalFiles & historicalFiles);
	bool PrepareMerge (GlobalId gid, HistoricalFiles & historicalFiles);
	TargetStatus GetTargetPath (GlobalId gid,
								std::string const & sourcePath,
								std::string & targetPath,
								FileType & targetType);
	void RequestRecovery (UserId fromMember, bool dontBlockCheckin);

	// Commands
	void ReadProjectDb (Project::Data & projData, bool blind = false);
	void MakeFileRequest (ShellMan::FileRequest & request);
	void MakeHistoricalFileDeleteRequest (GidList const & files,
										  bool isMerge,
										  bool afterScriptChange,
										  bool useProjectRelativePath,
										  FilePath & targetRoot,
										  ShellMan::FileRequest & request);
	void CopyProject (FilePath const & target, NewProjTransaction & projX);
	void MoveProject (int projId, FilePath const & newPath);
	void NewProject (NewProjectData & projectData);
	void NewProjectFromHistory (NewProjectData & projectData,
								ExportedHistoryTrailer const & trailer,
								std::string const & historyFilePath,
								Progress::Meter & progressMeter);
	void NewBranch (BranchProjectData & projData,
					Progress::Meter & progressMeter,
					NewProjTransaction & projX);
	void MarkBranchInHistory ();
	void JoinProject (JoinProjectData & projectData);
	bool InviteToProject (Project::InviteData & inviteData,
						  ProjectChecker & project,
						  Progress::Meter & meter);
	void ProjectOptions (Project::OptionsEx const & options);
	void EditorOptions (ToolOptions::Editor const & dlgData);
	void DifferOptions (ToolOptions::Differ const & dlgData);
	void MergerOptions (ToolOptions::Merger const & dlgData);
	void ProgramOptions (ProgramOptions::Data const & options);
	void MakeReadOnly (VerificationReport::Sequencer notReadOnlyFiles);
	void MakeUncontrolled (VerificationReport::Sequencer uncontrolledItems);
	void CleanupSyncArea (VerificationReport::Sequencer syncAreaItems);
	void RestoreLocalEdits (VerificationReport::Sequencer preservedLocalEdits);
	void MakeProjectFolder (VerificationReport::Sequencer absentFolders);
	void CleanupLocalEdits ();
	void CorrectIllegalNames (VerificationReport & report);
	bool RepairFiles (VerificationReport::Sequencer corruptedFiles);
	bool RecoverFiles (VerificationReport & report);
	bool Defect (ProjDefectData & projectData);
	void ProjectMembers (ProjectMembersData const & projectMembers, bool & isRecoveryNeeded);
	void ProjectNewAdmin (UserId newAdmin);
	bool ExportHistory (std::string const & tgtPath, Progress::Meter & progressMeter) const;
	std::unique_ptr<ExportedHistoryTrailer> RetrieveHistoryTrailer (std::string const & srcPath) const;
	void ImportHistory (std::string const & srcPath, Progress::Meter & progressMeter);
	std::string CreateBlameReport (GlobalId fileGid) const;
	void BuildHistoricalFileCopyist (FileCopyist & copyist,
									 GidList const & files,
									 bool useProjectRelativePath,
									 bool verifyTarget,
									 bool afterScriptChange,
									 bool isMerge);
	void BuildProjectFileCopyist (FileCopyist & copyist,
								  GlobalId scriptId,
								  TmpProjectArea & tmpArea,
								  bool includeLocalEdits,
								  Progress::Meter & meter);

	void GotoRoot ();
	bool FolderUp ();
	bool ChangeDirectory (std::string const & relPath);
	bool NewFile (NewFileData & dlgData);
	bool NewFolder (std::string const & folderName);
	bool CheckOut (GidList & files, bool includeFolders, bool recursive);
	void SetCheckoutNotification (bool isOn);
	bool AddFiles (std::vector<std::string> const & files, FileTyper & fileTyper);
	void AddFile (char const * fullTargetPath, GlobalId gid, FileType type);
	void CopyAddFile (std::string const & srcPath, 
					  std::string const & subDir, 
					  std::string const & fileName, 
					  FileType fileType);
	bool CheckIn (GidList const & files, CheckInUI & checkInUI, bool isInventory);
	bool Uncheckout (GidList const & userSelection, bool quiet);

	void Open (GlobalId gid, UniqueName const * uname = 0);
	void Open (std::string const & fullPath);
	void OpenHistoryDiff (GidList const & files, bool isMerge);
	static void ShellOpen (char const * filePath);
	static void ShellSearch (char const * filePath);

	bool ExecuteSetScript (ScriptConflictDlg & conflictDialog, 
						   Progress::Meter & overallMeter,
						   Progress::Meter & specificMeter,
						   bool & beginnerHelp);
	void ExecuteJoinRequest (ProjectChecker & project, 
							Progress::Meter & overallMeter, 
							Progress::Meter & specificMeter);
	bool AcceptSynch (WindowSeq & seq);
	void AcceptSynch (Progress::Meter & meter);

	bool DeleteFile (GidList const & controlledFiles,bool doDelete = true);
	void DeleteFile (std::string const & fullPath);
	void DeleteScript (SelectionSeq & seq);
	void RemoveFile (GidList const & corruptedFile);
	bool RenameFile (GlobalId renamedGid, UniqueName const & newUname);
	void RenameFile (GlobalId renamedGid, std::string const & newName);
	void ChangeFileType (GidList const & files, FileType newType);
	bool Cut (SelectionSeq & seq);
	bool Copy (SelectionSeq & seq);
	bool Paste (DirectoryListing & nonProjectFiles);
	void ReSendScript (GlobalId scriptId, ScriptRecipientsData const & dlgData);
	void RequestResend (GlobalId scriptId, ScriptRecipientsData const & dlgData);
	bool IsArchive () const;
	bool VersionLabel (CheckInUI & checkInUI);
	void Archive (GlobalId script);
	void UnArchive ();
	void SaveDiagnostics (DiagnosticsRequest const & request);
	void CreateRange (GidList const & selectedScriptIds,
					  GidSet const & selectedFiles,
					  bool extendedRange,
					  History::Range & range);
	bool UpdateHistoricalFiles (History::Range const & range, GidSet const & selectedFiles, bool extendedRange);
	bool UpdateMergedFiles (History::Range const & range, GidSet const & selectedFiles, bool extendedRange);
	void Revert (Progress::Meter & overallMeter, Progress::Meter & specificMeter);
	void Revert (GidList const & files,
				 Progress::Meter & overallMeter,
				 Progress::Meter & specificMeter);
	void MergeAttributes (std::string const & currentPath,
						  std::string const & newPath,
						  FileType newType);
	bool AllIncomingScriptsAreUnpacked () const { return _mailBox.Verify (); }
	void XDoImportHistory (ExportedHistoryTrailer const & trailer,
						   std::string const & historyFilePath,
						   Progress::Meter & meter,
						   bool isNewProjectFromHistory = false);

#if !defined (NDEBUG)
	void UndoScript (Workspace::HistorySelection & selection, GlobalId scriptId, MemoryLog & log);
#endif
	// Transaction support
	void BeginTransaction ();
	void PostCommitTransaction () throw ();
	void AbortTransaction ();
	void Clear () throw ();

	bool ConversionSupported (int versionRead) const { return versionRead >= 23; }
	void Serialize (Serializer& out) const;
	void Deserialize (Deserializer& in, int version);
	bool IsSection () const { return true; }
	int  SectionId () const { return 'RVCS'; }
	int  VersionNo () const { return modelVersion; }

	// Helpers
	void ForceDefect (Project::Data & projData);
	void DoDefect (int removedProjectId, bool forceDefect = false);
	void FindAllByName (std::string const & fileInfo, auto_vector<FileTag> & foundFiles) const;
	void FindAllByGid (GidList const & gids, auto_vector<FileTag> & foundFiles) const;
	void FindAllByComment (std::string const & keyword, GidList & scripts, auto_vector<FileTag> & foundFiles) const;
	void ExpandSubtrees (auto_vector<FileTag> & fileList) const;
	void NotifyDispatcher ();
	FileType GetFileType (GlobalId gid) const;
	void GetState (FileStateList & list);
	void GetVersionId (FileStateList & list, bool isCurrent) const;
	std::string GetVersionDescription (GlobalId versionGid) const;
	char const * GetFullPath (UniqueName const & uname)
	{
		return _pathFinder.GetFullPath (uname);
	}
	void ExtraRepair ();
	void UpdateEnlistmentAddress (std::string const & newHubId);
	void ListControlledFiles (GlobalId folderGid, NocaseSet & files);
	void RetrieveProjectPaths (SelectionSeq & seq, std::vector<std::string> & paths);
	void GetFilesChangedByScript (GlobalId scriptId, GidSet & files) const
	{
		_history.GetFilesChangedByScript (scriptId, files);
	}
	void PreserveLocalEdits (TransactionFileList & fileList);
	std::string GetCheckinAreaCaption () const;

	// Mailbox helpers
	bool HasIncomingOrMissingScripts () const { return _history.HasIncomingOrMissingScripts () ||
													   _mailBox.HasExecutableJoinRequest (); }
	bool HasToBeRejectedScripts () const { return _history.HasToBeRejectedScripts (); }
	bool HasScriptsFromFuture () const { return _mailBox.HasScriptsFromFuture (); }
	bool HasNextScript () const { return _history.HasNextScript (); }
	bool NextIsFullSynch () const { return _history.NextIsFullSynch (); }
	bool NextIsMilestone () const { return _history.NextIsMilestone (); }
	bool HasExecutableJoinRequest () const { return _mailBox.HasExecutableJoinRequest (); }
	std::unique_ptr<ScriptProps> GetScriptProps (GlobalId scriptGid) const;
	std::unique_ptr<ScriptProps> GetScriptProps (GlobalId scriptGid, FileFilter const * filter) const;
	std::unique_ptr<FileProps> GetFileProps (GlobalId fileGid) const;

	// Dump helpers
	void DumpMailbox (std::ostream & out) const { _mailBox.Dump (out); }

	// License service
	Permissions const & GetUserPermissions () const { return _userPermissions; }
	bool PromptForLicense (LicensePrompter & prompter);
	std::string GetLicenseDisplayString () const;
	void ChangeUserLicense (std::string const & newLicense);
	void PropagateLocalLicense ();

	// Table Provider
	std::unique_ptr<RecordSet> Query (Table::Id tableId, Restriction const & restrict);
	std::string QueryCaption (Table::Id tableId, Restriction const & restrict) const;
	bool IsEmpty (Table::Id tableId) const;
	DegreeOfInterest HowInteresting (Table::Id tableId) const;
	bool SupportsHierarchy (Table::Id tableId) const;

	// Dispatcher settings
	bool HasHub (std::string & computerName);
private:
	Table const & GetTable (Table::Id tableId) const;

public:
	// Notification handling
	bool FolderChange (FilePath const & folder);
	std::string GetName (GlobalId gid) const;
	char const * GetFullPath (GlobalId gid);

private:
	std::unique_ptr<FileTag> Model::MakeFileTag (GlobalId gid) const;
	std::unique_ptr<VerificationReport> VerifyProject (Progress::Meter & meter);
	void PreDeleteFile (GidList const & controlledFiles, GidList & controlledFolders);
	void PostDeleteFolder (GidList const & controlledFolders);
	void CollectDeletedFolders (Workspace::ScriptSelection const & scriptSelection, GidList & deletedFolders);
	FileData const * GetFileData (std::string const & fullPath) const;
	void GetDeadCheckoutNotifiers (GidList & members) const;
	// Script processing
	bool UnpackScripts (AckBox & ackBox, bool force = false);
	void SendAcknowledgments (AckBox const & ackBox);
	std::unique_ptr<CheckOut::List> GetCheckoutNotification () const;
	std::unique_ptr<CheckOut::List> XGetCheckoutNotification ();
	void BroadcastCheckoutNotification ();
	bool ProcessMissingScripts ();
	void ProcessScriptDuplicates ();
	void ProcessFullSynchRequests ();
	bool ForwardJoinRequest (std::string const & scriptPath);
	void VerifySender (ScriptHeader const & hdr) const;
	bool ExecuteSetChange (ScriptHeader const & hdr,
						   CommandList const & cmdList,
						   Progress::Meter & overallMeter,
						   Progress::Meter & specificMeter,
						   ConflictDetector const * detector = 0);
	void ExecuteMembershipChange (ScriptHeader const & hdr,
								  CommandList const & cmdList,
								  AckBox & ackBox);
	void MarkMissing (GlobalId scriptId);
	void ResolveConflict (ConflictDetector const & detector);
	void ExecuteForcedScripts (AckBox & ackBox, Progress::Meter & meter);
	void PrepareSetScriptList (ScriptList & setScriptList);
	void CreateFileInventory (GlobalId referenceId, ScriptList & fileScriptList);
	void XCreateFullSynch ( FullSynchData & fullSynchData,
							JoinContext & context,
							AckBox & ackBox);
	void XSendFullSynch (MemberInfo const & joineeMemberInfo,
						 std::string const & joineeOldUserId, 
						 ScriptHeader & fullSynchHdr,
						 ScriptList const & fullSynchPackage, 
						 ScriptHeader & newUserAnnouncementHdr,
						 CommandList const & newUserAnnouncementCmdList, 
						 bool broadcastNeeded,
						 JoinContext & context);
	bool CanAcceptJoinRequest (JoinContext & context);

	void FullSynchResend ();
	void InitPaths ();
	void SetCheckedOutMarker ();
	void SetIncomingScriptsMarker (bool isNewMissing = false);
	void NotifyOtherCoops (int localProjectId = -1) const;
	void BroadcastProjectChange (int localProjectId = -1) const;
	bool Execute (Workspace::Selection & selection, bool extendSelection = true);
	void XExecute (Workspace::Selection & selection,
				   TransactionFileList & fileList,
				   Progress::Meter & meter,
				   bool isVirtual = false);
	void XExecuteRepair (Workspace::RepairHistorySelection & selection,
						TransactionFileList & fileList,
						GidSet & unrecoverableFiles,
						Progress::Meter & meter);
	void ExecuteCheckin (Workspace::CheckinSelection & selection,
						 AckBox & ackBox,
						 std::string const & comment,
						 bool isInventory);
	void XCopy2Synch (Workspace::HistorySelection & conflictSelection,
					  TransactionFileList & fileList,
					  Progress::Meter & meter);
	void XPrepareSynch (Workspace::ScriptSelection & scriptSelection,
						GlobalId scriptId,
						bool isFromSelf);
	void XMerge (TransactionFileList & fileList, Progress::Meter & meter);
	void MergeContent (PhysicalFile & file, Progress::Meter & meter);
	void XInitializeDatabaseAndHistory (std::string const & projectName,
										MemberInfo const & userInfo,
										Project::Options const & options,
										bool isProjectCreated);
	void DoRevert (Workspace::HistorySelection & selection,
				   Progress::Meter & overallMeter,
				   Progress::Meter & specificMeter);
	void Reconstruct (Restorer & restorer);
	void XPrepareRevert (Restorer & restorer, TransactionFileList & fileList);
	void XRevertPath (History::Path const & path,
					  TmpProjectArea & area,
					  TransactionFileList & fileList);
	bool IsProjectFile (GlobalId gid) const;
	bool IsTextual (GlobalId gid) const;
	bool IsFolder (GlobalId gid) const;
	bool HasFolderContentsChanged (GlobalId gid);
	bool HasFileChanged (FileData const * file);
	void GetChangedFiles (GidList & changed);
	char const * GetCurrentPath (GlobalId gid, bool preserveLocalEdits = false);
	void FindCorruptedFiles (VerificationReport & report, 
							 bool & extraRepairNeeded,
							 Progress::Meter & progressMeter);
	bool FindMissingFolders (VerificationReport & report,
							 Progress::Meter & progressMeter);
	void CheckConsistency (VerificationReport & report,
						   Progress::Meter & progressMeter);
	void GetRenameInfo (std::string & info, UniqueName const & oldName, UniqueName const & newName);
	void GetChangeTypeInfo (std::string & info, FileType const & oldType, FileType const & newType);
	void XPublishDefectInfo (bool lastProjectMember, UserId thisUserId, int projId);
	void AddDifferPaths (XML::Tree & xmlTree, FileData const & fileData);
	void AddMergerPaths (XML::Tree & xmlTree, 
						FileData const & fileData,
						std::string const & srcTitle, 
						std::string const & targetTitle,
						std::string const & ancestorTitle);
	void OpenFolder (FileData const * fileData);

	static void ShellExecute (char const * progPath, char const * progArgs);
	void XCheckinStateChange (UserId userId, MemberState newState, AckBox & ackBox);
	void XCheckinMembershipChange (MemberInfo const & updateInfo,
								   AckBox & ackBox,
								   std::unique_ptr<DispatcherCmd> attachment = std::unique_ptr<DispatcherCmd> ());
	void ChangeMemberState (UserId userId, MemberState newState);
	void CleanupAfterAbortedCheckIn (GidList const & files);
	void BuildProjectInfrastructure (Project::Data & projData, PathFinder & pathFinder);
	void CreateEmptyProject (Project::Data & projectData, 
							MemberDescription const & member);
	bool CanDefect (ProjDefectData & projectData);
	void UpdateCatalog (Project::Data & projData, MemberDescription const & user);
	void UpdateAssociationList (Workspace::ScriptSelection const & scriptSelection);
	void XDoCopyProject (FilePath const & target,
						ShellMan::CopyRequest & request,
						NewProjTransaction & projX) const;
	void CleanupFolders (FilePath const & target, 
						 FileDataSequencer & folderSeq,
						 NewProjTransaction & projX) const;
	void XCopyProject (FilePath const & target,
					   TmpProjectArea & tmpArea,
					   bool makeDestinationReadWrite,
					   bool overwriteExisting,
					   NewProjTransaction & projX);
	void MakeTreeList (ShellMan::CopyRequest & request,
						FileDataSequencer & sequencer,
						FilePath const & target);
	void XMakeTreeList (ShellMan::CopyRequest & request,
						FileDataSequencer & sequencer,
						TmpProjectArea & tmpArea,
						FilePath const & target);
	void XPrepareFileDb (History::Range const & undoRange, 
						 TmpProjectArea & tmpArea,
						 Progress::Meter & progressMeter,
						 bool includeLocalEdits = false);
	void CreateUndoRange (GlobalId selectedVersion, History::Range & range) const;
	static bool VerifyNewFileName (std::string const & name);
	static bool VerifyNewFolderName (std::string const & name);
	void StartNotifying ();
	void CommitNotifications (bool delayRefresh) throw ();
	void AbortNotifying ();
	void XBroadcastConversionMembershipUpdate (AckBox & ackBox);
	// Dump helpers
	void DumpProject (XML::Node * enlistment, std::ostream & out, DiagnosticsRequest const & request) const;
	void DumpEmail (std::ostream & out);

	bool RepairProject (ProjectChecker & project, char const * task) const;
	void AcceptJoinRequest (JoinContext & context);

private:
	typedef std::vector<Table const *>::const_iterator TableIter;

private:

	bool						_needRefresh; // UI needs refresh
	std::vector<Table const *>	_table;
	int 						_changeCounter;
	Catalog 					_catalog;
	ActivityLog					_activityLog;
	PathFinder					_pathFinder;
	auto_active<MultiFolderWatcher>	_watcher;
	Directory					_directory;
	DataBase					_dataBase;
	CheckInArea 				_checkInArea;
	History::Db					_history;
	Sidetrack					_sidetrack;
	Mailbox::Db 				_mailBox;
	SynchArea					_synchArea;
	CheckOut::Db				_checkedOutFiles;
	Project::Dir				_project;
	ScriptMailer				_mailer;
	Permissions					_userPermissions;
	auto_vector<FileTag>		_fileClipboard;
	std::vector<FolderRequest>	_folderChange;
	FeedbackManager *			_uiFeedback;
	bool						_quickVisit;	// true, when project visit requested by external application
	bool						_isConverted;	// true, when model has been converted to version 4.5
	WikiDirectory				_wikiDirectory;
	ScriptDetails				_historicalFiles;
	MergeDetails				_mergedFiles;
	ActiveMergerWatcher			_pendingAutoMergers;
	DummyTable					_dummy;
};

#endif

