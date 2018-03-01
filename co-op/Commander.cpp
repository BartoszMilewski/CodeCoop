//----------------------------------
// (c) Reliable Software 1997 - 2009
//----------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "Commander.h"
#include "NewProj.h"
#include "ProjectBranchDlg.h"
#include "MoveFilesDlg.h"
#include "NewFile.h"
#include "JoinProj.h"
#include "JoinProjectData.h"
#include "ProjDefect.h"
#include "ExportProject.h"
#include "MoveProj.h"
#include "CheckInDlg.h"
#include "ProjectMembers.h"
#include "ScriptRecipients.h"
#include "Registry.h"
#include "FileTyper.h"
#include "AppHelp.h"
#include "AppInfo.h"
#include "AboutDlg.h"
#include "Predicate.h"
#include "GidPredicate.h"
#include "SelectionMan.h"
#include "DisplayMan.h"
#include "Model.h"
#include "OutputSink.h"
#include "TimeLock.h"
#include "Prompter.h"
#include "ConflictDlg.h"
#include "RevertDlg.h"
#include "AddFilesDlg.h"
#include "LicenseDlg.h"
#include "ProjectPropsDlg.h"
#include "ProgramOptionsDlg.h"
#include "ToolOptions.h"
#include "ToolOptionsDlg.h"
#include "PathSequencer.h"
#include "FolderContents.h"
#include "CmdLineSelection.h"
#include "InputSource.h"
#include "ChecksumDlg.h"
#include "VerificationReport.h"
#include "VerifyReportDlg.h"
#include "ProjectAdminDlg.h"
#include "ProjectChecker.h"
#include "CoopCaption.h"
#include "DispatcherProxy.h"
#include "FileDropDlg.h"
#include "ScriptHeader.h"
#include "ScriptProps.h"
#include "FileProps.h"
#include "ScriptPropertySheet.h"
#include "Workspace.h"
#include "UpdateHubIdDlg.h"
#include "HideExtensionDlg.h"
#include "ProjectInviteDlg.h"
#include "ProjectInviteData.h"
#include "OverdueScriptDlg.h"
#include "SelectProjectDlg.h"
#include "BackupDlg.h"
#include "CoopMemento.h"
#include "LicensePrompter.h"
#include "ProjectOptionsEx.h"
#include "Diagnostics.h"
#include "ReportDlg.h"
#include "CoopExceptions.h"
#include "UserIdPack.h"
#include "HistoryRange.h"
#include "HistoricalFiles.h"
#include "HistoryScriptState.h"
#include "HistoryFilter.h"
#include "Encryption.h"
#include "SaveFilesDlg.h"
#include "WikiDef.h"
#include "FileDropInfoCollector.h"
#include "Bookmark.h"
#include "HistoryChecker.h"
#include "MergeExec.h"
#include "MergerProxy.h"
#include "AltDiffer.h"
#include "SaveHtml.h"
#include "CoopMsg.h"
#include "UiStrings.h"
#include "SccProxy.h"
#include "CoopBlocker.h"
#include "ProjectRecoveryDlg.h"
#include "SelectRootPathDlg.h"
#include "ProjectMarker.h"
#include "BackupRestorer.h"
#include "BackupArchive.h"
#include "ProjectPathVerifier.h"
#include "PathRegistry.h"
#include "FilePropertiesDlg.h"
#include "FileCopyist.h"
#include "FtpProxy.h"
#include "ExportImportHistoryDlg.h"
#include "ExportedHistoryTrailer.h"

#if !defined (NDEBUG)
#include "MemoryLog.h"
#endif

#include <Ctrl/ProgressDialog.h>
#include <Ctrl/MultiProgressDialog.h>
#include <Com/Shell.h>
#include <Com/ShellRequest.h>
#include <Com/DragDrop.h>
#include <Ctrl/FileGet.h>
#include <Sys/WinString.h>
#include <Sys/Clipboard.h>
#include <Win/MsgLoop.h>
#include <Ex/WinEx.h>
#include <Ex/Error.h>
#include <Win/EnumProcess.h>
#include <Net/Ftp.h>
#include <XML/XmlTree.h>
#include <File/SafePaths.h>
#include <File/Vpath.h>

#include <auto_array.h>
#include <PtrUtil.h>

#include <fstream>

char const Commander::_mergeWarning [] = "Synchronization required automatic merging\n"
										 "of script changes with your local changes.\n\n"
										 "Please verify merge(s) and accept sync.";
void TriggerPostSync(char const * projectDir, Win::Dow::Handle hwnd)
{
	FilePath projectRootFolder(projectDir);
	char const * onSynchBatchPath = projectRootFolder.GetFilePath ("OnSync.bat");
	if (File::Exists (onSynchBatchPath))
	{
		ShellMan::Execute (hwnd, onSynchBatchPath, "");
	}
	onSynchBatchPath = projectRootFolder.GetFilePath ("OnSynch.bat");
	if (File::Exists (onSynchBatchPath))
	{
		ShellMan::Execute (hwnd, onSynchBatchPath, "");
	}
}

void TriggerPostCheckin(char const * projectDir, Win::Dow::Handle hwnd)
{
	FilePath projectRootFolder(projectDir);
	char const * onCheckinBatchPath = projectRootFolder.GetFilePath ("OnCheckin.bat");
	if (File::Exists (onCheckinBatchPath))
	{
		ShellMan::Execute (hwnd, onCheckinBatchPath, "", projectRootFolder.GetDir());
	}
}

Commander::Commander (Model & model,
					  Win::CritSection * critSect,
					  InputSource * inputSource,
					  Win::MessagePrepro & msgPrepro,
					  Win::Dow::Handle hwnd)
	: _model (model),
	  _critSect (critSect),
	  _hwnd (hwnd),
	  _inputSource (inputSource),
	  _msgPrepro (msgPrepro),
	  _rememberRecentProject (!_model.IsQuickVisit ())
{
	if (_model.IsQuickVisit ())
		_coopTips.Off ();
}

bool Commander::HasProgramExpired ()
{
#if defined BETA
	// Beta time bomb
	TimeLimit timeLimit;
	if (timeLimit.HasExpired ())
	{
		TheOutput.Display ("This beta version of Code Co-op has expired\n"
			"Visit our web page http:\\\\www.relisoft.com\n"
			"for the latest copy of Code Co-op", Out::Error);
		return true;
	}
	else if (timeLimit.AboutToExpire ())
	{
		std::string info ("This beta version of Code Co-op will expire on ");
		info += timeLimit.GetExpirationDateStr ();
		info += "\nVisit our web site http:\\\\www.relisoft.com\nfor the latest copy of Code Co-op";
		TheOutput.Display (info.c_str (), Out::Warning);
	}
#endif

	if (_model.IsQuickVisit ())
		return false;	// In server mode we don't check license or trial period

#if !defined BETA
	if (Registry::IsNagging () && !_model.GetUserPermissions ().HasValidLicense ())
	{
		LicensePrompter prompter ("HasProgramExpired");
		_model.PromptForLicense (prompter);
	}
#endif
	return false;
}

void Commander::Program_Update ()
{
	DispatcherProxy dispatcher;
	dispatcher.CheckForUpdate ();
}

void Commander::Program_Dispatching ()
{
	DispatcherProxy dispatcher;
	dispatcher.ShowConfigurationWizard ();
}

void Commander::Program_Licensing ()
{
	LicensePrompter prompter ("Program_Licensing");
	_model.PromptForLicense (prompter);
}

void Commander::Program_About ()
{
	std::unique_ptr<MemberDescription> thisUser = _model.RetrieveUserDescription (_model.GetMyId ());
    HelpAboutData dlgData (thisUser.get (),
						   _model.GetLicenseDisplayString (),
						   _model.GetProjectName (),
						   _model.GetProjectDir ());
    HelpAboutCtrl ctrl (&dlgData);
	ThePrompter.GetData (ctrl);
}

void Commander::Program_Exit ()
{
	throw Win::ExitException (0);
}

//
// Project commands
//

// Visit an existing local project
void Commander::Project_Visit ()
{
	if (IsProjectPane () || _model.IsQuickVisit ())
	{
		if (!_displayMan->IsEmptyPage (ProjectPage) && IsSelection ())
		{
			SelectionSeq seq (_selectionMan, Table::projectTableId);
			VisitProject (seq.GetGlobalId ());
		}
	}
	else
	{
		View_Projects ();
	}
}

Cmd::Status Commander::can_Project_Visit () const
{
	return _model.IsProjectCatalogEmpty () ? Cmd::Disabled : Cmd::Enabled;
}

bool Commander::CanCreateProjects () const
{
	if (!_model.GetUserPermissions ().CanCreateNewProject ())
	{
		LicensePrompter prompter ("Project_New");
		if (!_model.PromptForLicense (prompter))
			return false;		// No valid license provided
	}
	return true;
}

bool Commander::VisitProject (std::string const & sourcePath)
{
	std::unique_ptr<Memento> memento (CreateMemento ());
	LeaveProject ();
	Project::Data projData;
	if (!_model.GetCatalog ().GetProjectData (sourcePath, projData))
	{
		// Project could not be found in the catalog.
		// Search by source path failed.
		return false;
	}
	return EnterProject (projData, *memento);
}

bool Commander::VisitProject (int projId, bool remember)
{
	Assert (projId != -1);
	Project::Data projData;
	Catalog & projectCatalog = _model.GetCatalog ();
	projectCatalog.GetProjectData (projId, projData);
	if (!projData.IsValid ())
		return false;
	FilePath projectRootFolder (_model.GetProjectDir ());
	if (projectRootFolder.IsEqualDir (projData.GetRootDir ()))
	{
		// We are already in this project
		if (!_model.IsQuickVisit ())
			View_Default (); 
		return true;
	}

	std::unique_ptr<Memento> memento (CreateMemento ());
	LeaveProject ();
	_rememberRecentProject = remember;
	return EnterProject (projData, *memento);
}

bool Commander::EnterProject (Project::Data & projData, Memento & memento)
{
    try
    {
		for ( ; ; )
		{
			if (_model.TestConsistency (projData))
			{
				DoEnterProject (projData);
				return true;
			}
				// Consistency test failed
			if (!_model.GetCatalog ().IsProjectUnavailable (projData.GetProjectId ()))
			{
				// Project in use
				RevertTo (memento);
				return false;
			}

			// Project is unavailable - ask the user if he/she wants to move the
			// project to another place
			std::vector<Project::Data> unavailableProject;
			unavailableProject.push_back (projData);
			Project::PathClassifier pathClassifier (unavailableProject);
			Project::PathClassifier::Sequencer seq (pathClassifier);
			SelectRootData dlgData (seq.GetProjectSequencer ());
			SelectRootCtrl dlgCtrl (dlgData);
			if (ThePrompter.GetData (dlgCtrl))
			{
				// User OK'ed root path selection dialog.
				// Change project root path in the catalog.
				Assert (!dlgData.GetNewPrefix ().empty ());
				FilePath newPath (dlgData.GetNewPrefix ());
				for (FullPathSeq pathSeq (projData.GetRootDir ()); !pathSeq.AtEnd (); pathSeq.Advance ())
					newPath.DirDown (pathSeq.GetSegment ().c_str ());

				_model.GetCatalog ().MoveProjectTree (projData.GetProjectId (), newPath);
				projData.SetRootPath (newPath);
			}
			else
			{
				// User doesn't want to move unavailable project
				RevertTo (memento);
				return false;
			}
		}
    }
	catch (Win::ExitException)
	{
		throw;	// Pass it to the controller
	}
	catch (InaccessibleProject ix)
	{
        TheOutput.Display (ix);
		RevertTo (memento);
		_displayMan->Refresh (ProjectPage);
	}
    catch (Win::Exception ex)
    {
        TheOutput.Display (ex);
        CleanupCatalog (projData);
		RevertTo (memento);
		_displayMan->Refresh (ProjectPage);
    }
	return false;
}

void Commander::DoEnterProject (Project::Data & projData)
{
	_model.ReadProjectDb (projData);
	SetTitle ();
	std::unique_ptr<VerificationReport> report = _model.VerifyProject ();
	if (report.get () != 0 && !report->IsEmpty ())
	{
		// Project verification errors found -- display them
		Project::Path relativePath (_model.GetFileIndex ());
		VerifyReportCtrl ctrl (*report, _model.GetProjectName (), relativePath);
		ThePrompter.GetData (ctrl, _inputSource);
		// Perform corrective actions

		// First check and repair history
		if (!_model.VerifyHistory (History::Creation))
		{
			Progress::Meter meter;	
			_model.RepairHistory(History::Creation, meter);
		}

		_model.RecoverFiles (*report);
	}
	RefreshProject ();
	if (!_model.IsQuickVisit () && _model.IsProjectReady ())
	{
		// Check if this user has the right hub's email address
		std::unique_ptr<MemberDescription> thisUser = _model.RetrieveUserDescription (_model.GetMyId ());
		Catalog & catalog = _model.GetCatalog ();
		std::string catalogHubId (catalog.GetHubId ());
		if (!catalogHubId.empty () && !IsNocaseEqual (catalogHubId, thisUser->GetHubId ()))
		{
			if (Catalog::IsHubIdUnknown(thisUser->GetHubId ()))
			{
				// quietly change from "Unknown" to catalogHubId
				_model.UpdateEnlistmentAddress (catalogHubId);
			}
			else if (!Catalog::IsHubIdUnknown(catalogHubId)) // don't change to Unknown!
			{
				// Ask the user if he wants to update his hub's email address
				std::string userAddress (thisUser->GetHubId ());
				userAddress += ", ";
				userAddress += _model.GetProjectName ();
				userAddress += ", ";
				userAddress += thisUser->GetUserId ();
				UpdateHubIdCtrl ctrl (catalogHubId, userAddress);
				if (ThePrompter.GetData (ctrl, _inputSource))
				{
					_model.UpdateEnlistmentAddress (catalogHubId);
				}
			}
		}
		if (!_model.VerifyHistory (History::Membership))
			TheOutput.Display ("There are inconsistencies in your project history.\n\n"
							   "Please run Project>Repair.\n"
							   "If this doesn't help, contact support@relisoft.com");

		RecoveryMarker recovery(_model.GetCatalog(), _model.GetProjectId());
		if (recovery.Exists())
		{
			BlockedCheckinMarker blockedCheckin(_model.GetCatalog(), _model.GetProjectId());
			if (blockedCheckin.Exists())
			{
				TheOutput.Display("Check-ins are blocked awaiting verification report.\n"
					"Use Project>Request Verification if you've been waiting for too long.");
			}
			else
			{
				TheOutput.Display("Project is awaiting verification report.\n"
					"Use Project>Request Verification to ask another member for verification.");
			}
		}
	}
}

void Commander::Project_Move ()
{
	// Get new root path from user
	MoveProjectData dlgData (_model.GetCatalog ());
	MoveProjectCtrl ctrl (dlgData);
	if (!ThePrompter.GetData (ctrl, _inputSource))
		return;

	if (!dlgData.IsVirtualMove () && !CheckAndRepairProjectIfNecessary ("Project Move"))
	{
		TheOutput.Display ("Project cannot be moved until it is successfully repaired.");
		return;
	}
	FilePath oldRoot (_model.GetProjectDir ());
	int projId = _model.GetProjectId ();
	ShellMan::FileRequest request;
	{
		Project::Data const & project = dlgData.GetProject ();
		FilePath const & newRoot = project.GetRootPath ();
		Assert (project.GetProjectId () == -1); // don't remove it from Catalog
		// If copy doesn't succeed, projX will clean up the target
		NewProjTransaction projX (_model.GetCatalog (), project);
		if (!dlgData.IsVirtualMove ())
		{
			PathFinder::MaterializeFolderPath (project.GetRootDir ());
			if (dlgData.MoveProjectFilesOnly ())
			{
				// Copy enlisted files from oldRoot to newRoot
				_model.CopyProject (newRoot, projX);

				// Prepare files from the old root for deletion
				_model.MakeFileRequest (request);
			}
			else
			{
				ShellMan::CopyContents (_hwnd, oldRoot.GetDir (), newRoot.GetDir (), ShellMan::UiNoConfirmation);
			}
		}
		// Register new root in the catalog
		_model.MoveProject (projId, newRoot);
		projX.Commit ();
	}

	VisitProject (projId);

	if (!dlgData.IsVirtualMove ())
	{
		if (dlgData.MoveProjectFilesOnly ())
		{
			// Delete project files from source root -- place them in the recycle bin
			request.DoDelete (_hwnd);
			char const * path = oldRoot.GetDir ();
			if (File::CleanupTree (path))
				ShellMan::QuietDelete (_hwnd, path);
		}
		else
		{
			// Delete whole source project tree -- place its contents in the recycle bin
			ShellMan::Delete (_hwnd, oldRoot.GetDir (), ShellMan::UiNoConfirmationAllowUndo);
		}
	}
}

Cmd::Status Commander::can_Project_Move () const
{
	return IsThisProjectSelected () ? Cmd::Enabled : Cmd::Disabled;
}

// Create a New project from scratch
void Commander::Project_New ()
{
	dbg << "Commander::Project_New" << std::endl;
	if (!CanCreateProjects ())
		return;

	NewProjectData newProjectData (_model.GetCatalog ());
	NewProjectHndlrSet newProjectHndlrSet (newProjectData);
	if (ThePrompter.GetData (newProjectHndlrSet, _inputSource))
    {
		Project::Data & project = newProjectData.GetProject ();

		{
			NewProjTransaction projX (_model.GetCatalog (), project);
			_model.NewProject (newProjectData);
			projX.Commit ();
		}

		VisitProject (project.GetProjectId ());
		_displayMan->Refresh (ProjectPage);
		try
		{
			AddInitialProjectInventory (project.GetRootDir (),
										newProjectData.GetOptions ().IsWiki ());
			_displayMan->Refresh (FilesPage);
		}
		catch ( ... )
		{
			// Any exception during adding initial file inventory
			// doesn't break new project creation. Files are not added
			// to the project, but project is created.
			Win::ClearError ();
		}
    }
}

void Commander::Project_NewFromHistory ()
{
	if (!CanCreateProjects ())
		return;

	SafeTmpFile tmpProjectHistory;
	if (GetHistoryFile (tmpProjectHistory))
	{
		std::unique_ptr<ExportedHistoryTrailer> trailer =
			_model.RetrieveHistoryTrailer (tmpProjectHistory.GetFilePath ());

		NewProjectData newProjectData (_model.GetCatalog ());
		Project::Options & options = newProjectData.GetOptions ();
		options.SetNewFromHistory (true);
		Project::Data & project = newProjectData.GetProject ();
		project.SetProjectName (trailer->GetProjectName ());
		NewProjectHndlrSet newProjectHndlrSet (newProjectData);
		if (ThePrompter.GetData (newProjectHndlrSet, _inputSource))
		{
			std::unique_ptr<Progress::Dialog> meterDialog (CreateProgressDlg ("New Project",
																			true,	// User can cancel
																			0));	// Show immediately
			std::string caption ("Creating project '");
			caption += newProjectData.GetProject ().GetProjectName ();
			caption += "' from the history of '";
			caption += trailer->GetProjectName ();
			caption += "' project.";
			meterDialog->SetCaption (caption);
			Project::Data & project = newProjectData.GetProject ();

			{
				NewProjTransaction projX (_model.GetCatalog (), project);
				_model.NewProjectFromHistory (newProjectData,
											  *trailer,
											  tmpProjectHistory.GetFilePath (),
											  meterDialog->GetProgressMeter ());
				projX.Commit ();
			}

			VisitProject (project.GetProjectId ());
			_displayMan->Refresh (ProjectPage);
		}
	}
}

Cmd::Status Commander::can_Project_NewFromHistory () const
{
#if defined (COOP_PRO)
	return Cmd::Enabled;
#else
	return Cmd::Invisible;
#endif
}

void Commander::Project_Branch ()
{
	DoBranch (gidInvalid);
}

Cmd::Status Commander::can_Project_Branch () const
{
	return IsThisProjectSelected () ? Cmd::Enabled : Cmd::Disabled;
}

void Commander::Project_Export ()
{
	DoExport (_model.IsQuickVisit (), gidInvalid);
}

Cmd::Status Commander::can_Project_Export () const
{
	return IsThisProjectSelected () ? Cmd::Enabled : Cmd::Disabled;
}

class BegHelpOff
{
public:
	BegHelpOff (BeginnerMode & beg)
		: _beg (beg)
	{
		_wasOn = _beg.IsModeOn ();
		_beg.Off ();
	}
	~BegHelpOff ()
	{
		if (_wasOn)
			_beg.On ();
	}
private:
	bool			_wasOn;
	BeginnerMode &	_beg;
};

// Returns true when user selected some files
bool Commander::GetFolderContents (std::string const & path,
								   NocaseSet const & filter,
								   std::vector<std::string> & files,
								   bool excludeSpecial)
{
	std::string caption ("Scanning folder ");
	caption += path; 
	std::unique_ptr<Progress::Dialog> meterDialog (CreateProgressDlg ("Add More Files"));
	meterDialog->SetCaption (caption);
	// Scan folder contents
	FolderContents contents (path, filter, meterDialog->GetProgressMeter (), excludeSpecial);
	if (!contents.IsEmpty ())
	{
		contents.Sort (meterDialog->GetProgressMeter ());
		meterDialog.reset ();
		AddFilesCtrl ctrl (&contents);
		if (ThePrompter.GetData (ctrl))
		{
			for (FolderContentsSequencer selectedFiles (contents);
				 !selectedFiles.AtEnd ();
				 selectedFiles.Advance ())
			{
				char const * filePath = selectedFiles.GetFilePath ();
				files.push_back (filePath);
			}
			return true;
		}
	}
	else
	{
		meterDialog.reset ();
		if (!filter.empty ())
			TheOutput.Display ("Nothing to add. All files in the project tree are already controlled.");
	}
	return false;
}

class PathVectorSequencer : public PathSequencer
{
public:
	PathVectorSequencer (std::vector<std::string> const & files)
		: _size (files.size ()),
		  _cur (files.begin ()),
		  _end (files.end ())
	{}

	bool AtEnd () const { return _cur == _end; }
	void Advance () { ++_cur; }
	unsigned int GetCount () const { return _size; }

	char const * GetFilePath () const { return (*_cur).c_str (); }

private:
	unsigned int								_size;
	std::vector<std::string>::const_iterator	_cur;
	std::vector<std::string>::const_iterator	_end;
};

void Commander::AddInitialProjectInventory (std::string const & path, bool makeWiki)
{
	if (_model.IsQuickVisit ())
		return;

	BegHelpOff noHelp (_coopTips);
	std::vector<std::string> files;
	NocaseSet emptyFilter;
	GetFolderContents (path, emptyFilter, files);

	std::string caption ("Adding project ");
	caption += _model.GetProjectName ();
	caption += " initial file inventory.";
	std::unique_ptr<Progress::Dialog> meterDialog (CreateProgressDlg ("New Project",
																	true,	// Can cancel
																	0));	// Show immediately
	meterDialog->SetCaption (caption);
	Progress::Meter & meter = meterDialog->GetProgressMeter ();
	meter.SetRange (0, 3);
	meter.SetActivity ("Storing files in the database.");
	meter.StepAndCheck ();

	if (makeWiki)
	{
		FilePath rootPath = path;
		char const * wikiFilePath = rootPath.GetFilePath ("index.wiki");
		std::vector<std::string>::const_iterator it =
			std::find_if (files.begin (), files.end (), NocaseEqual (wikiFilePath));
		if (it == files.end ())
		{
			File wikiFile (wikiFilePath, File::OpenAlwaysMode ());
			files.push_back (wikiFilePath);
		}
	}

	if (files.empty ())
		return;		// User didn't select any files

	FileTyper fileTyper (_hwnd, _model.GetCatalog (), _inputSource);
	_model.AddFiles (files, fileTyper);

	// Notice: we are creating a temporary substitute for
	// the selection manager. It will be used by Selection_Add
	// as the source of files to be added to project.
	// Temporary InputSource will be used by All_CheckIn for comment

	meter.SetActivity ("Checking in files and folders.");
	meter.StepAndCheck ();

	InputSource newInputSource;
	InputParser parser ("-All_CheckIn comment:\"File(s) added during project creation\"",
						newInputSource);
	parser.Parse ();

	CmdLineSelection newSelMan (_model, _model.GetDirectory ());
	// Add files to the selection
	PathVectorSequencer seq (files);
	newSelMan.SetSelection (seq);

	TempSubstPtr<SelectionManager> subst (_selectionMan);
	subst.Switch (&newSelMan);
	// Now check the files in
	TempSubstPtr<InputSource> substInput (_inputSource);
	substInput.Switch (&newInputSource);
	DoAllCheckIn (true);	// Check-in initial file inventory
}

class VirtualFolder
{
public:
	VirtualFolder (Directory &  directory)
		: _directory (directory),
		  _startRelativeFolder (_directory.GetCurrentRelativePath ()),
		  _wasChanged (false)
	{
	}
	~VirtualFolder ()
	{
		try
		{
			if (_wasChanged)
			{
				_directory.Change (_startRelativeFolder);
			}
		}
		catch ( ... )
		{
		}
	}

	void Change (std::string const & path)
	{
		_wasChanged = _directory.Change (path) || _wasChanged;
	}

	void Down (std::string const & folder)
	{
		_wasChanged = true;
		_directory.Down (folder);
	}

private:
	Directory & _directory;
	std::string	_startRelativeFolder;
	bool		_wasChanged;
};

// Called by the executor object using PostCommand("DoDrop", ...);
void Commander::DoDrop ()
{
	Assert (IsProjectReady () && IsFilePane ());
	StringExtractor targetExtractor ("DropTargetString");
	ThePrompter.GetData (targetExtractor);
	std::string const & targetName = targetExtractor.GetString ();
	StringExtractor fileDropExtractor ("FileDropHandle");
	ThePrompter.GetData (fileDropExtractor);
	unsigned long handle = 0;
	::HexStrToUnsigned(fileDropExtractor.GetString ().c_str (), handle);
	Win::FileDropHandle fileDrop (handle);
	Win::FileDropHandle::Sequencer fileDropSeq (fileDrop);

	if (targetName.empty ())
	{
		// Drop target not specified
		// File drop in the current folder
		if (DoDropFiles (fileDropSeq, false))	// Not a paste operation
			_displayMan->Refresh (FilesPage);
	}
	else
	{
		// Drop target specified
		bool refreshFileView = false;
		StringExtractor targetTypeExtractor ("TargetType");
		ThePrompter.GetData (targetTypeExtractor);
		std::string const & targetType = targetTypeExtractor.GetString ();
		Directory & directory = _model.GetDirectory ();
		VirtualFolder currentFolder (directory);
		if (targetType == "RootRelative")
		{
			// File drop in some other folder
			// Drop target from folder hierarchy
			currentFolder.Change (targetName);
		}
		else
		{
			// Drop target from file view
			if (File::IsFolder (directory.GetFilePath(targetName)))
			{
				// File drop in some other folder
				currentFolder.Down (targetName);
			}
			else
			{
				// Received target is not a folder
				// File drop in the current folder
				refreshFileView = true;
			}
		}

		DoDropFiles (fileDropSeq, false);	// Not a paste operation
		if (refreshFileView)
			_displayMan->Refresh (FilesPage);
	}
}

void Commander::PasteFiles (Win::FileDropHandle const & fileDrop)
{
	if (!(IsProjectReady () && IsFilePane ()))
		return;

	Win::FileDropHandle::Sequencer fileDropSeq (fileDrop);
	if (DoDropFiles (fileDropSeq, true))
		_displayMan->Refresh (FilesPage);

}

