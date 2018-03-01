//----------------------------------
// (c) Reliable Software 2002 - 2008
//----------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "Model.h"
#include "CheckInDlg.h"
#include "CmdExec.h"
#include "ScriptHeader.h"
#include "Diff.h"
#include "Transformer.h"
#include "FileTyper.h"
#include "FileList.h"
#include "FileTrans.h"
#include "OutputSink.h"
#include "FeedbackMan.h"
#include "Workspace.h"
#include "ProjectPath.h"

#include <Ctrl/ProgressMeter.h>
#include <Dbg/Out.h>
#include <Dbg/Assert.h>

class XExecutor
{
public:
	XExecutor (DataBase & dataBase, PathFinder & pathFinder, TransactionFileList & fileList)
		: _dataBase (dataBase),
		  _pathFinder (pathFinder),
		  _fileList (fileList)
	{}

	GlobalId Add (Workspace::Item & item);
	FileState Move (Workspace::Item const & item);
	FileState Edit (Workspace::Item const & item);
	FileState Delete (GlobalId itemGid);
	FileState Remove (GlobalId itemGid);
	FileState ResolveNameConflict (GlobalId itemGid, bool isVirtual);
	FileState Checkout (GlobalId itemGid);
	FileState Uncheckout (GlobalId itemGid);

	void Synch (Workspace::ScriptItem const & scriptItem);
	FileState Undo (Workspace::HistoryItem const & undoItem,
					Workspace::Selection const & selection,
					bool isVirtual,
					Progress::Meter & meter);
	void Redo (Workspace::HistoryItem const & redoItem,
			   Progress::Meter & meter);

private:
	void UndoFileCmd (FileCmd const & cmd, bool isFilePresentInProject);

private:
	DataBase &				_dataBase;
	PathFinder &			_pathFinder;
	TransactionFileList &	_fileList;
};

