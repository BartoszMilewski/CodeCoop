//----------------------------------
// (c) Reliable Software 1997 - 2008
//----------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "Model.h"
#include "CheckInDlg.h"
#include "Transformer.h"
#include "FileTyper.h"
#include "FileList.h"
#include "FileTrans.h"
#include "PhysicalFile.h"
#include "SelectIter.h"
#include "OutputSink.h"
#include "PathSequencer.h"
#include "FeedbackMan.h"
#include "MemberInfo.h"
#include "CmdLineSelection.h"
#include "Predicate.h"
#include "Workspace.h"
#include "ScriptHeader.h"
#include "AckBox.h"
#include "ProjectMarker.h"

#include <Dbg/Out.h>

bool Model::AddFiles (std::vector<std::string> const & files, FileTyper & fileTyper) 
{
	Assert (!files.empty ());
	PathParser pathParser (_directory);
	dbg << "AddFiles: create selection" << std::endl;
	Workspace::Selection selection (files, pathParser);
	selection.SetType (fileTyper, _pathFinder);
	dbg << "Model::AddFiles initial selection" << std::endl;
	dbg << selection;
	return Execute (selection);
}

void Model::AddFile (char const * fullTargetPath, GlobalId gid, FileType type)
{
	// Adds new or changes existing file data for the specified file/folder
	PathParser pathParser (_directory);
	UniqueName const * uName = pathParser.Convert (fullTargetPath);
	Assert (uName != 0 && !uName->IsRootName ());

	Transaction xact (*this, _pathFinder);

	UniqueName addedFileUname;
	if (uName->IsNormalized ())
	{
		addedFileUname.Init (*uName);
	}
	else
	{
		// Recycle absent folders
		// WARNING: this will stop working when folder move is implemented!
		GlobalId currentParentGid = uName->GetParentId ();
		PartialPathSeq pathSeq (uName->GetName ().c_str ());
		for ( ; !pathSeq.IsLastSegment (); pathSeq.Advance ())
		{
			UniqueName segmentUname (currentParentGid, pathSeq.GetSegment ());
			FileData * fd = _dataBase.XFindAbsentFolderByName (segmentUname);
			if (fd == 0)
				throw Win::InternalException ("Cannot merge file/folder whose parent folder has not been merged.",
											  fullTargetPath);
			currentParentGid = fd->GetGlobalId ();
			fd->SetState (StateBrandNew ());
			_checkInArea.Notify (changeAdd, currentParentGid);
			_directory.Notify (changeEdit, currentParentGid);
		}
		Assert (currentParentGid != gidInvalid);
		Assert (pathSeq.IsLastSegment ());
		addedFileUname.Init (currentParentGid, pathSeq.GetSegment ());
	}

	Assert (addedFileUname.IsNormalized ());
	FileData const * fileData = _dataBase.XFindByGid (gid);
	if (fileData == 0)
	{
		// File/folder not recorded in the project database.
		// Add file data with the required global id.
		FileData * fd = _dataBase.XAddForeignFile (addedFileUname, gid, type);
		fd->SetState (StateBrandNew ());
	}
	else
	{
		// File/folder recorded in the project database.
		FileState state = fileData->GetState ();
		if (state.IsNone ())
		{
			Assume (_dataBase.XFindProjectFileByName (addedFileUname) == 0, addedFileUname.GetName ().c_str ());
			FileData * fd = _dataBase.XGetEdit (gid);
			fd->SetName (addedFileUname);
			fd->SetType (type);
			fd->SetState (StateBrandNew ());
		}
		else if (state.IsToBeDeleted ())
		{
			FileData * fd = _dataBase.XGetEdit (gid);
			if (!fd->GetUniqueName ().IsEqual (addedFileUname))
			{
				// Rename file to required unique name
				fd->Rename (addedFileUname);
			}
			if (!fd->GetType ().IsEqual (type))
			{
				// Change type to the required one
				fd->ChangeType (type);
			}
			// Correct file/folder state - no longer to be deleted
			state.SetCoDelete (false);
			state.SetPresentIn (Area::Project, true);
			fd->SetState (state);
		}
		else
		{
			Project::Path path (_dataBase);
			GlobalIdPack pack (fileData->GetGlobalId ());
			std::string info (path.MakePath (fileData->GetUniqueName ()));
			info += ' ';
			info += pack.ToBracketedString ();
			Win::ClearError ();
			throw Win::Exception ("You cannot add file/folder to the project, "
								  "because there is already file/folder "
								  "with the same name in the project.", 
								  info.c_str ());
		}
	}
	_checkInArea.Notify (changeAdd, gid);
	_directory.Notify (changeEdit, gid);
	xact.Commit ();
}