// Returns true when files dropped sucessfuly
bool Commander::DoDropFiles (PathSequencer & pathSeq,
							 bool isPaste,
							 bool allControlledFileOverride,
							 bool allControlledFolderOverride)
{
	if (pathSeq.GetCount () == 0)
		return true;	// Done

	FileDropInfoCollector collector (_hwnd,
									 _model.GetProjectName (),
									 CanMakeChanges (),
									 isPaste,
									 _model.GetDirectory (),
									 _model.GetFileIndex ());
	if (!collector.GatherFileInfo (pathSeq, allControlledFileOverride, allControlledFolderOverride))
		return false;	// User canceled drop preparation

	if (collector.HasFilesToCheckout ())
	{
		GidList & controlledFiles = collector.GetFilesToCheckout ();
		_model.CheckOut (controlledFiles, true, false);	// Include folders and non-recursive checkout
	}

	bool copyFailed = false;
	try
	{
		ShellMan::CopyRequest & copyRequest = collector.GetCopyRequest ();
		copyRequest.DoCopy (_hwnd, "Copying files");
		copyRequest.MakeDestinationReadWrite ();
	}
	catch (Win::Exception ex)
	{
		TheOutput.DisplayException (ex, _hwnd);
		copyFailed = true;
	}
	catch ( ... )
	{
		copyFailed = true;
	}

	if (copyFailed)
	{
		// Copy opeartion aborted by the user or error - cleanup
		collector.Cleanup ();
		if (collector.HasFilesToCheckout ())
		{
			GidList const & controlledFiles = collector.GetFilesToCheckout ();
			_model.Uncheckout (controlledFiles, true);	// Quiet - don't ask are you sure question
		}
	}
	else if (collector.HasFilesToAdd ())
	{
		// Add dropped files to the project
		FileTyper fileTyper (_hwnd, _model.GetCatalog (), _inputSource);
		std::vector<std::string> const & filesToAdd = collector.GetFilesToAdd ();
		dbg << "Drop: adding file: " << filesToAdd[0] << std::endl;
		_model.AddFiles (filesToAdd, fileTyper);
	}

	return !copyFailed;
}

// Join a project that was created by somebody else
void Commander::Project_Join ()
{
	dbg << "--> Commander::Project_Join" << std::endl;
	// Project Join command can be called in the following context:
	//		1. User executed menu item Project>Join
	//		2. Dispatcher called Co-op.exe with command line arguments

	if (_inputSource != 0 || ThePrompter.HasNamedValues ())
	{
		// There are command line arguments
		dbg << "Command line arguments present" << std::endl;
		TheOutput.SetVerbose (true);
		JoinProjectData  joinData (_model.GetCatalog (), false); // Don't force observer
		if (_inputSource != 0)
		{
			dbg << "Getting arguments from the input source" << std::endl;
			JoinProjectCtrl ctrl (joinData);
			if (!ThePrompter.GetData (ctrl, _inputSource))
				return;
		}
		else
		{
			dbg << "Getting arguments from ThePrompter" << std::endl;
			Assert (ThePrompter.HasNamedValues ());
			JoinDataExtractor extractor (joinData);
			if (!ThePrompter.GetData (extractor))
				return;
		}
		ExecuteJoin (joinData);
	}
	else
	{
		// No command line arguments - must be GUI Project>Join
		Assert (!_model.IsQuickVisit ());
		// In GUI mode notify Dispatcher about the user
		// intend to join the project
		DispatcherProxy dispatcher;
		// User can join ONLY as observer when he doesn't have a valid license
		// and trial period is over (cannot create new projects).
		bool onlyAsObserver = !_model.GetUserPermissions ().CanCreateNewProject ();
		dispatcher.JoinRequest (onlyAsObserver);
	}
	dbg << "<-- Commander::Project_Join" << std::endl;
}

void Commander::ExecuteJoin (JoinProjectData & joinData)
{
	// Command line mode Dispatcher callback
#if 0
	// Invitation:
	// uncomment for testing Dispatcher behavior in a case when a project is not created by Co-op
	if (joinData.IsInvitation ())
		return;
#endif
	Project::Data & project = joinData.GetProject ();
	// Accept only after Dispatcher was configured
	if (Catalog::IsHubIdUnknown(joinData.GetMyHubId ()))
	{
		if (!Catalog::IsHubIdUnknown(joinData.GetAdminHubId ()))
		{
			std::string msg ("You have requested to join the project:\n\n");
			msg += project.GetProjectName ();
			msg += "\n\nCode Co-op Dispatcher must send your Join Request by e-mail. "
				   "\nYou need to configure the Dispatcher to use e-mail. "
				   "\nFind Dispatcher's icon on the system tray, right click on it and"
				   "\nselect Collaboration Wizard... ."
				   "\nConfigure your Dispatcher as E-mail Peer."
				   "\nOnce you are done join the project again."
				   "\n\nThe current request will be ignored.";
			TheOutput.Display (msg.c_str (), Out::Error);
			return;
		}
	}

	// Create new project - send out join request script if not invitation
	NewProjTransaction projX (_model.GetCatalog (), project);
	_model.JoinProject (joinData);
	projX.Commit ();

	// Visit the newly created project
	int projId = project.GetProjectId ();
	VisitProject (projId);

	// In command-line version user may specify path to the full sync script
	if (joinData.HasFullSyncScript ())
	{
		std::string const & scriptPath = joinData.GetFullSyncScriptPath ();
		if (!File::Exists (scriptPath.c_str ()))
		{
			std::string msg ("The full sync script file not found:\n");
			msg += scriptPath;
			TheOutput.Display (msg.c_str (), Out::Error);
		}
		else
		{
			// Copy the full sync script from the PublicInbox to the project local inbox
			Catalog & cat = _model.GetCatalog ();
			FilePath inbox = cat.GetProjectInboxPath (projId);
			PathSplitter splitter (scriptPath.c_str ());
			std::string scriptName (splitter.GetFileName ());
			scriptName += splitter.GetExtension ();
			File::Copy (scriptPath.c_str (), inbox.GetFilePath (scriptName));

			_model.ProcessMail ();	// Will unpack and execute full sync script
		}
	}
	DisplayTip (IDS_JOIN);
}

// Invite someone to join your project
void Commander::Project_Invite ()
{
	if (_model.GetMyId () != _model.GetAdminId ())
	{
		std::string msg = "Only the project Administrator can invite a new member to the project.\n\n";
		msg += "If you'd like to be invited to a project pass the administrator the following data:\n\n";
		msg += "My hub's e-mail address: ";
		msg += _model.GetCatalog ().GetHubId ();
		std::string computerName;
		if (_model.HasHub (computerName))
		{
			if (computerName.empty ())
			{
				throw Win::InternalException ("Code Co-op Dispatcher has invalid configuration."
						"\nCo-op was not able to retrieve computer name."
						"\nPlease contact support@relisoft.com");
			}
			msg += "\nMy computer name: ";
			msg += computerName;
		}
		msg += ".";
		TheOutput.Display (msg.c_str ());
		return;

	}

	if (!_model.IsSynchAreaEmpty ())
	{
		throw Win::InternalException ("Synch Area is not empty.\n"
			"Complete previous synchronization script\n"
			"before inviting a new member to the project.");
		return;
	}
	if (!_model.IsMembershipUpToDate ())
	{
		throw Win::InternalException ("Missing membership updates.\n"
			"You have to wait for automatic re-sends to complete\n"
			"before you can send an invitation.");
	}

	std::vector<std::string> remoteHubs;
	for (HubListSeq seq (_model.GetCatalog ()); !seq.AtEnd (); seq.Advance ())
	{
		std::string email;
		Transport transport;
		seq.GetHubEntry (email, transport);
		remoteHubs.push_back (email);
	}
	std::string myHubId = _model.GetCatalog ().GetHubId ();
	if (Catalog::IsHubIdUnknown(myHubId))
		throw Win::InternalException("Cannot send invitation:\n"
				"You haven't configured Code Co-op for collaboration.");

	Project::InviteData inviteData (_model.GetProjectName ());
	ProjectInviteHandlerSet projectInviteHandlerSet (inviteData,
													 remoteHubs,
													 myHubId);
	if (ThePrompter.GetData (projectInviteHandlerSet, _inputSource))
	{
		ProjectChecker projectChecker (_model);
		{
			std::unique_ptr<Progress::Dialog> meterDialog (CreateProgressDlg ("Invite To Project",
																			true,	// User can cancel
																			1000,	// 1 second initial delay
																			true));	// Multi meter dialog
			projectChecker.Verify (meterDialog->GetOverallMeter(), meterDialog->GetSpecificMeter());
		}
		{
			std::unique_ptr<Progress::Dialog> meterDialog1 = CreateProgressDlg ("Invite To Project");
			meterDialog1->SetCaption ("Building project invitation.");
			if (!_model.InviteToProject (inviteData, projectChecker, meterDialog1->GetProgressMeter ()))
				return;
		}

		if (inviteData.IsManualInvitationDispatch () || inviteData.IsTransferHistory ())
		{
			std::string msg;
			if (inviteData.IsManualInvitationDispatch () && inviteData.IsTransferHistory ())
			{
				msg = "The invitation to the project ";
				msg += inviteData.GetProjectName ();
				msg += " for the user ";
				msg += inviteData.GetUserName ();
				msg += " and the project history have been saved ";
			}
			else if (inviteData.IsManualInvitationDispatch ())
			{
				msg = "The invitation to the project ";
				msg += inviteData.GetProjectName ();
				msg += " for the user ";
				msg += inviteData.GetUserName ();
				msg += " has been saved ";
			}
			else
			{
				msg = "The history of the project ";
				msg += inviteData.GetProjectName ();
				msg += " has been saved ";
			}

			if (inviteData.IsStoreOnInternet ())
			{
				msg += "on \n\n";
				std::string url ("ftp://");
				Ftp::SmartLogin const & ftpLogin = inviteData.GetFtpLogin ();
				url += ftpLogin.GetServer ();
				url += "/";
				url += ftpLogin.GetFolder ();
				msg += url;
			}
			else
			{
				msg += "in \n\n";
				msg += inviteData.GetLocalFolder ();
			}

			msg += "\n\n";
			if (inviteData.IsManualInvitationDispatch ())
			{
				msg += "The recipient must copy invitation manually to the Public Inbox directory "
					   "(usually c:\\co-op\\PublicInbox) on his or her ";
				if (inviteData.IsOnSatellite ())
					msg += "hub computer.";
				else
					msg += "computer.";
			}

			if (inviteData.IsTransferHistory ())
			{
				msg += "\nOnce the invitation has been processed, the history file "
					   "has to be imported by executing History>Import.";
			}

			TheOutput.Display (msg.c_str ());
		}
	}
}

Cmd::Status Commander::can_Project_Invite () const
{
	if (IsThisProjectSelected ())
	{
		if (_model.GetUserPermissions ().IsDistributor ())
			return Cmd::Disabled;

		return Cmd::Enabled;
	}
	return Cmd::Disabled;
}

void Commander::Project_OpenInvitation ()
{
	Project::OpenInvitationRequest request;
	OpenFileDlgCtrl dlgCtrl (request, _hwnd);
	if (ThePrompter.GetData (dlgCtrl, _inputSource))
	{
		FilePath publicInbox (Registry::GetCatalogPath ());
		publicInbox.DirDown ("PublicInbox");
		if (request.IsStoreOnInternet () || request.IsStoreOnLAN ())
		{
			// Copy project invitation to this computer
			if (request.IsStoreOnInternet ())
			{
				SafeTmpFile tmpInvitation;
				tmpInvitation.SetFileName (request.GetFileName ());
				std::unique_ptr<Progress::Dialog> copyMeterDialog (CreateProgressDlg ("Open Project Invitation"));
				Ftp::SmartLogin const & login = request.GetFtpLogin ();
				std::string caption ("Downloading project invitation ");
				caption += request.GetFileName ();
				caption += " from ";
				caption += login.GetServer ();
				caption += '/';
				caption += login.GetFolder ();
				copyMeterDialog->SetCaption (caption.c_str ());
				Ftp::Site ftpSite (login, login.GetFolder ());
				if (!ftpSite.Download (request.GetFileName (),
									   tmpInvitation.GetFilePath (),
									   copyMeterDialog->GetProgressMeter ()))
				{
					// FTP download failed
					return;
				}

				File::Move (tmpInvitation.GetFilePath (),
							publicInbox.GetFilePath (request.GetFileName ()));
				if (!ftpSite.DeleteFile (request.GetFileName ()))
					ftpSite.DisplayErrors ();
			}
			else
			{
				Assert (request.IsStoreOnLAN ());
				ShellMan::CopyRequest copyRequest;
				FilePath sourcePath (request.GetPath ());
				char const * sourceFilePath = sourcePath.GetFilePath (request.GetFileName ());
				copyRequest.AddCopyRequest (sourceFilePath,
											publicInbox.GetFilePath (request.GetFileName ()));
				copyRequest.DoCopy (_hwnd, "Open Project Invitation");
				File::Delete (sourceFilePath);
			}
		}
		else
		{
			// Project invitation present on this computer
			Assert (request.IsStoreOnMyComputer ());
			FilePath sourcePath (request.GetPath ());
			char const * sourceFilePath = sourcePath.GetFilePath (request.GetFileName ());
			File::Move (sourceFilePath, publicInbox.GetFilePath (request.GetFileName ()));
		}
	}
}

void Commander::Project_Members ()
{
	ProjectMembersData dlgData (_model.GetProjectDb (), 
								_model.GetUserPermissions ().GetTrialDaysLeft ());
    ProjectMembersCtrl ctrl (dlgData, _model.GetCatalog (), _model.GetActivityLog (), _hwnd);
    if (ThePrompter.GetData (ctrl, _inputSource))
    {
		bool isRecoveryNeeded = false;
        _model.ProjectMembers (dlgData, isRecoveryNeeded);
		ProjectRecoveryData recoveryData (_model.GetProjectDb (),
			"You are changing your status from Observer to Voting Member.\n\n"
			"As an observer you might have missed some scripts and it would "
			"be dangerous for you to make any check-ins immediately. "
			"Code Co-op needs to ask some other Voting Member to verify your status.");
		if (isRecoveryNeeded && !recoveryData.AmITheOnlyVotingMember ())
		{
			UserId verifierId = recoveryData.GetSelectedRecipientId (); // default verifier
			// We must send verification request to somebody
			// Revisit: remove this message box once we release Code Co-op Lite
			TheOutput.Display (
"As an observer you might have missed some important scripts without triggering re-send requests.\n"
"If you make a check-in now, you might cause conflicts that will disrupt the work of others.\n"
"After an Observer-to-Voting Member switch it is recommended to wait for the next incoming script\n"
"to let Code Co-op sort things out before making a check-in.", Out::Information);

			// Revisit: disable this dialog in Code Co-op Lite
			if (!_model.IsQuickVisit ())
			{
				// The user must select one
				ProjectRecoveryRecipientsCtrl recoveryRecipientsCtrl (recoveryData);
				while (!ThePrompter.GetData (recoveryRecipientsCtrl))
				{
					Out::Answer userChoice = TheOutput.Prompt (
						"You are changing your status from Observer to Voting Member\n"
						"without verification. If you make a check-in in this state,\n"
						"you might force other project members to make a lot of merges!"
						"\n\nAre you sure?",
						Out::PromptStyle (Out::YesNo, Out::No, Out::Question),
						_hwnd);
					if (userChoice == Out::Yes)
					{
						verifierId = gidInvalid;
						break;
					}
				}
				if (verifierId != gidInvalid)
					verifierId = recoveryData.GetSelectedRecipientId ();
			}
			if (verifierId != gidInvalid)
				_model.RequestRecovery (verifierId, recoveryData.IsDontBlockCheckin ());
		}
		
		if (recoveryData.AmITheOnlyVotingMember ())
			_model.RequestRecovery(gidInvalid, true); // unblock check-ins

		// revisit: we shouldn't have to check for quick visit
		if (!_model.IsQuickVisit ())
		{
			if (IsCheckInAreaPane ())
			{
				// Changing member status may affect check-in area view.
				_displayMan->Refresh (CheckInAreaPage);
			}
		}
    }
}

Cmd::Status Commander::can_Project_Members () const
{
	return IsThisProjectSelected () ? Cmd::Enabled : Cmd::Disabled;
}

void Commander::Project_Admin ()
{
	ProjectAdminData dlgData (_model.GetProjectDb ());
    ProjectAdminCtrl ctrl (&dlgData);
    if (ThePrompter.GetData (ctrl))
    {
		MemberInfo const & newAdmin = dlgData.GetNewAdmin ();
        _model.ProjectNewAdmin (newAdmin.Id ());
    }
}

Cmd::Status Commander::can_Project_Admin () const
{
	return IsThisProjectSelected () ? Cmd::Enabled : Cmd::Disabled;
}

void Commander::History_ExportHistory ()
{
	History::ExportRequest request (_model.GetProjectName ());
	SaveFileDlgCtrl dlgCtrl (request);
	if (ThePrompter.GetData (dlgCtrl, _inputSource))
	{
		SafeTmpFile tmpProjectHistory (request.GetFileName ());
		FilePath targetPath (request.GetPath ());
		std::string info ("Exporting project '");
		info += _model.GetProjectName ();
		info += "' history.";

		{
			// Progress::MeterDialog scope
			std::unique_ptr<Progress::Dialog> meterDialog (CreateProgressDlg ("History Export"));
			meterDialog->SetCaption (info);

			if (!_model.ExportHistory (tmpProjectHistory.GetFilePath (), meterDialog->GetProgressMeter ()))
				return;	// Export canceled by the user
		}

		if (request.IsStoreOnInternet () || request.IsStoreOnLAN ())
		{
			if (request.IsStoreOnInternet ())
			{
				tmpProjectHistory.Commit ();	// Don't delete here - FTP applet will delete it
				Ftp::SmartLogin const & login = request.GetFtpLogin ();
				ExecuteFtpUpload (tmpProjectHistory.GetFilePath (),
								  login.GetFolder (),
								  login);
			}
			else
			{
				Assert (request.IsStoreOnLAN ());
				ShellMan::CopyRequest copyRequest;
				if (request.IsOverwriteExisting ())
					copyRequest.OverwriteExisting ();
				copyRequest.AddCopyRequest (tmpProjectHistory.GetFilePath (),
											targetPath.GetFilePath (request.GetFileName ()));
				copyRequest.DoCopy (_hwnd, info.c_str ());
			}
		}
		else
		{
			Assert (request.IsStoreOnMyComputer ());
			char const * targetFilePath = targetPath.GetFilePath (request.GetFileName ());
			File::Delete (targetFilePath);
			File::Move (tmpProjectHistory.GetFilePath (), targetFilePath);
		}
    }
}

Cmd::Status Commander::can_History_Export () const
{
	return IsThisProjectSelected () ? Cmd::Enabled : Cmd::Disabled;
}

bool Commander::GetHistoryFile (SafeTmpFile & historyFile) const
{
	History::OpenRequest request;
	OpenFileDlgCtrl dlgCtrl (request, _hwnd);
	if (ThePrompter.GetData (dlgCtrl, _inputSource))
	{
		if (request.IsStoreOnInternet () || request.IsStoreOnLAN ())
		{
			// Copy history to this computer
			historyFile.SetFileName (request.GetFileName ());
			if (request.IsStoreOnInternet ())
			{
				std::unique_ptr<Progress::Dialog> copyMeterDialog (CreateProgressDlg ("Open History File"));
				Ftp::SmartLogin const & login = request.GetFtpLogin ();
				std::string caption ("Downloading history file ");
				caption += request.GetFileName ();
				caption += " from ";
				caption += login.GetServer ();
				caption += '/';
				caption += login.GetFolder ();
				copyMeterDialog->SetCaption (caption.c_str ());
				Ftp::Site ftpSite (login, login.GetFolder ());
				if (!ftpSite.Download (request.GetFileName (),
									   historyFile.GetFilePath (),
									   copyMeterDialog->GetProgressMeter ()))
				{
					// FTP download failed
					return false;
				}
			}
			else
			{
				Assert (request.IsStoreOnLAN ());
				ShellMan::CopyRequest copyRequest;
				if (request.IsOverwriteExisting ())
					copyRequest.OverwriteExisting ();
				FilePath sourcePath (request.GetPath ());
				copyRequest.AddCopyRequest (sourcePath.GetFilePath (request.GetFileName ()),
											historyFile.GetFilePath ());
				copyRequest.DoCopy (_hwnd, "Open History File");
			}
		}
		else
		{
			// History file present on this computer
			Assert (request.IsStoreOnMyComputer ());
			historyFile.SetFilePath (request.GetPath ());
			historyFile.SetFileName (request.GetFileName ());
			historyFile.Commit ();	// Don't delete after use
		}
		return true;
	}
	return false;
}

void Commander::History_ImportHistory ()
{
	SafeTmpFile tmpProjectHistory;
	if (GetHistoryFile (tmpProjectHistory))
	{
		std::string info ("Importing project '");
		info += _model.GetProjectName ();
		info += "' history.";
		std::unique_ptr<Progress::Dialog> meterDialog (CreateProgressDlg ("History Import",
																		true,	// User can cancel
																		0));	// Show immediately
		meterDialog->SetCaption (info);
		_model.ImportHistory (tmpProjectHistory.GetFilePath (),
							  meterDialog->GetProgressMeter ());
	}		
}

Cmd::Status Commander::can_History_Import () const
{
	return IsThisProjectSelected () ? Cmd::Enabled : Cmd::Disabled;
}

void Commander::Program_Options ()
{
	ProgramOptions::Data optionsData (_model.GetCatalog ());
	ProgramOptions::HandlerSet programOptionsHndlrSet (optionsData);
	if (ThePrompter.GetData (programOptionsHndlrSet, _inputSource))
		_model.ProgramOptions (optionsData);
}

void Commander::Project_Options ()
{
	Encryption::KeyMan keyMan (_model.GetCatalog (), _model.GetProjectName ());
	Project::OptionsEx options (_model.GetProjectDb (), keyMan, _model.GetCatalog ());
    ProjectOptionsHndlrSet ctrlSet (options);
    if (ThePrompter.GetData (ctrlSet, _inputSource))
    {
		keyMan.SetKey (options.GetEncryptionKey ());
        _model.ProjectOptions (options);
		if (_model.IsAutoSynch ())
		{
			View_Mailbox ();
			All_Synch ();
			View_Default ();
		}
    }
}

Cmd::Status Commander::can_Project_Options () const
{
	return IsThisProjectSelected () ? Cmd::Enabled : Cmd::Disabled;
}

void Commander::Project_AddMoreFiles ()
{
	Directory & directory = _model.GetDirectory ();
	NocaseSet alreadyControlled;
	GlobalId rootFolderGid = directory.GetRootId ();
	_model.ListControlledFiles (rootFolderGid, alreadyControlled);
	std::string path (_model.GetProjectDir ());
	std::vector<std::string> files;
	if (GetFolderContents (path, alreadyControlled, files))
	{
		if (!files.empty ())
		{
			FileTyper fileTyper (_hwnd, _model.GetCatalog (), _inputSource);
			bool done = _model.AddFiles (files, fileTyper);
			if (done)
				DisplayTip (IDS_ADDFILE);
		}
	}
}

Cmd::Status Commander::can_Project_AddMoreFiles () const
{
	return IsThisProjectSelected () ? Cmd::Enabled : Cmd::Disabled;
}

void Commander::Project_SelectMergeTarget ()
{
	dbg << "--> Project>SelectMergeTarget" << std::endl;

	Catalog & catalog =  _model.GetCatalog ();
	int targetProjectId = -1;
	GlobalId forkScriptId = gidInvalid;
	std::unique_ptr<HistoryChecker> historyChecker = _model.GetHistoryChecker ();
	StringExtractor extr ("Target");
	ThePrompter.GetData (extr);
	if (extr.GetString ().empty ())
	{
		// Prompt the user to select target project
		ProjectList projects (catalog, _model.GetProjectId (), *historyChecker);
		SelectProjectCtrl dlgCtrl (projects);
		if (!ThePrompter.GetData (dlgCtrl))
			return;	// User canceled dialog

		targetProjectId = projects.GetSelectedProjectId ();
		forkScriptId = projects.GetForkId ();
	}
	else if (extr.GetString () == CurrentProject)
	{
		Assert (_model.GetMergedFiles ().IsLocalMerge ());
		return;
	}
	else
	{
		dbg << "   GetMergedFiles" << std::endl;
		MergeDetails & mergedFiles = _model.GetMergedFiles ();
		if (mergedFiles.IsTargetProjectSet ())
		{
			std::string currentTarget = catalog.GetProjectName (mergedFiles.GetTargetProjectId ());
			if (currentTarget == extr.GetString ())
				return;
		}
		// Scan catalog to find project id
		for (ProjectSeq seq (catalog); !seq.AtEnd (); seq.Advance ())
		{
			std::string catalogName = seq.GetProjectName ();
			if (IsNocaseEqual (catalogName, extr.GetString ()))
			{
				if (targetProjectId == -1)
				{
					targetProjectId = seq.GetProjectId ();
					dbg << "   Target Project selected, id: " << targetProjectId << std::endl;
				}
				else
				{
					// More then one project with the same name
					// Prompt the user to select target project
					ProjectList projects (catalog,
										  _model.GetProjectId (),
										  *historyChecker,
										  extr.GetString ());
					SelectProjectCtrl dlgCtrl (projects);
					if (!ThePrompter.GetData (dlgCtrl))
						return;	// User canceled dialog

					targetProjectId = projects.GetSelectedProjectId ();
					forkScriptId = projects.GetForkId ();
					break;
				}
			}
		}
		if (targetProjectId == -1)
		{
			mergedFiles.UpdateTargetHints (extr.GetString (), true);	// Remove hint
			_displayMan->RefreshPane (ProjectMergePage, Table::mergeDetailsTableId);
			std::string info ("Cannot find project '");
			info += extr.GetString ();
			info += "' in the project catalog.";
			TheOutput.Display (info.c_str ());
			return;
		}
	}

	Assert (targetProjectId != -1);

	dbg << "   Check selected script state" << std::endl;
	SelectionSeq scriptSelection (_selectionMan, Table::historyTableId);
	while (!scriptSelection.AtEnd ())
	{
		History::ScriptState state (scriptSelection.GetState ());
		if (state.IsRejected ())
		{
			std::string info ("You cannot merge the branched (rejected) script ");
			info += GlobalIdPack (scriptSelection.GetGlobalId ()).ToString ();
			info += " into the branch project ";
			info += catalog.GetProjectName (targetProjectId);
			info += ".\nYou have to first merge the branch script into this project trunk and\n"
					"then merge the trunk script into the branch project.";
			TheOutput.Display (info.c_str ());
			return;
		}
		scriptSelection.Advance ();
	}

	if (forkScriptId == gidInvalid)
	{
		dbg << "   Check if target project is related" << std::endl;
		forkScriptId = historyChecker->IsRelatedProject (targetProjectId);
		if (forkScriptId == gidInvalid)
		{
			historyChecker->DisplayError (catalog, _model.GetProjectId (), targetProjectId);
			return;
		}
	}

	Assert (forkScriptId != gidInvalid);
	dbg << "SetMergeTarget" << std::endl;
	_model.SetMergeTarget (targetProjectId, forkScriptId);
	dbg << "GetInterestingScripts" << std::endl;
	GidSet interestingScripts;
	_model.GetInterestingScripts (interestingScripts);
	dbg << "SetInterestingItems" << std::endl;
	_displayMan->SetInterestingItems (interestingScripts);
	_displayMan->Refresh (ProjectMergePage);

	if (scriptSelection.Count () != 0)
	{
		std::unique_ptr<Progress::Dialog> meterDialog (CreateProgressDlg ("Merge"));
		meterDialog->SetCaption ("Refreshing files merge status.");
		_model.PrepareMerge (meterDialog->GetProgressMeter (), _model.GetMergedFiles ());
		_displayMan->RefreshPane (ProjectMergePage, Table::mergeDetailsTableId);
	}
	dbg << "<-- Project>SelectMergeTarget" << std::endl;
}

void Commander::Project_SetMergeType ()
{
	MergeDetails & mergeDetails = _model.GetMergedFiles ();

	StringExtractor extractor ("CumulativeMerge");
	ThePrompter.GetData (extractor);
	std::string const & cumulativeMerge = extractor.GetString ();
	Assert (!cumulativeMerge.empty ());

	mergeDetails.SetCumulativeMerge (cumulativeMerge == "yes");

	StringExtractor extractor1 ("AncestorMerge");
	ThePrompter.GetData (extractor1);
	std::string const & ancestorMerge = extractor1.GetString ();
	Assert (!ancestorMerge.empty ());

	mergeDetails.SetAncestorMerge (ancestorMerge == "yes");

	CreateRange ();
}