// Executes selection
void Model::XExecute (Workspace::Selection & selection,
					  TransactionFileList & fileList,
					  Progress::Meter & meter,
					  bool isVirtual)
{
    selection.Sort ();
	dbg << "Complete selection:" << std::endl;
	dbg << selection;
	meter.SetRange (0, selection.size (), 1);
	XExecutor executor (_dataBase, _pathFinder, fileList);
	for (Workspace::Selection::Sequencer seq (selection); !seq.AtEnd (); seq.Advance ())
	{
		meter.StepAndCheck ();
		Workspace::Item const & item = seq.GetItem ();
		Workspace::Operation operation = item.GetOperation ();
		Assert (operation != Workspace::Undefined);
		Assert (operation != Workspace::Checkin);
		if (operation == Workspace::Add)
		{
			Workspace::Item & newItem = seq.GetItemForEdit ();
			GlobalId newGid = executor.Add (newItem);
			if (newGid != gidInvalid)
			{
				char const * newName = newItem.GetEffectiveTargetName ().c_str ();
				meter.SetActivity (newName);
				_directory.Notify (changeAll, newName);
				_checkInArea.Notify (changeAdd, newGid);
			}
		}
		else if (operation == Workspace::Synch)
		{
			// Script operation
			Workspace::ScriptItem const & scriptItem = seq.GetScriptItem ();
			if (scriptItem.GetFileCmd ().GetSynchKind () == synchNew)
				meter.SetActivity (scriptItem.GetEffectiveTarget ().GetName ().c_str ());
			else
				meter.SetActivity (_pathFinder.GetRootRelativePath (scriptItem.GetItemGid ()));
			executor.Synch (scriptItem);
		}
		else
		{
			GlobalId itemGid = item.GetItemGid ();
			meter.SetActivity (_pathFinder.GetRootRelativePath (itemGid));
			Assert (itemGid != gidInvalid);
			FileState resultState;
			switch (operation)
			{
			case Workspace::Move:		// Change name and/or position in the project tree
				resultState = executor.Move (item);
				// Folder will be notified by external notification from folder watcher
				_checkInArea.Notify (changeAdd, itemGid);
				break;
			case Workspace::Edit:		// Change file type
				resultState = executor.Edit (item);
				_checkInArea.Notify (changeAdd, itemGid);
				_directory.Notify (changeEdit, itemGid);
				break;
			case Workspace::Delete:		// Remove a name from the project name space and file from disk
				resultState = executor.Delete (itemGid);
				_directory.Notify (item.IsFolder () ? changeEdit : changeRemove, itemGid);
				_checkInArea.Notify (changeAdd, itemGid);
				break;
			case Workspace::Remove:		// Remove a name from the project name space but leave file on disk
				resultState = executor.Remove (itemGid);
				_directory.Notify (changeEdit, itemGid);
				_checkInArea.Notify (changeAdd, itemGid);
				break;
			case Workspace::Resolve:	// Current file/folder name cannot be used. Change it to “Previous …” name
				resultState = executor.ResolveNameConflict (itemGid, isVirtual);
				if (!isVirtual)
				{
					_directory.Notify (changeEdit, itemGid);
					_checkInArea.Notify (changeEdit, itemGid);
				}
				break;
			case Workspace::Checkout:
				resultState = executor.Checkout (itemGid);
				_directory.Notify (changeEdit, itemGid);
				_checkInArea.Notify (changeAdd, itemGid);
				break;
			case Workspace::Uncheckout:
				{
					Workspace::ExistingItem const & uncheckedItem = seq.GetExistingItem ();
					FileData const & fd = uncheckedItem.GetFileData ();
					FileState state = fd.GetState ();
					bool uncheckoutRemove = (!state.IsPresentIn (Area::Project) && (!state.IsCoDelete () || fd.GetType ().IsFolder ()));
					bool uncheckoutDelete = !state.IsPresentIn (Area::Project) && state.IsCoDelete ();
					resultState = executor.Uncheckout (itemGid);
					if (uncheckoutRemove)
					{
						// Removed items stay on disk, so they are visible in the file view after operation.
						// However theirs global ids are set to gidInvalid, so we have lookup them up by name.
						UniqueName const & uname = item.GetEffectiveTarget ();
						_directory.Notify (changeEdit, uname.GetName ().c_str ());
					}
					else if (uncheckoutDelete)
					{
						_directory.Notify (changeAdd, itemGid);
					}
					else
					{
						_directory.Notify (changeEdit, itemGid);
					}
				}
				_checkInArea.Notify (changeRemove, itemGid);
				break;
			case Workspace::Undo:
				resultState = executor.Undo (seq.GetHistoryItem (), selection, isVirtual, meter);
				if (!isVirtual)
				{
					if (resultState.IsPresentIn (Area::Project))
					{
						_directory.Notify (changeEdit, itemGid);
					}
					else
					{
						// Notify by name
						if (item.HasEffectiveSource ())
						{
							UniqueName const & uname = item.GetEffectiveSource ();
							_directory.Notify (changeEdit, uname.GetName ().c_str ());
						}
						if (item.HasEffectiveTarget())
						{
							UniqueName const & uname = item.GetEffectiveTarget ();
							_directory.Notify (changeEdit, uname.GetName ().c_str ());
						}
					}
					_checkInArea.Notify (changeAdd, itemGid);
				}
				break;
			case Workspace::Redo:
				executor.Redo (seq.GetHistoryItem (), meter);
				break;
			}
			if (resultState.IsRelevantIn (Area::Synch))
				_synchArea.Notify (changeEdit, itemGid);
		}
		if (!item.IsOverwriteInProject ())
			fileList.SetOverwrite (item.GetItemGid (), false);
	}
}