void Model::CopyAddFile (std::string const & srcPath, 
						 std::string const & subDir, 
						 std::string const & fileName, 
						 FileType fileType)
{
	FilePath tgtDir (subDir);
	std::string tgtPath = tgtDir.GetFilePath (fileName);
	File::Copy (srcPath.c_str (), tgtPath.c_str ());

	PathParser pathParser (_directory);
	Workspace::Selection selection (tgtPath, pathParser);
	selection.SetType (fileType);
	dbg << "Model::CopyAddFile" << std::endl;
	dbg << selection;
	Execute (selection);
}

void Model::SetCheckoutNotification (bool isOn)
{
	Project::Db const & projectDb = _dataBase.GetProjectDb ();
	UserId myId = projectDb.GetMyId ();
	MemberState newState = projectDb.GetMemberState (myId);
	if (newState.IsCheckoutNotification () != isOn)
	{
		newState.SetCheckoutNotification (isOn);
		ChangeMemberState (myId, newState);
	}
}

class FolderFilter : public std::unary_function<GlobalId, bool>
{
public:
	FolderFilter (DataBase const & dataBase)
		: _dataBase (dataBase)
	{}

	bool operator() (GlobalId gid) const
	{
		FileData const * fd = _dataBase.GetFileDataByGid (gid);
		return fd->GetType ().IsFolder ();
	}

private:
	DataBase const & _dataBase;
};

bool Model::CheckOut (GidList & files, bool includeFolders, bool recursive)
{
	Assert (!files.empty ());
	GidList::iterator rootIter = std::find (files.begin (), files.end (), gidRoot);
	if (rootIter != files.end ())
	{
		// Project root selected - replace it with root folder content
		files.erase (rootIter);
		_dataBase.ListFolderContents (gidRoot, files, recursive);
		includeFolders = recursive;	// Include root folder sub folders only when recursive
	}
	FolderFilter isFolder (_dataBase);
	bool foldersSelected = false;
	if (includeFolders)
	{
		GidList::const_iterator firstFolder = std::find_if (files.begin (), files.end (), isFolder);
		foldersSelected = (firstFolder != files.end ());
	}
	else
	{
		// Remove any selected folders
		GidList::iterator newEnd = std::remove_if (files.begin (), files.end (), isFolder);
		files.erase (newEnd, files.end ());
	}
	Workspace::Selection selection (files, _dataBase, Workspace::Checkout);
	if (recursive || foldersSelected)
	{
		Assert (includeFolders);
		selection.AddContents (_dataBase, recursive);
	}
	dbg << "Model::CheckOut initial selection" << std::endl;
	dbg << selection;
	return Execute (selection);
}

//
// Check In operations
//

void Model::CleanupAfterAbortedCheckIn (GidList const & files)
{
	SimpleMeter meter (_uiFeedback);
	meter.SetRange (0, files.size (), 1);
	meter.SetActivity ("Cleaning up after aborted check-in");
	for (GidList::const_iterator iter = files.begin (); iter != files.end (); ++iter)
    {
        GlobalId gid = *iter;
		Assert (gid != gidInvalid);
        FileData const * fileData = _dataBase.GetFileDataByGid (gid);
        PhysicalFile file (*fileData, _pathFinder);
        file.MakeReadWriteIn (Area::Project);
		meter.StepIt ();
    }
}