// Repair project files
void Commander::Project_Repair ()
{
	bool syncAreaWasOriginalyEmpty = _model.IsSynchAreaEmpty ();
	if (!syncAreaWasOriginalyEmpty)
	{
		Out::Answer userChoice = TheOutput.Prompt (
			"You have pending script changes. During project repair\n"
			"Code Co-op may need to accept script changes for some of the files.\n\n"
			"Do you want to continue?",
			Out::PromptStyle (Out::YesNo, Out::Yes, Out::Question),
			_hwnd);
		if (userChoice == Out::No)
			return;
	}

	ProjectChecker projectChecker (_model);

	std::unique_ptr<Progress::Dialog> meterDialog (CreateProgressDlg ("Project Repair",
																	true,	// User can cancel
																	1000,	// 1 second initial delay
																	true));	// Multi meter dialog
	std::string caption ("Project ");
	caption += _model.GetProjectName ();
	caption += " verification.";
	meterDialog->SetCaption (caption);

	projectChecker.Verify (meterDialog->GetOverallMeter (), meterDialog->GetSpecificMeter ());
	meterDialog.reset ();
	// History
	bool membershipHistoryOk = _model.VerifyHistory (History::Membership);
	bool fileHistoryOk = _model.VerifyHistory (History::Creation);
	bool membershipOk = _model.VerifyMembership ();
	bool checkoutNotificationsOk = _model.VerifyCheckoutNotifications ();
	if (!projectChecker.MissingFolderFound () && !projectChecker.IsFileRepairNeeded () 
		&& membershipHistoryOk && fileHistoryOk && membershipOk && checkoutNotificationsOk)
	{
		TheOutput.Display ("Nothing to repair. Project seems to be OK.");
		return;
	}

	if (!fileHistoryOk)
	{
		std::unique_ptr<Progress::Dialog> meterDialog (CreateProgressDlg ("Project Repair"));
		meterDialog->SetCaption ("Repairing project history.");
		_model.RepairHistory (History::Creation, meterDialog->GetProgressMeter ());
		meterDialog.reset ();

		if (_model.VerifyHistory (History::Creation))
			TheOutput.Display ("Project history has been corrected.");
		else
			TheOutput.Display ( "Could not repair project history.\n\n"
								"Please contact support@relisoft.com");
	}

	if (!membershipHistoryOk)
	{
		std::unique_ptr<Progress::Dialog> meterDialog (CreateProgressDlg ("Project Repair"));
		meterDialog->SetCaption ("Repairing membership history.");
		_model.RepairHistory (History::Membership, meterDialog->GetProgressMeter ());
		meterDialog.reset ();

		if (_model.VerifyHistory (History::Membership))
			TheOutput.Display ("Project membership history has been corrected.");
		else
			TheOutput.Display ( "Could not repair project membership history.\n\n"
								"Please contact support@relisoft.com");
	}

	if (projectChecker.IsFileRepairNeeded ())
		projectChecker.Repair ();

	if (!syncAreaWasOriginalyEmpty && _model.IsSynchAreaEmpty ())
	{
		// Sync area is empty
		CloseSyncAreaView ();
	}

	if (!membershipOk)
	{
		_model.RepairMembership ();
		if (_model.VerifyMembership ())
			TheOutput.Display ("Membership history has been corrected.");
		else
			TheOutput.Display ( "Could not repair membership history.\n\n"
								"Please contact support@relisoft.com");
	}
	if (!checkoutNotificationsOk)
	{
		_model.RepairCheckoutNotifications ();
		_displayMan->Refresh (FilesPage);
		TheOutput.Display ("Checkout notifications have been corrected.");
	}
	if (projectChecker.MissingFolderFound ())
		TheOutput.Display ("Missing project folders have been recreated.");
}

Cmd::Status Commander::can_Project_Repair () const
{
	return IsThisProjectSelected () ? Cmd::Enabled : Cmd::Disabled;
}

void Commander::Project_RequestVerification ()
{
	ProjectRecoveryData recoveryData (_model.GetProjectDb (),
		"Code Co-op will send an emergency verification request to the selected member.\n"
		"Use emergency verification only when you suspect that membership scripts were lost,\n"
		"and that your membership list is inconsistent with that of other project members;\n"
		"or when previous verification request was unsuccessful.");

	if (recoveryData.AmITheOnlyVotingMember ())
	{
		TheOutput.Display (
			"You cannot request project verification,\n"
			"because you are the only voting member in this project.");
		_model.RequestRecovery(gidInvalid, true); // unblock check-ins
		_displayMan->Refresh (CheckInAreaPage);
		return;
	}
	ProjectRecoveryRecipientsCtrl recoveryRecipientsCtrl (recoveryData);
	if (ThePrompter.GetData (recoveryRecipientsCtrl))
	{
		_model.RequestRecovery (recoveryData.GetSelectedRecipientId (),
								recoveryData.IsDontBlockCheckin ());
		if (IsCheckInAreaPane ())
		{
			// Requesting project verification may affect check-in area view.
			_displayMan->Refresh (CheckInAreaPage);
		}
	}
}

Cmd::Status Commander::can_Project_RequestVerification () const
{
	return IsThisProjectSelected () ? Cmd::Enabled : Cmd::Disabled;
}

void Commander::Project_VerifyRootPaths ()
{
	Project::RootPathVerifier pathVerifier (_model.GetCatalog ());
	if (pathVerifier.GetProjectCount () == 0)
	{
		TheOutput.Display ("There are no projects on this computer.\n"
						   "Select \"New Project\" or \"Join Project\" from the Project menu to create or join one.");
		return;
	}

	bool verificationOk = true;

	{
		// Progress::Dialog scope
		std::unique_ptr<Progress::Dialog> meterDialog (CreateProgressDlg ("Project Root Path Verification"));
		meterDialog->SetCaption ("Checking project root paths.");
		verificationOk = pathVerifier.Verify (meterDialog->GetProgressMeter (),
											  false);	// Don't skip unavailable projects
	}

	if (verificationOk)
		TheOutput.Display ("All projects root paths exists.");
	else
		pathVerifier.PromptForNew ();
}

// Defect from (cease to be a member of) a project
void Commander::Project_Defect ()
{
	if (_model.IsInProject ())
	{
		// Defecting from the current project
		ProjDefectData dlgData (_model.GetProjectName ());
		ProjDefectCtrl ctrl (&dlgData);
		if (ThePrompter.GetData (ctrl, _inputSource))
		{
			if (_model.Defect (dlgData))
			{
				Assert (!_model.IsInProject ());
				_displayMan->ClearAll (true); // for good
			}
		}
	}
	else
	{
		// Forced defect from the project with network problems
		Assert (!_model.IsQuickVisit ());
		SelectionSeq seq (_selectionMan, Table::projectTableId);
		int projId = seq.GetGlobalId ();
		Assert (projId != -1);
		Project::Data projData;
		Catalog & projectCatalog = _model.GetCatalog ();
		Assert (projectCatalog.IsProjectUnavailable (projId));
		projectCatalog.GetProjectData (projId, projData);
		std::string info ("You are defecting from the inaccessible project '");
		info += projData.GetProjectName ();
		info += "'\n(";
		info += projData.GetRootPath ().GetDir ();
		info += ")\n\nDefecting will remove project history stored on this computer\n"
				"and leave source files intact on the network computer.\nDo you want to continue ?";
		Out::Answer userChoice = TheOutput.Prompt (info.c_str (), Out::PromptStyle (Out::YesNo, Out::No, Out::Question));
		if (userChoice == Out::No)
			return;

		LeaveProject ();
		_model.ForceDefect (projData);
	}
	_displayMan->Refresh (ProjectPage);
	SetTitle ();
	SaveState ();
	View_Projects ();
}

Cmd::Status Commander::can_Project_Defect () const
{
	if (_model.IsInProject ())
	{
		// Visiting project -- allow defect only from the visited project
		if (IsProjectPane ())
		{
			// In the project view check if selected item represents current project,
			// so the user is not confused about the project he/she is defecting from
			IsThisProject isCurrentProject (_model.GetProjectId ());
			return _selectionMan->SelCount () == 1 &&
				   _selectionMan->FindIfSelected (isCurrentProject) 
				? Cmd::Enabled : Cmd::Disabled;
		}
		return Cmd::Enabled;
	}
	else if (IsProjectPane ())
	{
		// Not visiting any project -- allow defect only if selected project's
		// source folder is a inaccessible network folder 
		return _selectionMan->SelCount () == 1 &&
			   _selectionMan->FindIfSelected (IsProjectUnavailable) 
			? Cmd::Enabled : Cmd::Disabled;
	}
	return Cmd::Disabled;
}

//
// Version commands
//

void Commander::Selection_Revert ()
{
	SelectionSeq seq (_selectionMan, Table::historyTableId);
	GlobalId selectedVersion = seq.GetGlobalId ();
	if (!CanRevert (selectedVersion))
		return;

	ThePrompter.SetNamedValue ("Extend", "yes");
	CreateRange ();
	ThePrompter.ClearNamedValues ();
	Restriction const & restriction = _displayMan->GetPresentationRestriction ();
	VersionInfo info;
	_model.RetrieveVersionInfo (selectedVersion, info);
    RevertData dlgData (info, restriction.IsFilterOn ());
	// Tell the user what will happen
    RevertCtrl ctrl (dlgData);
    if (!ThePrompter.GetData (ctrl))
		return;	// User wants to review the changes before the revert operation

	// Go ahead and revert project state
	std::unique_ptr<Progress::Dialog> meterDialog (CreateProgressDlg ("Reverting Changes",
																	true,	// User can cancel
																	1000,	// 1 second initial delay
																	true));	// Multi meter dialog
	std::string caption ("Restoring project state to version:\r\n");
	GlobalIdPack pack (selectedVersion);
	caption += pack.ToBracketedString ();
	caption += " -- ";
	caption += info.GetComment ();
	caption += "\r\nfrom ";
	StrTime timeStamp (info.GetTimeStamp ());
	caption += timeStamp.GetString ();
	meterDialog->SetCaption (caption);

	_model.Revert (meterDialog->GetOverallMeter (), meterDialog->GetSpecificMeter ());
	_displayMan->SelectPage (CheckInAreaPage);
	DisplayTip (IDS_RESTORE);
}

void Commander::Selection_MergeBranch ()
{
	Assert (IsHistoryPage ());
	bool isLocalMerge = _selectionMan->FindIfSelected (IsRejected);
	if (isLocalMerge)
		_model.SetMergeTarget (_model.GetProjectId ());

	// Remember filtering in the history view
	Restriction const & restriction = _displayMan->GetPresentationRestriction ();
	FileFilter const * historyFilter = restriction.GetFileFilter ();
	std::unique_ptr<FileFilter> filter;
	if (historyFilter != 0)
		filter.reset (new FileFilter (*historyFilter));

	std::vector<Bookmark> bookmarks;
	_displayMan->GetScrollBookmarks (bookmarks);
	GidList selectedScriptIds;
	// Remember history selection before switching to the project merge page
	for (SelectionSeq seq (_selectionMan, Table::historyTableId); !seq.AtEnd (); seq.Advance ())
	{
		GlobalId scriptId = seq.GetGlobalId ();
		if (scriptId != gidInvalid)
			selectedScriptIds.push_back (scriptId);
	}

	MergeDetails & mergedFiles = _model.GetMergedFiles ();
	if (isLocalMerge)
	{
		// Default to cumulative merge type
		mergedFiles.SetAncestorMerge (true);
		mergedFiles.SetCumulativeMerge (true);
	}
	else
	{
		// Default to selective merge type
		mergedFiles.SetAncestorMerge (false);
		mergedFiles.SetCumulativeMerge (false);
	}
	// Switch to the project merge page
	_displayMan->SelectPage (ProjectMergePage);
	// Restore history filtering in the project merge page
	if (filter.get () != 0)
		_displayMan->SetFileFilter (_displayMan->GetCurrentPage (), std::move(filter));
	_displayMan->SetScrollBookmarks (bookmarks);
	// Restore history selection in the project merge page
	_selectionMan->SelectIds (selectedScriptIds, Table::historyTableId);
	CreateRange ();

	if (_model.IsSynchAreaEmpty ())
		_coopTips.Display (isLocalMerge ? IDS_MERGELOCALBRANCH : IDS_GLOBALMERGE, true);	// Force - ignore global tip on/off setting
}

Cmd::Status Commander::can_Selection_MergeBranch () const
{
	if (IsProjectReady () && IsHistoryPage ())
	{
		// We can merge to the current project version if:
		//	1. only one script in the history pane has been selected
		return (IsHistoryPane () 
					&& _selectionMan->SelCount () == 1 
					&& _model.GetHistoricalFiles ().RangeIsAllRejected ())
			   ? Cmd::Enabled : Cmd::Disabled;

	}
	return Cmd::Invisible;
}

// Hidden command, called only through SccDll
void Commander::RestoreVersion ()
{
	dbg << "--> Commander::RestoreVersion" << std::endl;
	Assert (_inputSource != 0);
	std::string version = _inputSource->GetNamedArguments ().GetValue ("version");
	GidList selectedScripts;
	if (version == "previous")
	{
		dbg << "     Restore previous version" << std::endl;
		selectedScripts.push_back (_model.GetSecondLatestVersion ());
	}
	else
	{
		GlobalIdPack pack (version);
		dbg << "     Restore version " << pack.ToString () << std::endl;
		selectedScripts.push_back (pack);
	}
	if (selectedScripts.front () == gidInvalid)
		return;

	History::Range range;
	GidSet emptyFileFilter;
	_model.CreateRange (selectedScripts, emptyFileFilter, true, range);	// Extend range
	_model.UpdateHistoricalFiles (range, emptyFileFilter, true); // Extended range
	Progress::Meter dummyMeter;	
	_model.Revert (dummyMeter, dummyMeter);
	dbg << "<-- Commander::RestoreVersion" << std::endl;
}

// Hidden command, called only through SccDll
void Commander::Maintenance ()
{
	Assert (_inputSource != 0);
	NamedValues const & args = _inputSource->GetNamedArguments ();
	std::string argValue = args.GetValue ("PropagateLocalLicense");
	if (IsNocaseEqual (argValue, "yes"))
	{
		// Propagate local project license to the global database
		_model.PropagateLocalLicense ();
	}
	argValue = args.GetValue ("UpdateHubId");
	if (IsNocaseEqual (argValue, "yes"))
	{
		// Update this project hub id from catalog
		Catalog & catalog = _model.GetCatalog ();
		std::string catalogHubId (catalog.GetHubId ());
		if (!Catalog::IsHubIdUnknown(catalogHubId)) // don't change to Unknown!
		{
			_model.UpdateEnlistmentAddress (catalogHubId);
		}
	}
}

// Hidden command, called only through SccDll
void Commander::GetForkIds (GidList const & clientForkIds,
							bool deepForks,
							GlobalId & youngestFoundScriptId,
							GidList & myYoungerForkIds)
{
	youngestFoundScriptId = _model.CheckForkIds (clientForkIds, deepForks, myYoungerForkIds);
}

// Hidden command, called only through SccDll
void Commander::GetTargetPath (GlobalId sourceGid,
							   std::string const & sourcePath,
							   std::string & targetPath,
							   unsigned long & targetType,
							   unsigned long & targetStatus)
{
	FileType fileType;
	TargetStatus status = _model.GetTargetPath (sourceGid, sourcePath, targetPath, fileType);
	targetType = fileType.GetValue ();
	targetStatus = status.GetValue ();
}

// Hidden command, called only through SccDll
void Commander::ReCreateFile ()
{
	Assert (_inputSource != 0);
	NamedValues const & args = _inputSource->GetNamedArguments ();
	std::string targetPath = args.GetValue ("target");
	GlobalIdPack pack (args.GetValue ("gid"));
	FileType type (::ToInt (args.GetValue ("type")));
	_model.AddFile (targetPath.c_str (), pack, type);
}

// Hidden command, called only through SccDll
void Commander::MergeAttributes ()
{
	Assert (_inputSource != 0);
	NamedValues const & args = _inputSource->GetNamedArguments ();
	std::string currentPath = args.GetValue ("currentPath");
	std::string newPath = args.GetValue ("newPath");
	FileType type (::ToInt (args.GetValue ("type")));
	_model.MergeAttributes (currentPath, newPath, type);
}

// Hidden command, called only from command line; available only in DEBUG build
#if !defined (NDEBUG)

void Commander::UndoScript (GlobalId scriptId)
{
	FilePath userDesktopPath;
	ShellMan::VirtualDesktopFolder userDesktop;
	userDesktop.GetPath (userDesktopPath);
	MemoryLog revertLog;
	bool scriptUndone = false;
	try
	{
		//REVISIT: temporarily disabled this functionality - this is debug only
		//method used by the command line applet rejecting single script in the history
		//Progress::Meter meter;
		//revertLog << "Reverting script: " << GlobalIdPack (scriptId) << std::endl;
		//FileRestorer fileRestorer (scriptId, gidInvalid, _model, 0, meter);
		//Assert (fileRestorer.CanRestore ());
		//Workspace::HistorySelection * diffSelection = fileRestorer.GetDiffSelection ();
		//_model.UndoScript (*diffSelection, scriptId, revertLog);
		//scriptUndone = true;
	}
	catch (Win::Exception ex)
	{
		std::string exMsg (Out::Sink::FormatExceptionMsg (ex));
		revertLog << exMsg << std::endl;
		char const * logPath = userDesktopPath.GetFilePath ("HistoryRepairLog.txt");
		revertLog.Save (logPath);
	}
	catch ( ... )
	{
		revertLog << "Unknown exception during project history repair." << std::endl;
		char const * logPath = userDesktopPath.GetFilePath ("HistoryRepairLog.txt");
		revertLog.Save (logPath);
	}
	if (!scriptUndone)
	{
		Win::ClearError ();
		throw Win::Exception ("History repair failed. Please, send the log file 'HistoryRepairLog.txt'\ncreated on your desktop to the support@relisoft.com");
	}
}

void Commander::RestoreOneVersion ()
{
	if (!_model.IsSynchAreaEmpty ())
		 return;

	Assert (_inputSource != 0);
	GlobalId selectedVersion = gidInvalid;
	std::string version = _inputSource->GetNamedArguments ().GetValue ("version");
	GlobalIdPack pack (version);
	selectedVersion = pack;
	if (selectedVersion == gidInvalid)
		return;

	UndoScript (selectedVersion);
}
#endif

Cmd::Status Commander::can_Selection_Revert () const
{
	if (IsProjectReady () && IsHistoryPage ())
	{
		// We can revert to the project version if:
		//	1. only one script in the history has been selected  AND
		//	2. selected script is NOT a current project version AND
		//	3. selected script is NOT a rejected change script
		return IsSingleHistoricalExecutedScriptSelected () ? Cmd::Enabled : Cmd::Disabled;
	}
	return Cmd::Invisible;
}

void Commander::Selection_Compare ()
{
	_coopTips.Display (IDS_COMPARE_SCRIPT_CHANGES, true);	// Force - ignore global tip on/off setting
}

Cmd::Status Commander::can_Selection_Compare () const
{
	if (IsProjectReady () && IsHistoryPage ())
	{
		return (IsHistoryPane () 
				&& _selectionMan->SelCount () < 3 
				&& !_model.GetHistoricalFiles ().RangeHasRejectedScripts ())
			? Cmd::Enabled : Cmd::Disabled;
}
	return Cmd::Invisible;
}

void Commander::Selection_CompareWithPrevious ()
{
	_coopTips.Display (IDS_THIS_SCRIPT_CHANGES, true);	// Force - ignore global tip on/off setting
}

Cmd::Status Commander::can_Selection_CompareWithPrevious () const
{
	if (IsProjectReady () && IsHistoryPage ())
	{
		if (IsSingleHistoricalExecutedScriptSelected ())
		{
			return _selectionMan->FindIfSelected (IsLabel)
				? Cmd::Disabled : Cmd::Enabled;
		}
		return Cmd::Disabled;
	}
	return Cmd::Invisible;
}

void Commander::Selection_CompareWithCurrent ()
{
	ThePrompter.SetNamedValue ("Extend", "yes");
	CreateRange ();
	ThePrompter.ClearNamedValues ();
	_coopTips.Display (IDS_SINCE_THIS_SCRIPT_CHANGES, true);	// Force - ignore global tip on/off setting
}

Cmd::Status Commander::can_Selection_CompareWithCurrent () const
{
	if (IsProjectReady () && IsHistoryPage ())
	{
		return IsSingleHistoricalExecutedScriptSelected () ? Cmd::Enabled : Cmd::Disabled;
	}
	return Cmd::Invisible;
}

void Commander::Selection_Branch ()
{
	SelectionSeq seq (_selectionMan, Table::historyTableId);
	History::ScriptState state = seq.GetState ();
	GlobalId scriptId = seq.GetGlobalId ();
	DoBranch (scriptId);
}

Cmd::Status Commander::can_Selection_Branch () const
{
	if (IsProjectReady () && IsHistoryPage ())
	{
		// We can create project branch if:
		//	1. only one script in the history has been selected  AND
		//	2. selected script is not a rejected change script
		return IsSingleExecutedScriptSelected () ? Cmd::Enabled : Cmd::Disabled;
	}
	return Cmd::Invisible;
}

void Commander::Selection_VersionExport ()
{
	bool isCmdLine = _model.IsQuickVisit ();
	if (isCmdLine)
	{
		DoExport (true); // command line version
	}
	else
	{
		SelectionSeq seq (_selectionMan, Table::historyTableId);
		if (seq.IsEmpty ())
			return;
		DoExport (false, seq.GetGlobalId ()); // GUI version
	}
}

Cmd::Status Commander::can_Selection_VersionExport () const
{
	if (IsProjectReady () && IsHistoryPage ())
	{
		// We can export project version if:
		//	1. only one script in the history has been seletecd  AND
		//	2. selected script is not a rejected change script
		return IsSingleExecutedScriptSelected () ? Cmd::Enabled : Cmd::Disabled;
	}
	return Cmd::Invisible;
}

void Commander::History_AddLabel ()
{
	if (!CanCheckin ())
		return;

	CheckInUI checkInUI (_hwnd, _inputSource);
	if (_model.VersionLabel (checkInUI))
	{
		View_History ();
		DisplayTip (IDS_CHECKIN);
	}
}

Cmd::Status Commander::can_History_AddLabel () const
{
	if (IsProjectReady () && IsHistoryPane () && CanMakeChanges ())
	{
		bool blocked = false;
		bool canCheckin = _model.CanCheckIn (blocked);
		if (blocked)
			return Cmd::Enabled;	// Enable command, so we can display error message
		else
			return canCheckin ? Cmd::Enabled : Cmd::Disabled;
	}
	return Cmd::Disabled;
}

void Commander::Selection_Archive ()
{
	SelectionSeq seq (_selectionMan, Table::historyTableId);
	_model.Archive (seq.GetGlobalId ());
}

Cmd::Status Commander::can_Selection_Archive () const
{
	if (IsProjectReady () && IsHistoryPage ())
	{
		// We can't archive the current project version or rejected version
		return IsSingleHistoricalExecutedScriptSelected () ? Cmd::Enabled : Cmd::Disabled;
	}
	return Cmd::Invisible;
}

void Commander::Selection_UnArchive ()
{
	_model.UnArchive ();
	_displayMan->Refresh (HistoryPage);
}

Cmd::Status Commander::can_Selection_UnArchive () const
{
	if (IsProjectReady () && IsHistoryPane ())
	{
		if (_model.IsArchive ())
			return Cmd::Enabled;
		return Cmd::Disabled;
	}
	return Cmd::Invisible;
}

void Commander::Selection_Add2FileFilter ()
{
	Restriction const & restriction = _displayMan->GetPresentationRestriction ();
	NocaseSet extFilter = restriction.GetExtensionFilter ();
	for (SelectionSeq seq (_selectionMan, Table::folderTableId); !seq.AtEnd (); seq.Advance ())
	{
		std::string const & fileName = seq.GetName ();
		PathSplitter splitter (fileName);
		if (splitter.HasExtension ())
			extFilter.insert (splitter.GetExtension ());
	}
	_displayMan->SetExtensionFilter (extFilter);
	_displayMan->SetRestrictionFlag ("FileFiltering", true);
	_displayMan->Refresh (FilesPage);
}

Cmd::Status Commander::can_Selection_Add2FileFilter () const
{
	if (IsProjectReady () && IsFilePane ())
	{
		return (IsSelection () &&
			    _selectionMan->FindIfSelected (IsFile)) ? Cmd::Enabled : Cmd::Disabled;
	}
	return Cmd::Invisible;
}

void Commander::Selection_RemoveFromFileFilter ()
{
	Restriction const & restriction = _displayMan->GetPresentationRestriction ();
	NocaseSet extFilter = restriction.GetExtensionFilter ();
	for (SelectionSeq seq (_selectionMan, Table::folderTableId); !seq.AtEnd (); seq.Advance ())
	{
		std::string const & fileName = seq.GetName ();
		PathSplitter splitter (fileName);
		if (splitter.HasExtension ())
			extFilter.erase (splitter.GetExtension ());
	}
	_displayMan->SetExtensionFilter (extFilter);
}

Cmd::Status Commander::can_Selection_RemoveFromFileFilter () const
{
	if (IsProjectReady () && IsFilePane ())
	{
		return (IsSelection () && !IsFilterOn () &&
			    _selectionMan->FindIfSelected (IsFile)) ? Cmd::Enabled : Cmd::Disabled;
	}
	return Cmd::Invisible;
}

void Commander::Selection_HistoryFilterAdd ()
{
	SelectionSeq fileSeq (_selectionMan, _displayMan->GetCurrentTableId ());
	Assert (!fileSeq.AtEnd ());
	std::unique_ptr<FileFilter> newFilter (new FileFilter (fileSeq, _model.GetDirectory ()));
	Restriction const & restriction = _displayMan->GetPresentationRestriction ();
	FileFilter const * oldFilter = restriction.GetFileFilter ();
	if (oldFilter != 0)
	{
		// Copy old file filter to the new file filter
		for (FileFilter::Sequencer seq (*oldFilter); !seq.AtEnd (); seq.Advance ())
		{
			newFilter->AddFile (seq.GetGid (), seq.GetPath ());
		}
	}
	_displayMan->SetFileFilter (_displayMan->GetCurrentPage (), std::move(newFilter));
}

Cmd::Status Commander::can_Selection_HistoryFilterAdd () const
{
	if (IsHistoryPage () || IsProjectMergePage ())
	{
		if (IsProjectReady () && IsSelection ())
		{
			Restriction const & restriction = _displayMan->GetPresentationRestriction ();
			return restriction.IsFilterOn () ? Cmd::Disabled : Cmd::Enabled;
		}
		return Cmd::Disabled;
	}

	return Cmd::Invisible;
}

void Commander::Selection_HistoryFilterRemove ()
{
	std::unique_ptr<FileFilter> newFilter (new FileFilter ());
	_displayMan->SetFileFilter (_displayMan->GetCurrentPage (), std::move(newFilter));
}

Cmd::Status Commander::can_Selection_HistoryFilterRemove () const
{
	if (IsHistoryPage () || IsProjectMergePage ())
	{
		if (IsProjectReady ())
		{
			Restriction const & restriction = _displayMan->GetPresentationRestriction ();
			return restriction.IsFilterOn () ? Cmd::Enabled : Cmd::Disabled;
		}
		return Cmd::Disabled;
	}

	return Cmd::Invisible;
}

void Commander::Selection_SaveFileVersion ()
{
	SelectionSeq seq (_selectionMan, _displayMan->GetCurrentTableId ());
	DoSaveFileVersion (seq);
}

Cmd::Status Commander::can_Selection_SaveFileVersion () const
{
	if (IsProjectReady () && (IsHistoryPage () || IsMailboxPage () || IsProjectMergePage ()))
	{
		if (IsSelection ())
		{
			if (IsMergeDetailsPane ())
				return _selectionMan->FindIfSelected (IsAbsent) ? Cmd::Disabled : Cmd::Enabled;
			else if (IsScriptDetailsPane ())
				return Cmd::Enabled;
		}
		return Cmd::Disabled;
	}
	return Cmd::Invisible;
}

void Commander::Selection_RestoreFileVersion ()
{
	SelectionSeq seq (_selectionMan, Table::historyTableId);
	GlobalId selectedVersion = seq.GetGlobalId ();
	if (!CanRevert (selectedVersion))
		return;

	GidList files;
	for (SelectionSeq fileSeq (_selectionMan, Table::scriptDetailsTableId);
		 !fileSeq.AtEnd ();
		 fileSeq.Advance ())
	{
		GlobalId gid = fileSeq.GetGlobalId ();
		files.push_back (gid);
	}

	HistoricalFiles const & historicalFiles = _model.GetHistoricalFiles ();
	if (!historicalFiles.RangeIncludesMostRecentVersion ())
	{
		// Current history range doesn't include project most recent version.
		// Extend current range.
		ThePrompter.SetNamedValue ("Extend", "yes");
		CreateRange ();
		ThePrompter.ClearNamedValues ();
		_selectionMan->SelectIds (files, Table::scriptDetailsTableId);
	}
	VersionInfo info;
	_model.RetrieveVersionInfo (selectedVersion, info);
	RevertData dlgData (info, true);	// File restore
	// Tell the user what will happen
	RevertCtrl ctrl (dlgData);
	if (!ThePrompter.GetData (ctrl))
		return;	// User wants to review the changes before the revert operation

	// Go ahead and revert project state
	std::unique_ptr<Progress::Dialog> meterDialog (CreateProgressDlg ("Reverting Changes",
																	true,	// User can cancel
																	1000,	// 1 second initial delay
																	true));	// Multi meter dialog
	std::string caption ("Restoring selected files to version:\r\n");
	GlobalIdPack pack (selectedVersion);
	caption += pack.ToBracketedString ();
	caption += " -- ";
	caption += info.GetComment ();
	caption += "\r\nfrom ";
	StrTime timeStamp (info.GetTimeStamp ());
	caption += timeStamp.GetString ();
	meterDialog->SetCaption (caption);

	_model.Revert (files, meterDialog->GetOverallMeter (), meterDialog->GetSpecificMeter ());
	_displayMan->SelectPage (CheckInAreaPage);
	DisplayTip (IDS_RESTORE);
}

