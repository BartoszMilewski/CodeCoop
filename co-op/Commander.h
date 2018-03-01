#if !defined (COMMANDER_H)
#define COMMANDER_H
//----------------------------------
// (c) Reliable Software 1998 - 2008
//----------------------------------

#include "Memento.h"
#include "BeginnerMode.h"
#include "GlobalId.h"
#include "Table.h"

#include <Ctrl/Command.h>
#include <Win/Win.h> // Revisit

class Model;
class SelectionManager;
class DisplayManager;
class InputSource;
namespace Win
{
	class MessagePrepro;
	class FileDropHandle;
}
namespace Project
{
	class Data;
	class Operation;
}
namespace History
{
	class Range;
}
class PathSequencer;
namespace Progress
{
	class Meter;
	class Dialog;
}
class WindowSeq;
class ProjectChecker;
class FileStateList;
class HistoricalFiles;
class CoopBlocker;
class SafeTmpFile;
class JoinProjectData;
namespace CommandIpc
{
	class Queue;
	class Context;
}

class Commander : public Reversable
{
	friend class ProjectChecker;
	friend class CommandIpc::Context;

public:
	Commander (Model & model,
				Win::CritSection * critSect,
				InputSource * inputSource,
				Win::MessagePrepro & msgPrepro,
				Win::Dow::Handle hwnd);

    // Command execs and testers
	//Program
	void Program_Update ();
    void Program_Options ();
	void Program_Dispatching ();
	void Program_Licensing ();
    void Program_About ();
    void Program_Exit ();

	//Project
    void Project_Visit ();
	Cmd::Status can_Project_Visit () const;
    void Project_Repair ();
	Cmd::Status can_Project_Repair () const;
	void Project_RequestVerification ();
	Cmd::Status can_Project_RequestVerification () const;
	void Project_VerifyRootPaths ();
	void Project_Move ();
	Cmd::Status can_Project_Move () const;
    void Project_Join ();
    void Project_Invite ();
	Cmd::Status can_Project_Invite () const;
    void Project_OpenInvitation ();
    void Project_New ();
	void Project_NewFromHistory ();
	Cmd::Status can_Project_NewFromHistory () const;
	void Project_Branch ();
	Cmd::Status can_Project_Branch () const;
	void Project_Export ();
	Cmd::Status can_Project_Export () const;
    void Project_Members ();
	Cmd::Status can_Project_Members () const;
    void Project_Defect ();
	Cmd::Status can_Project_Defect () const;
    void Project_Admin ();
	Cmd::Status can_Project_Admin () const;
    void Project_Options ();
	Cmd::Status can_Project_Options () const;
    void Project_AddMoreFiles ();
	Cmd::Status can_Project_AddMoreFiles () const;
	void Project_SelectMergeTarget ();
	void Project_SetMergeType ();
	bool VisitProject (int projId, bool remember = true);

	// History
	void History_ExportHistory ();
	Cmd::Status can_History_Export () const;
    void History_ImportHistory ();
	Cmd::Status can_History_Import () const;
	void History_AddLabel ();
	Cmd::Status can_History_AddLabel () const;

	//View
	// Web browsing support
    void View_Back ();
	Cmd::Status can_View_Back () const;
    void View_Forward ();
	Cmd::Status can_View_Forward () const;
    void View_Home ();
	Cmd::Status can_View_Home () const;

	void View_Refresh ();
	Cmd::Status can_View_Refresh () const;
	void View_Default (); // either file view or browser view
    void View_Projects ();
	Cmd::Status can_View_Projects () const;
    void View_MergeProjects ();
	Cmd::Status can_View_MergeProjects () const;
    void View_Files ();
	Cmd::Status can_View_Files () const;
    void View_Mailbox ();
	Cmd::Status can_View_Mailbox () const;
    void View_CheckIn ();
	Cmd::Status can_View_CheckIn () const;
    void View_Synch ();
	Cmd::Status can_View_Synch () const;
    void View_History ();
	Cmd::Status can_View_History () const;
	void View_Browser ();
	Cmd::Status can_View_Browser () const;
    void View_ApplyFileFilter ();
	Cmd::Status can_View_ApplyFileFilter () const;
    void View_ChangeFileFilter ();
	Cmd::Status can_View_ChangeFileFilter () const;
    void View_Hierarchy ();
	Cmd::Status can_View_Hierarchy () const;
	void View_Active_Projects ();
	Cmd::Status can_View_Active_Projects () const;
	void View_ClosePage ();
	Cmd::Status can_View_ClosePage () const;
    void View_Next ();
    void View_Previous ();
	void GoBack ();