// Executes history repair selection
void Model::XExecuteRepair (Workspace::RepairHistorySelection & selection,
							TransactionFileList & fileList,
							GidSet & unrecoverableFiles,
							Progress::Meter & meter)
{
    selection.Sort ();
	dbg << "Complete repair selection:" << std::endl;
	dbg << selection;
	meter.SetRange (0, selection.size (), 1);
	XExecutor executor (_dataBase, _pathFinder, fileList);
	for (Workspace::Selection::Sequencer seq (selection); !seq.AtEnd (); seq.Advance ())
	{
		meter.StepAndCheck ();
		Workspace::HistoryItem const & item = seq.GetHistoryItem ();
		Assume (item.GetOperation () == Workspace::Redo, "History repair: action is not Redo");
		GlobalId itemGid = item.GetItemGid ();
		meter.SetActivity (_pathFinder.GetRootRelativePath (itemGid));
		Assume (itemGid != gidInvalid, "Attempt to repair file with invalid ID");
		FileState resultState;
		try
		{
			executor.Redo (item, meter);
			Transformer trans (_dataBase, itemGid);
			trans.MoveReference2Project (_pathFinder, fileList);
			trans.MakeCheckedIn (_pathFinder, fileList);
		}
		catch (Win::Exception ex)
		{
			dbg << Out::Sink::FormatExceptionMsg(ex);
			unrecoverableFiles.insert(itemGid);
		}
	}
}

GlobalId XExecutor::Add (Workspace::Item & item)
{
	if (item.GetEffectiveTargetType ().IsInvalid ())
	{
		// User didn't specify file type -- treat this as abort add for that file
		return gidInvalid;
	}

	// For add operation target unique name can have the following forms:
	//    a. <gid>\File or Folder Name
	//    b. <gidInvalid>\Folder
	//    c. <anchor gid>\FolderA\FolderB\File or Folder Name
	Assert (item.HasEffectiveTarget ());
	UniqueName targetUname (item.GetEffectiveTarget ());
	if (targetUname.GetParentId () == gidInvalid || !targetUname.IsNormalized ())
	{
		// <gidInvalid>\Folder or <anchor gid>\FolderA\FolderB\FileName
		// Target folder global id not set yet -- read it from the selection
		Workspace::Item const * targetFolderItem = item.GetTargetParentItem ();
		Assert (targetFolderItem != 0);
		Assert (targetFolderItem->GetItemGid () != gidInvalid);
		targetUname.SetParentId (targetFolderItem->GetItemGid ());
		if (!targetUname.IsNormalized ())
		{
			// <anchor gid>\FolderA\FolderB\File or Folder Name
			targetUname.SetName (item.GetEffectiveTargetName ().c_str ());
		}
	}

	FileData const * fileData = _dataBase.XFindProjectFileByName (targetUname);
	if (fileData != 0)
	{
		// File to-be-deleted is being re-added
		Assert ((!fileData->GetType ().IsFolder () && fileData->GetState ().IsToBeDeleted ()) ||
				( fileData->GetType ().IsFolder () && fileData->GetState ().IsCheckedIn ()));
		GlobalId recycledGid = fileData->GetGlobalId ();
		if (fileData->GetState ().IsToBeDeleted ())
		{
			Transformer trans (_dataBase, recycledGid);
			trans.ReviveToBeDeleted ();
		}
		item.SetItemGid (recycledGid);
		return recycledGid;
	}
	else
	{
		Transformer trans (_dataBase, targetUname, item.GetEffectiveTargetType ());
		trans.AddFile (_pathFinder, _fileList);
		GlobalId newGid = trans.GetGlobalId ();
		item.SetItemGid (newGid);
		item.SetTargetName(targetUname.GetName());
		return newGid;
	}
	return gidInvalid;
}