bool Model::CanCheckIn (bool & blocked) const
{
	UserId thisUserId = _dataBase.GetMyId ();
	if (thisUserId != gidInvalid)
	{
		// Only Voting Member can perform check in
		MemberState state = _dataBase.GetMemberState (thisUserId);
		if (state.IsVotingMember () && !state.IsReceiver ())
		{
			// Voting member - check if project is under recovery and check-in is blocked
			BlockedCheckinMarker marker (_catalog, GetProjectId ());
			blocked = marker.Exists ();
			return !blocked;
		}
	}
	return false;
}

bool Model::CheckIn (GidList const & files, CheckInUI & checkInUI, bool isInventory)
{
	Assert (!files.empty ());
    if (!_synchArea.IsEmpty ("proceeding with the check-in"))
		return false;

	Workspace::CheckinSelection selection (files, _dataBase);
	dbg << "Model::CheckIn - Initial selection:" << std::endl;
	dbg << selection;
	selection.Extend (_dataBase);
	dbg << "Extended selection:" << std::endl;
	dbg << selection;

	Project::Db const & projectDb = _dataBase.GetProjectDb ();
	if (!selection.DetectChanges (_pathFinder))
	{
		checkInUI.NoChangesDetected (projectDb.IsKeepCheckedOut ());
		if (!checkInUI.IsKeepCheckedOut ())
		{
			// Uncheck-out files/folders selected for the check-in
			selection.SetOperation (Workspace::Uncheckout);
			Execute (selection, false);	// don't extend selection
		}
		return false;
	}

	if (!selection.IsComplete (_dataBase))
		return false;

	if (!_history.CheckCurrentLineage ())
		throw Win::Exception ("Code Co-op cannot perform check-in operation, becuase history corruption has been detected.\nPlease, contact support@relisoft.com");
	std::string lastRejectedComment = _history.RetrieveLastRejectedScriptComment ();
	// Get check-in comment from the user
	if (!checkInUI.Query (lastRejectedComment, projectDb.IsKeepCheckedOut ()))
		return false;	// User canceled check-in

	if (checkInUI.IsKeepCheckedOut ())
		selection.KeepCheckedOut ();	// Remember to keep checked out the original selection

	AckBox ackBox;
	try
	{
		ExecuteCheckin (selection, ackBox, checkInUI.GetComment (), isInventory);
	}
	catch (Win::Exception e)
	{
		if (e.GetMessage () != 0)
		{
			TheOutput.Display (e);
		}
		CleanupAfterAbortedCheckIn (files);
		return false;
	}
	catch (std::bad_alloc bad)
	{
		Win::ClearError ();
		TheOutput.Display ("Check-in aborted: not enough memory.\nSynchronization script was not created.");
		CleanupAfterAbortedCheckIn (files);
		return false;
	}
	catch ( ... )
	{
		Win::ClearError ();
        TheOutput.Display ("Check-in aborted due to unknown error.\nSynchronization script was not created.");
		CleanupAfterAbortedCheckIn (files);
		return false;
	}

	SendAcknowledgments (ackBox);
	return true;
}

bool Model::Uncheckout (GidList const & userSelection, bool quiet)
{
	if (!quiet)
	{
		bool changesDetected = false;

		{
			SimpleMeter meter (_uiFeedback);
			meter.SetActivity ("Looking for changed files in the Check-in Area");
			meter.SetRange (0, userSelection.size (), 1);
			for (GidList::const_iterator iter = userSelection.begin ();
				 !changesDetected && iter != userSelection.end ();
				 ++iter)
			{
				GlobalId gid = *iter;
				Assert (gid != gidInvalid);
				FileData const * file = _dataBase.GetFileDataByGid (gid);
				FileState state = file->GetState ();
				meter.StepIt ();
				// Don't warn about unchecking out new or deleted files/folders
				if (state.IsNew () || state.IsToBeDeleted ())
					continue;

				if (state.IsRelevantIn (Area::Original))
				{
					FileType type = file->GetType ();
					Assert (!type.IsRoot ());
					if (type.IsFolder ())
						changesDetected = HasFolderContentsChanged (gid);
					else
						changesDetected = HasFileChanged (file);
				}
			}
		}

		if (changesDetected)
		{
			Out::Answer userChoice = TheOutput.Prompt (
				"Are you sure you want to restore changed files to their previous state?\n"
				"All edits, file renames, type changes, etc. you've made since check-out will be lost.",
				Out::PromptStyle (Out::YesNo, Out::No, Out::Question));
			if (userChoice != Out::Yes)
				return false;
		}
	}

	Workspace::Selection selection (userSelection, _dataBase, Workspace::Uncheckout);
	dbg << "Model::Uncheckout -- initial selection" << std::endl;
	dbg << selection;
	return Execute (selection);
}