Cmd::Status Commander::can_Selection_RestoreFileVersion () const
{
	if (IsProjectReady () && IsHistoryPage () && IsScriptDetailsPane () && IsSelection ())
	{
		return _model.GetHistoricalFiles ().RangeHasRejectedScripts ()
			? Cmd::Disabled : Cmd::Enabled;
	}
	return Cmd::Invisible;
}

bool Commander::CanMergeFiles (HistoricalFiles const & historicalFiles) const
{
	if (!historicalFiles.IsTargetProjectSet ())
	{
		TheOutput.Display ("Please, select target project.");
		return false;
	}

	if (historicalFiles.RangeHasMissingPredecessors ())
	{
		std::string info ("Sorry, Code Co-op cannot merge selected file(s)\n"
			"because the branch script(s) depend on some missing script(s).\n\n"
			"You'll be able to merge file(s) once "
			"you've received the missing script(s).");
		TheOutput.Display (info.c_str ());
		return false;
	}
	return true;
}

void Commander::ReverifyMergeStatus (GlobalId gid, HistoricalFiles & historicalFiles)
{
	if (!_model.PrepareMerge (gid, historicalFiles))
	{
		std::string info ("Co-op Server busy.\n"
						  "Cannot get file information from the target project.\n\n");
		Restorer & restorer = historicalFiles.GetRestorer (gid);
		info += restorer.GetRootRelativePath ();
		TheOutput.Display (info.c_str ());
	}
}

void Commander::Selection_MergeFileVersion ()
{
	if (IsProjectMergePage ())
	{
		DoMergeFileVersion (_model.GetMergedFiles (), Table::mergeDetailsTableId);
	}
	else
	{
		Assert (IsHistoryPage ());
		DoMergeFileVersion (_model.GetHistoricalFiles (), Table::scriptDetailsTableId);
	}
}

// Lock SccDll in the memory, thus speeding up its consecutive loads
class SccDllLock
{
public:
	void LockDll ()
	{
		_sccDll.reset (new CodeCoop::Dll ());
	}

private:
	std::unique_ptr<CodeCoop::Dll>	_sccDll;
};

void Commander::DoMergeFileVersion (HistoricalFiles & historicalFiles, Table::Id fileTableId)
{
	if (!CanMergeFiles (historicalFiles))
		return;

	Assert (historicalFiles.IsTargetProjectSet ());
	GidList files;
	for (SelectionSeq fileSeq (_selectionMan, fileTableId); !fileSeq.AtEnd (); fileSeq.Advance ())
	{
		GlobalId gid = fileSeq.GetGlobalId ();
		if (historicalFiles.HasTargetData (gid))
			files.push_back (gid);
	}

	SccDllLock sccDllLock;
	if (!historicalFiles.IsLocalMerge ())
		sccDllLock.LockDll ();

	for (GidList::const_iterator iter = files.begin (); iter != files.end (); ++iter)
	{
		GlobalId gid = *iter;
		ReverifyMergeStatus (gid, historicalFiles);
		MergeStatus mergeStatus = historicalFiles.GetMergeStatus (gid);
		if (mergeStatus.IsIdentical ())
		{
			_displayMan->RefreshPane (_displayMan->GetCurrentPage (), fileTableId);
			TheOutput.Display ("Files are identical");
		}
		else if (mergeStatus.IsMergeParent ())
		{
			Restorer & restorer = historicalFiles.GetRestorer (gid);
			std::string info = "The parent folder of \"";
			info += restorer.GetFileName ();
			info += "\" must be merged first";
			TheOutput.Display (info.c_str ());
		}
		else if (mergeStatus.IsAbsent ())
		{
			Restorer & restorer = historicalFiles.GetRestorer (gid);
			std::string info ("Nothing to merge. The file\n\n");
			info += restorer.GetRootRelativePath ();
			info += "\n\ndid not exist before or after.";
			TheOutput.Display (info.c_str ());
		}
		else if (mergeStatus.IsValid ())
		{
			std::unique_ptr<MergeExec> exec = historicalFiles.GetMergeExec (gid);
			if (exec->VerifyMerge (_model.GetFileIndex (), _model.GetDirectory ()))
			{
				bool useXml = false;
				if (!UsesAltMerger (useXml))
				{
					_coopTips.Display (IDS_MERGE_NOMERGER, true);// Force - ignore global tip on/off setting
				}
				bool trivialMerge;
				if (historicalFiles.IsLocalMerge ())
				{
					LocalMergerProxy merger (_model);
					trivialMerge = exec->DoMerge (merger);
				}
				else
				{
					RemoteMergerProxy merger (historicalFiles.GetTargetProjectId ());
					trivialMerge = exec->DoMerge (merger);
				}

				ReverifyMergeStatus (gid, historicalFiles);
				_displayMan->RefreshPane (_displayMan->GetCurrentPage (), fileTableId);

				if (trivialMerge)
				{
					_coopTips.Display (IDS_MERGE_NOCONFLICTS, true);// Force - ignore global tip on/off setting
				}
			}
		}
	}
}

Cmd::Status Commander::can_Selection_MergeFileVersion () const
{
	if (IsProjectReady () && (IsProjectMergePage () || IsHistoryPage ()))
	{
		if (IsProjectMergePage ())
		{
			if (IsSelection ())
			{
				if (IsMergeDetailsPane ())
					return _selectionMan->FindIfSelected (IsDifferent) ? Cmd::Enabled : Cmd::Disabled;
			}
			return Cmd::Disabled;
		}
		else
		{
			Assert (IsHistoryPage ());
			HistoricalFiles & historicalFiles = _model.GetHistoricalFiles ();
			if (historicalFiles.RangeIsAllRejected ())
			{
				return (IsSelection () && IsScriptDetailsPane ()) ? Cmd::Enabled : Cmd::Disabled;
			}
			else
			{
				return Cmd::Invisible;
			}
		}
	}
	else
		return Cmd::Invisible;
}

void Commander::Selection_AutoMergeFileVersion ()
{
	if (IsProjectMergePage ())
	{
		DoAutoMergeFileVersion (_model.GetMergedFiles (), Table::mergeDetailsTableId);
	}
	else
	{
		Assert (IsHistoryPage ());
		DoAutoMergeFileVersion (_model.GetHistoricalFiles (), Table::scriptDetailsTableId);
	}
}

void Commander::DoAutoMergeFileVersion (HistoricalFiles & historicalFiles, Table::Id fileTableId)
{
	if (!CanMergeFiles (historicalFiles))
		return;

	Assert (historicalFiles.IsTargetProjectSet ());
	GidList files;
	for (SelectionSeq fileSeq (_selectionMan, fileTableId); !fileSeq.AtEnd (); fileSeq.Advance ())
	{
		GlobalId gid = fileSeq.GetGlobalId ();
		if (historicalFiles.HasTargetData (gid))
			files.push_back (gid);
	}

	SccDllLock sccDllLock;
	if (!historicalFiles.IsLocalMerge ())
		sccDllLock.LockDll ();

	std::unique_ptr<Progress::Dialog> meterDialog (CreateProgressDlg ("Auto Merge"));
	meterDialog->SetCaption ("Merging selected files.");
	Progress::Meter & meter = meterDialog->GetProgressMeter ();
	meter.SetRange (0, files.size (), 1);
	bool mergeConflicts = false;
	try
	{
		for (GidList::const_iterator iter = files.begin (); iter != files.end (); ++iter)
		{
			GlobalId gid = *iter;
			ReverifyMergeStatus (gid, historicalFiles);
			meter.StepAndCheck ();
			MergeStatus mergeStatus = historicalFiles.GetMergeStatus (gid);
			if (mergeStatus.IsIdentical () || mergeStatus.IsAbsent ())
			{
				continue;
			}
			else if (mergeStatus.IsMergeParent ())
			{
				Restorer & restorer = historicalFiles.GetRestorer (gid);
				std::string info = "The parent folder of \"";
				info += restorer.GetFileName ();
				info += "\" must be merged first";
				TheOutput.Display (info.c_str ());
			}
			else if (mergeStatus.IsValid ())
			{
				Restorer & restorer = historicalFiles.GetRestorer (gid);
				meter.SetActivity (restorer.GetRootRelativePath ().c_str ());
				std::unique_ptr<MergeExec> exec = historicalFiles.GetMergeExec (gid, true);	// Auto merge
				if (exec->VerifyMerge (_model.GetFileIndex (), _model.GetDirectory ()))
				{
					bool successfullMerge;
					if (historicalFiles.IsLocalMerge ())
					{
						LocalMergerProxy merger (_model);
						successfullMerge = exec->DoMerge (merger);
					}
					else
					{
						RemoteMergerProxy merger (historicalFiles.GetTargetProjectId ());
						successfullMerge = exec->DoMerge (merger);
					}

					if (successfullMerge)
					{
						// Set merge status to merged
						MergeStatus newStatus;
						newStatus.SetMerged (true);
						historicalFiles.SetMergeStatus (gid, newStatus);
					}
					else
					{
						// Set merge status to conflict
						mergeConflicts = true;
						MergeStatus newStatus;
						newStatus.SetConflict (true);
						historicalFiles.SetMergeStatus (gid, newStatus);
					}
				}
				else
				{
					// Set merge status to conflict
					mergeConflicts = true;
					MergeStatus newStatus;
					newStatus.SetConflict (true);
					historicalFiles.SetMergeStatus (gid, newStatus);
				}
			}
		}
	}
	catch (Win::Exception ex)
	{
		if (ex.GetMessage () != 0)
			throw ex;
	}

	meterDialog.reset ();
	_displayMan->RefreshPane (_displayMan->GetCurrentPage (), fileTableId);

	if (mergeConflicts)
	{
		TheOutput.Display ("Automatic merging of some files resulted in conflicts.\n"
						   "Files with the merge status set to 'conflict' should be merged manually.");
	}
}

Cmd::Status Commander::can_Selection_AutoMergeFileVersion () const
{
	return can_Selection_MergeFileVersion ();
}

void Commander::Selection_CopyList ()
{
	PathFinder & pathFinder = _model.GetPathFinder ();
	std::string list;
	for (SelectionSeq seq (_selectionMan, _displayMan->GetCurrentTableId ());
		 !seq.AtEnd ();
		 seq.Advance ())
	{
		GlobalId gid = seq.GetGlobalId ();
		list += pathFinder.GetRootRelativePath (gid);
		list += "\n";
	}

	if (!list.empty ())
	{
		Clipboard clipboard (_hwnd);
		clipboard.PutText (list.c_str (), list.length ());
	}
}

Cmd::Status Commander::can_Selection_CopyList () const
{
	if (IsProjectReady () && (IsHistoryPage () || IsMailboxPage () || IsProjectMergePage ()))
	{
		return (IsSelection () && (IsScriptDetailsPane () || IsMergeDetailsPane ())) ? Cmd::Enabled : Cmd::Disabled;
	}
	return Cmd::Invisible;
}

void Commander::GoBrowse ()
{
	std::string url = _selectionMan->GetInputText ();
	if (!url.empty ())
		_displayMan->Navigate (url, 0);
}

void Commander::OnBrowse ()
{
 	StringExtractor path ("Path");
	ThePrompter.GetData (path);
	Assert (!path.GetString ().empty ());

 	StringExtractor url ("Url");
	ThePrompter.GetData (url);

	StringExtractor scroll ("ScrollPos");
	ThePrompter.GetData (scroll);
	Assert (!path.GetString ().empty ());
	int scrollPos = std::atoi (scroll.GetString ().c_str ());

	_model.GetWikiDirectory ().PushPath (path.GetString (), url.GetString (), scrollPos);
}

void Commander::Navigate ()
{
 	StringExtractor urlString ("url");
	ThePrompter.GetData (urlString);
	std::string url (urlString.GetString ());
	if (url.empty ())
		return;

	StringExtractor isStart ("isStart");
	ThePrompter.GetData (isStart);
	if (isStart.GetString () == "true")
	{
		_model.GetWikiDirectory ().GetStartUrl (url);
	}
	_displayMan->Navigate (url, 0);
}

void Commander::OpenFile ()
{
 	StringExtractor path ("Path");
	ThePrompter.GetData (path);
	Assert (!path.GetString ().empty ());
	_model.Open (path.GetString ());
}

void Commander::Selection_Edit ()
{
	int scrollPos;
	std::string path = _model.GetWikiDirectory ().GetCurrentPath (scrollPos);
	XML::Tree xmlArgs;
	XML::Node * root = xmlArgs.SetRoot ("edit");
	XML::Node * child = root->AddEmptyChild ("file");
	child->AddAttribute ("role", "current");
	child->AddTransformAttribute ("path", path);
	ExecuteEditor (xmlArgs);
}

Cmd::Status Commander::can_Selection_Edit () const
{
	if (!IsBrowserPane ())
		return Cmd::Invisible;
	// Check if it's a local wiki file
	int scrollPos;
	std::string path = _model.GetWikiDirectory ().GetCurrentPath (scrollPos);
	// Allow editing of any file that's accessible
	if (!File::Exists (path))
		return Cmd::Disabled;
	return Cmd::Enabled;
}

void Commander::Selection_Previous ()
{
	int scrollPos = 0;
	std::string url = _model.GetWikiDirectory ().PopPath (scrollPos);
	_displayMan->Navigate (url, scrollPos);
}

Cmd::Status Commander::can_Selection_Previous () const
{
	if (!IsBrowserPane ())
		return Cmd::Invisible;
	return _model.GetWikiDirectory ().HasPrevPath ()? Cmd::Enabled: Cmd::Disabled;
}

void Commander::Selection_Reload ()
{
	int scrollPos = 0;
	std::string url = _model.GetWikiDirectory ().GetCurrentPath (scrollPos);
	_displayMan->Navigate (url, scrollPos);
}

Cmd::Status Commander::can_Selection_Reload () const
{
	if (!IsBrowserPane ())
		return Cmd::Invisible;
	return _model.GetWikiDirectory ().HasCurrentPath ()? Cmd::Enabled: Cmd::Disabled;
}

void Commander::Selection_Next ()
{
	int scrollPos = 0;
	std::string path = _model.GetWikiDirectory ().NextPath (scrollPos);
	_displayMan->Navigate (path, scrollPos);
}

Cmd::Status Commander::can_Selection_Next () const
{
	if (!IsBrowserPane ())
		return Cmd::Invisible;
	return _model.GetWikiDirectory ().HasNextPath ()? Cmd::Enabled: Cmd::Disabled;
}

void Commander::Selection_Home ()
{
	int scrollPos = 0;
	std::string path = _model.GetWikiDirectory ().GetHomePath (scrollPos);
	_displayMan->Navigate (path, scrollPos);
}

Cmd::Status Commander::can_Selection_Home () const
{
	if (!IsBrowserPane ())
		return Cmd::Invisible;
	// must have at least previous path to be able to go home
	return _model.GetWikiDirectory ().HasPrevPath ()? Cmd::Enabled: Cmd::Disabled;
}

void Commander::Selection_CopyScriptComment ()
{
	std::string comment;
	SelectionSeq seq (_selectionMan, IsMailboxPage()? Table::mailboxTableId: Table::historyTableId);
	Assert (!seq.AtEnd ());
	int selectionCount = seq.Count () - 1;
	for ( ; !seq.AtEnd (); seq.Advance ())
	{
		GlobalId scriptId = seq.GetGlobalId ();
		VersionInfo info;
		if (_model.RetrieveVersionInfo (scriptId, info, IsMailboxPage ()))
		{
			comment += info.GetComment ();
			if (selectionCount > 0)
				comment += "\r\n";
		}
		--selectionCount;
	}

	if (!comment.empty ())
	{
		Clipboard clipboard (_hwnd);
		clipboard.PutText (comment.c_str (), comment.length ());
	}
}

Cmd::Status Commander::can_Selection_CopyScriptComment () const
{
	if (IsProjectReady () && (IsHistoryPage () || IsMailboxPage () || IsProjectMergePage ()))
	{
		return IsSelection () ? Cmd::Enabled : Cmd::Disabled;
	}

	return Cmd::Invisible;
}

class DragHolder
{
public:
	DragHolder(SelectionManager & selMan, Win::Dow::Handle win, unsigned id)
		: _selectionMan(selMan)
	{
		_selectionMan.BeginDrag(win, id);
	}
	~DragHolder()
	{
		_selectionMan.EndDrag();
	}
private:
	SelectionManager & _selectionMan;
};

void Commander::DoDrag ()
{
	WindowExtractor winExtr("WindowFrom");
	ThePrompter.GetData(winExtr);
	Win::Dow::Handle winFrom = winExtr.GetHandle();

	NumberExtractor idExtr("IdFrom");
	ThePrompter.GetData(idExtr);
	unsigned idFrom = idExtr.GetNumber();

	DragHolder dragHolder(*_selectionMan, winFrom, idFrom);

	SelectionSeq seq (_selectionMan);
	Assert (seq.Count () != 0);
	std::vector<std::string> paths;
	_model.RetrieveProjectPaths (seq, paths);
	Assert (paths.size () == seq.Count ());

	StringExtractor extr ("Button");
	ThePrompter.GetData (extr);
	std::string const & button = extr.GetString ();
	Assert (!button.empty ());

	Win::FileDragger fileDragDrop (paths, button == "right");
	fileDragDrop.Do ();
}

Cmd::Status Commander::can_Drag () const
{
	return IsProjectReady () ? Cmd::Enabled : Cmd::Disabled;
}

//
//	View commands
//

void Commander::View_Back ()
{
	Selection_Previous ();
}

Cmd::Status Commander::can_View_Back () const
{
	return can_Selection_Previous ();
}

void Commander::View_Forward ()
{
	Selection_Next ();
}

Cmd::Status Commander::can_View_Forward () const
{
	return can_Selection_Next ();
}

void Commander::View_Home ()
{
	Selection_Home ();
}

Cmd::Status Commander::can_View_Home () const
{
	return can_Selection_Home ();
}

void Commander::View_Default ()
{
	if (_model.IsWikiFolder ())
		View_Browser ();
	else
		View_Files ();
}

void Commander::View_Projects ()
{
	if (!_model.IsQuickVisit ())
		_displayMan->SelectPage (ProjectPage); 
}

void Commander::View_Files ()
{ 
	if (!_model.IsQuickVisit ())
		_displayMan->SelectPage (FilesPage); 
}

void Commander::View_History ()
{ 
	if (!_model.IsQuickVisit ())
		_displayMan->SelectPage (HistoryPage);
}

void Commander::View_Mailbox ()
{ 
	if (!_model.IsQuickVisit ())
		_displayMan->SelectPage (MailBoxPage);
}

void Commander::View_Synch ()
{ 
	if (!_model.IsQuickVisit ())
		_displayMan->SelectPage (SynchAreaPage);
}

void Commander::View_CheckIn ()
{ 
	if (!_model.IsQuickVisit ())
		_displayMan->SelectPage (CheckInAreaPage);
}

void Commander::View_Browser ()
{
	if (!_model.IsQuickVisit ())
		_displayMan->SelectPage (BrowserPage);
}

Cmd::Status Commander::can_View_Browser () const
{
	if (_model.IsWikiFolder ())
	{
		return IsBrowserPane () ? Cmd::Checked : Cmd::Enabled;
	}
	return Cmd::Disabled;
}

void Commander::View_MergeProjects ()
{
	if (!_model.IsQuickVisit ())
	{
		if (IsHistoryPage ())
		{
			Selection_MergeBranch ();
		}
		else
		{
			HistoricalFiles & mergedFiles = _model.GetMergedFiles ();
			mergedFiles.Clear ();
			_displayMan->SelectPage (ProjectMergePage);
		}
	}
}

Cmd::Status Commander::can_View_MergeProjects () const
{
	if (IsThisProjectSelected ())
	{
		return IsProjectMergePage () ? Cmd::Checked : Cmd::Enabled;
	}
	return Cmd::Disabled;
}

void Commander::View_Refresh ()
{
	if (IsMailboxPage ())
	{
		DoRefreshMailbox (true);
	}
	else if (IsProjectMergePage ())
	{
		std::unique_ptr<Progress::Dialog> meterDialog (CreateProgressDlg ("View Refresh"));
		meterDialog->SetCaption ("Refreshing files merge status.");
		_model.PrepareMerge (meterDialog->GetProgressMeter (), _model.GetMergedFiles ());
		_displayMan->RefreshPane (ProjectMergePage, Table::mergeDetailsTableId);
	}
	else
	{
		_displayMan->Refresh (_displayMan->GetCurrentPage ());
	}
}

void Commander::View_Next ()
{
	_displayMan->SwitchPage (true);
}

void Commander::View_Previous ()
{
	_displayMan->SwitchPage (false);
}

void Commander::View_Hierarchy ()
{ 
	_displayMan->ToggleHierarchyView ();
}

Cmd::Status Commander::can_View_Hierarchy () const
{
	bool isVisible = false;
	if (! _displayMan->CanShowHierarchy (isVisible))
		return Cmd::Invisible;
	else
		return isVisible? Cmd::Checked : Cmd::Enabled;
}

void Commander::View_Active_Projects ()
{ 
	_displayMan->ToggleDetailsView ();
}

Cmd::Status Commander::can_View_Active_Projects () const
{
	bool isVisible = false;
	if (!_displayMan->CanShowDetails (isVisible))
		return Cmd::Invisible;
	else
		return isVisible? Cmd::Checked : Cmd::Enabled;
}

void Commander::View_ClosePage ()
{
	ViewPage page = _displayMan->GetCurrentPage ();
	View_Default ();
	_displayMan->ClosePage (page);
	if (page == ProjectMergePage)
		_model.ClearMergeTarget (true);	// Save target hints
}

Cmd::Status Commander::can_View_ClosePage () const
{
	return IsProjectMergePage () ? Cmd::Enabled : Cmd::Invisible;
}

Cmd::Status Commander::can_View_Refresh () const
{
	return Cmd::Enabled;
}

Cmd::Status Commander::can_View_Projects () const
{
	return IsProjectPage () ? Cmd::Checked : Cmd::Enabled;
}

Cmd::Status Commander::can_View_Files () const
{
	return IsFilePane () ? Cmd::Checked : Cmd::Enabled;
}

Cmd::Status Commander::can_View_Mailbox () const
{
	return IsMailboxPage () ? Cmd::Checked : Cmd::Enabled;
}

Cmd::Status Commander::can_View_CheckIn () const
{
	return IsCheckInAreaPage () ? Cmd::Checked : Cmd::Enabled;
}

Cmd::Status Commander::can_View_Synch () const
{
	if (_model.IsSynchAreaEmpty ())
		return Cmd::Disabled;

	return IsSynchAreaPage () ? Cmd::Checked : Cmd::Enabled;
}

Cmd::Status Commander::can_View_History () const
{
	return IsHistoryPage () ? Cmd::Checked : Cmd::Enabled;
}

void Commander::View_ApplyFileFilter ()
{
	_displayMan->SetRestrictionFlag ("FileFiltering", !IsFilterOn ());
	_displayMan->Refresh (FilesPage);
}

Cmd::Status Commander::can_View_ApplyFileFilter () const
{
	if (IsProjectReady () && IsFilePage ())
	{
		return IsFilterOn () ? Cmd::Checked : Cmd::Enabled;
	}
	return Cmd::Invisible;
}

void Commander::View_ChangeFileFilter ()
{
	Restriction const & restriction = _displayMan->GetPresentationRestriction ();
	NocaseSet extFilter = restriction.GetExtensionFilter ();
	bool hideNonProject = restriction.IsOn ("HideNonProject");
	HideExtensionCtrl ctrl (extFilter, hideNonProject, _model.GetProjectDir ());
    if (ThePrompter.GetData (ctrl))
    {
		_displayMan->SetExtensionFilter (extFilter);
		_displayMan->SetRestrictionFlag ("HideNonProject", hideNonProject);
		if (extFilter.empty () && !hideNonProject)
			_displayMan->SetRestrictionFlag ("FileFiltering", false);
		else
			_displayMan->SetRestrictionFlag ("FileFiltering", true);
		_displayMan->Refresh (FilesPage);
    }
}

Cmd::Status Commander::can_View_ChangeFileFilter () const
{
	return IsProjectReady () && IsFilePane () ? Cmd::Enabled : Cmd::Invisible;
}

void Commander::RefreshMailbox ()
{
	DoRefreshMailbox (false);
}

void Commander::DoRefreshMailbox (bool force)
{
	// Eat all exceptions
	try
	{
		do
		{
			bool projectWasReady = _model.IsProjectReady ();
			// Unpack incoming scripts and execute control & forced scripts
			_model.ProcessMail (force);
			if (_model.IsInProject ())
			{
				if (IsProjectMergePage ())
				{
					// Unpacked incoming script while showing project merge view.
					HistoricalFiles const & mergedFiles = _model.GetMergedFiles ();
					if (mergedFiles.IsTargetProjectSet () && !mergedFiles.IsLocalMerge ())
					{
						// If external merge target then update 'interesting scripts'
						// in the current view restriction, so the just unpacked scripts
						// become selectable.
						GidSet interestingScripts;
						_model.GetInterestingScripts (interestingScripts);
						_displayMan->SetInterestingItems (interestingScripts);
						_displayMan->Refresh (ProjectMergePage);
					}
				}

				if (IsProjectPage ())
				{
					// If we unpacked project verification package we need to refresh
					// project view and check-in area view
					_displayMan->Refresh (ProjectPage);
					_displayMan->Refresh (CheckInAreaPage);
				}

				if (_model.HasNextScript () && _model.IsSynchAreaEmpty ())
				{
					// We have unpacked script marked as next
					// and we don't have pending sync
					if (_model.NextIsFullSynch ())
					{
						// Next to be executed is the full sync script.
						// Don't execute it. If auto full sync is on then
						// the full sync script is executed as forced script.
					}
					else if (_model.IsAutoSynch ())
					{
						// Execute all unpacked scripts
						ExecuteAndAcceptAllScripts ();
					}
					else if (_model.NextIsMilestone())
					{
						// milestones are executed automatically
						do
							ExecuteAndAcceptScript();
						while (_model.NextIsMilestone());
					}
				}

				if (_model.IsAutoJoin () && _model.HasExecutableJoinRequest ())
					ExecuteJoinRequest ();

				if (!_model.IsQuickVisit ())
				{
					_displayMan->Refresh (MailBoxPage);
					_displayMan->Refresh (FilesPage);
					if (IsCheckInAreaPage ())
						_displayMan->Refresh (CheckInAreaPage);
					if (!projectWasReady && _model.GetMyId () != gidInvalid)
					{
						// Unpacked full sync package -- refresh GUI and switch to mailbox view
						_displayMan->RefreshAll ();
						View_Mailbox ();
					}
				}
			}
			else if (!_model.IsQuickVisit ())
			{
				// Not in the project after processing incoming scripts
				_displayMan->Refresh (ProjectPage);
				SetTitle ();
				View_Projects ();
			}
		}
		while (_model.IsInProject () && !_model.AllIncomingScriptsAreUnpacked ());
	}
	catch ( ... )
	{
		Win::ClearError ();
	}
}

//
// File & Folder commands
// 

void Commander::Folder_GotoRoot ()
{
    _model.GotoRoot ();
	_displayMan->GotoRoot ();
	AfterDirChange ();
}

Cmd::Status Commander::can_Folder_GotoRoot () const
{
	if (IsProjectReady () && IsFilePane ())
	{
		return _model.IsRootFolder () ? Cmd::Disabled : Cmd::Enabled;
	}
	return Cmd::Disabled;
}

void Commander::Folder_GoUp ()
{
    _model.FolderUp ();
    _displayMan->GoUp ();
	AfterDirChange ();
}

Cmd::Status Commander::can_Folder_GoUp () const
{
	if (IsProjectReady () && IsFilePane ())
	{
		return _model.IsRootFolder () ? Cmd::Disabled : Cmd::Enabled;
	}
	return Cmd::Disabled;
}