FileState XExecutor::Move (Workspace::Item const & item)
{
	GlobalId itemGid = item.GetItemGid ();
	Assert (itemGid != gidInvalid);
	Transformer trans (_dataBase, itemGid);
	// For move operation target unique name can have the following forms:
	//    a. <itemGid>\FileName
	//    b. <anchor gid>\FolderA\FolderB\FileName
	Assert (item.HasEffectiveTarget ());
	Assert (item.HasBeenMoved ());
	UniqueName targetUname (item.GetEffectiveTarget ());
	if (!targetUname.IsNormalized ())
	{
		// <anchor gid>\FolderA\FolderB\FileName
		// Target folder global id not set yet -- read it from the selection
		Workspace::Item const * targetFolderItem = item.GetTargetParentItem ();
		Assert (targetFolderItem != 0);
		Assert (targetFolderItem->GetItemGid () != gidInvalid);
		targetUname.Init (targetFolderItem->GetItemGid (), item.GetEffectiveTargetName ());
	}
	GlobalId conflictGid = _dataBase.XIsUnique (itemGid, targetUname);
	if (conflictGid != gidInvalid)
	{
		FileData const * conflictFileData = _dataBase.XGetFileDataByGid (conflictGid);
		if (conflictFileData->GetState ().IsCheckedIn ())
		{
			// Can't move -- project file with the same name already exists in target folder
			std::string info;
			info += _pathFinder.XGetFullPath (targetUname);
			GlobalIdPack pack (conflictFileData->GetGlobalId ());
			info += ' ';
			info += pack.ToBracketedString ();
			throw Win::Exception ("Cannot move/rename file, because another project file with the"
								  " same name already exists in the project.",
								  info.c_str ());
		}
		Assert (conflictFileData->IsRenamedIn (Area::Original));
	}
	trans.MoveFile (_pathFinder, _fileList, targetUname);
	return trans.GetState ();
}

FileState XExecutor::Edit (Workspace::Item const & item)
{
	GlobalId itemGid = item.GetItemGid ();
	Assert (itemGid != gidInvalid);
	Transformer trans (_dataBase, itemGid);
	FileType curType = trans.GetFileType ();
	if (!curType.IsFolder ())
	{
		FileType targetType = item.GetEffectiveTargetType ();
		if (!curType.IsEqual (targetType))
		{
			trans.ChangeFileType (_pathFinder, _fileList, targetType);
		}
	}
	return trans.GetState ();
}

FileState XExecutor::Delete (GlobalId itemGid)
{
	Transformer trans (_dataBase, itemGid);
	// We never remove folders from disk
	trans.DeleteFile (_pathFinder, _fileList, trans.IsFolder () ? false : true);	// Delete files from disk
	return trans.GetState ();
}

FileState XExecutor::Remove (GlobalId itemGid)
{
	Transformer trans (_dataBase, itemGid);
	trans.DeleteFile (_pathFinder, _fileList, false);	// Leave on disk
	return trans.GetState ();
}

FileState XExecutor::ResolveNameConflict (GlobalId itemGid, bool isVirtual)
{
	Transformer trans (_dataBase, itemGid);
	trans.ResolveNameConflict (_pathFinder, _fileList, isVirtual);
	return trans.GetState ();
}

FileState XExecutor::Checkout (GlobalId itemGid)
{
	Transformer trans (_dataBase, itemGid);
	trans.CheckOut (_pathFinder, _fileList, false);	// Don't verify checksum, we have already done that
													// before starting the check-out operation.
	return trans.GetState ();
}

FileState XExecutor::Uncheckout (GlobalId itemGid)
{
	Transformer trans (_dataBase, itemGid);
	if (trans.IsFolder () && !trans.GetState ().IsNew ())
	{
		// Unchecking out folder -- make sure that the folder exists on disk
		char const * folderPath = _pathFinder.XGetFullPath (itemGid, Area::Project);
		if (!File::Exists (folderPath))
		{
			// Recreate folder on disk
			File::CreateFolder (folderPath, false);	// Throw exception when folder cannot be created
		}
	}
	trans.Uncheckout (_pathFinder, _fileList);
	return trans.GetState ();
}