bool Model::HasFolderContentsChanged (GlobalId folder)
{
	GidList contents;
	_dataBase.ListProjectFiles (folder, contents);
	bool changesDetected = false;
	for (GidList::const_iterator iter = contents.begin ();
		 iter != contents.end () && !changesDetected;
		 ++iter)
	 {
		GlobalId gid = *iter;
		Assert (gid != gidInvalid);
		FileData const * file = _dataBase.GetFileDataByGid (gid);
		FileType type = file->GetType ();
		Assert (!type.IsRoot ());
		if (type.IsFolder ())
			changesDetected = HasFolderContentsChanged (gid);
		else
			changesDetected = HasFileChanged (file);
	 }
	return changesDetected;
}

bool Model::HasFileChanged (FileData const * file)
{
	if (file->GetState ().IsNew () || file->GetState ().IsToBeDeleted () ||
		file->IsRenamedIn (Area::Original) ||
		file->IsTypeChangeIn (Area::Original))
	{
		return true;
	}
	else
	{
		try
		{
			PhysicalFile physFile (*file, _pathFinder);
			return physFile.IsDifferent (Area::Project, Area::Original);
		}
		catch ( ... ) 
		{
			Win::ClearError ();
			return true;
		}
	}
}

void Model::GetChangedFiles (GidList & changed)
{
	GidList checkedOut;
	_dataBase.ListCheckedOutFiles (checkedOut);
	for (GidList::const_iterator iter = checkedOut.begin (); iter != checkedOut.end (); ++iter)
	{
		GlobalId gid = *iter;
		FileData const * fd = _dataBase.GetFileDataByGid (gid);
		if (HasFileChanged (fd))
			changed.push_back (gid);
	}
}

bool Model::VersionLabel (CheckInUI & checkInUI)
{
	Assert (IsProjectReady ());
    if (!_synchArea.IsEmpty ("proceeding with creating a milestone"))
		return false;

	AckBox ackBox;
	try
	{
		bool scriptBroadcast = false;
		ScriptHeader hdr (ScriptKindSetChange (), gidInvalid, _dataBase.ProjectName ());
		CommandList emptyCmdList;
		std::string lastRejectedComment = _history.RetrieveLastRejectedScriptComment ();
		// Get label comment from user
		if (checkInUI.Query (lastRejectedComment, false))
		{
			{
				// Transaction scope
				TransactionFileList fileList;
				FileTransaction xact (*this, _pathFinder, fileList);
				hdr.SetScriptId (_dataBase.XMakeScriptId ());
				hdr.AddComment (checkInUI.GetComment ());
				scriptBroadcast = _dataBase.XScriptNeeded ();
				_history.XGetLineages (hdr, scriptBroadcast ? UnitLineage::Minimal : UnitLineage::Empty);
				_history.XAddCheckinScript (hdr, emptyCmdList, ackBox);
				xact.Commit ();
			}

			if (scriptBroadcast)
			{
				// Send script to others
				std::unique_ptr<CheckOut::List> notification = GetCheckoutNotification ();
				_mailer.Broadcast (hdr, emptyCmdList, notification.get ());
			}
		}
		else
		{
			// User canceled labeling
			return false;
		}
	}
	catch (Win::Exception e)
	{
		if (e.GetMessage () == 0)
		{
			TheOutput.Display ("Version labeling aborted by the user.\nSynchronization script was not created.");
		}
		else
		{
			TheOutput.Display (e);
		}
		return false;
	}
	catch ( ... )
	{
		Win::ClearError ();
        TheOutput.Display ("Version labeling aborted due to unknown error.\nSynchronization script was not created.");
		return false;
	}
	SendAcknowledgments (ackBox);
	return true;
}