// called after backspace
void Commander::GoBack ()
{
	if (IsFilePane () && can_Folder_GoUp () == Cmd::Enabled)
		Folder_GoUp ();
	if (IsBrowserPane () && can_View_Back () == Cmd::Enabled)
		View_Back ();
}

void Commander::Folder_NewFile ()
{
	bool done = false;
    NewFileData dlgData (_model.GetCopyright (), 
						 _model.GetDirectory ().GetCurrentPath (),
						 HeaderFile ());
    NewFileCtrl ctrl (&dlgData);
    if (ThePrompter.GetData (ctrl))
    {
        if (dlgData.IsDefault () && dlgData.IsNewCopyright ())
            _model.SetCopyright (dlgData.GetContent ());

		done = _model.NewFile (dlgData);
    }
	if (done)
	{
		if (IsNocaseEqual (dlgData.GetName (), "index.wiki"))
		{
			WikiDirectory & wikiDir = _model.GetWikiDirectory ();
			wikiDir.AfterDirChange (_model.GetDirectory ().GetCurrentPath ());
			_displayMan->Rebuild (true);
			View_Default ();
		}
		DisplayTip (IDS_NEWFILE);
	}
}

void Commander::DoNewFile ()
{
 	StringExtractor fileName ("FileName");
	ThePrompter.GetData (fileName);
	Assert (!fileName.GetString ().empty ());
	StringExtractor nameSpace ("Namespace");
	ThePrompter.GetData (nameSpace);
	StringExtractor doOpen ("Open");
	ThePrompter.GetData (doOpen);
	StringExtractor doAdd ("Add");
	ThePrompter.GetData (doAdd);
	StringExtractor folderPath ("FolderPath");
	ThePrompter.GetData (folderPath);
	if (IsNocaseEqual (nameSpace.GetString (), "Image"))
	{
		FileGetter openFileDlg;
		openFileDlg.SetFilter (Wiki::ImageOpenPattern);
		std::string title ("Find image: ");
		title += fileName.GetString ();
		if (openFileDlg.GetExistingFile (TheAppInfo.GetWindow (), title.c_str ()))
		{
			PathSplitter srcSplitter (openFileDlg.GetFileName ());
			PathSplitter tgtSplitter (fileName.GetString ());
			// use the actual extension of the source image
			std::string fileName = tgtSplitter.GetFileName ();
			fileName += srcSplitter.GetExtension ();
			PathFinder::MaterializeFolderPath (folderPath.GetString ().c_str ());
			_model.CopyAddFile (openFileDlg.GetPath (), folderPath.GetString (), fileName, BinaryFile ());
		}
	}
	else
	{
		PathFinder::MaterializeFolderPath (folderPath.GetString ().c_str ());
 		StringExtractor fileContents ("Contents");
		ThePrompter.GetData (fileContents);
		NewFileData dlgData (fileContents.GetString (), 
							folderPath.GetString (),
							TextFile ());
		dlgData.SetUseContent (true);
		dlgData.SetName (fileName.GetString ());
		dlgData.SetDoOpen (doOpen.GetString () == "true");
		dlgData.SetDoAdd (doAdd.GetString () == "true");
		_model.NewFile (dlgData);
	}
}

void Commander::DoDeleteFile ()
{
 	StringExtractor filePath ("FilePath");
	ThePrompter.GetData (filePath);
	Assert (!filePath.GetString ().empty ());
	_model.DeleteFile (filePath.GetString ());
}

Cmd::Status Commander::can_Folder_NewFile () const
{
	return (IsProjectReady () && IsFilePane ()) ? Cmd::Enabled : Cmd::Disabled;
}

void Commander::Folder_Wikify ()
{
	FilePath const & wikiPath = _model.GetCatalog ().GetWikiPath ();
	char const * wikiIndexPath = wikiPath.GetFilePath ("index.wiki");
	std::string contents = "=Edit Me\n";
	if (File::Exists (wikiIndexPath))
	{
		MemFileReadOnly indexFile (wikiIndexPath);
		contents.assign (indexFile.GetBuf (), indexFile.GetBufSize ());
	}
	char const * curPath = _model.GetDirectory ().GetCurrentPath ();
    NewFileData dlgData (contents, 
						 curPath,
						 TextFile ());
	dlgData.SetUseContent (true);
	dlgData.SetName ("index.wiki");
	dlgData.SetDoOpen (true);

	bool done = _model.NewFile (dlgData);

	if (done)
	{
		_model.GetWikiDirectory ().AfterDirChange (curPath);
		_displayMan->Rebuild (true);
		View_Default ();
	}
}

Cmd::Status Commander::can_Folder_Wikify () const
{
	if (IsProjectReady () && IsFilePane () && !_model.IsWikiFolder ())
		return Cmd::Enabled;

	return Cmd::Invisible;
}

void Commander::Folder_ExportHtml ()
{
	SaveHtmlData data (_model.GetWikiDirectory ().GetRootName ());
	SaveHtmlCtrl ctrl (data);
	if (!ThePrompter.GetData (ctrl))
		return;	// User canceled dialog
	WikiDirectory & browser = _model.GetWikiDirectory ();
	browser.ExportHtml (data.GetTargetFolder ());
}

Cmd::Status Commander::can_Folder_ExportHtml () const
{
	return _model.IsWikiFolder ()? Cmd::Enabled: Cmd::Disabled;
}

// Starts new folder operation (activated from menu)
void Commander::Folder_NewFolder ()
{
    _displayMan->BeginNewFolderEdit ();
}

Cmd::Status Commander::can_Folder_NewFolder () const
{
	return (IsProjectReady () && IsFilePane ()) ? Cmd::Enabled : Cmd::Disabled;
}

// Finishes new folder operation (activated from notification handler)
void Commander::DoNewFolder ()
{
	try
	{
		StringExtractor extr ("NewName");
		ThePrompter.GetData (extr);
		Assert (!extr.GetString ().empty ());
		if (!_model.NewFolder (extr.GetString ().c_str ()))
			_displayMan->AbortNewFolderEdit ();
	}
	catch (Win::Exception ex)
	{
		TheOutput.Display (ex);
		_displayMan->AbortNewFolderEdit ();
	}
	catch (...)
	{
		Win::ClearError ();
		TheOutput.Display ("New Folder -- Unknown Error", Out::Error);
		_displayMan->AbortNewFolderEdit ();
	}
}

void Commander::Folder_AddMoreFiles ()
{
	Directory & directory = _model.GetDirectory ();
	NocaseSet alreadyControlled;
	GlobalId currentFolderGid = directory.GetCurrentId ();
	std::string path (directory.GetCurrentPath ());
	if (currentFolderGid != gidInvalid)
		_model.ListControlledFiles (currentFolderGid, alreadyControlled);
	std::vector<std::string> files;
	if (GetFolderContents (path, alreadyControlled, files))
	{
		if (!files.empty ())
		{
			FileTyper fileTyper (_hwnd, _model.GetCatalog (), _inputSource);
			bool done = _model.AddFiles (files, fileTyper);
			if (done)
				DisplayTip (IDS_ADDFILE);
		}
	}
}

Cmd::Status Commander::can_Folder_AddMoreFiles () const
{
	return (IsProjectReady () && IsFilePane ()) ? Cmd::Enabled : Cmd::Disabled;
}

// Finishes current folder selection (activated from notification handler)
void Commander::SetCurrentFolder ()
{
	try
	{
		StringExtractor extr ("FolderName");
		ThePrompter.GetData (extr);
		if (_model.ChangeDirectory (extr.GetString ()))
		{
			_displayMan->Refresh (_displayMan->GetCurrentPage ());
			AfterDirChange ();
		}
	}
	catch (Win::Exception ex)
	{
		TheOutput.Display (ex);
	}
	catch (...)
	{
		Win::ClearError ();
		TheOutput.Display ("Set Current Folder -- Unknown Error", Out::Error);
	}
}

void Commander::AfterDirChange ()
{
	VerifyRecordSet ();
	std::string curPath = _model.GetDirectory ().GetCurrentPath ();
	bool isWiki = _model.IsWikiFolder ();
	if (isWiki)
	{
		bool isChanged = _model.GetWikiDirectory ().AfterDirChange (curPath);
		_displayMan->Rebuild (isWiki);
		if (isChanged)
			View_Default ();
	}
}

//
// All commands
//

void Commander::All_CheckOut ()
{
	// In command line mode use file area table selection
	// In GUI mode use current window selection.
    AllSeq seq (_selectionMan,
		_model.IsQuickVisit () ? Table::folderTableId : _displayMan->GetCurrentTableId ());
	GidList controlledFiles;
	SplitSelection (seq, controlledFiles);
	if (controlledFiles.empty ())
		return;

	if (IsChecksumOK (controlledFiles, false, true)) // Non recursive during checkout
	{
		// Checkout all, except folders
		DoCheckout (controlledFiles, false, false);
	}
}

Cmd::Status Commander::can_All_CheckOut () const
{
	if (IsProjectReady () && (IsFilePage () || IsSynchAreaPane ()))
	{
		if (CanMakeChanges ())
		{
			return _selectionMan->FindIfSome (IsCheckoutable) ?
					Cmd::Enabled : Cmd::Disabled;
		}
	}
	return Cmd::Disabled;
}

void Commander::All_DeepCheckOut ()
{
    AllSeq seq (_selectionMan, Table::folderTableId);
	GidList controlledFiles;
	SplitSelection (seq, controlledFiles);
	if (controlledFiles.empty ())
		return;

	if (IsChecksumOK (controlledFiles, true, true)) // Recursive during checkout
	{
		// Checkout all; folders recursively
		DoCheckout (controlledFiles, true, true);
	}
}

Cmd::Status Commander::can_All_DeepCheckOut () const
{
	if (IsProjectReady () && IsFilePane () && CanMakeChanges ())
	{
		if (!_displayMan->IsEmptyPage (FilesPage))
		{
			return _selectionMan->FindIfSome (IsFolderCheckedIn)
				? Cmd::Enabled : Cmd::Disabled;
		}
	}
	return Cmd::Invisible;
}

void Commander::DoAllCheckIn (bool isInventory)
{
	if (!CanCheckin ())
		return;

	AllSeq seq (_selectionMan, Table::checkInTableId);
	GidList controlledFiles;
	SplitSelection (seq, controlledFiles);
	if (controlledFiles.empty ())
		return;

	CheckInUI checkInUI (_hwnd, _inputSource);
	if (_model.CheckIn (controlledFiles, checkInUI, isInventory))
	{
		TriggerPostCheckin(_model.GetProjectDir(), _hwnd);
		View_Default ();
		DisplayTip (IDS_CHECKIN);
	}
}

void Commander::All_CheckIn ()
{
	if (!_model.IsQuickVisit ())
		_selectionMan->SelectAll ();
	DoAllCheckIn ();
}

Cmd::Status Commander::can_All_CheckIn () const
{
	if (IsProjectReady () && CanMakeChanges () && IsCheckInAreaPane ())
	{
		if (_displayMan->IsEmptyPage (CheckInAreaPage))
			return Cmd::Disabled;	// Nothing to check-in

		bool blocked = false;
		bool canCheckin = _model.CanCheckIn (blocked);
		if (blocked)
			return Cmd::Enabled;	// Enable command, so we can display error message
		else
			return canCheckin ? Cmd::Enabled : Cmd::Disabled;
	}
	return Cmd::Disabled;
}

void Commander::All_Uncheckout ()
{
	// In command line mode use check-in area table selection
	// In GUI mode use current window selection.
	AllSeq seq (_selectionMan,
		_model.IsQuickVisit () ? Table::checkInTableId : _displayMan->GetMainTableId ());
	GidList controlledFiles;
	SplitSelection (seq, controlledFiles);
	if (controlledFiles.empty ())
		return;

    bool done = _model.Uncheckout (controlledFiles, _model.IsQuickVisit ());

	if (_model.IsQuickVisit ())
		return;
	// Switch to sync area view if not empty,
	// else switch to file view.
	if (IsCheckInAreaPane ())
	{
		_displayMan->Refresh (CheckInAreaPage);
		if (_model.IsSynchAreaEmpty ())
			View_Default ();
		else
			View_Synch ();
	}
	if (done)
		DisplayTip (IDS_UNCHECKOUT);
}

Cmd::Status Commander::can_All_Uncheckout () const
{
	if (IsProjectReady () && CanMakeChanges () && (IsCheckInAreaPane () || IsFilePane ()))
	{
		if (IsCheckInAreaPane ())
			return _displayMan->IsEmptyPage (CheckInAreaPage) ? Cmd::Disabled : Cmd::Enabled;
		else if (IsFilePane ())
			return _selectionMan->FindIfSome (IsCheckedOut) 
				? Cmd::Enabled : Cmd::Disabled;
	}
	return Cmd::Disabled;
}

void Commander::All_AcceptSynch ()
{
    AllSeq seq (_selectionMan, Table::synchTableId);
	DoAcceptSynch (seq);
}

void Commander::DoAcceptSynch (WindowSeq & seq)
{
	if (!_model.AcceptSynch (seq))
	{
		TheOutput.Display ("There are unresolved conflicts in the sync/merge area.\n"
						   "Double-click on each file in the conflict state and perform the merge.");
		return;
	}

	if (!_model.IsSynchAreaEmpty ())
		return;

	// Sync area is empty
	CloseSyncAreaView ();
}

void Commander::CloseSyncAreaView ()
{
	Assert (_model.IsSynchAreaEmpty ());
	if (_model.HasIncomingOrMissingScripts ())
	{
		View_Mailbox ();
		if (_model.IsAutoSynch ())
		{
			// Force mailbox update
			_displayMan->Refresh (MailBoxPage);
			All_Synch ();
		}
	}
	else if (!_model.IsCheckInAreaEmpty ())
		View_CheckIn ();
	else
		View_Default ();

	Win::UserMessage um (UM_CLOSE_PAGE);
	um.SetWParam (SynchAreaPage);
	_hwnd.PostMsg (um);
}

Cmd::Status Commander::can_All_AcceptSynch () const
{
	if (IsProjectReady () && IsSynchAreaPane ())
	{
		return _model.IsSynchAreaEmpty () ? Cmd::Disabled : Cmd::Enabled;
	}
	return Cmd::Invisible;
}

void Commander::ExecuteJoinRequest ()
{
	// Execute in non-gui mode only if (not distributor and auto-join)
	if (_model.IsQuickVisit ()
		&& (_model.GetUserPermissions ().IsDistributor () || !_model.IsAutoJoin ()))
		return; 

	if (_model.IsQuickVisit ())
	{
		// In command line mode
		Assert (_model.IsAutoJoin ());
	}

	if (!_model.IsSynchAreaEmpty ())
	{
		throw Win::InternalException ("Synch Area is not empty.\n"
			"Complete previous synchronization script\n"
			"before accepting the join request.");
	}

	if (!_model.IsMembershipUpToDate ())
	{
		throw Win::InternalException ("Missing membership updates.\n"
			"You have to wait for automatic re-sends to complete\n"
			"before you can accept the join request.");
	}

	std::unique_ptr<Progress::Dialog> meterDialog (CreateProgressDlg ("Project Join",
																	true,	// User can cancel
																	1000,	// 1 second initial delay
																	true));	// Multi meter dialog
	ProjectChecker projectChecker (_model);
	_model.ExecuteJoinRequest (projectChecker, meterDialog->GetOverallMeter(), meterDialog->GetSpecificMeter());
}

// Returns true if file script executed successfully
bool Commander::ExecuteSetScript (bool & beginnerHelp)
{
	ScriptConflictDlg conflictDialog;
	std::unique_ptr<Progress::Dialog> meterDialog (CreateProgressDlg ("Sync Script",
																	true,	// User can cancel
																	1000,	// 1 second initial delay
																	true));	// Multi meter dialog
	VersionInfo scriptInfo;
	_model.RetrieveNextVersionInfo (scriptInfo);
	std::string caption ("Executing synchronization script:\r\n");
	caption += scriptInfo.GetComment ();
	meterDialog->SetCaption (caption);

	bool done = _model.ExecuteSetScript (conflictDialog,
										 meterDialog->GetOverallMeter (),
										 meterDialog->GetSpecificMeter (),
										 beginnerHelp);
	if (!_model.IsQuickVisit () && done)
	{
		if (IsMailboxPage ())
		{
			ThePrompter.SetNamedValue ("Extend", "preserve");
			CreateRange ();
			ThePrompter.ClearNamedValues ();
			_displayMan->Refresh (MailBoxPage);
		}
		_displayMan->Refresh (ProjectPage);
	}
	return done;
}

// Returns true when script executed and accepted successfully
bool Commander::ExecuteAndAcceptScript ()
{
	bool beginnerHelp;
	if (ExecuteSetScript (beginnerHelp))
	{
		if (_model.IsMerge ())
		{
			Assume (!_model.IsSynchAreaEmpty (), _model.GetProjectDb ().ProjectName ().c_str ());
			// Merge conflict
			_displayMan->Refresh (SynchAreaPage);
			if (!_model.IsQuickVisit ())
			{
				// In GUI mode display merge warning
				TheOutput.Display (_mergeWarning);
				View_Synch ();
			}
		}
		else
		{
			std::unique_ptr<Progress::Dialog> meterDialog (CreateProgressDlg ("Sync Script",
																			false));	// Cannot cancel
			meterDialog->SetCaption ("Executing commands from the synchronization script.");
			_model.AcceptSynch (meterDialog->GetProgressMeter ());
			Assume (_model.IsSynchAreaEmpty (), _model.GetProjectDb ().ProjectName ().c_str ());
			return true;
		}
	}
	return false;
}

void Commander::ExecuteAndAcceptAllScripts ()
{
	bool fileScriptAccepted = false;
	while (_model.HasNextScript ())
	{
		if (!ExecuteAndAcceptScript ())
			return;

		fileScriptAccepted = true;
	}

	Assert (!_model.HasNextScript ());
	if (!fileScriptAccepted)
		return;

	if (_model.IsMerge ())
	{
		throw Win::InternalException (
			"Internal Error: Merge did not stop script execution.\n"
			"Please contact support@relisoft.com");
	}

	if (_model.IsAutoSynch ())
	{
		// Run post auto-synch external command if present
		TriggerPostSync(_model.GetProjectDir(), _hwnd);
	}
}

void Commander::All_Synch ()
{
	if (_model.IsQuickVisit ())
		_model.ProcessMail ();

	ExecuteAndAcceptAllScripts ();
	while (_model.HasExecutableJoinRequest ())
		ExecuteJoinRequest ();

	if (_model.IsQuickVisit ())
		return;

	_displayMan->Refresh (ProjectPage);
	if (!_model.IsSynchAreaEmpty ())
		View_Synch ();
	else
		View_Default ();
}

Cmd::Status Commander::can_All_Synch () const
{
	if (IsMailboxPage () && !_displayMan->IsEmptyPage (MailBoxPage))
	{
		// Execute set scripts and join requests
		if (_selectionMan->IsDefaultSelection ())
			return Cmd::Enabled;
		else
			return Cmd::Disabled;
	}
	return Cmd::Invisible;
}

void Commander::All_Report ()
{
	DoReport (true);	// Report all items
}

Cmd::Status Commander::can_All_Report () const
{
	return IsProjectReady () ? Cmd::Enabled : Cmd::Disabled;
}

//  Select All -- command accessible only via keyboard accelerator.
void Commander::All_Select ()
{
	_selectionMan->SelectAll ();
}

Cmd::Status Commander::can_All_Select () const
{
	return Cmd::Enabled;
}

void Commander::All_SaveFileVersion ()
{
	AllSeq  seq (_selectionMan, Table::scriptDetailsTableId);
	DoSaveFileVersion (seq);
}

Cmd::Status Commander::can_All_SaveFileVersion () const
{
	return can_Selection_SaveFileVersion ();
}

//
// Selection commands
//

void Commander::Selection_CheckOut ()
{
	// In command line mode use file area table selection
	// In GUI mode use current window selection.
	SelectionSeq seq (_selectionMan,
		_model.IsQuickVisit () ? Table::folderTableId : _displayMan->GetCurrentTableId ());
	GidList controlledFiles;
	SplitSelection (seq, controlledFiles);
	if (controlledFiles.empty ())
		return;

	if (IsChecksumOK (controlledFiles, false, true)) // Non-recursive during checkout
	{
		// Checkout selected folders non-recursively
		DoCheckout (controlledFiles, true, false);
	}
}

Cmd::Status Commander::can_Selection_CheckOut () const
{
	if (IsProjectReady () && (IsFilePage () || IsSynchAreaPane ()))
	{
		if (IsSelection () && CanMakeChanges ())
		{
			return _selectionMan->FindIfSelected (IsCheckoutable) 
						? Cmd::Enabled : Cmd::Disabled;
		}
		return Cmd::Disabled;
	}
	return Cmd::Invisible;
}

void Commander::Selection_DeepCheckOut ()
{
    SelectionSeq seq (_selectionMan, Table::folderTableId);
	GidList controlledFiles;
	SplitSelection (seq, controlledFiles);
	if (controlledFiles.empty ())
		return;

	if (IsChecksumOK (controlledFiles, true, true)) // Recursive during checkout
	{
		// Checkout selected folders recursively
		DoCheckout (controlledFiles, true, true);
	}
}

Cmd::Status Commander::can_Selection_DeepCheckOut () const
{
	if (IsProjectReady () && IsFilePane ())
	{
		if (IsSelection () && CanMakeChanges ())
		{
			return _selectionMan->FindIfSelected (IsFolderCheckedIn) 
				? Cmd::Enabled : Cmd::Disabled;
		}
		return Cmd::Disabled;
	}
	return Cmd::Invisible;	
}

void Commander::Selection_CheckIn ()
{
	if (!CanCheckin ())
		return;

	SelectionSeq seq (_selectionMan, Table::checkInTableId);
	GidList controlledFiles;
	SplitSelection (seq, controlledFiles);
	if (controlledFiles.empty ())
		return;

	CheckInUI checkInUI (_hwnd, _inputSource);

	bool isRegularCheckIn = _model.CheckIn (controlledFiles, checkInUI, false);

	if (!isRegularCheckIn) // "Not regular" means initial file inventory
		return;

	TriggerPostCheckin(_model.GetProjectDir(), _hwnd);

	if (_model.IsQuickVisit ())
		return;

	if (_model.IsCheckInAreaEmpty ())
	{
		// If check-in area becomes empty switch to file view
		View_Default ();
	}
	else
	{
		_displayMan->Refresh (CheckInAreaPage);
	}
	DisplayTip (IDS_CHECKIN);
}

Cmd::Status Commander::can_Selection_CheckIn () const
{
	if (IsProjectReady () && IsCheckInAreaPane ())
	{
		if (IsSelection () && CanMakeChanges ())
		{
			bool blocked = false;
			bool canCheckin = _model.CanCheckIn (blocked);
			if (blocked)
				return Cmd::Enabled;	// Enable command, so we can display error message
			else
				return canCheckin ? Cmd::Enabled : Cmd::Disabled;
		}
		return Cmd::Disabled;
	}
	return Cmd::Invisible;
}

void Commander::Selection_Uncheckout ()
{
	// In command line mode use check-in area table selection
	// In GUI mode use current window selection.
	SelectionSeq seq (_selectionMan,
		_model.IsQuickVisit () ? Table::checkInTableId : _displayMan->GetCurrentTableId ());
	GidList controlledFiles;
	SplitSelection (seq, controlledFiles);
	if (controlledFiles.empty ())
		return;

    bool done = _model.Uncheckout (controlledFiles, _model.IsQuickVisit ());

	if (_model.IsQuickVisit ())
		return;
	// If check-in area becomes empty switch to
	// file view if sync area is empty
	// else switch to sync area.
	if (IsCheckInAreaPane ())
	{
		if (_model.IsCheckInAreaEmpty ())
		{
			_displayMan->Refresh (CheckInAreaPage);
			if (_model.IsSynchAreaEmpty ())
				View_Default ();
			else
				View_Synch ();
		}
	}
	if (done)
		DisplayTip (IDS_UNCHECKOUT);
}

Cmd::Status Commander::can_Selection_Uncheckout () const
{
	if (IsProjectReady () && (IsFilePage () || IsCheckInAreaPane () || IsSynchAreaPane ()))
	{
		if (IsSelection ())
		{
			return _selectionMan->FindIfSelected (IsCheckedOut) 
								? Cmd::Enabled : Cmd::Disabled;
		}
		return Cmd::Disabled;
	}
	return Cmd::Invisible;
}

void Commander::Selection_Properties ()
{
	if (IsHistoryPane ())
	{
		Selection_OpenHistoryScript ();
	}
	else
	{
		Assert (IsFilePane () || IsCheckInAreaPage ());
		SelectionSeq seq (_selectionMan, _displayMan->GetCurrentTableId ());
		Assert (!seq.AtEnd ());
		std::unique_ptr<FileProps> fileProps = _model.GetFileProps (seq.GetGlobalId ());
		Assert (fileProps.get () != 0);
		FilePropertyHndlrSet dlgCtrl (*fileProps);
		if (ThePrompter.GetData (dlgCtrl))
			_model.SetCheckoutNotification (fileProps->IsCheckoutNotificationOn ());
	}
}

Cmd::Status Commander::can_Selection_Properties () const
{
#if defined (COOP_PRO)
	if (!IsProjectReady ())
		return Cmd::Invisible;

	if (IsHistoryPage () || IsProjectMergePage ())
	{
		if (_selectionMan->SelCount () != 1)
			return Cmd::Disabled;

   		if (IsHistoryPane ())
   			return _selectionMan->FindIfSelected (IsCurrent) ? Cmd::Disabled : Cmd::Enabled;
	}
	else if (IsFilePane () || IsCheckInAreaPage ())
	{
		if (_selectionMan->SelCount () != 1)
			return Cmd::Disabled;

		if (IsFilePane ())
			return _selectionMan->FindIfSelected (IsFileInProject) ? Cmd::Enabled : Cmd::Disabled;

		return Cmd::Enabled;
	}
#endif
	return Cmd::Invisible;
}

void Commander::Selection_Open ()
{
	Table::Id tableId = _displayMan->GetCurrentTableId ();
	switch (tableId)
	{
	case Table::folderTableId:
		Selection_OpenFile ();
		break;
	case Table::checkInTableId:
		Selection_OpenCheckInDiff ();
		break;
	case Table::mailboxTableId:
		Selection_OpenIncommingScript ();
		break;
	case Table::synchTableId:
		Selection_OpenSynchDiff ();
		break;
	case Table::historyTableId:
		if (IsHistoryPage ())
		{
			HistoricalFiles const & historicalFiles = _model.GetHistoricalFiles ();
			if (historicalFiles.IsEmpty ())
				return;

			GidList files;
			historicalFiles.GetFileList (files);
			if (files.size () == 1)
			{
				std::vector<unsigned> items;
				items.push_back (0);
				_selectionMan->SelectItems (items, Table::scriptDetailsTableId);
				Selection_OpenHistoryDiff ();
			}
			else
			{
				Selection_OpenHistoryScript ();
			}
		}
		else if (IsProjectMergePage ())
		{
			Selection_OpenHistoryScript ();
		}
		break;
	case Table::scriptDetailsTableId:
		Selection_OpenHistoryDiff ();
		break;
	case Table::mergeDetailsTableId:
		if (can_Selection_MergeFileVersion ())
			Selection_MergeFileVersion ();
		break;
	case Table::projectTableId:
		Selection_OpenProject ();
		break;
	}
}

Cmd::Status Commander::can_Selection_Open () const
{
	if (IsSelection ())
	{
		if (IsProjectPane () && !_displayMan->IsEmptyPage (ProjectPage))
			return Cmd::Enabled;

		if (IsProjectReady ())
		{
			if (IsFilePage ())
				return _selectionMan->FindIfSelected (IsProjectRoot) ? Cmd::Disabled : Cmd::Enabled;

			return Cmd::Enabled;
		}
	}
	return Cmd::Disabled;
}