void XExecutor::Synch (Workspace::ScriptItem const & scriptItem)
{
	GlobalId itemGid = scriptItem.GetItemGid ();
	if (scriptItem.IsRestored ())
	{
		// Item is participationg in the script conflict
		// Received script wants to edit restored file.
		// If restored file is unrecoverable we cannot
		// unpack the script.
		if (!scriptItem.IsRecoverable ())
		{
			std::string info ("The script attempts to edit unrecoverable file:\n\n");
			info += _pathFinder.XGetFullPath (itemGid, Area::Project);
			info += "\n\nYou cannot unpack this script. To continue development in this project\n"
					"you have to defect and re-enlist.";
			TheOutput.Display (info.c_str ());
			throw Win::Exception ();
		}
	}
	else
	{
		// Item is not participating in the script conflict
		// If script adds new file to the project, transformer constructor will add
		// new file data to the local database.
		Transformer trans (_dataBase, _pathFinder, _fileList, scriptItem.GetScriptFileData ());
		// If item is checked out create relevant reference, otherwise create unrelevant reference.
		// Unrelevant reference is a temporary reference file created just for the purpose of
		// script unpacking.
		// Files created in the Reference Area because of script conflict or
		// because they were checked out at the time of script unpacking are relevent there.
		if (trans.IsCheckedOut ())
			trans.CreateRelevantReferenceIfNecessary (_pathFinder, _fileList);
		else
			trans.CreateUnrelevantReference (_pathFinder, _fileList);
	}

	FileCmd const & fileCmd = scriptItem.GetFileCmd ();
	std::unique_ptr<CmdFileExec> exec = fileCmd.CreateExec ();
	// Acting on Reference create Synch
	exec->Do (_dataBase, _pathFinder, _fileList);

	Transformer postRecreateTrans (_dataBase, itemGid);
	postRecreateTrans.CleanupReferenceIfPossible (_pathFinder, _fileList);
}

FileState XExecutor::Undo (Workspace::HistoryItem const & undoItem,
						   Workspace::Selection const & selection,
						   bool isVirtual,
						   Progress::Meter & meter)
{
	if (!undoItem.HasEffectiveSource () && !undoItem.HasEffectiveTarget ())
	{
		FileState none;
		return none;
	}

	Project::XPath projPath (_dataBase);
	Transformer trans (_dataBase, undoItem.GetItemGid ());
	// Go over item commands and undo them
	Workspace::HistoryItem::BackwardCmdSequencer seq (undoItem);
	meter.SetRange (1, seq.Count ());
	Assert (!seq.AtEnd ());
	if (trans.IsStateNone ())
	{
		// Copy file data from cmd to transformer
		trans.Init (seq.GetCmd ().GetFileData ());
	}
	// These operations are idempotent
	// so no need to check the state before doing them.
	trans.CheckOutIfNecessary (_pathFinder, _fileList);
	trans.CreateRelevantReferenceIfNecessary (_pathFinder, _fileList);
	for ( ; !seq.AtEnd (); seq.Advance ())
	{
		meter.StepIt ();
		FileCmd const & cmd = seq.GetCmd ();
		FileData const & cmdFileData = cmd.GetFileData ();
		SynchKind synchKind = cmd.GetSynchKind ();
		// Undo file state changes
		if (synchKind == synchNew)
		{
			// "Undo add" means remove from project
			trans.RemoveFromReference ();
		}
		else if (synchKind == synchDelete || synchKind == synchRemove)
		{
			// "Undo delete/remove" means add to project
			trans.CopyToReference (_pathFinder, _fileList);
			// Use the name and type as they were just before file deletion
			trans.UndoRename (cmdFileData.GetUniqueName ());
			trans.UndoChangeType (cmdFileData.GetType ());
		}
		else
		{
			Assert (synchKind == synchEdit || synchKind == synchRename || synchKind == synchMove);
			if (cmdFileData.IsRenamedIn (Area::Original))
				trans.UndoRename (cmdFileData.GetUnameIn (Area::Original));
			if (cmdFileData.IsTypeChangeIn (Area::Original))
				trans.UndoChangeType (cmdFileData.GetTypeIn (Area::Original));
		}

		if (undoItem.IsFolder ())
		{
			if (!isVirtual && (synchKind == synchDelete || synchKind == synchRemove))
			{
				// Make sure that folder is present on disk
				UniqueName const & target = undoItem.GetEffectiveTarget ();
				char const * targetPath = _pathFinder.GetFullPath (target);
				File::CreateFolder (targetPath, false);	// Don't be quiet
			}
		}
		else
		{
			// Now undo file contents changes
			UndoFileCmd (cmd, !trans.IsStateNone ());
		}
	}
	if (!isVirtual && undoItem.IsRecoverable ())
	{
		GlobalId gidConflict = trans.IsNameConflict (Area::Reference);
		if (gidConflict != gidInvalid && !selection.IsIncluded (gidConflict))
		{
			// The real revert operation on selected files/folders.
			// If gidConflict is not included in the selection abort revert.
			std::string info;
			info += "In order to avoid a name conflict in the project,\n"
				"you also have to revert the following file/folder:\n\n";
			info += projPath.XMakePath (gidConflict);
			GlobalIdPack pack (gidConflict);
			info += ' ';
			info += pack.ToBracketedString ();
			TheOutput.Display (info.c_str ());
			throw Win::Exception ();
		}
	}
	return trans.GetState ();
}