	//Folder
    void Folder_GotoRoot ();
	Cmd::Status can_Folder_GotoRoot () const;
    void Folder_GoUp ();
	Cmd::Status can_Folder_GoUp () const;
    void Folder_NewFile ();
	Cmd::Status can_Folder_NewFile () const;
    void Folder_NewFolder ();
	Cmd::Status can_Folder_NewFolder () const;
    void Folder_AddMoreFiles ();
	Cmd::Status can_Folder_AddMoreFiles () const;
	void Folder_Wikify ();
	Cmd::Status can_Folder_Wikify () const;
	void Folder_ExportHtml ();
	Cmd::Status can_Folder_ExportHtml () const;
	void DoNewFile ();
	void DoDeleteFile ();

	//All
    void All_CheckIn ();
	Cmd::Status can_All_CheckIn () const;
    void All_CheckOut ();
	Cmd::Status can_All_CheckOut () const;
	void All_DeepCheckOut ();
	Cmd::Status can_All_DeepCheckOut () const;
    void All_Uncheckout ();
	Cmd::Status can_All_Uncheckout () const;
    void All_AcceptSynch ();
	Cmd::Status can_All_AcceptSynch () const;
    void All_Synch ();
	Cmd::Status can_All_Synch () const;
    void All_Report ();
	Cmd::Status can_All_Report () const;
    void All_Select ();			//  Select -- command accessible only via keyboard accelerator.
	Cmd::Status can_All_Select () const;
	void All_SaveFileVersion ();
	Cmd::Status can_All_SaveFileVersion () const;

	//Selection
    void Selection_CheckOut ();
	Cmd::Status can_Selection_CheckOut () const;
	void Selection_DeepCheckOut ();
	Cmd::Status can_Selection_DeepCheckOut () const;
    void Selection_CheckIn ();
	Cmd::Status can_Selection_CheckIn () const;
    void Selection_Uncheckout ();
	Cmd::Status can_Selection_Uncheckout () const;
    void Selection_Open ();
	void Selection_OpenHistoryDiff ();
	Cmd::Status can_Selection_Properties () const;
    void Selection_Properties ();
	Cmd::Status can_Selection_Open () const;
    void Selection_OpenWithShell ();
	Cmd::Status can_Selection_OpenWithShell () const;
	void Selection_SearchWithShell ();
	Cmd::Status can_Selection_SearchWithShell () const;
    void Selection_Add ();
	Cmd::Status can_Selection_Add () const;
    void Selection_Remove ();
	Cmd::Status can_Selection_Remove () const;
    void Selection_Delete ();
	Cmd::Status can_Selection_Delete () const;
    void Selection_DeleteFile ();
    void Selection_DeleteScript ();
    void Selection_Rename ();
	Cmd::Status can_Selection_Rename () const;
    void Selection_MoveFiles ();
	Cmd::Status can_Selection_MoveFiles () const;
	void Selection_ChangeFileType ();
	Cmd::Status can_Selection_ChangeFileType () const;
    void Selection_AcceptSynch ();
	Cmd::Status can_Selection_AcceptSynch () const;
    void Selection_Synch ();
	Cmd::Status can_Selection_Synch () const;
	void Selection_EditSync ();
	Cmd::Status can_Selection_EditSync () const;
	void Selection_ShowHistory ();
	Cmd::Status can_Selection_ShowHistory () const;
	void Selection_Blame ();
	Cmd::Status can_Selection_Blame () const;
	void Selection_ShowHistoryByFile (Table::Id tableId);
	void Selection_ShowHistoryByFilter ();
	void Selection_SendScript ();
	Cmd::Status can_Selection_SendScript () const;
    void Selection_Report ();
	Cmd::Status can_Selection_Report () const;
    void Selection_Cut ();
	Cmd::Status can_Selection_Cut () const;
    void Selection_Copy ();
	Cmd::Status can_Selection_Copy () const;
    void Selection_Paste ();
	Cmd::Status can_Selection_Paste () const;
	void Selection_Reconstruct ();
	Cmd::Status can_Selection_Reconstruct () const;
	void Selection_Compare ();
	Cmd::Status can_Selection_Compare () const;
	void Selection_CompareWithPrevious ();
	Cmd::Status can_Selection_CompareWithPrevious () const;
	void Selection_CompareWithCurrent ();
	Cmd::Status can_Selection_CompareWithCurrent () const;
	void Selection_Revert ();
	Cmd::Status can_Selection_Revert () const;
	void Selection_Merge ();
	Cmd::Status can_Selection_Merge () const;
	void Selection_MergeBranch ();
	Cmd::Status can_Selection_MergeBranch () const;
	void Selection_Branch ();
	Cmd::Status can_Selection_Branch () const;
	void Selection_VersionExport ();
	Cmd::Status can_Selection_VersionExport () const;
	void Selection_Archive ();
	Cmd::Status can_Selection_Archive () const;
	void Selection_UnArchive ();
	Cmd::Status can_Selection_UnArchive () const;
    void Selection_Add2FileFilter ();
	Cmd::Status can_Selection_Add2FileFilter () const;
    void Selection_RemoveFromFileFilter ();
	Cmd::Status can_Selection_RemoveFromFileFilter () const;
	void Selection_HistoryFilterAdd ();
	Cmd::Status can_Selection_HistoryFilterAdd () const;
	void Selection_HistoryFilterRemove ();
	Cmd::Status can_Selection_HistoryFilterRemove () const;
	void Selection_SaveFileVersion ();
	Cmd::Status can_Selection_SaveFileVersion () const;
	void Selection_RestoreFileVersion ();
	Cmd::Status can_Selection_RestoreFileVersion () const;
	void Selection_MergeFileVersion ();
	Cmd::Status can_Selection_MergeFileVersion () const;
	void Selection_AutoMergeFileVersion ();
	Cmd::Status can_Selection_AutoMergeFileVersion () const;
	void Selection_CopyList ();
	Cmd::Status can_Selection_CopyList () const;
	void Selection_RequestResend();
	Cmd::Status can_Selection_RequestResend() const;
	// Browser/Editor page
	void Selection_Edit ();
	Cmd::Status can_Selection_Edit () const;
	void Selection_Next ();
	Cmd::Status can_Selection_Next () const;
	void Selection_Previous ();
	Cmd::Status can_Selection_Previous () const;
	void Selection_Home ();
	Cmd::Status can_Selection_Home () const;
	void Selection_Reload ();
	Cmd::Status can_Selection_Reload () const;
	void Commander::Selection_CopyScriptComment ();
	Cmd::Status Commander::can_Selection_CopyScriptComment () const;
	void GoBrowse ();
	void OnBrowse ();
	void Navigate ();
	void OpenFile ();