void Commander::Selection_OpenWithShell ()
{
	// Open files using Windows Shell
	if (IsFilePane ())
	{
		Directory & directory = _model.GetDirectory ();
		for (SelectionSeq seq (_selectionMan, Table::folderTableId); !seq.AtEnd (); seq.Advance ())
		{
			UniqueName uname;
			seq.GetUniqueName (uname);
			std::string curPath = directory.GetProjectPath (uname);
			Win::UnlockPtr unlock (_critSect);
			_model.ShellOpen (curPath.c_str ());
		}
	}
	else if (IsCheckInAreaPane ())
	{
		for (SelectionSeq seq (_selectionMan, Table::checkInTableId); !seq.AtEnd (); seq.Advance ())
		{
			char const * curPath = _model.GetFullPath (seq.GetGlobalId ());
			Win::UnlockPtr unlock (_critSect);
			_model.ShellOpen (curPath);
		}
	}
	else
	{
		Assert (IsSynchAreaPane ());
		for (SelectionSeq seq (_selectionMan, Table::synchTableId); !seq.AtEnd (); seq.Advance ())
		{
			char const * curPath = _model.GetFullPath (seq.GetGlobalId ());
			Win::UnlockPtr unlock (_critSect);
			_model.ShellOpen (curPath);
		}
	}
}

Cmd::Status Commander::can_Selection_OpenWithShell () const
{
	if (IsProjectReady () && (IsFilePane () || IsCheckInAreaPane () || IsSynchAreaPane ()))
	{
		return IsSelection () ? Cmd::Enabled : Cmd::Disabled;
	}
	return Cmd::Invisible;
}

void Commander::Selection_SearchWithShell ()
{
	// Search folder using Windows Shell
	Directory & directory = _model.GetDirectory ();
	for (SelectionSeq seq (_selectionMan, Table::folderTableId); !seq.AtEnd (); seq.Advance ())
	{
		UniqueName uname;
		seq.GetUniqueName (uname);
		std::string curPath = directory.GetProjectPath (uname);
		Win::UnlockPtr unlock (_critSect);
		_model.ShellSearch (curPath.c_str ());
	}
}

Cmd::Status Commander::can_Selection_SearchWithShell () const
{
	if (IsProjectReady () && IsFilePane ())
	{
		return IsSelection () && _selectionMan->FindIfSelected (IsFolder) ? Cmd::Enabled : Cmd::Disabled;
	}
	return Cmd::Invisible;
}

void Commander::Selection_OpenProject ()
{
	if (_model.IsProjectCatalogEmpty ())
	{
		TheOutput.Display ("Select \"New Project\" or \"Join Project\" "
			"from the Project menu to create or join one.");
	}
	else
	{
		SelectionSeq seq (_selectionMan, Table::projectTableId);
		GlobalId projectId = seq.GetGlobalId ();
		VisitProject (projectId);
	}
}

void Commander::Selection_OpenFile ()
{
    try
    {
		Directory & directory = _model.GetDirectory ();
		UniqueName folderName;
		bool openFolder = false;
		bool firstFolderOpened = false;
		// Open files
        for (SelectionSeq seq (_selectionMan, Table::folderTableId); !seq.AtEnd (); seq.Advance ())
        {
			UniqueName uname;
			seq.GetUniqueName (uname);
			std::string curPath = directory.GetProjectPath (uname);
			if (File::IsFolder (curPath.c_str ()))
			{
				if (firstFolderOpened)
				{
					std::string info ("You can open only one folder at a time.\nThe folder '");
					info += uname.GetName ();
					info += "' will be ignored.";
					TheOutput.Display (info.c_str ());
				}
				else if (curPath != directory.GetCurrentPath ())
				{
					folderName.Init (uname);
					openFolder = true;
					firstFolderOpened = true;
				}
			}
			else
			{
				_model.Open (seq.GetGlobalId (), &uname);
			}
        }
		// Open first selected folder -- all other selected folders have been ignored
		if (openFolder)
		{
			directory.Down (folderName.GetName ());
			_displayMan->GoDown (folderName.GetName ());
			AfterDirChange ();
		}
    }
    catch (Win::Exception ex)
    {
        TheOutput.Display (ex);
    }
    catch (...)
    {
		Win::ClearError ();
        TheOutput.Display ("Opening File -- Unknown Error", Out::Error);
    }
}

void Commander::Selection_OpenIncommingScript ()
{
	if (!IsProjectReady ())
	{
		TheOutput.Display ("To execute the full sync script use Selection>Synch Script menu item.");
		return;
	}

	try
    {
        for (SelectionSeq seq (_selectionMan, Table::mailboxTableId); !seq.AtEnd (); seq.Advance ())
        {
			std::unique_ptr<ScriptProps> props = _model.GetScriptProps (seq.GetGlobalId ());
			if (props.get () != 0)
			{
				ScriptPropertyHndlrSet scriptPropsHndlrSet (*props);
				ThePrompter.GetData (scriptPropsHndlrSet);
			}
        }
    }
    catch (Win::Exception ex)
    {
        TheOutput.Display (ex);
    }
    catch (...)
    {
		Win::ClearError ();
        TheOutput.Display ("Opening Incoming Script -- Unknown Error", Out::Error);
    }
}

void Commander::Selection_OpenCheckInDiff ()
{
    try
    {
        for (SelectionSeq seq (_selectionMan, Table::checkInTableId);
             !seq.AtEnd ();
             seq.Advance ())
        {
			_model.Open (seq.GetGlobalId ());
        }
    }
    catch (Win::Exception ex)
    {
        TheOutput.Display (ex);
    }
    catch (...)
    {
		Win::ClearError ();
        TheOutput.Display ("Opening Checkin Diff -- Unknown Error", Out::Error);
    }
}

void Commander::Selection_OpenSynchDiff ()
{
    try
    {
        for (SelectionSeq seq (_selectionMan, Table::synchTableId);
             !seq.AtEnd ();
             seq.Advance ())
        {
			_model.Open (seq.GetGlobalId ());
        }
    }
    catch (Win::Exception ex)
    {
        TheOutput.Display (ex);
    }
    catch (...)
    {
		Win::ClearError ();
        TheOutput.Display ("Opening Synch Diff -- Unknown Error", Out::Error);
    }
}

void Commander::Selection_OpenHistoryScript ()
{
    try
    {
		Restriction const & restriction = _displayMan->GetPresentationRestriction ();
		SelectionSeq seq (_selectionMan, Table::historyTableId);
		std::unique_ptr<ScriptProps> props =
			_model.GetScriptProps (seq.GetGlobalId (), restriction.GetFileFilter ());
		if (props.get () == 0)
        {
			TheOutput.Display ("Cannot display script contents because its format\n"
								"is no longer supported by this version of Code Co-op");
			return;
		}
		bool showScriptProps = true;
		if (props->IsOverdue ())
		{
			// In case user wants to remove overdue members from project
			std::set<UserId> membersToRemove;
			showScriptProps = false; // to be set in the dialog
			OverdueScriptCtrl overdueCtrl (*props, membersToRemove, showScriptProps);
			ThePrompter.GetData (overdueCtrl);
			// Remove selected members
			if (!membersToRemove.empty())
			{
				ProjectMembersData membersData (_model.GetProjectDb (), 
												_model.GetUserPermissions ().GetTrialDaysLeft ());
				for (unsigned idx = 0; idx < membersData.MemberCount(); ++idx)
				{
					MemberInfo const & info = membersData.GetMemberInfo(idx);
					if (membersToRemove.find(info.Id()) != membersToRemove.end())
					{
						MemberState newState = info.State();
						newState.MakeDead();
						membersData.ChangeMemberState(idx, newState);
					}
				}
				bool isRecoveryNeeded = false;
				_model.ProjectMembers (membersData, isRecoveryNeeded);
			}
		}
		if (showScriptProps)
		{
			ScriptPropertyHndlrSet scriptPropsHndlrSet (*props);
			ThePrompter.GetData (scriptPropsHndlrSet);
        }
    }
    catch (Win::Exception ex)
    {
        TheOutput.Display (ex);
    }
    catch (...)
    {
		Win::ClearError ();
        TheOutput.Display ("Opening History Script -- Unknown Error", Out::Error);
    }
}

void Commander::Selection_OpenHistoryDiff ()
{
	bool isMerge = IsMergeDetailsPane ();
	if (_model.RangeHasMissingPredecessors (isMerge))
	{
		std::string info ("Sorry, Code Co-op cannot show the changes to the selected file(s)\n"
			"because the selected script(s) depend on some missing script(s).\n\n"
			"You'll be able to view the changes again once "
			"you've received the missing script(s).");
		TheOutput.Display (info.c_str ());
		return;
	}
	
	SelectionSeq seq (_selectionMan, isMerge ? Table::mergeDetailsTableId : Table::scriptDetailsTableId);
	GidList okFiles;
	CheckFileReconstruction (seq, okFiles);

	if (okFiles.empty ())
		return;

	_model.OpenHistoryDiff (okFiles, isMerge);
}

void Commander::Selection_Add ()
{
	Directory & curFolder = _model.GetDirectory ();
	std::vector<std::string> files;
    for (SelectionSeq seq (_selectionMan, Table::folderTableId); !seq.AtEnd (); seq.Advance ())
	{
		GlobalId gid = seq.GetGlobalId ();
		if (gid != gidInvalid)
			continue;	// Skip already controlled files
		UniqueName uname;
		seq.GetUniqueName (uname);
		std::string path;
		if (curFolder.GetCurrentId () == uname.GetParentId ())
		{
			// Name from the current folder
			path.assign (curFolder.GetProjectPath (uname));
		}
		else
		{
			// Name from some other project folder
			path.assign (_model.GetFullPath (uname));
		}

		files.push_back (path);
		if (!_model.IsQuickVisit () && File::IsFolder (path.c_str ()))
		{
			// Allow the user to select folder contents
			NocaseSet emptyFilter;
			GetFolderContents (path, emptyFilter, files);
		}
	}
	if (!files.empty ())
	{
		FileTyper fileTyper (_hwnd, _model.GetCatalog (), _inputSource);
		bool done = _model.AddFiles (files, fileTyper);
		if (done)
			DisplayTip (IDS_ADDFILE);
	}
}

Cmd::Status Commander::can_Selection_Add () const
{
	if (IsProjectReady () && IsFilePane () )
	{
		if (IsSelection () && CanMakeChanges ())
		{
			return _selectionMan->FindIfSelected (IsStateNone) 
				? Cmd::Enabled : Cmd::Disabled;
		}
		return Cmd::Disabled;
	}
	else if (IsFilePage ())
		return Cmd::Disabled;
	else
		return Cmd::Invisible;
}

void Commander::Selection_Remove ()
{
    SelectionSeq seq (_selectionMan, Table::folderTableId);
	GidList controlledFiles;
	SplitSelection (seq, controlledFiles);
	if (controlledFiles.empty ())
		return;

	if (IsChecksumOK (controlledFiles, false, false)) // Non-recursive during delete
	{
		bool done = _model.DeleteFile (controlledFiles, false);	// Don't delete, just remove from the project
		if (done)
			DisplayTip (IDS_REMOVE);
	}
}

Cmd::Status Commander::can_Selection_Remove () const
{
	if (IsProjectReady () && IsFilePane ()  )
	{
		if (IsSelection () && CanMakeChanges ())
			return _selectionMan->FindIfSelected (IsStateInProject) 
				? Cmd::Enabled : Cmd::Disabled;
		return Cmd::Disabled;
	}
	return Cmd::Invisible;
}

void Commander::Selection_Delete ()
{
	if (IsFilePane ())
	{
		Selection_DeleteFile ();
	}
	else if (IsMailboxPane ())
	{
		Selection_DeleteScript ();
	}
}

Cmd::Status Commander::can_Selection_Delete () const
{
	if (IsProjectReady () && IsFilePage () || IsMailboxPage ())
	{
		if (IsSelection () && CanMakeChanges ())
		{
			if (IsFilePane ())
				return _selectionMan->FindIfSelected (IsProjectRoot)
						? Cmd::Disabled : Cmd::Enabled;
			else if (IsMailboxPane ())
				return _selectionMan->FindIfSelected (CanDeleteScript)
						? Cmd::Enabled : Cmd::Disabled;
			else
				return Cmd::Disabled;

		}
		return Cmd::Disabled;
	}
	return Cmd::Invisible;
}

void Commander::Selection_DeleteFile ()
{
	SelectionSeq seq (_selectionMan, Table::folderTableId);
	GidList controlledFiles;
	std::vector<std::string> notControlledFiles;
	SplitSelection (seq, controlledFiles, &notControlledFiles);
	Directory & directory = _model.GetDirectory ();
	if (!controlledFiles.empty () && IsChecksumOK (controlledFiles, false, false)) // Non recursive during delete
	{
		if (directory.GetCurrentId() == controlledFiles.back())
			Folder_GoUp (); // deleting current folder
		bool done = _model.DeleteFile (controlledFiles);
		if (done)
			DisplayTip (IDS_DELETE);
	}

	if (notControlledFiles.size () != 0)
	{
		if (directory.IsCurrentFolder (notControlledFiles.back()))
			Folder_GoUp (); // deleting current folder
		ShellMan::FileRequest nonProjDelReq;
		nonProjDelReq.MakeItQuiet ();
		for (std::vector<std::string>::const_iterator iter = notControlledFiles.begin ();
			iter != notControlledFiles.end ();
			++iter)
		{
			std::string const & path = *iter;
			nonProjDelReq.AddFile (path);
		}
		nonProjDelReq.DoDelete (TheAppInfo.GetWindow ());
	}
}

void Commander::Selection_DeleteScript ()
{
	SelectionSeq seq (_selectionMan, Table::mailboxTableId);
	_model.DeleteScript (seq);
	if (!_model.IsQuickVisit ())
		_displayMan->Refresh (MailBoxPage);
}

void Commander::Selection_ChangeFileType  ()
{
	FileTyper fileTyper (_hwnd, _model.GetCatalog (), _inputSource);
    SelectionSeq seq (_selectionMan, Table::folderTableId);
	std::set<std::string> extentions;
	GidList controlledFiles;
	for ( ; !seq.AtEnd (); seq.Advance ())
	{
		GlobalId gid = seq.GetGlobalId ();
		if (gid != gidInvalid && ! seq.GetType ().IsFolder ())
		{
			controlledFiles.push_back (gid);
			char const * fileName = seq.GetName ();
        	PathSplitter splitter (fileName);
        	char const * ext = splitter.GetExtension ();
			extentions.insert (ext);
		}
	}
	
	if (controlledFiles.empty ())
		return;

	FileTypeChangeData dlgData (extentions, controlledFiles, _model.GetFileIndex ());
	FileTypeChangerCtrl ctrl (&dlgData);
	if (ThePrompter.GetData (ctrl, _inputSource))
	{
		if (dlgData.IsTypeSelected ())
		{
			FileType newFileType = dlgData.GetType ();
			_model.ChangeFileType (controlledFiles, newFileType);
			if (dlgData.AddAssociation ())
			{
				AssociationList assocDb (_model.GetCatalog ());
				for (std::set<std::string>::const_iterator it = extentions.begin ();
					 it != extentions.end ();
				 	 ++it)
				{
					assocDb.Add (it->c_str (), newFileType);
				}
			}
		}
	}
}

Cmd::Status Commander::can_Selection_ChangeFileType () const
{
	if (IsProjectReady () && IsFilePane ())
	{
		if (IsSelection () && CanMakeChanges ())
		{
			return _selectionMan->FindIfSelected (IsFileInProject) 
				? Cmd::Enabled : Cmd::Disabled;
		}		
		return Cmd::Disabled;
	}
	return Cmd::Invisible;
}

// Command invoked from menu (starts file rename)
void Commander::Selection_Rename ()
{
	SelectionSeq seq (_selectionMan, Table::folderTableId);
	_displayMan->InPlaceEdit (seq.GetRow ());
}

Cmd::Status Commander::can_Selection_Rename () const
{
	if (IsProjectReady () && IsFilePane ())
	{
		if (IsSelection () && CanMakeChanges ())
		{
			return _selectionMan->FindIfSelected (IsFile) 
				? Cmd::Enabled : Cmd::Disabled;
		}
		return Cmd::Disabled;
	}
	return Cmd::Invisible;
}

// Command invoked from notification handler of the item list view (finished file rename)
// Or from command-line:
// -Project_Visit 23 -DoRenameFile NewName:"foo.txt" "c:\projects\co-op50\usertype.dat"
void Commander::DoRenameFile ()
{
    SelectionSeq seq (_selectionMan, Table::folderTableId);
	GidList controlledFiles;
	std::vector<std::string> notControlledFiles;
	SplitSelection (seq, controlledFiles, &notControlledFiles);
	StringExtractor extr ("NewName");
	ThePrompter.GetData (extr, _inputSource);
	Assert (!extr.GetString ().empty ());
	Directory const & directory = _model.GetDirectory ();

	if (!controlledFiles.empty ())
	{
		Assert (directory.GetCurrentId () != gidInvalid);
		Assert (controlledFiles.size () == 1);
		try
		{
			UniqueName newUname (directory.GetCurrentId (), extr.GetString ());
			if (_model.RenameFile (controlledFiles.front (), newUname))
				DisplayTip (IDS_RENAME);
		}
		catch (Win::Exception ex)
		{
			TheOutput.Display (ex);
		}
		catch ( ... )
		{
			Win::ClearError ();
			TheOutput.Display ("Rename File -- Unknown Error", Out::Error); 
		}
	}

	if (!notControlledFiles.empty ())
	{
		Assert (notControlledFiles.size () == 1);
		std::string newName = extr.GetString ();
		std::string const & oldPath = notControlledFiles [0];
		std::string newPath = directory.GetFilePath (newName);
		if (File::Exists (newPath.c_str ()))
		{
			// File system tells us that the target file already exists.
			// Check if the file names differ only in case.
			bool renamingFolder = File::IsFolder (oldPath.c_str ());
			bool newIsFolder = File::IsFolder (newPath.c_str ());
			FileSeq seq (newPath.c_str ());
			std::string fileNameOnDisk (seq.GetName ());
			bool exactMatch = (fileNameOnDisk == newName);
			if (exactMatch || (!renamingFolder && newIsFolder))
			{
				// File names match exactly or they differ only in case but
				// there is already a folder using the newName on disk.
				PathSplitter splitter (oldPath);
				std::string info ("Cannot rename ");
				info += (renamingFolder ? "folder" : "file");
				info += " from '";
				info += splitter.GetFileName ();
				info += splitter.GetExtension ();
				info += "' to '";
				info += newName;
				info += "',\nbecause ";
				info += (newIsFolder ? "folder" : "file");
				info += " with the same name already exists on disk.";
				TheOutput.Display (info.c_str ());
				return;
			}
		}
		ShellMan::FileMove (TheAppInfo.GetWindow (), oldPath.c_str (), newPath.c_str ());
	}
}

void Commander::Selection_MoveFiles()
{
    SelectionSeq seq (_selectionMan, Table::folderTableId);
	std::string currentPath = _model.GetDirectory().GetCurrentPath();
	std::string rootPath = _model.GetDirectory().GetRootPath();
	MoveFilesData data(seq, currentPath, rootPath);
	MoveFilesCtrl ctrl(data);
    if (ThePrompter.GetData (ctrl, _inputSource))
	{
		// Verify target path (assume it's a valid path whose prefix is the root path
		std::string target = data.GetTarget();
		GlobalId gidTarget = _model.GetGlobalId(target);
		if (gidTarget == gidInvalid)
			throw Win::InternalException("Target folder is not in project", target.c_str());
		FileData const * fileData = _model.GetFileIndex().FindByGid(gidTarget);
		if (fileData == 0 || (!fileData->GetType().IsFolder() && !fileData->GetType().IsRoot()))
			throw Win::InternalException("Target is not a folder", target.c_str());

		seq.Reset();
		_model.Cut(seq);

		// Change to target directory
		std::string relativePath;
		if (target.length() > rootPath.length())
			relativePath = target.substr(rootPath.length() + 1);
		_model.ChangeDirectory (relativePath);

		Clipboard sysClipboard (_hwnd);
		sysClipboard.Clear ();
		Selection_Paste();

		// Change display directory
		File::Vpath relativeVpath(relativePath);
		_displayMan->GoTo (relativeVpath);
	}
}

Cmd::Status Commander::can_Selection_MoveFiles() const
{
	if (IsProjectReady() && IsFilePage())
	{
		return IsSelection() && !_selectionMan->FindIfSelected(IsFolder) 
			? Cmd::Enabled : Cmd::Disabled;
	}
	return Cmd::Invisible;
}

void Commander::Selection_Cut ()
{
    SelectionSeq seq (_selectionMan, Table::folderTableId);
    bool done = _model.Cut (seq);
	if (done)
		DisplayTip (IDS_CUT);
}

Cmd::Status Commander::can_Selection_Cut () const
{
	if (IsProjectReady () && IsFilePane ())
	{
		if (IsSelection () && CanMakeChanges ())
			return _selectionMan->FindIfSelected (IsFile) 
				? Cmd::Enabled : Cmd::Disabled;
		return Cmd::Disabled;
	}
	else if (IsFilePage ())
		return Cmd::Disabled;
	else
		return Cmd::Invisible;
}

void Commander::Selection_Copy ()
{
    SelectionSeq seq (_selectionMan, Table::folderTableId);
    bool done = _model.Copy (seq);
	if (done)
		DisplayTip (IDS_COPY);
}

Cmd::Status Commander::can_Selection_Copy () const
{
	if (IsProjectReady () && IsFilePane ())
	{
		if (IsSelection ())
			return _selectionMan->FindIfSelected (IsFile) ? Cmd::Enabled : Cmd::Disabled;

		return Cmd::Disabled;
	}
	else if (IsFilePage ())
		return Cmd::Disabled;
	else
		return Cmd::Invisible;
}

void Commander::Selection_Paste ()
{
	Assert(!_model.IsClipboardEmpty () || Clipboard::IsFormatFileDrop ());

	// Check system clipboard
	Win::FileDropHandle sysDrop;
	if (Clipboard::IsFormatFileDrop ())
	{
		Clipboard sysClipboard (_hwnd);
		Win::FileDropHandle sysClipboardHandle(sysClipboard);
		if (_model.IsClipboardContentsEqual (sysClipboardHandle))
		{
			// Clear system clipboard and use model clipboard
			sysClipboard.Clear ();
		}
		else
		{
			// Use system clipboard
			sysDrop = sysClipboardHandle;
			_model.ClearClipboard ();
		}
	}

	// System clipboard has higher precedence
	if (!sysDrop.IsNull())
	{
		PasteFiles (sysDrop);
		return;
	}

	if (_model.IsClipboardEmpty ())
		return;

	// Paste files from the Code Co-op clipboard
	DirectoryListing nonProjectFiles;
    bool done = _model.Paste (nonProjectFiles);
	if (nonProjectFiles.size () != 0)
	{
		DirectoryListing::Sequencer fileDropSeq (nonProjectFiles);
		if (DoDropFiles (fileDropSeq, true))
		{
			// Non-project files from Code Co-op clipboard were cut at source folder
			// and pasted at target foder. Now after paste has been completed delete files
			// from source folder.
			for (DirectoryListing::Sequencer seq (nonProjectFiles); !seq.AtEnd (); seq.Advance ())
			{
				char const * path = seq.GetFilePath ();
				ShellMan::Delete (_hwnd, path, ShellMan::SilentAllowUndo);
			}
		}
	}
	if (done)
		DisplayTip (IDS_PASTE);
}

Cmd::Status Commander::can_Selection_Paste () const
{
	if (IsProjectReady () && IsFilePane ())
	{
		return _model.IsClipboardEmpty () && !Clipboard::IsFormatFileDrop () ? Cmd::Disabled : Cmd::Enabled;
	}
	else if (IsFilePage ())
		return Cmd::Disabled;
	else
		return Cmd::Invisible;
}

void Commander::Selection_AcceptSynch ()
{
    SelectionSeq seq (_selectionMan, Table::synchTableId);
	DoAcceptSynch (seq);
	DisplayTip (IDS_ACCEPT);
}

Cmd::Status Commander::can_Selection_AcceptSynch () const
{
	if (IsProjectReady () && IsSynchAreaPane ()) 
		return IsSelection () ? Cmd::Enabled : Cmd::Disabled;
	return Cmd::Invisible;		
}

void Commander::Selection_Synch ()
{
	if (_model.HasExecutableJoinRequest ())
		ExecuteJoinRequest ();
	else
		ExecuteAndAcceptScript ();

	if (_model.HasIncomingOrMissingScripts ())
	{
		while (_model.NextIsMilestone())
		{
			// Milestones are executed automatically
			ExecuteAndAcceptScript();
		}
		return;
	}

	if (_model.IsQuickVisit ())
		return;

	_displayMan->Refresh (ProjectPage);
	if (!_model.IsSynchAreaEmpty ())
		View_Synch ();
	else
		View_Default ();
}

Cmd::Status Commander::can_Selection_Synch () const
{
	return can_All_Synch();
}

void Commander::Selection_EditSync ()
{
	bool beginnerHelp;
	ExecuteSetScript (beginnerHelp);
	View_Synch ();
}

Cmd::Status Commander::can_Selection_EditSync () const
{
	if (IsProjectReady () && IsMailboxPage ())
	{
		// In GUI mode execute and edit only set scripts
		if (_model.HasNextScript ())
			return Cmd::Enabled;
		else
			return Cmd::Disabled;
	}
	return Cmd::Invisible;
}

void Commander::Selection_ShowHistory ()
{
	if (IsFilePane ())
	{
		Selection_ShowHistoryByFile (Table::folderTableId);
	}
	if (IsCheckInAreaPane())
	{
		Selection_ShowHistoryByFile (Table::checkInTableId);
	}
	else if (IsHistoryPane ())
	{
		Selection_ShowHistoryByFilter ();
	}
}

void Commander::Selection_ShowHistoryByFile (Table::Id tableId)
{
    SelectionSeq seq (_selectionMan, tableId);
	std::unique_ptr<FileFilter> fileFilter (new FileFilter (seq, _model.GetDirectory ()));
	if (fileFilter->IsFileFilterOn ())
	{
		auto_vector<FileTag> projectFileList;
		for (FileFilter::Sequencer seq (*fileFilter); !seq.AtEnd (); seq.Advance ())
		{
			GlobalId gid = seq.GetGid ();
			std::string path = seq.GetPath ();
			std::unique_ptr<FileTag> file (new FileTag (gid, path.c_str ()));
			projectFileList.push_back (std::move(file));
		}
		unsigned int count = projectFileList.size ();
		_model.ExpandSubtrees (projectFileList);
		for (unsigned int j = count; j < projectFileList.size (); ++j)
		{
			FileTag const * fileTag = projectFileList [j];
			fileFilter->AddFile (fileTag->Gid (), fileTag->Path ());
		}
		_displayMan->SelectPage (HistoryPage);
		_displayMan->SetFileFilter (HistoryPage, std::move(fileFilter));
	}
}

void Commander::Selection_ShowHistoryByFilter ()
{
}

Cmd::Status Commander::can_Selection_ShowHistory () const
{
	if (IsProjectReady () && (IsFilePane () || IsCheckInAreaPane()))
	{
		if (IsSelection ())
		{
			return _selectionMan->FindIfSelected (IsStateInProject) 
				? Cmd::Enabled : Cmd::Disabled;
		}		
		return Cmd::Disabled;
	}
	else if (IsFilePage ())
		return Cmd::Disabled;
	else
		return Cmd::Invisible;
}

void Commander::Selection_Blame ()
{
	SelectionSeq seq (_selectionMan, Table::folderTableId);
	Assert (!seq.AtEnd ());
	GlobalId fileGid = seq.GetGlobalId ();
	std::string wikiReportFilePath  = _model.CreateBlameReport (fileGid);
	if (!wikiReportFilePath.empty ())
	{
		View_Browser ();
		std::string url ("file://");
		url += wikiReportFilePath;
		_displayMan->Navigate (url, 0);
	}
}

Cmd::Status Commander::can_Selection_Blame () const
{
#if 0
	if (IsProjectReady () && IsFilePane ())
	{
		if (_selectionMan->SelCount () == 1)
		{
			return _selectionMan->FindIfSelected (IsStateInProject ()) 
				? Cmd::Enabled : Cmd::Disabled;
		}		
		return Cmd::Disabled;
	}
	else if (IsFilePage ())
		return Cmd::Disabled;
	else
		return Cmd::Invisible;
#else
	return Cmd::Invisible;
#endif
}

void Commander::Selection_SendScript ()
{
	std::vector<MemberInfo> recipients(_model.RetrieveRecipients());
    ScriptRecipientsData dlgData (recipients, _model.GetAdminId ());
    ScriptRecipientsCtrl ctrl (&dlgData);
    if (ThePrompter.GetData (ctrl, _inputSource))
    {
		if (dlgData.IsSelection ())
		{
			if (dlgData.HasScriptId ())
			{
				// from command line
				_model.ReSendScript (dlgData.GetScriptId (), dlgData);
			}
			else
			{
				for (SelectionSeq seq (_selectionMan, Table::historyTableId); 
					!seq.AtEnd (); 
					seq.Advance ())
				{
					_model.ReSendScript (seq.GetGlobalId (), dlgData);
				}
			}
			DisplayTip (IDS_SENDSCRIPT);
		}
    }
}