void XExecutor::Redo (Workspace::HistoryItem const & redoItem,
					  Progress::Meter & meter)
{
	if (!redoItem.HasEffectiveSource () && !redoItem.HasEffectiveTarget ())
		return;

	Workspace::HistoryItem::ForwardCmdSequencer seq (redoItem);
	meter.SetRange (1, seq.Count ());
	for ( ; !seq.AtEnd (); seq.Advance ())
	{
		meter.StepIt ();
		FileCmd const & cmd = seq.GetCmd ();
		std::unique_ptr<CmdFileExec> exec = cmd.CreateExec ();
		// Redo edits in the Reference
		exec->Do (_dataBase, _pathFinder, _fileList, true);	// In place -- don't create Synch
	}
}

void XExecutor::UndoFileCmd (FileCmd const & cmd, bool isFilePresentInProject)
{
	if (!cmd.IsContentsChanged ())
		return;

	FileData const & fd = cmd.GetFileData ();
	char const * refPath = _pathFinder.XGetFullPath (fd.GetGlobalId (), Area::Reference);
	// Make sure that we start from the right reference version
	CheckSum csFile (refPath);
	if (cmd.GetNewCheckSum () != csFile)
	{
		throw Win::InternalException ("Error reverting a script command: File checksums don't match.",
									  cmd.GetName ());
	}
	std::unique_ptr<CmdFileExec> exec = cmd.CreateExec ();
	File::MakeReadWrite (refPath); // Prepare for modifications
	std::unique_ptr<Undo::Buffer> buffer = exec->Undo (refPath);
	// Make sure that we got the right result
	if (buffer->GetChecksum () != cmd.GetOldCheckSum ())
	{
		throw Win::InternalException ("Error reverting a script command: "
									  "Restored file checksum does not match.",
									  cmd.GetName ());
	}
	// Now overwirte reference version with the undone contents
	if (isFilePresentInProject)
	{
		buffer->SaveTo (refPath);
	}
	// Leave it as ReadOnly
	File::MakeReadOnly (refPath);
}

// Merges selection with the workgroup and executes it during transaction
bool Model::Execute (Workspace::Selection & selection, bool extendSelection)
{
	if (extendSelection)
	{
		selection.Extend (_dataBase);
		dbg << "Extended selection" << std::endl;
		dbg << selection;
	}

	SimpleMeter meter (_uiFeedback);

	{
		// Transaction scope
		TransactionFileList fileList;
		FileTransaction xact (*this, _pathFinder, fileList);
		Workspace::XGroup workgroup (_dataBase, selection);
		selection.XMerge (workgroup);
		XExecute (selection, fileList, meter);
		xact.Commit ();
	}

	SetCheckedOutMarker ();
	BroadcastCheckoutNotification ();
	return true;
}