	// Tools
	void Tools_Differ ();
	void Tools_Merger ();
	void Tools_Editor ();
	void Tools_CreateBackup ();
	Cmd::Status can_Tools_CreateBackup () const;
	void Tools_RestoreFromBackup ();
	Cmd::Status can_Tools_RestoreFromBackup () const;
	void Tools_MoveToMachine();

	//Help
    void Help_Contents ();
    void Help_Index ();
	void Help_Tutorial ();
	void Help_Support ();
	void Help_SaveDiagnostics ();
	void Help_RestoreOriginal ();
	Cmd::Status can_Help_RestoreOriginal () const;
	void Help_BeginnerMode ();
	Cmd::Status can_Help_BeginnerMode () const;

	// Undo interface
	std::unique_ptr<Memento> CreateMemento ();
	void RevertTo (Memento const & memento);
	void Clear () { LeaveProject (); }

	// Commands triggered by controls other then menu and tool bar
	void ChangeFilter ();
	bool GetScriptSelection (GidList & scriptIds) const;
	void GetFileSelection (GidSet & fileIds, GidList const & selectedScripts) const;
	void CreateRange ();

	// Commands triggered by control notification handlers
	void DoNewFolder ();
	void DoRenameFile ();
	void DoDrag ();
	Cmd::Status can_Drag () const;
	void SetCurrentFolder ();

	bool HasProgramExpired ();
	void RestoreLastProject ();
	bool DoRecovery (CoopBlocker & coopBlocker);
	void RepairProjects ();

	// Commands used by SccDll
    void Selection_OpenCheckInDiff ();
	void RefreshMailbox ();
	void RestoreVersion ();
	void Maintenance ();
	void GetForkIds (GidList const & clientForkIds,
					 bool deepForks,
					 GlobalId & youngestFoundScriptId,
					 GidList & myYoungerForkIds);
	void GetTargetPath (GlobalId sourceGid,
						std::string const & sourcePath,
						std::string & targetPath,
						unsigned long & targetType,
						unsigned long & targetStatus);
	void ReCreateFile ();
	void MergeAttributes ();
#if !defined (NDEBUG)
	void UndoScript (GlobalId scriptId);
	void RestoreOneVersion ();
#endif