Cmd::Status Commander::can_Selection_SendScript () const
{
	if (IsProjectReady () && (IsHistoryPage ()))
	{
		if (IsSelection () && IsHistoryPane ())
			return _selectionMan->FindIfSelected (CanSendScript)
			? Cmd::Enabled : Cmd::Disabled;

		return Cmd::Disabled;
	}
	return Cmd::Invisible;
}

void Commander::Selection_RequestResend ()
{
	std::vector<MemberInfo> recipients(_model.RetrieveRecipients());
    ScriptRecipientsData dlgData (recipients, _model.GetAdminId ());
    ScriptRecipientsCtrl ctrl (&dlgData);
    if (ThePrompter.GetData (ctrl, _inputSource))
    {
		if (dlgData.IsSelection ())
		{
			if (dlgData.HasScriptId ())
			{
				// from command line
				_model.RequestResend (dlgData.GetScriptId (), dlgData);
			}
			else
			{
				for (SelectionSeq seq (_selectionMan, Table::mailboxTableId); 
					!seq.AtEnd (); 
					seq.Advance ())
				{
					_model.RequestResend (seq.GetGlobalId (), dlgData);
				}
			}
		}
    }
}

Cmd::Status Commander::can_Selection_RequestResend () const
{
	if (IsProjectReady () && (IsMailboxPage ()))
	{
		if (IsSelection () && IsMailboxPane ())
			return _selectionMan->FindIfSelected (IsMissingScript)
			? Cmd::Enabled : Cmd::Disabled;

		return Cmd::Disabled;
	}
	return Cmd::Invisible;
}

void Commander::Selection_Report ()
{
	DoReport (false);	// Report only selected items
}

Cmd::Status Commander::can_Selection_Report () const
{
	return IsProjectReady () && IsSelection () ? Cmd::Enabled : Cmd::Disabled;
}

// Returns true when the 'Current Version' item selected
bool Commander::GetScriptSelection (GidList & scriptIds) const
{
	bool currentVersionSelected = false;
	if (IsHistoryPage () || IsProjectMergePage ())
	{
		for (SelectionSeq seq (_selectionMan, Table::historyTableId); !seq.AtEnd (); seq.Advance ())
		{
			History::ScriptState state (seq.GetState ());
			if (!state.IsInteresting ())
				continue;	// Skip not interesting scripts

			GlobalId scriptId;
			if (state.IsCurrent ())
			{
				currentVersionSelected = true;
				if (seq.Count () == 1)
					continue;	// Only current version selected - ignore it
				scriptId = _model.GetCurrentVersion ();
			}
			else
			{
				scriptId = seq.GetGlobalId ();
			}
			Assert (scriptId != gidInvalid);
			if (IsProjectMergePage ())
			{
				MergeDetails & mergedFiles = _model.GetMergedFiles ();
				if (state.IsRejected ())
				{
					// Local branch script selected in the project merge view
					if (mergedFiles.IsTargetProjectSet ())
					{
						if (!mergedFiles.IsLocalMerge ())
							continue;	// Target is external project - ignore local branch script
					}
					else
					{
						// For rejected script suggest current project as merge target
						_model.SetMergeTarget (_model.GetProjectId ());
					}
				}
				else
				{
					// Trunk script selected in the project merge view
					if (mergedFiles.IsTargetProjectSet () && mergedFiles.IsLocalMerge ())
					{
						// Current merge target is this project. It is no longer valid
						// merge target for a trunk script.
						mergedFiles.ClearTargetProject ();
					}
				}
			}

			scriptIds.push_back (scriptId);
		}

		if (IsProjectMergePage ())
		{
			MergeDetails & mergedFiles = _model.GetMergedFiles ();
			if (!mergedFiles.IsSelectiveMerge ())
			{
				if (!mergedFiles.IsLocalMerge () && !scriptIds.empty ())
				{
					GlobalId firstBranchId = mergedFiles.GetFirstBranchId ();
					if (firstBranchId != gidInvalid)
						scriptIds.push_back (firstBranchId);
				}
			}
		}
	}
	else
	{
		Assert (IsMailboxPage ());
		for (SelectionSeq seq (_selectionMan, Table::mailboxTableId); !seq.AtEnd (); seq.Advance ())
		{
			Mailbox::ScriptState state (seq.GetState ());
			if (state.IsForThisProject ())
			{
				if (state.IsFromFuture () || state.IsCorrupted () || state.IsIllegalDuplicate ())
					continue;

				if (!state.IsMissing () && !state.IsJoinRequest ())
					scriptIds.push_back (seq.GetGlobalId ());
			}
		}
	}

	return currentVersionSelected;
}

void Commander::GetFileSelection (GidSet & fileIds, GidList const & selectedScripts) const
{
	Restriction const & restriction = _displayMan->GetPresentationRestriction ();
	FileFilter const * fileFilter = restriction.GetFileFilter ();
	if (fileFilter != 0 && fileFilter->IsFileFilterOn ())
	{
		for (FileFilter::Sequencer seq (*fileFilter); !seq.AtEnd (); seq.Advance ())
		{
			GlobalId gid = seq.GetGid ();
			fileIds.insert (gid);
		}
	}

	if (IsProjectMergePage () && fileIds.empty () && !selectedScripts.empty ())
	{
		MergeDetails const & mergedFiles = _model.GetMergedFiles ();
		if (!mergedFiles.IsSelectiveMerge () && !mergedFiles.IsCumulativeMerge ())
		{
			_model.GetFilesChangedByScript (selectedScripts.front (), fileIds);
		}
	}
}

// Returns true when range is cumulative
bool Commander::BuildRange (History::Range & range, GidList &  selectedScriptIds, GidSet & selectedFiles)
{
	bool currentVersionSelected = GetScriptSelection (selectedScriptIds);
	range.SetFromCurrentVersion (currentVersionSelected);
	GetFileSelection (selectedFiles, selectedScriptIds);

	bool cumulativeRange;
	if (IsProjectMergePage ())
	{
		MergeDetails const & mergedFiles = _model.GetMergedFiles ();
		if (mergedFiles.IsTargetProjectSet ())
			cumulativeRange = mergedFiles.IsCumulativeMerge ();
		else
			cumulativeRange = false;
	}
	else
	{
		Assert (IsHistoryPage () || IsMailboxPage ());
		StringExtractor extractor ("Extend");
		ThePrompter.GetData (extractor);
		std::string const & value = extractor.GetString ();
		if (value == "yes")
			cumulativeRange = true;
		else if (value == "no")
			cumulativeRange = false;
		else
		{
			// Preserve range extension
			ScriptDetails const & historicalFiles = _model.GetHistoricalFiles ();
			cumulativeRange = historicalFiles.IsExtendedRange ();
		}
	}

	_model.CreateRange (selectedScriptIds, selectedFiles, cumulativeRange, range);
	return cumulativeRange;
}

void Commander::CreateRange ()
{
	dbg << "--> CreateRange" << std::endl;
	if (!_model.IsProjectReady () || !_model.IsInProject ())
		return;
	// this message may arrive after a page switch
	if (!IsHistoryPage () && !IsMailboxPage () && !IsProjectMergePage ())
		return;

	if (IsProjectMergePage () && !_model.IsSynchAreaEmpty ())
	{
		TheOutput.Display ("You have to first accept changes in the Sync Merge Area,\n"
						   "before continuing with branch merging.");
		_displayMan->SelectPage (SynchAreaPage);
		return;
	}

	GidList selectedScriptIds;
	GidSet selectedFiles;
	History::Range range;
	bool extendedRange = BuildRange (range, selectedScriptIds, selectedFiles);

	if (IsProjectMergePage ())
	{
		bool prepareMerge = false;
		if (_model.UpdateMergedFiles (range, selectedFiles, extendedRange))
		{
			prepareMerge = (selectedScriptIds.size () != 0);
		}
		else
		{
			// Current and new ranges are identical - do we have target data?
			MergeDetails const & mergedFiles = _model.GetMergedFiles ();
			for (GidSet::const_iterator iter = selectedFiles.begin (); iter != selectedFiles.end (); ++iter)
			{
				if (!mergedFiles.HasTargetData (*iter))
				{
					prepareMerge = true;
					break;
				}
			}
		}

		if (prepareMerge)
		{
			std::unique_ptr<Progress::Dialog> meterDialog (CreateProgressDlg ("File Merge"));
			meterDialog->SetCaption ("Preparing project files for the merge operation.");
			_model.PrepareMerge (meterDialog->GetProgressMeter (), _model.GetMergedFiles ());
		}
	}
	else
	{
		Assert (IsHistoryPage () || IsMailboxPage ());
		if (_model.UpdateHistoricalFiles (range, selectedFiles, extendedRange))
		{
			if (IsHistoryPage () &&
				_model.GetHistoricalFiles ().RangeIsAllRejected () &&
				_model.IsSynchAreaEmpty ())
			{
				std::unique_ptr<Progress::Dialog> meterDialog (CreateProgressDlg ("File Merge"));
				meterDialog->SetCaption ("Preparing history branch changes for the merge operation.");
				_model.PrepareMerge (meterDialog->GetProgressMeter (), _model.GetHistoricalFiles ());
			}
		}
	}
	_selectionMan->SetRange (range);
	dbg << "<-- CreateRange" << std::endl;
}

//
// Tools commands
//

void Commander::Tools_Differ ()
{
	ToolOptions::Differ optionsData;
	DifferOptionsDlg differDlg (optionsData);
	if (ThePrompter.GetData (differDlg, _inputSource))
		_model.DifferOptions (optionsData);
}

void Commander::Tools_Merger ()
{
	ToolOptions::Merger optionsData;
	MergerOptionsDlg mergerDlg (optionsData);
	if (ThePrompter.GetData (mergerDlg, _inputSource))
		_model.MergerOptions (optionsData);
}

void Commander::Tools_Editor ()
{
	ToolOptions::Editor optionsData;
	EditorOptionsDlg editorDlg (optionsData);
	if (ThePrompter.GetData (editorDlg, _inputSource))
		_model.EditorOptions (optionsData);
}

void Commander::Tools_CreateBackup ()
{
	dbg << "--> Commander::Tools_CreateBackup" << std::endl;
	if (_model.IsProjectCatalogEmpty ())
	{
		if (!_model.IsQuickVisit ())
		{
			TheOutput.Display ("There are no Code Co-op projects on this computer.\n"
							   "There is no need to create a backup archive.",
							   Out::Information,
							   _hwnd);
		}
		return;
	}

	unsigned projectCount = 0;
	for (ProjectSeq seq (_model.GetCatalog ()); !seq.AtEnd (); seq.Advance ())
		++projectCount;
	
	// Revisit: Two meters???
	std::unique_ptr<Progress::Dialog> blockerMeterDialog =
		CreateProgressDlg ("Code Co-op Backup",
							true,	// User can cancel
							0);		// Show immediately
	blockerMeterDialog->SetCaption ("Waiting for the background Code Co-op to complete its tasks.");
	CoopBlocker blocker (blockerMeterDialog->GetProgressMeter (), _hwnd);
	BackupRequest request (blocker);
	BackupControlHandler dlgCtrlHandler (request);
	if (ThePrompter.HasNamedValues ())
	{
		dbg << "Getting arguments from named values" << std::endl;
		BackupRequestExtractor extractor (request);
		// GetData fills the request and validates it
		if (!ThePrompter.GetData (extractor))
		{
			// Either blocker failed (and displayed error), or data is invalid
			if (!request.IsValid ())
			{
				request.DisplayErrors (_hwnd);
			}
			return;
		}
	}
	else if (_inputSource != 0)
	{
		// Server mode: no user feedback
		dbg << "Getting arguments from input source" << std::endl;
		request.SetQuiet (true);
		if (!ThePrompter.GetData (dlgCtrlHandler, _inputSource))
			return;
	}
	else
	{
		dbg << "Getting arguments from the user" << std::endl;
		if (!ThePrompter.GetData (dlgCtrlHandler, _inputSource))
			return;
	}

	std::unique_ptr<Progress::Dialog> meterDialog = CreateProgressDlg ("Code Co-op Backup");
	meterDialog->SetCaption ("Archiving all Code Co-op projects.");

	Progress::Meter & meter = meterDialog->GetProgressMeter ();
	meter.SetRange (1, 20 * projectCount + 5, 1);
	meter.SetActivity ("Leaving current project.");
	meter.StepAndCheck ();
	LeaveProject ();

	meter.SetActivity ("Creating backup archive.");
	meter.StepAndCheck ();

	SafeTmpFile tmpBackupArchive (request.GetFileName ());
	BackupArchive backupArchive (tmpBackupArchive.GetFilePath ());

	backupArchive.Create (_model.GetCatalog (), meter);

	if (!backupArchive.VerifyLog())
	{
		std::string errString = "Error: Code Co-op database archive was not created.";
		if (backupArchive.SaveLogToDesktop())
		{
			errString += "\n\nThe log file, ";
			errString += BackupArchive::cabArcLogFileName;
			errString += ", has been saved to your desktop.";
		}
		TheOutput.Display (errString.c_str());
		return;
	}

	if (!backupArchive.VerifyNewArchive(meterDialog->GetProgressMeter ()))
	{
		std::string errString = "Error: Archive creation failed.\n\n";
		errString += "The contents of the archive is different than the contenst of the database\n";
		errString += "The list of mismatched files, ";
		errString += BackupArchive::mismatchLogFileName;
		errString += ", has been saved to your desktop.\n\n";
		errString += "The most likely cause is the arrival of a new script during backup.\n";
		errString += "To avoid this, disconnect your machine from the network before starting backup.\n";
		TheOutput.Display(errString.c_str());
		return;
	}
	meterDialog.reset (0);
	blocker.Resume ();

	// We have a backup archive in temporary storage
	// Move it to final destination
	std::string msg;
	if (request.IsStoreOnInternet ())
	{
		tmpBackupArchive.Commit ();	// Don't delete here - FTP applet will delete it
		Ftp::SmartLogin const & login = request.GetFtpLogin ();
		ExecuteFtpUpload (tmpBackupArchive.GetFilePath (),
						  login.GetFolder (),
						  login);
		msg = "The upload of the archived Code Co-op database to:\n   ";
		msg += login.GetServer ();
		msg += "/";
		msg += login.GetFolder ();
		msg += "\nhas been successfully started.\n";
		msg += "It will now continue in the background.";
	}
	else if (request.IsStoreOnLAN ())
	{
		ShellMan::CopyRequest copyRequest;
		if (request.IsOverwriteExisting ())
			copyRequest.OverwriteExisting ();
		FilePath targetPath (request.GetPath ());
		copyRequest.AddCopyRequest (tmpBackupArchive.GetFilePath (),
									targetPath.GetFilePath (request.GetFileName ()));
		copyRequest.DoCopy (_hwnd, "Code Co-op Backup Archive");
		msg = "Archived Code Co-op database was successfully copied to:\n";
		msg += targetPath.GetDir ();
	}
	else
	{
		Assert (request.IsStoreOnMyComputer ());
		FilePath targetPath (request.GetPath ());
		char const * targetFilePath = targetPath.GetFilePath (request.GetFileName ());
		if (request.IsOverwriteExisting ())
			File::Delete (targetFilePath);
		File::Move (tmpBackupArchive.GetFilePath (), targetFilePath);
		msg = "Archive of Code Co-op database was successfully created in:\n";
		msg += targetPath.GetDir ();
	}
	TheOutput.Display (msg.c_str ());
	dbg << "<-- Commander::Tools_CreateBackup" << std::endl;
}

Cmd::Status Commander::can_Tools_CreateBackup () const
{
#if defined (COOP_PRO)
	return Cmd::Enabled;
#else
	return Cmd::Invisible;
#endif
}

void Commander::Tools_RestoreFromBackup ()
{
	if (_model.IsProjectCatalogEmpty ())
	{
		// No projects on this computer - we can restore from backup
		std::unique_ptr<Progress::Dialog> blockerMeterDialog =
			CreateProgressDlg ("Restore From Backup Archive",
							   true,	// User can cancel
							   0);		// Show immediately
		blockerMeterDialog->SetCaption ("Waiting for the background Code Co-op to complete its tasks.");
		CoopBlocker blocker (blockerMeterDialog->GetProgressMeter (), _hwnd);
		RestoreRequest request (blocker);
		RestoreControlHandler dlgCtrlHandler (request, _hwnd);
		if (ThePrompter.GetData (dlgCtrlHandler, _inputSource))
		{
			try
			{
				std::unique_ptr<Progress::Dialog> copyMeterDialog =
					CreateProgressDlg ("Restore From Backup Archive");
				std::string archiveFilePath;
				SafeTmpFile tmpBackupArchive;
				// Copy archive to this computer
				if (request.IsStoreOnInternet () || request.IsStoreOnLAN ())
				{
					copyMeterDialog->SetCaption ("Downloading the backup archive.");
					tmpBackupArchive.SetFileName(request.GetFileName());
					if (request.IsStoreOnInternet ())
					{
						Ftp::SmartLogin const & login = request.GetFtpLogin ();
						Ftp::Site ftpSite (login, login.GetFolder ());
						ftpSite.Download (request.GetFileName (),
										  tmpBackupArchive.GetFilePath (),
										  copyMeterDialog->GetProgressMeter ());
					}
					else
					{
						Assert (request.IsStoreOnLAN ());
						ShellMan::CopyRequest copyRequest;
						if (request.IsOverwriteExisting ())
							copyRequest.OverwriteExisting ();
						FilePath sourcePath (request.GetPath ());
						copyRequest.AddCopyRequest (sourcePath.GetFilePath (request.GetFileName ()),
													tmpBackupArchive.GetFilePath ());
						copyRequest.DoCopy (_hwnd, "Code Co-op Backup Archive");
					}
					archiveFilePath = tmpBackupArchive.GetFilePath ();
				}
				else
				{
					FilePath sourcePath (request.GetPath ());
					archiveFilePath = sourcePath.GetFilePath (request.GetFileName ());
				}
				copyMeterDialog->SetCaption ("Restoring Code Co-op projects from the backup archive.");
				BackupArchive backupArchive (archiveFilePath);
				// Recreate local project databases from the archive
				backupArchive.Extract (Registry::GetCatalogPath (),
									   copyMeterDialog->GetProgressMeter ());
				if (!backupArchive.VerifyLog())
				{
					std::string errString = "Error: Code Co-op database was not recreated from archive.";
					if (backupArchive.SaveLogToDesktop())
					{
						errString += "\n\nThe log file, ";
						errString += BackupArchive::cabArcLogFileName;
						errString += ", has been saved to your desktop.";
					}
					TheOutput.Display (errString.c_str());
					return;
				}
				if (!backupArchive.VerifyRestoreArchive(copyMeterDialog->GetProgressMeter ()))
				{
					std::string errString = "Error: Restore from archive failed.\n\n";
					errString += "The contents of the archive is different than the contenst of the restored database\n";
					errString += "The list of mismatched files, ";
					errString += BackupArchive::mismatchLogFileName;
					errString += ", has been saved to your desktop.\n\n";
					errString += "Please contact support@relisoft.com";
					TheOutput.Display(errString.c_str());
					return;
				}
				// Change Dispatcher registry setting 'ConfigurationState' to 'Restored'
				// because we have only part of configutation data - the part coming from
				// the restored catalog. What is missing are the registry entires for e-mail.
				// When the 'ConfigurationState' is equal to 'Restored' then Dispatcher
				// upon its first run will ask the user to confirm the configuration state.
				Registry::DispatcherUserRoot dispatcher;
				dispatcher.Key ().SetValueString ("ConfigurationState", "Restored");
			}
			catch (Win::InternalException ex)
			{
				if (ex.GetMessageA () == 0)
				{
					TheOutput.Display ("Extracting Code Co-op database from backup was canceled by the user.",
									   Out::Information,
									   _hwnd);
				}
				else
					TheOutput.DisplayException (ex, _hwnd);

				return;
			}
			catch (Win::Exception ex)
			{
				TheOutput.DisplayException (ex, _hwnd);
				return;
			}
			catch ( ... )
			{
				TheOutput.Display ("Unknow exception during extracting Code Co-op database from backup.",
								   Out::Error,
								   _hwnd);
				return;
			}

			if (!DoRecovery (blocker))
			{
				// Restore failed
				throw Win::ExitException ("Not all projects were restored sucessfully.\n"
										  "Code Co-op will exit now.\n\n"
										  "Run Code Co-op again to resume the restore operation.");
			}
		}
	}
	else if (!_model.IsQuickVisit ())
	{
		TheOutput.Display ("Restore is used only after a disk crash or to move a database to a new computer.\n"
						   "This computer already contains Code Co-op projects and the restore cannot proceed.",
						   Out::Information,
						   _hwnd);
	}
}

Cmd::Status Commander::can_Tools_RestoreFromBackup () const
{
#if defined (COOP_PRO)
	return Cmd::Enabled;
#else
	return Cmd::Invisible;
#endif
}

void Commander::Tools_MoveToMachine()
{
	AppHelp::Display(AppHelp::MoveToMachine, "\"how to move Code Co-op to new machine\"", TheAppInfo.GetWindow());
}

//
// Help commands
//

void Commander::Help_Contents ()
{
	AppHelp::Display(AppHelp::ContentsTopic, "help", TheAppInfo.GetWindow());
}

void Commander::Help_Index ()
{
	AppHelp::Display(AppHelp::IndexTopic, "index", TheAppInfo.GetWindow());
}

void Commander::Help_Tutorial ()
{
	AppHelp::Display(AppHelp::TutorialTopic, "tutorial", TheAppInfo.GetWindow());
}

void Commander::Help_Support ()
{
	AppHelp::Display (AppHelp::SupportTopic, "support", TheAppInfo.GetWindow());
}

void Commander::Help_BeginnerMode ()
{
	_coopTips.SetStatus (!_coopTips.IsModeOn ());
}

Cmd::Status Commander::can_Help_BeginnerMode () const
{
	return _coopTips.IsModeOn () ? Cmd::Checked : Cmd::Enabled;
}

void Commander::Help_SaveDiagnostics ()
{
	DiagnosticsRequest request (_model.GetProjectId ());
	DiagnosticsCtrl diagCtrl (request);
	if (ThePrompter.GetData (diagCtrl, _inputSource))
	{
		_model.SaveDiagnostics (request);
	}
}

void Commander::Help_RestoreOriginal ()
{
	std::string uninstallerPath = TheAppInfo.GetUninstallerPath ();
	if (!File::Exists (uninstallerPath.c_str ()))
		throw Win::Exception ("The Code Co-op installation is corrupted.\n"
							  "The Code Co-op Uninstaller could not be found.\n"
							  "Please run the Code Co-op Installer to repair your installation.");

	std::string cmd = "\"" + uninstallerPath + "\" -restore";
	Win::ChildProcess uninstaller (cmd);
	Win::ClearError ();
	if (!uninstaller.Create (0))
		throw Win::Exception ("Failed to run the Code Co-op Uninstaller.");
}

Cmd::Status Commander::can_Help_RestoreOriginal () const
{
	if (TheAppInfo.IsTemporaryUpdate ())
		return Cmd::Enabled;
	else
		return Cmd::Invisible;
}

//
// Command test helpers
//

bool Commander::IsProjectReady () const
{
	return _model.IsProjectReady ();
}                  

bool Commander::CanRevert (GlobalId selectedVersion) const
{
	VersionInfo info;
	if (!_model.RetrieveVersionInfo (selectedVersion, info))
	{
		TheOutput.Display ("Cannot revert this project version, because it has a very old format\r\n"
						   "no longer supported by Code Co-op");
		return false;
	}
	if (!_model.IsSynchAreaEmpty ())
	{
		TheOutput.Display ("Synch Area is not empty.  Complete previous synchronization script\n"
						   "before restoring earlier version of the project.");
		return false;
	}
	if (_model.HasToBeRejectedScripts ())
	{
		TheOutput.Display ("You have incoming scripts in your inbox.\n"
						   "You have to execute incoming scripts,\n"
						   "before restoring earlier version of the project.");
		return false;
	}
	return true;
}

bool Commander::CanMakeChanges () const
{
	Permissions const & myPermissions = _model.GetUserPermissions ();
	return !myPermissions.IsReceiver ();
}

bool Commander::CanCheckin () const
{
	bool blockedCheckin = false;
	bool canCheckin = _model.CanCheckIn (blockedCheckin);
	if (canCheckin)
		return !IsInConflictWithMailbox ();

	if (_model.IsQuickVisit ())
	{
		if (blockedCheckin)
			throw Win::InternalException ("Project waiting for the verification report -- check-in not available.");
		if (CanMakeChanges ())
			throw Win::InternalException ("Project observer -- check-in not available.");
		else
			throw Win::InternalException ("Project receiver -- check-in not available.");
	}
	else if (blockedCheckin)
	{
		std::string msg = _model.GetCheckinAreaCaption ();
		msg += "\n\nTo request the verification report again execute Project>Request Verification.";
		TheOutput.Display (msg.c_str ());
	}

	return false;
}

bool Commander::IsSelection () const
{
	return _selectionMan->SelCount () != 0;
}

bool Commander::IsSingleExecutedScriptSelected () const
{
	if (IsSelection () && IsHistoryPane ())
	{
		return _selectionMan->SelCount () == 1 && !_selectionMan->FindIfSelected (IsRejected);
	}
	return false;
}

bool Commander::IsSingleHistoricalExecutedScriptSelected () const
{
	if (IsSelection () && IsHistoryPane ())
	{
		return _selectionMan->SelCount () == 1 &&
			  !_selectionMan->FindIfSelected (IsRejected) &&
			  !_selectionMan->FindIfSelected (IsCurrent);
	}
	return false;
}

bool Commander::IsProjectPane () const
{
	return _displayMan->GetCurrentTableId () == Table::projectTableId;
}

bool Commander::IsProjectPage () const
{
	return _displayMan->GetCurrentPage () == ProjectPage;
}

bool Commander::IsThisProjectSelected () const
{
	if (IsProjectReady ())
	{
		// Visiting project
		if (IsProjectPane ())
		{
			// In the project view check if selected item represents current project,
			// so the user is not confused about the project he/she is working with.
			IsThisProject isCurrentProject (_model.GetProjectId ());
			return _selectionMan->SelCount () == 1 &&
				   _selectionMan->FindIfSelected (isCurrentProject); 
		}
		return true;
	}
	return false;
}

bool Commander::IsFilePane () const
{
	return _displayMan->GetCurrentTableId () == Table::folderTableId;
}

bool Commander::IsFilePage () const
{
	return _displayMan->GetCurrentPage () == FilesPage;
}

bool Commander::IsFilterOn () const
{
	Restriction const & restriction = _displayMan->GetPresentationRestriction ();
	return restriction.IsOn ("FileFiltering");
}

bool Commander::IsMailboxPane () const
{
	return _displayMan->GetCurrentTableId () == Table::mailboxTableId;
}

bool Commander::IsMailboxPage () const
{
	return _displayMan->GetCurrentPage () == MailBoxPage;
}

bool Commander::IsCheckInAreaPane () const
{
	return _displayMan->GetCurrentTableId () == Table::checkInTableId;
}

bool Commander::IsCheckInAreaPage () const
{
	return _displayMan->GetCurrentPage () == CheckInAreaPage;
}

bool Commander::IsSynchAreaPane () const
{
	return _displayMan->GetCurrentTableId () == Table::synchTableId;
}

bool Commander::IsSynchAreaPage () const
{
	return _displayMan->GetCurrentPage () == SynchAreaPage;
}

bool Commander::IsHistoryPane () const
{
	return _displayMan->GetCurrentTableId () == Table::historyTableId;
}

bool Commander::IsHistoryPage () const
{
	return _displayMan->GetCurrentPage () == HistoryPage;
}

bool Commander::IsProjectMergePage () const
{
	return _displayMan->GetCurrentPage () == ProjectMergePage;
}

bool Commander::IsMergeDetailsPane () const
{
	return _displayMan->GetCurrentTableId () == Table::mergeDetailsTableId;
}

bool Commander::IsScriptDetailsPane () const
{
	return _displayMan->GetCurrentTableId () == Table::scriptDetailsTableId;
}

bool Commander::IsBrowserPane () const
{
	return _displayMan->GetCurrentTableId () == Table::WikiDirectoryId;
}

//------------