void Model::ExecuteCheckin (Workspace::CheckinSelection & selection,
							AckBox & ackBox,
							std::string const & comment,
							bool isInventory)
{
	GidList keepCheckedOut;
	ScriptHeader hdr (ScriptKindSetChange (), gidInvalid, _dataBase.ProjectName ());
	CommandList cmdList;
	bool doBroadcast = false;
	SimpleMeter meter (_uiFeedback);

	{
		// Check-in transaction --------------------------------------------------------------
		TransactionFileList fileList;
		FileTransaction xact (*this, _pathFinder, fileList);
		Workspace::XGroup workgroup (_dataBase, selection);
		selection.XMerge (workgroup);
		selection.Sort ();
		dbg << "Complete selection:" << std::endl;
		dbg << selection;
		meter.SetRange (0, selection.size () + 2, 1);
		for (Workspace::Selection::Sequencer seq (selection); !seq.AtEnd (); seq.Advance ())
		{
			Workspace::Item const & item = seq.GetItem ();
			Assert (item.GetOperation () == Workspace::Checkin);
			GlobalId gid = item.GetItemGid ();
			Assert (gid != gidInvalid);
			// Update progress meter
			meter.SetActivity (_pathFinder.XGetFullPath (gid, Area::Project));
			meter.StepIt ();
			Transformer trans (_dataBase, gid);
			Assert (!trans.IsSynchedOut ());
			trans.CheckIn (_pathFinder, fileList, cmdList);
			if (item.IsKeepCheckedOut ())
			{
				// Item from the original check-in selection and user
				// wants to keep file/folder checked out.
				FileState afterCheckinState = trans.GetState ();
				if (afterCheckinState.IsCheckedIn ())
					keepCheckedOut.push_back (gid);
			}

			_checkInArea.Notify (changeRemove, gid);
			_directory.Notify (changeEdit, gid);
		}

		// Store script in the history and send it out if necessary
		meter.SetActivity ("Storing check-in script in the history");
		hdr.SetScriptId (_dataBase.XMakeScriptId ());
		hdr.AddComment (comment);
		doBroadcast = _dataBase.XScriptNeeded ();
		_history.XGetLineages (hdr, doBroadcast ? UnitLineage::Minimal : UnitLineage::Empty);
		_history.XAddCheckinScript (hdr, cmdList, ackBox, isInventory);
		meter.StepIt ();
		meter.SetActivity ("Saving project database");
		xact.Commit ();
		meter.StepIt ();
		// End of check-in transaction ------------------------------------------------------
	}

	if (doBroadcast)
	{
		// Send script to others
		std::unique_ptr<CheckOut::List> notification = GetCheckoutNotification ();
		_mailer.Broadcast (hdr, cmdList, notification.get ());
	}

	if (!keepCheckedOut.empty ())
	{
			// Update progress meter
		meter.SetRange (0, keepCheckedOut.size (), 1);
		meter.SetActivity ("Keeping files checked out");
		try
		{
			TransactionFileList fileList;
			FileTransaction xact (*this, _pathFinder, fileList);
			for (GidList::const_iterator iter = keepCheckedOut.begin (); iter != keepCheckedOut.end (); ++iter)
			{
				Transformer trans (_dataBase, *iter);
				trans.CheckOut (_pathFinder, fileList);
				meter.StepIt ();
				_checkInArea.Notify (changeAdd, *iter);
				_directory.Notify (changeEdit, *iter);
			}
			xact.Commit ();
		}
		catch ( ... )
		{
			// Ignore exceptions during keep checked out transaction
		}
	}

	SetCheckedOutMarker ();
}