	// Public utilities
	void ConnectGUI (SelectionManager * selectionMan, DisplayManager * displayMan)
	{
		_selectionMan = selectionMan;
		_displayMan = displayMan;
	}
	SelectionManager * GetActiveSelectionMan () { return _selectionMan; }
	void SetInputSource (InputSource * inputSource) { _inputSource = inputSource; }
	void SetAppWin (Win::Dow::Handle win) { _hwnd = win; }
	Win::Dow::Handle  Window () const { return _hwnd; }
	void GetFileState (FileStateList & list);
	void GetVersionId (FileStateList & list, bool isCurrent) const;
	void GetVersionDescription (GlobalId versionGid, std::string & versionDescr) const;
	void PasteFiles (Win::FileDropHandle const & fileDrop);
	void DoDrop ();
	bool IsProjectReady () const;
	// used by Controller
	void DoRefreshMailbox (bool force);
    void RefreshProject (bool force = false);
    void SaveState ();
    void LeaveProject ();

private:
	// Project operation helpers
	bool CanCreateProjects () const;
	bool VisitProject (std::string const & sourcePath);
	bool EnterProject (Project::Data & projData, Memento & memento);
	void DoEnterProject (Project::Data & projData);
	void DoBranch (GlobalId scriptId);
	void DoExport (bool isCmdLine, GlobalId scriptId = gidInvalid);
	void AfterDirChange ();
	void ReverifyMergeStatus (GlobalId gid, HistoricalFiles & historicalFiles);
	bool BuildRange (History::Range & range, GidList & selectedScriptIds, GidSet & selectedFiles);
	void DoMergeFileVersion (HistoricalFiles & historicalFiles, Table::Id fileTableId);
	void DoAutoMergeFileVersion (HistoricalFiles & historicalFiles, Table::Id fileTableId);
	bool GetHistoryFile (SafeTmpFile & historyFile) const;

	void ExecuteJoin (JoinProjectData & joinData);
	// Selection_Open helpers
	void Selection_OpenProject ();
    void Selection_OpenFile ();
    void Selection_OpenIncommingScript ();
    void Selection_OpenHistoryScript ();
    void Selection_OpenSynchDiff ();

    // Command testers helpers
	bool CanRevert (GlobalId selectedVersion) const;
	bool CanMakeChanges () const;
	bool CanCheckin () const;
	bool CanMergeFiles (HistoricalFiles const & historicalFiles) const;
	bool IsSelection () const;
	bool IsSingleExecutedScriptSelected () const;
	bool IsSingleHistoricalExecutedScriptSelected () const;
	bool IsProjectPane () const;
	bool IsProjectPage () const;
	bool IsThisProjectSelected () const;
	bool IsFilePane () const;
	bool IsFilePage () const;
	bool IsMailboxPane () const;
	bool IsMailboxPage () const;
	bool IsCheckInAreaPane () const;
	bool IsCheckInAreaPage () const;
	bool IsSynchAreaPane () const;
	bool IsSynchAreaPage () const;
	bool IsHistoryPane () const;
	bool IsHistoryPage () const;
	bool IsProjectMergePage () const;
	bool IsMergeDetailsPane () const;
	bool IsScriptDetailsPane () const;
	bool IsBrowserPane () const;
	bool IsFilterOn () const;

    void CleanupCatalog (Project::Data & projData);
    void InitWatchers ();
	bool GetFolderContents (std::string const & path,
							NocaseSet const &filter,
							std::vector<std::string> & files,
							bool excludeSpecial = true);
	void AddInitialProjectInventory (std::string const & path, bool makeWiki);
	void DoAllCheckIn (bool isInventory = false);
	void DoReport (bool reportAll) const;
	bool IsChecksumOK (GidList const & files, bool recursive, bool isCheckout);
	void DoCheckout (GidList & files, bool includeFolders, bool recursive);
	void VerifyRecordSet () const;
	bool CheckAndRepairProjectIfNecessary (std::string const & operation);
	void ExecuteJoinRequest ();
	bool ExecuteSetScript (bool & beginnerHelp);
	bool ExecuteAndAcceptScript ();
	void ExecuteAndAcceptAllScripts ();
	bool IsInConflictWithMailbox () const;
	bool DoDropFiles (PathSequencer & pathSeq,
					  bool isPaste,
					  bool allControlledFileOverride = false,
					  bool allControlledFolderOverride = false);

    void SetTitle ();
	void SplitSelection (WindowSeq & seq,
						 GidList & controlledFiles,
						 std::vector<std::string> * notControlledFiles = 0) const;
	bool CheckFileReconstruction (WindowSeq & seq, GidList & okFiles) const;
	void DoSaveFileVersion (WindowSeq & seq);
	void DoAcceptSynch (WindowSeq & seq);
	void CloseSyncAreaView ();

	void DisplayTip (int msgId, bool isForce = false);
	std::unique_ptr<Progress::Dialog> CreateProgressDlg (std::string const & title,
													   bool canCancel = true,
													   unsigned initialDelay = 1000,
													   bool multiMeter = false) const;

private:
	static char const 	_mergeWarning [];

private:
	// Commander is already locked when executing commands
	// This is only for occasional use in dialog controllers
	// (Dialogs are unlocked to allow server actions)
	Win::CritSection *	_critSect;
	Model &				_model;
	SelectionManager *	_selectionMan;
	DisplayManager *	_displayMan;
	InputSource *		_inputSource;
	Win::MessagePrepro &_msgPrepro;
	BeginnerMode		_coopTips;
	Win::Dow::Handle	_hwnd;	// Needed by progress meter
	bool				_rememberRecentProject;
};

#endif