void Commander::RestoreLastProject ()
{
	if (_model.IsQuickVisit ())
		return;

	// Restore recent project
	std::string sourcePath = Registry::GetRecentProject ();
	bool visitAnotherProject = sourcePath.empty ();
	if (visitAnotherProject)
	{
		// No recent project
		DisplayTip (IDS_NOPROJECT);
	}
	else
	{
		// Try to visit recent project
		visitAnotherProject = !VisitProject (sourcePath);
	}
	if (visitAnotherProject)
	{
		// There's no recent project, or recent project cannot be visited -- let the user choose another project
		View_Projects ();
		SetTitle ();
	}
	_rememberRecentProject = true;
}

bool Commander::DoRecovery (CoopBlocker & coopBlocker)
{
	try
	{
		// Recreate local project inboxes if necessary
		for (ProjectSeq seq (_model.GetCatalog ()); !seq.AtEnd (); seq.Advance ())
		{
			FilePath projectInboxPath = seq.GetProjectInboxPath ();
			if (File::Exists (projectInboxPath.GetDir ()))
				continue;

			File::CreateFolder (projectInboxPath.GetDir ());
		}

		std::unique_ptr<Progress::Dialog> meterDialog (CreateProgressDlg ("Restore From Backup"));
		meterDialog->SetCaption ("Restoring Code Co-op projects. \nThis may take a (very) long time.");

		BackupRestorer backupRestorer (_model);

		backupRestorer.RestoreRootPaths (meterDialog->GetProgressMeter ());
		backupRestorer.RequestVerification (meterDialog->GetProgressMeter ());
		coopBlocker.Resume ();

		backupRestorer.RepairProjects (meterDialog->GetProgressMeter ());
		meterDialog.reset ();
		backupRestorer.Summarize ();
	}
	catch (Win::Exception ex)
	{
		std::string exMsg = Out::Sink::FormatExceptionMsg (ex);
		if (exMsg.empty ())
		{
			// User canceled restore from backup
			TheOutput.Display ("Restoring Code Co-op project from the backup was canceled by the user.\n"
							   "Code Co-op will automatically resume project restore when started again.",
							   Out::Information,
							   _hwnd);
		}
		else
		{
			// Fatal exception - suggest repeating Tools>Restore From Backup
			// Problems:
			//	1. As long as backup marker exists the user will never have the chance to
			//     execute Tools>Restore From Backup
			//	2. If we delete backup marker then Tools>Restore From Backup will tell the user
			//     that catalog is not empty
			// So, should we clear the co-op\Database folder?
			std::string info ("Restoring Code Co-op projects from backup failed.\n\n");
			info += exMsg;
			info += "\n\nPlease, try executing Tools>Restore From Backup again.";
			TheOutput.Display (info.c_str (), Out::Error, _hwnd);
		}
		return false;
	}
	return true;
}

void Commander::RepairProjects ()
{
	Out::Answer userChoice = TheOutput.Prompt ("After restoring Code Co-op projects from the backup archive\n"
											   "there are still some un-repaired projects.\n\n"
											   "Do you want to repair them now?",
											   Out::PromptStyle (Out::YesNo, Out::Yes, Out::Question),
											   _hwnd);
	if (userChoice == Out::No)
		return;

	try
	{
		std::unique_ptr<Progress::Dialog> meterDialog (CreateProgressDlg ("Restore From The Backup Archive"));
		meterDialog->SetCaption ("Repairing Code Co-op projects.");
		BackupRestorer backupRestorer (_model);

		backupRestorer.RepairProjects (meterDialog->GetProgressMeter ());
		meterDialog.reset ();
		backupRestorer.Summarize ();
		// Make sure that repair list really is removed from disk.
		RepairList repairList;
		repairList.Delete ();
	}
	catch (Win::Exception ex)
	{
		std::string exMsg = Out::Sink::FormatExceptionMsg (ex);
		if (exMsg.empty ())
		{
			// User canceled restore from backup
			TheOutput.Display ("Repairing Code Co-op projects was canceled by the user.\n"
							   "Code Co-op will automatically resume project repair when started again.",
							   Out::Information,
							   _hwnd);
		}
		else
		{
			std::string info ("Repairing Code Co-op projects failed.\n\n");
			info += exMsg;
			TheOutput.Display (info.c_str (), Out::Error, _hwnd);
		}
	}
}

void Commander::RefreshProject (bool force)
{
	Assert (_model.IsInProject ());
	// Initialize Commander to function properly in the project
	_model.VerifyLegalStatus ();
	if (!force && _model.IsQuickVisit ())
		return;

	// Refresh mailbox from disk and process any ACK scripts if there are any.
	// Also if auto-sync is on, process incoming scripts.
	// Do all of the above only when this is not a quick visit.
	DoRefreshMailbox (false);
	if (_model.IsQuickVisit ())
	{
		// Forced project entry in command line mode
		Assert (force);
		return;
	}

	_displayMan->RefreshAll ();
	WikiDirectory & wikiDir = _model.GetWikiDirectory ();
	bool isWiki = _model.IsWikiFolder ();
	if (isWiki)
		wikiDir.AfterDirChange (_model.GetDirectory ().GetCurrentPath ());
	else
		wikiDir.ExitWiki ();

	_displayMan->Rebuild (isWiki);
	if (!_model.IsSynchAreaEmpty ())
		View_Synch ();
	else if (_model.HasIncomingOrMissingScripts ())
		View_Mailbox ();
	else
	{
		View_Default ();
		VerifyRecordSet ();
	}
}

// Clear volatile data structures

void Commander::LeaveProject ()
{
	_model.LeaveProject ();
	SetTitle ();

	if (!_model.IsQuickVisit ())
	{
		_displayMan->ClearAll (true); // for good
		_displayMan->RefreshAll ();
		View_Projects ();
		_rememberRecentProject = true;
	}
}

// Undo interface

std::unique_ptr<Memento> Commander::CreateMemento ()
{
	return _model.CreateMemento ();
}

void Commander::RevertTo (Memento const & memento)
{
	// Set NOT IN PROJECT state
	LeaveProject ();
	if (_model.RevertTo (memento))
	{
		SetTitle ();
		RefreshProject ();
	}
}

// Commands triggered by controls other then menu and toolbar
void Commander::ChangeFilter ()
{
	// Retrieve history filter string typed by the user
	StringExtractor filterExtractor ("Filter");
	ThePrompter.GetData (filterExtractor);
	std::string newFilter (filterExtractor.GetString ());
	ThePrompter.ClearNamedValues ();

	std::unique_ptr<FileFilter> filter (new FileFilter);
	filter->SetFilterPattern (newFilter);
	GlobalId selectedScriptId = gidInvalid;

	// Parse history filter and build new file filter
	if (newFilter [0] == ':')
	{
		// Select all scripts containig selected keyword in the script comment
		GidList scripts;
		auto_vector<FileTag> projectFiles;
		_model.FindAllByComment (newFilter.substr (1), scripts, projectFiles);
		filter->AddScripts (scripts);
		filter->AddFiles (projectFiles);
	}
	else
	{
		GidList selectedFileIds;
		std::vector<std::string> selectedFileNames;

		History::FilterScanner scanner (newFilter);
		History::FilterScanner::Token currentToken = scanner.Look ();

		while (currentToken != History::FilterScanner::tokenEnd &&
			   currentToken != History::FilterScanner::tokenStarDotStar)
		{
			if (currentToken == History::FilterScanner::tokenFileId)
			{
				GlobalIdPack pack (scanner.GetTokenString ());
				GlobalId gid = pack;
				selectedFileIds.push_back (gid);
			}
			else if (currentToken == History::FilterScanner::tokenScriptId)
			{
				GlobalIdPack pack (scanner.GetTokenString ());
				GlobalId gid = pack;
				selectedScriptId = gid;
			}
			else
			{
				Assert (currentToken == History::FilterScanner::tokenName);
				std::string filePath = scanner.GetTokenString ();
				if (FilePath::IsValidPattern (filePath.c_str ()))
				{
					selectedFileNames.push_back (filePath);
				}
				else
				{
					std::string msg ("Invalid file name '");
					msg += filePath;
					msg += "'.\r\nFile has been ignored in the search.";
					TheOutput.Display (msg.c_str ());
				}
			}
			scanner.Accept ();
			currentToken = scanner.Look ();
		}

		// Done scannig history filter string - interpret result
		if (selectedScriptId == gidInvalid && selectedFileIds.size () != 0)
		{
			// Select all files having selected gids
			auto_vector<FileTag> projectFiles;
			_model.FindAllByGid (selectedFileIds, projectFiles);
			filter->AddFiles (projectFiles);
		}
		else if (selectedFileNames.size () != 0)
		{
			// Select all project files matching the selected file names
			for (std::vector<std::string>::const_iterator iter = selectedFileNames.begin ();
				 iter != selectedFileNames.end ();
				 ++iter)
			{
				// Searching project files by name may return many hits
				auto_vector<FileTag> projectFiles;
				std::string const & filePath = *iter;
				_model.FindAllByName (filePath, projectFiles);
				_model.ExpandSubtrees (projectFiles);
				if (projectFiles.size () > 0)
				{
					filter->AddFiles (projectFiles);
				}
				else
				{
					std::string msg ("File '");
					msg += filePath;
					msg += "' not found in the project.\r\nFile has been ignored in the search.";
					TheOutput.Display (msg.c_str ());
				}
			}
		}
	}

	_displayMan->SetFileFilter (_displayMan->GetCurrentPage (), std::move(filter));
	ThePrompter.SetNamedValue ("Extend", "preserve");
	CreateRange ();
	if (selectedScriptId != gidInvalid)
	{
		GidList scripts;
		scripts.push_back(selectedScriptId);
		_selectionMan->SelectIds (scripts, _displayMan->GetMainTableId());
	}
	ThePrompter.ClearNamedValues ();
}

void Commander::CleanupCatalog (Project::Data & projData)
{
	if (_model.IsQuickVisit ())
		return;
	// Give the user opportunity to delete from the catalog project that cannot be visited
	std::string info ("Code Co-op cannot visit the following project:\n\n'");
	info += projData.GetProjectName ();
	info += " (";
	info += projData.GetRootDir ();
	info += ")'\n\nDo you want to remove this project from the catalog?";
	Out::Answer userChoice = TheOutput.Prompt (info.c_str (),
											   Out::PromptStyle (Out::YesNo, Out::No, Out::Error),
											   _hwnd);
	if (userChoice == Out::Yes)
	{
		std::string info ("Removing the following project from the catalog:\n\n'");
		info += projData.GetProjectName ();
		info += " (";
		info += projData.GetRootDir ();
		info += ")'\n\nwill make it unaccessible in the future. Do you want to continue ?";
		Out::Answer areYouSure = TheOutput.Prompt (info.c_str (),
												   Out::PromptStyle (Out::YesNo, Out::No, Out::Warning),
												   _hwnd);
		if (areYouSure == Out::Yes)
		{
			_model.ForceDefect (projData);
			_displayMan->Refresh (ProjectPage);
		}
	}
}

void Commander::SetTitle ()
{
	if (_hwnd.IsNull ())
		return;
	if (_model.IsInProject ())
	{
		std::string className;
		long projId = _model.GetProjectId ();
		_hwnd.SetProperty ("Project ID", projId);

		if (!_model.IsQuickVisit ())
		{
			CoopCaption caption (_model.GetProjectName (), _model.GetProjectDir ());
			_hwnd.SetText (caption.str ().c_str ());
		}
	}
	else
	{
		long projId;
		_hwnd.RemoveProperty ("Project ID", projId);

		if (!_model.IsQuickVisit ())
			_hwnd.SetText (ApplicationName);
	}
}

void Commander::SplitSelection (WindowSeq & seq,
								GidList & controlledFiles,
								std::vector<std::string> * notControlledFiles) const
{
	// Split selected files into controlled and not-controlled groups
	for (; !seq.AtEnd (); seq.Advance ())
	{
		if (seq.GetGlobalId () != gidInvalid)
			controlledFiles.push_back (seq.GetGlobalId ());
		else if (notControlledFiles != 0)
		{
			UniqueName uname;
			seq.GetUniqueName (uname);
			Directory const & directory = _model.GetDirectory ();
			notControlledFiles->push_back (directory.GetProjectPath (uname));
		}
	}
}

// Returns true when no merge conflict detected
bool Commander::CheckFileReconstruction (WindowSeq & seq, GidList & okFiles) const
{
	GidList badFiles;
	for ( ; !seq.AtEnd (); seq.Advance ())
	{
		GlobalId fileGid = seq.GetGlobalId ();
		if (_model.CanRestoreFile (fileGid))
			okFiles.push_back (fileGid);
		else
			badFiles.push_back (fileGid);
	}

	if (!badFiles.empty ())
	{
		std::string info ("Sorry, Code Co-op cannot recreate the appropriate version of the following ");
		if (badFiles.size () == 1)
			info += "file:";
		else
			info += "files:";
		info += "\n\n";
		unsigned count = 0;
		PathFinder const & pathFinder = _model.GetPathFinder ();
		for (GidList::const_iterator iter = badFiles.begin (); iter != badFiles.end (); ++iter)
		{
			if (count > 4)
			{
				info += " ... ";
				break;
			}
			info += pathFinder.GetRootRelativePath (*iter);
			info += '\n';
			++count;
		}
		info += "\n\nbecause of a pending merge.\n\n";
		info += "You'll be able to save the files after you've resolved and accepted the merge.";
		TheOutput.Display (info.c_str ());
	}
	return badFiles.empty ();
}

void Commander::DoSaveFileVersion (WindowSeq & seq)
{
	bool isMerge = IsProjectMergePage ();
	if (_model.RangeHasMissingPredecessors (isMerge))
	{
		std::string info ("Sorry, Code Co-op cannot save selected file(s)\n"
			"because the selected script(s) depend on some missing script(s).\n\n"
			"You'll be able to save file(s) once "
			"you've received the missing script(s).");
		TheOutput.Display (info.c_str ());
		return;
	}

	GidList files;
	if (!CheckFileReconstruction (seq, files))
		return;	// Merge conflict detected

	Assert (!files.empty ());
	SaveFilesData dlgData;
	SaveFilesCtrl ctrl (dlgData);
	if (ThePrompter.GetData (ctrl))
	{
		FileCopyRequest const & copyRequest = dlgData.GetFileCopyRequest ();
		FileCopyist copyist (copyRequest);
		_model.BuildHistoricalFileCopyist (copyist,
										   files,
										   copyRequest.IsUseFolderNames (),
										   !copyRequest.IsInternet (),
										   dlgData.IsSaveAfterChange (),
										   isMerge);
		if (copyist.HasFilesToCopy ())
		{
			if (copyRequest.IsInternet ())
			{
				Ftp::SmartLogin const & login = copyRequest.GetFtpLogin ();
				copyist.DoUpload (login, _model.GetProjectName (), login.GetFolder ());
			}
			else
			{
				Assert (copyRequest.IsLAN () || copyRequest.IsMyComputer ());
				copyist.DoCopy ();
			}
		}
		else if (!dlgData.IsPerformDeletes ())
		{
			// No files to copy and the user didn't request deletions at target.
			// Display message.
			if (files.size () == 1)
			{
				Restorer & restorer = isMerge ? _model.GetMergedFiles ().GetRestorer (files.back ()) :
												_model.GetHistoricalFiles ().GetRestorer (files.back ());
				std::string info ("There is nothing to save, because the following file:\n\n");
				info += restorer.GetRootRelativePath ();
				info += "\n\ndoes not exist in the version ";
				info += (dlgData.IsSaveAfterChange () ? "\"after\"" : "\"before\"");
				info += ".";
				TheOutput.Display (info.c_str ());
			}
			else
			{
				std::string info ("There is nothing to save, because the selected files did not exist in the version ");
				info += (dlgData.IsSaveAfterChange () ? "\"after\"" : "\"before\"");
				info += ".";
				TheOutput.Display (info.c_str ());
			}
		}

		if (dlgData.IsPerformDeletes ())
		{
			// Perform requested deletes
			ShellMan::FileRequest deleteRequest;
			FilePath targetRoot (copyRequest.GetTargetFolder ());
			_model.MakeHistoricalFileDeleteRequest (files,
													isMerge,
													dlgData.IsSaveAfterChange (),
													copyRequest.IsUseFolderNames (),
													targetRoot,
													deleteRequest);
			if (copyRequest.IsInternet ())
			{
				Ftp::SmartLogin const & login = copyRequest.GetFtpLogin ();
				Ftp::Site ftpSite (login, login.GetFolder ());
				ftpSite.DeleteFiles (deleteRequest);
			}
			else
			{
				Assert (copyRequest.IsLAN () || copyRequest.IsMyComputer ());
				deleteRequest.DoDelete (_hwnd, true);	// Quiet
				std::string const & path = copyRequest.GetTargetFolder ();
				if (File::CleanupTree (path.c_str ()))
					ShellMan::QuietDelete (_hwnd, path.c_str ());
			}
		}
	}
}

void Commander::SaveState ()
{
	if (_rememberRecentProject)
		Registry::SetRecentProject (_model.GetProjectDir ());

	_model.UpdateRecentProjects ();
}

void Commander::GetFileState (FileStateList & list)
{
	_model.GetState (list);
}

void Commander::GetVersionId (FileStateList & list, bool isCurrent) const
{
	_model.GetVersionId (list, isCurrent);
}

void Commander::GetVersionDescription (GlobalId versionGid, std::string & versionDescr) const
{
	versionDescr = _model.GetVersionDescription (versionGid);
}

void Commander::DoBranch (GlobalId scriptId)
{
	bool canDisplay = !_model.IsQuickVisit ();
	Permissions const & userPermissions = _model.GetUserPermissions ();
	VersionInfo versionInfo;
	if (userPermissions.CanCreateNewBranch ())
	{
		if (!_model.RetrieveVersionInfo (scriptId, versionInfo))
		{
			if (canDisplay)
			{
				std::string info ("Cannot branch project version after the changes of the script ");
				info += GlobalIdPack (scriptId).ToString ();
				info += ",\nbecause the script format is very old format and no longer supported by Code Co-op.";
				TheOutput.Display (info.c_str ());
			}
			return;
		}
	}
	else if (userPermissions.IsReceiver ())
	{
		if (canDisplay)
			TheOutput.Display ("You don't have permission to branch this project.");
		return;
	}
	else
	{
		LicensePrompter prompter ("Selection_Branch");
		if (!_model.PromptForLicense (prompter))
			return;	// No valid license provided
	}

	if (!_model.IsSynchAreaEmpty ())
	{
		if (canDisplay)
		{
			TheOutput.Display ("Synch Area is not empty.  Complete previous synchronization script\n"
							   "before creating project branch.");
			return;
		}
		else
		{
			Win::ClearError ();
			throw Win::Exception ("Synch Area is not empty.  Complete previous synchronization script\n"
								  "before creating project branch.");
		}
	}

	Project::Db const & projectDb = _model.GetProjectDb ();
	MemberState myState = projectDb.GetMemberState (projectDb.GetMyId ());
	std::unique_ptr<MemberDescription> myDescription =
		projectDb.RetrieveMemberDescription (projectDb.GetMyId ());
	BranchProjectData newBranchData (versionInfo,
									 *myDescription,
									 _model.GetCatalog (),
									 myState.IsReceiver ());
	ProjectBranchHndlrSet newBranchHndlrSet (newBranchData);
	if (ThePrompter.GetData (newBranchHndlrSet, _inputSource))
	{
		if (!CheckAndRepairProjectIfNecessary ("Project Branch"))
		{
			if (canDisplay)
			{
				TheOutput.Display ("Branch cannot be created until project is successfully repaired.");
				return;
			}
			else
			{
				Win::ClearError ();
				throw Win::Exception ("Branch cannot be created until project is successfully repaired.");
			}
		}

		Project::Data & project = newBranchData.GetProject ();
		std::string branchDir (project.GetRootDir ());
		bool rootDirExisted = File::Exists (branchDir.c_str ());

		{
			std::unique_ptr<Progress::Dialog> meterDialog (CreateProgressDlg ("Project Branch"));
			meterDialog->SetCaption ("Preparing project files for the branch operation.");
			NewProjTransaction projX (_model.GetCatalog (), project);
			_model.NewBranch (newBranchData, meterDialog->GetProgressMeter (), projX);
			projX.Commit ();
		}

		// Visit the new project branch
		VisitProject (branchDir);
		_displayMan->Refresh (ProjectPage);
	}
}

void Commander::DoExport (bool isCmdLine, GlobalId scriptId)
{
	bool const canDisplay = !isCmdLine;

	if (!_model.IsSynchAreaEmpty ())
	{
		if (canDisplay)
		{
			TheOutput.Display ("Synch Area is not empty.  Complete previous synchronization script\n"
							   "before exporting project files.");
			return;
		}
		else
		{
			throw Win::InternalException (  "Synch Area is not empty.\n "
											"Complete previous synchronization script\n"
											"before exporting project files.");
		}
	}

	VersionInfo versionInfo;
	if (!isCmdLine)
	{
		// scriptId is valid, get version info
		if (!_model.RetrieveVersionInfo (scriptId, versionInfo))
		{
			std::string info ("Cannot export project version after the changes of the script ");
			info += GlobalIdPack (scriptId).ToString ();
			info += ",\nbecause the script format is very old format and no longer supported by Code Co-op.";
			TheOutput.Display (info.c_str ());
			return;
		}
	}

	// In cmd line, the prompter will retrieve script ID
	ExportProjectData dlgData (versionInfo, _model.GetCatalog ());
	ExportProjectCtrl ctrl (dlgData);
	if (ThePrompter.GetData (ctrl, _inputSource))
	{
		Assert (isCmdLine || scriptId == dlgData.GetVersionId ());
		if (!CheckAndRepairProjectIfNecessary ("Project Export"))
		{
			if (canDisplay)
			{
				TheOutput.Display ("Project cannot be exported until it is successfully repaired.");
				return;
			}
			else
			{
				Win::ClearError ();
				throw Win::Exception ("Project cannot be exported until it is successfully repaired.");
			}
		}
		// Export project files
		FileCopyRequest const & copyRequest = dlgData.GetFileCopyRequest ();
		FileCopyist copyist (copyRequest);
		TmpProjectArea tmpArea;
		{
			std::unique_ptr<Progress::Dialog> meterDialog (CreateProgressDlg ("Project Export"));
			meterDialog->SetCaption ("Exporting project files.");
			// Long operation
			_model.BuildProjectFileCopyist (copyist,
											dlgData.GetVersionId (),
											tmpArea,
											dlgData.IsIncludeLocalEdits (),
											meterDialog->GetProgressMeter ());
		}
		if (copyist.HasFilesToCopy ())
		{
			if (copyRequest.IsInternet ())
			{
				Ftp::SmartLogin const & login = copyRequest.GetFtpLogin ();
				copyist.DoUpload (login, _model.GetProjectName (), login.GetFolder ());
			}
			else
			{
				Assert (copyRequest.IsLAN () || copyRequest.IsMyComputer ());
				copyist.DoCopy ();
			}
		}
		tmpArea.Cleanup (_model.GetPathFinder ());
	}
}

void Commander::DoReport (bool reportAll) const
{
	Table::Id tableId = Table::emptyTableId;
	if (!_model.IsQuickVisit ())
		tableId = _displayMan->GetCurrentTableId ();

	ReportRequest dlgData (_model.GetProjectId (), tableId);
	ReportCtrl dlgCtrl (dlgData);
	if (ThePrompter.GetData (dlgCtrl, _inputSource))
	{
		tableId = dlgData.GetTableId ();

		OutStream report (dlgData.GetTargetFilePath ());
		_selectionMan->DumpRecordSet (report, tableId, reportAll);
#if !defined (NDEBUG)
		if (tableId == Table::mailboxTableId)
		{
			// Report in mailbox view is for problem reporting.
			// Add lineages used to order incoming scripts
			_model.DumpMailbox (report);
		}
#endif
	}
}

void Commander::DoCheckout (GidList & files, bool includeFolders, bool recursive)
{
	bool done = _model.CheckOut (files, includeFolders, recursive);
	if (done)
		DisplayTip (IDS_CHECKOUT);
}

bool Commander::IsChecksumOK (GidList const & files, bool recursive, bool isCheckout)
{
	std::unique_ptr<VerificationReport> report = _model.VerifyChecksum (files, recursive);
	if (report->IsEmpty ())
		return true; // No checksum mismatch detected -- go ahead

	// Checksum mismatch detected
	if (_model.IsQuickVisit ())
	{
		// During background operation
		throw Win::Exception ("Checksum mismatch: Run Repair from the Code Co-op Project menu.");
	}

	// Inform the user about his options
	Project::Path relativePath (_model.GetFileIndex ());
	ChecksumMismatchData dlgData (*report, relativePath, isCheckout);
	ChecksumMismatchCtrl ctrl (dlgData);
	if (ThePrompter.GetData (ctrl))
	{
		if (dlgData.IsRepair ())
		{
			_model.RecoverFiles (*report);
			return false; // Checksum mismatch detected -- after project repair
			              // do not continue with current operation
		}
		else
		{
			Assert (dlgData.IsAdvanced ());
			// User selected advanced option
			if (isCheckout)
			{
				// For checkout advanced option is remove file from project
				VerificationReport::Sequencer seq = report->GetSequencer (VerificationReport::Corrupted);
				GidList files;
				for ( ; !seq.AtEnd (); seq.Advance ())
					files.push_back (seq.Get ());

				_model.RemoveFile (files);
				DisplayTip (IDS_REMOVE);
				return false;  // Do not continue with checkout
			}
			else
			{
				// For delete advanced option is continue delete making files unrecoverable
				return true;
			}
		}
	}
	return false; // Checksum mismatch detected -- cancel
}

void Commander::VerifyRecordSet () const
{
	if (!_model.IsQuickVisit ())
		_selectionMan->VerifyRecordSet ();
}

bool Commander::CheckAndRepairProjectIfNecessary (std::string const & operation)
{
	ProjectChecker projectChecker (_model);

	std::unique_ptr<Progress::Dialog> meterDialog (CreateProgressDlg (operation,
																	true,	// User can cancel
																	1000,	// 1 second initial delay
																	true));	// Multi meter dialog
	std::string caption ("Project ");
	caption += _model.GetProjectName ();
	caption += " verification.";
	meterDialog->SetCaption (caption);

	projectChecker.Verify (meterDialog->GetOverallMeter (), meterDialog->GetSpecificMeter ());
	meterDialog.reset();
	if (projectChecker.IsFileRepairNeeded ())
		return projectChecker.Repair ();
	return true;
}

// Returns true when there are pending scripts in the mailbox (unpacked or from the future)
bool Commander::IsInConflictWithMailbox () const
{
	if (_model.IsQuickVisit () &&
		(_model.HasIncomingOrMissingScripts () || _model.HasScriptsFromFuture ()))
	{
		throw Win::InternalException ("Project with incoming or missing script(s) -- check-in not available.");
	}

	bool incomingScripts = _model.HasIncomingOrMissingScripts ();
	if (incomingScripts || _model.HasScriptsFromFuture ())
	{
		if (incomingScripts && _model.HasToBeRejectedScripts ())
		{
			Out::Answer userChoice = TheOutput.Prompt (
				"You have incoming scripts in your inbox that are in conflict with your history.\n"
				"A branch will be created that will require a merge.\n\n"
				"If you check-in your changes now, they will be added to the branch, and will also have to be merged.\n\n"
				"Do you want to continue with your check-in?",
				Out::PromptStyle (Out::YesNo, Out::No, Out::Question),
				_hwnd);
			if (userChoice == Out::Yes)
				return false;	// Go ahead with the branch check-in
			else
				return true;
		}
		else
		{
			TheOutput.Display ("You have incoming scripts in your inbox.\n"
							   "You have to execute incoming scripts,\n"
							   "before you can check-in your own changes.");
		}
		return true;
	}

	return false;
}

void Commander::DisplayTip (int msgId, bool isForce)
{
	if (!_model.IsQuickVisit ())
	{
		_coopTips.Display (msgId, isForce);
	}
}

std::unique_ptr<Progress::Dialog> Commander::CreateProgressDlg (std::string const & title,
															  bool canCancel,
															  unsigned initialDelay,
															  bool multiMeter) const
{
	if (_model.IsQuickVisit ())
	{
		return std::unique_ptr<Progress::Dialog> (new Progress::BlindDialog (_msgPrepro));
	}
	else if (multiMeter)
	{
		return std::unique_ptr<Progress::Dialog> (new Progress::MultiMeterDialog (title,
																				_hwnd,
																				_msgPrepro,
																				canCancel,
																				initialDelay));
	}
	else
	{
		return std::unique_ptr<Progress::Dialog> (new Progress::MeterDialog (title,
																		   _hwnd,
																		   _msgPrepro,
																		   canCancel,
																		   initialDelay));
	}
}
