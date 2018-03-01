//----------------------------------
// (c) Reliable Software 1997 - 2007
//----------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "Transformer.h"
#include "DataBase.h"
#include "SynchArea.h"
#include "FileList.h"
#include "PathFind.h"
#include "PhysicalFile.h"
#include "OutputSink.h"
#include "FileTyper.h"
#include "Predicate.h"
#include "ScriptCommandList.h"

#include <Ex/WinEx.h>
#include <File/Path.h>
#include <Dbg/Out.h>
#include <Com/Shell.h>
#include <StringOp.h>

// Always created under transaction

Transformer::Transformer (DataBase & dataBase, UniqueName const & uname, FileType type)
	: _dataBase (dataBase)
{
	FileData const * fd = _dataBase.XFindProjectFileByName (uname);
	if (fd != 0)
	{
		// Found another project file with the same name.
		FileState state = fd->GetState ();
		if (state.IsPresentIn (Area::Project))
		{
			std::string info;
			Project::Path path (_dataBase);
			GlobalIdPack pack (fd->GetGlobalId ());
			info += path.MakePath (fd->GetUniqueName ());
			info += ' ';
			info += pack.ToBracketedString ();
			Win::ClearError ();
			throw Win::Exception ("You cannot add file/folder to the project, "
								  "because there is already file/folder "
								  "with the same name in the project.", 
								  info.c_str ());
		}
	}

	if (type.IsFolder ())
	{
		// Recycle folders if possible.
		// WARNING: this will stop working when folder move is implemented!
		_fileData = _dataBase.XFindAbsentFolderByName (uname);
		if (_fileData != 0)
		{
			dbg << "Transformer constructor 1 (recycled folder)" << std::endl;
			dbg << *_fileData;
			// Finding absent folder is case insentive - use the actual name
			_fileData->SetName (uname);
			return;
		}
	}

	_fileData = _dataBase.XAddFile (uname, type);
	dbg << "Transformer constructor 1 (new file/folder)" << std::endl;
	dbg << *_fileData;
}

Transformer::Transformer (DataBase & dataBase, GlobalId gid)
	: _dataBase (dataBase)
{
	_fileData = dataBase.XGetEdit (gid);
	if (!_fileData)
		throw Win::Exception ("Corrupt database: missing file information");
	_state = _fileData->GetState ();
	dbg << "Transformer constructor 2" << std::endl;
	dbg << *_fileData;
}

Transformer::Transformer (DataBase & dataBase,
						  PathFinder & pathFinder,
						  TransactionFileList & fileList,
						  FileData const & scriptFileData)
	: _dataBase (dataBase)
{
	dbg << "Transformer constructor 3 (file data from the incoming script)" << std::endl;
	dbg << scriptFileData;
	// Try gid
	_fileData = _dataBase.XGetEdit (scriptFileData.GetGlobalId ());
	if (_fileData != 0)
	{
		// File/folder changed by the script is already present in the recipients project database.
		FileState scriptFileState = scriptFileData.GetState ();
		FileState localFileState = _fileData->GetState ();
		FileType type = _fileData->GetType ();
		if (scriptFileState.IsNew ())
		{
			// Incoming script adds this file/folder to the project and recipient knows its global id.
			// This can happen only when this file/folder was earlier deleted from the project and now
			// the incoming script brings it back to the project.
			if (localFileState.IsNone () ||
				localFileState.IsRelevantIn (Area::Original) && type.IsFolder ())
			{
				// We have received a script reversing file delete and script recipient did not restored it or
				// we have received a script reversing folder delete (script recipient can also revert the folder delete,
				// but this doesn't matter in case of folders).
				// Recycle local FileData by initializing it with script FileData
				_fileData->DeepCopy (scriptFileData);
				_fileData->ClearAliases ();
				_state.Reset ();
				_fileData->SetState (_state);
				dbg << "Constructor 3 -- Recycling file data (reversing delete)" << std::endl;
				return;
			}
			else if (!localFileState.IsNew ())
			{
				// We cannot accept this file/folder addition because the file/folder
				// already exisits in the recipient's project database.
				std::string info;
				GlobalIdPack pack (scriptFileData.GetGlobalId ());
				info += pack.ToBracketedString ();
				info += ' ';
				info += scriptFileData.GetUniqueName ().GetName ();
				throw Win::Exception ("Corrupted script -- cannot add file/folder to the project, "
									  "because it already exists.",
									  info.c_str ());
			}
			else
			{
				// Merge local file restore with the incoming script restore
				Assert (localFileState.IsNew () && !type.IsFolder ());
			}
		}

		Assert (localFileState.IsCheckedIn () || localFileState.IsRelevantIn (Area::Original));

		// We are changing existing project file
		dbg << "Constructor 3 -- We are changing existing project file (local file data)" 
			<< std::endl;
		dbg << *_fileData;
		_state = _fileData->GetState ();
		UniqueName unameScript = scriptFileData.GetUniqueName ();
		if (!unameScript.IsEqual (_fileData->GetUniqueName ()))
		{
			// File names or parent ids different
			// Incomming rename/move or local rename/move
			if (scriptFileData.IsRenamedIn (Area::Original))
				VerifyRenameFile (scriptFileData);
		}
		if (!scriptFileData.GetType ().IsEqual (_fileData->GetType ()))
		{
			if (!scriptFileData.GetType ().IsRecoverable ())
			{
				// Incomming script makes file unrecoverable
				_fileData->SetType (scriptFileData.GetType ());
			}
		}
	}
	else
	{
		// Script creates brand new file/folder in the project -- add its FileData to the recipients project database
		_fileData = _dataBase.XAddForeignFile (scriptFileData);
		_state.Reset ();
		_fileData->SetState (_state);
		dbg << "Constructor 3 -- New file data added to the local database" << std::endl;
	}
}

void Transformer::Init (FileData const & fileData)
{
	_fileData->DeepCopy (fileData);
	_fileData->ClearAliases ();
	_fileData->SetState (_state);
}

void Transformer::ResolveNameConflict (PathFinder & pathFinder, TransactionFileList & fileList, bool isVirtual)
{
	if (isVirtual)
	{
		// Temporarily change file state to new
		Assert (!IsFolder ());
		_state.SetPresentIn (Area::Project, true);
		_state.SetRelevantIn (Area::Original, true);
		_state.SetPresentIn (Area::Original, false);
		_fileData->SetState (_state);
	}
	Assert (_state.IsNew ()							||	// Local add or local restore delete
			_fileData->IsRenamedIn (Area::Original) ||	// Local rename/move
			_state.IsSynchDelete ());					// Synch delete (directly or because of script conflict)
	GlobalId gid = _fileData->GetGlobalId ();
	XPhysicalFile file (gid, pathFinder, fileList);
	UniqueName nonConflictName (_fileData->GetUniqueName ());
	char const * projectPath = file.GetFullPath (Area::Project);
	std::string copyPath;
	do
	{
		// Create unique 'Previous <filename>' name
		copyPath = File::CreateUniqueName (projectPath, "previous");
		// Extract the 'Previous <filename>' name
		PathSplitter splitter (copyPath.c_str ());
		std::string newName = splitter.GetFileName ();
		newName += splitter.GetExtension ();
		nonConflictName.SetName (newName.c_str ());
		// Repeat this until 'Previous <filename>' is unique in the database.
		// If 'Previous <filename>' is not unique in the database this means
		// that we have already in the database file 'Previous <filename>'
		// but not yet on the disk in the project area -- file waits in the
		// staging area to be copied over to project area.
	} while (_dataBase.XIsUnique (gid, nonConflictName) != gidInvalid);

	if (!isVirtual)
	{
		UniqueName originalName (_fileData->GetUniqueName ());
		// If real name conflict resolve it on disk
		if (IsFolder ())
		{
			Assert (_state.IsNew ()			||	// Local new folder or local restore delete folder
					_state.IsSynchDelete ());	// Synch deletes folder (directly or because of script conflict)
			File::CreateNewFolder (copyPath.c_str (), false);	// Not quiet
			fileList.RememberCreated (copyPath.c_str (), true);
			ShellMan::CopyContents (0, projectPath, copyPath.c_str ());
			// Resolve name conflict in the database -- rename folder
			_fileData->Rename (nonConflictName);
		}
		else
		{
			XPhysicalFile file (gid, pathFinder, fileList);
			// Copy over to project also renames file in the database
			CopyOverToProject (Area::Project, nonConflictName, file);
		}

		// REVISIT: message box during transaction
		// Tell the user what have happened -- use project root relative paths
		Project::Path rootRelPath (_dataBase);
		std::string info (IsFolder () ? "Folder" : "File");
		info += " name conflict in project has been resolved\n";
		info += "by renaming the following local project ";
		if (IsFolder ())
			info += "folder";
		else
			info += "file";
		info += " from:\n\n";
		GlobalIdPack gidPack (gid);
		info += rootRelPath.MakePath (originalName);
		info += ' ';
		info += gidPack.ToBracketedString ();
		info += "\n\nto:\n\n";
		info += rootRelPath.MakePath (nonConflictName);
		TheOutput.Display (info.c_str ());
	}
	_state.SetResolvedNameConflict (true);
	_fileData->SetState (_state);
}

void Transformer::MakeProjectFolder ()
{
	Assert (IsFolder ());
	Assert (_state.IsNone ());
	FileState state;
	state.SetPresentIn (Area::Project, true);
	// Set the state of the folder to checked in.
	SetState (state);
	// Check parent etc.
	GidList parentPath;
	FileData const * parentData = _dataBase.XGetParentFileData (_fileData);
	FileState parentState = parentData->GetState ();
	FileType parentType = parentData->GetType ();
	while (!parentType.IsRoot ())
	{
		if (parentState.IsNone ())
			parentPath.push_back (parentData->GetGlobalId ());
		parentData = _dataBase.XGetParentFileData (parentData);
		parentState = parentData->GetState ();
		parentType = parentData->GetType ();
	}

	for (GidList::const_iterator iter = parentPath.begin (); iter != parentPath.end (); ++iter)
	{
		Transformer trans (_dataBase, *iter);
		trans.SetState (state);
	}
}

void Transformer::MakeCheckedIn (PathFinder & pathFinder,
								 TransactionFileList & fileList,
								 Area::Location areaFrom)
{
	// Check-in file without sending script. When do we need this strange operation?
	// During full synch unpack/prepare and version branch/export.
	_state.Reset ();	// Clear all state bits
	_state.SetPresentIn (Area::Project, true);
	XPhysicalFile file (_fileData->GetGlobalId (), pathFinder, fileList);
	if (!IsFolder ())
	{
		CheckSum checksum = file.GetCheckSum (areaFrom);
		_fileData->SetCheckSum (checksum);
		if (_fileData->IsRenamedIn (areaFrom))
		{
			// Temporarily rename file to the name in area from
			UniqueName const & historicalName = _fileData->GetUnameIn (areaFrom);
			_fileData->SetName (historicalName);
		}
		if (_fileData->IsTypeChangeIn (areaFrom))
		{
			// Temporarily change file type the type in area from
			FileType historicalType = _fileData->GetTypeIn (areaFrom);
			_fileData->SetType (historicalType);
		}
		file.MakeReadOnlyIn (Area::Project);
	}
	_fileData->SetState (_state);
	_fileData->ClearAliases ();
	dbg << "Transformer::MakeCheckedIn" << std::endl;
	dbg << _fileData->GetState () << std::endl;
}

void Transformer::MakeNotInProject (bool recursive)
{
	if (IsFolder () && recursive)
	{
		MakeNotInProjectFolderContents ();
	}
	_state.Reset ();
	_fileData->SetState (_state);
	dbg << "Transformer::MakeNotInProject" << std::endl;
	dbg << _fileData->GetState () << std::endl;
}

void Transformer::MakeNotInProjectFolderContents ()
{
	GidList contents;
	_dataBase.XListProjectFiles (_fileData->GetGlobalId (), contents);
	for (unsigned int i = 0; i < contents.size (); i++)
	{
		Transformer trans (_dataBase, contents [i]);
		trans.MakeNotInProject ();
	}
}

void Transformer::AddFile (PathFinder & pathFinder, TransactionFileList & fileList)
{
	Assert (_state.IsNone ());
	Assert (_fileData != 0);
	_state.SetPresentIn (Area::Project, true);
	_state.SetRelevantIn (Area::Original, true);
	if (IsFolder ())
	{
		XPhysicalFolder folder (_fileData->GetGlobalId (), pathFinder, fileList);
		// Create folder in Project Area
		folder.CreateFolder ();
	}
	else
	{
		Assert (!IsRoot ());
		XPhysicalFile file (_fileData->GetGlobalId (), pathFinder, fileList);
		file.MakeReadWriteIn (Area::Project);
	}
	_fileData->SetState (_state);
	dbg << "Transformer::AddFile" << std::endl;
	dbg << _fileData->GetState () << std::endl;
}

void Transformer::ReviveToBeDeleted ()
{
	_state.SetPresentIn (Area::Project, true);
	_state.SetCoDelete (false);
	_fileData->SetState (_state);
	dbg << "Transformer::ReviveToBeDeleted" << std::endl;
	dbg << _fileData->GetState () << std::endl;
}

bool Transformer::CheckOut (PathFinder & pathFinder,
							TransactionFileList & fileList,
							bool verifyChecksum,
							bool ignoreChecksum)
{
	Assert (_fileData != 0);
	if (_state.IsRelevantIn (Area::Original))
	{
		// File/folder already checked out -- do nothing
		return false;
	}

	XPhysicalFile file (_fileData->GetGlobalId (), pathFinder, fileList);
	if (!IsFolder () && verifyChecksum)
	{
		if (_fileData->GetCheckSum () != GetCheckSum (Area::Project, file))
		{
			if (ignoreChecksum)
			{
				// remove garbage bits if any
				_state.Reset();
				_state.SetPresentIn(Area::Project, true);
				_fileData->GetTypeEdit ().SetUnrecoverable (true);
			}
			else
			{
				throw Win::Exception ("Checksum mismatch: Run Repair from the Project menu",
									  file.GetFullPath (Area::Project));
			}
		}
	}
	if (_state.IsRelevantIn (Area::Synch))
	{
		// Pre-sync contains file before applying sync changes.
		// Use it as original and reference.
		Assert (!_state.IsPresentIn (Area::PreSynch) ||
			    File::IsReadOnly (file.GetFullPath (Area::PreSynch)));
		Copy (Area::PreSynch, Area::Reference, file);
		if (_state.IsPresentIn (Area::Synch))
		{
			// Edited by sync
			if (_state.IsPresentIn (Area::PreSynch))
				Copy (Area::PreSynch, Area::Original, file);
			else
				Copy (Area::Synch, Area::Original, file);
		}
		else 
		{
			// Deleted/removed by sync, user wants it back.
			// File/folder becomes new in the project
			_state.SetRelevantIn (Area::Original, true);
			if (IsFolder ())
			{
				// Folder deleted/removed from the project by the sync
				_state.SetPresentIn (Area::Project, true);
				if  (!File::Exists (file.GetFullPath (Area::Project)))
					throw Win::InternalException ("File doesn't exist", file.GetFullPath (Area::Project));
			}
			else
			{
				// File deleted/removed from the project by the sync
				if (_state.IsSoDelete ())
				{
					// File deleted from disk -- bring it back
					CopyToProject (Area::PreSynch, file, pathFinder, fileList);
				}
				else
				{
					// File removed from the project, but still present on disk
					_state.SetPresentIn (Area::Project, true);
					if  (!File::Exists (file.GetFullPath (Area::Project)))
						throw Win::InternalException ("File doesn't exist", file.GetFullPath (Area::Project));
				}
			}
			// If file/folder returns back to the project make sure
			// that file's/folder's parent also returns to the project
			Assert (_dataBase.XGetParentFileData (_fileData)->GetType ().IsRoot () ||
					_dataBase.XGetParentFileData (_fileData)->GetState ().IsPresentIn (Area::Project));
		}
	}
	else
	{
		Assert (_state.IsCheckedIn ());
		// No sync, just a simple check-out
		Copy (Area::Project, Area::Original, file);
		file.MakeReadOnlyIn (Area::Original);
	}

	// Make write able in project
	file.MakeReadWriteIn (Area::Project);
	_fileData->SetState (_state);
	dbg << "Transformer::CheckOut" << std::endl;
	dbg << _fileData->GetState () << std::endl;
	return true;
}

void Transformer::CheckOutIfNecessary (PathFinder & pathFinder, TransactionFileList & fileList)
{
	Assert (_fileData != 0);
	if (_state.IsPresentIn (Area::Project))
	{
		CheckOut (pathFinder, fileList);
	}
	else
	{
		_state.SetRelevantIn (Area::Original, true);
		char const * filePath = pathFinder.XGetFullPath (_fileData->GetGlobalId (), Area::Project);
		if (!File::Exists (filePath))
			_state.SetCoDelete (true);
		_fileData->SetState (_state);
	}
	dbg << "Transformer::CheckOutIfNecessary" << std::endl;
	dbg << _fileData->GetState () << std::endl;
}

//
// Create reference file for script conflict resolution. Such a file is marked as relevant
// in the Reference Area.
//
void Transformer::CreateRelevantReferenceIfNecessary (PathFinder & pathFinder, TransactionFileList & fileList)
{
	// File must have been checked-out -- user checked out file before executing script or there is script resolution
	Assert (_state.IsRelevantIn (Area::Original));
	Assert (_fileData != 0);

	if (_state.IsRelevantIn (Area::Reference))
		return;

	XPhysicalFile file (_fileData->GetGlobalId (), pathFinder, fileList);
	if (_state.IsRelevantIn (Area::Synch))
	{
		// Conflict resolution.
		// Use synch as reference.
		Copy (Area::Synch, Area::Reference, file);
	}
	else
	{
		// Local changes before script execution
		// Use original as reference.
		Copy (Area::Original, Area::Reference, file);
	}
	Assert (!_state.IsPresentIn (Area::Reference) ||
			_fileData->GetType ().IsFolder () ||
			File::IsReadOnly (file.GetFullPath (Area::Reference)));

	_fileData->SetState (_state);

	dbg << "Transformer::CreateCoReferenceIfNecessary" << std::endl;
	dbg << _fileData->GetState () << std::endl;
}

// A reference file that was not created for the purpose of undo doesn't have the r/o bit set
// but it might have the R bit set (if it's not a totally New file)
void Transformer::CreateUnrelevantReference (PathFinder & pathFinder, TransactionFileList & fileList)
{
	Assert (_fileData != 0);
	Assert (!_state.IsRelevantIn (Area::Reference));

	XPhysicalFile file (_fileData->GetGlobalId (), pathFinder, fileList);
	if (_state.IsRelevantIn (Area::Original))
	{
		Assert (!_state.IsPresentIn (Area::Original) || File::IsReadOnly (file.GetFullPath (Area::Original)));
		Copy (Area::Original, Area::Reference, file);
	}
	else
	{
		Copy (Area::Project, Area::Reference, file);
	}

	// Notice, it's not relevant!
	_state.SetRelevantIn (Area::Reference, false);

	_fileData->SetState (_state);

	dbg << "Transformer::CreateTmpReferenceIfNecessary" << std::endl;
	dbg << _fileData->GetState () << std::endl;
	Assert (!_state.IsRelevantIn (Area::Reference));
}

void Transformer::CleanupReferenceIfPossible (PathFinder & pathFinder, TransactionFileList & fileList)
{
	Assert (_fileData != 0);
	if (!_state.IsRelevantIn (Area::Reference))
	{
		dbg << "Transformer::CleanupReferenceIfPossible" << std::endl;
		XPhysicalFile file (_fileData->GetGlobalId (), pathFinder, fileList);
		DeleteFrom (Area::Reference, file);
		_fileData->SetState (_state);
		dbg << _fileData->GetState () << std::endl;
		_fileData->DumpAliases ();
	}
}

void Transformer::MakeInArea (Area::Location area)
{
	_state.SetRelevantIn (area, true);
	_state.SetPresentIn (area, true);
	_fileData->SetState (_state);
	dbg << "Transformer::MakeInArea" << std::endl;
	dbg << _fileData->GetState () << std::endl;
}

void Transformer::SetMergeConflict (bool flag)
{
	_state.SetMergeConflict (flag);
	_fileData->SetState (_state);
	dbg << "Transformer::SetMergeConflict" << std::endl;
	dbg << _fileData->GetState () << std::endl;
}

//
// Return true if file was physically copied
//
bool Transformer::CopyReference2Synch (PathFinder & pathFinder, TransactionFileList & fileList)
{
	Assert (_fileData != 0);
	if (_state.IsRelevantIn (Area::Synch))
		return false;

	XPhysicalFile file (_fileData->GetGlobalId (), pathFinder, fileList);
	Copy (Area::Reference, Area::Synch, file);
	Assert (!_state.IsPresentIn (Area::Synch) || File::IsReadOnly (file.GetFullPath (Area::Synch)));

	_fileData->SetState (_state);

	dbg << "Transformer::Copy2Synch" << std::endl;
	dbg << _fileData->GetState () << std::endl;
	return _state.IsPresentIn (Area::Synch);
}

void Transformer::MoveReference2Project (PathFinder & pathFinder, TransactionFileList & fileList)
{
	Assert (_fileData != 0);
	if (!_state.IsRelevantIn (Area::Reference))
		return;

	XPhysicalFile file (_fileData->GetGlobalId (), pathFinder, fileList);
	CopyToProject (Area::Reference, file, pathFinder, fileList);
	DeleteFrom (Area::Reference, file);
	_state.SetRelevantIn (Area::Reference, false);
	_state.SetCoDelete (false);

	if (!_state.IsPresentIn (Area::Project) && !_state.IsPresentIn (Area::Original))
	{
		// File/folder was reverted to the state before add to the project -- none
		// Revisit: User will be surprised, maybe we should tell him something?
		_state.SetRelevantIn (Area::Original, false);
		Assert (_state.IsNone ());
	}

	_fileData->SetState (_state);
	dbg << "Transformer::MoveReference2Project" << std::endl;
	dbg << _fileData->GetState () << std::endl;
}

void Transformer::Merge (PathFinder & pathFinder,
						 TransactionFileList & fileList,
						 SynchArea & synchArea,
						 Progress::Meter & meter)
{
	Assert (_fileData != 0);

	if (IsFolder ())
		MergeFolder (pathFinder, fileList);
	else
		MergeFile (pathFinder, fileList, synchArea, meter);

	if (_state.IsResolvedNameConflict ())
		synchArea.XUpdateName (_fileData->GetGlobalId (), _fileData->GetName ());

	_state.SetResolvedNameConflict (false);
	_fileData->SetState (_state);
}

bool Transformer::Uncheckout (PathFinder & pathFinder, TransactionFileList & fileList, bool isVirtual)
{
	Assert (_fileData != 0);
	if (!IsCheckedOut ())
		return false;

	if (IsFolder () && !_state.IsNew ())
	{
		// Folder uncheckout
		Assume (!_fileData->IsRenamedIn (Area::Original), GlobalIdPack (_fileData->GetGlobalId ()).ToBracketedString ().c_str ());
	}
	FileData const * parentData;
	if (_fileData->IsRenamedIn (Area::Original))
		parentData = _dataBase.XGetFileDataByGid (_fileData->GetUnameIn (Area::Original).GetParentId ());
	else
		parentData = _dataBase.XGetParentFileData (_fileData);
	FileState parentState = parentData->GetState ();
	// The parent folder might have already been unchecked-out in this transaction, so its state may be None
	Assume (parentState.IsCheckedIn () ||
			parentState.IsRelevantIn (Area::Original) ||
			parentState.IsNone (), ::ToHexString (parentState.GetValue ()).c_str ());
	Assume (isVirtual || parentState.IsNew () || File::Exists (pathFinder.XGetFullPath (parentData->GetUniqueName ())),
			GlobalIdPack (parentData->GetGlobalId ()).ToBracketedString ().c_str ());

	XPhysicalFile file (_fileData->GetGlobalId (), pathFinder, fileList);
	if (_state.IsRelevantIn (Area::Synch))
	{
		// Uncheckout in the presence of sync changes.
		if (_fileData->GetCheckSum () == GetCheckSum (Area::Synch, file))
		{
			// If sync area copy is not corrupted then
			// copy whatever sync proposed to project area.
			CopyToProject (Area::Synch, file, pathFinder, fileList);
		}
		// During sync, pre-sync area contains file before applying sync changes
		Copy (Area::Reference, Area::PreSynch, file);
		DeleteFrom (Area::Reference, file);
		_state.SetRelevantIn (Area::Reference, false);
	}
	else // Uncheckout without pending sync
	{
		// Copy original to project
		if (!_state.IsPresentIn (Area::Project) && !IsRecoverable ())
		{
			// Uncheckout of deleted file
			// If this was forced delete (deleting file with checksum mismatch)
			// restore file type.  File still has checksum mismatch, and we
			// just un-checkout forced delete.
			_fileData->GetTypeEdit ().SetUnrecoverable (false);
		}
		CopyToProject (Area::Original, file, pathFinder, fileList);
		_fileData->ClearAliases ();
	}

	if (_state.IsPresentIn (Area::Project) && !IsFolder ())
		file.MakeReadOnlyIn (Area::Project);

	DeleteFrom (Area::Original, file);
	_state.SetRelevantIn (Area::Original, false);
	_state.SetCoDelete (false);
	_state.SetMerge (false);
	_state.SetMergeConflict (false);
	_state.SetResolvedNameConflict (false);
	_fileData->SetState (_state);
	dbg << "Transformer::Uncheckout" << std::endl;
	dbg << _fileData->GetState () << std::endl;
	return true;
}

bool Transformer::DeleteFile (PathFinder & pathFinder, TransactionFileList & fileList, bool doDelete)
{
	Assert (_fileData != 0);
	if (_state.IsToBeDeleted () && _state.IsCoDelete () == doDelete)
		return false;

	XPhysicalFile file (_fileData->GetGlobalId (), pathFinder, fileList);
	if (_state.IsCheckedIn ())
		CheckOut (pathFinder, fileList, true, true); // Verify checksum, but ignore mismatch.
													 // Mark the deleted file as unrecoverable

	if (doDelete)
	{
		Assert (!IsFolder ());
		file.DeleteFrom (Area::Project);
		file.TouchIn (Area::Original);
	}

	_state.SetPresentIn (Area::Project, false);
	if (!_state.IsPresentIn (Area::Original) &&
		 _state.IsRelevantIn (Area::Original) &&
		!_state.IsRelevantIn (Area::Reference))
	{
		// File/folder is not present in the Original area but is relevant there and is not relevant in
		// refrence area (is not restored or taking part in script conflict).
		// We are deleting new file/folder -- clear the relevant bit in the Original area,
		// because deleting new file/folder is the same as deleting non-project file/folder
		_state.SetRelevantIn (Area::Original, false);
		Assert (_state.IsNone ());
	}
	else
	{
		_state.SetCoDelete (doDelete);
	}
	_fileData->SetState (_state);

	dbg << "Transformer::DeleteFile" << std::endl;
	dbg << _fileData->GetState () << std::endl;
	return true;
}

void Transformer::MoveFile (PathFinder & pathFinder, 
							TransactionFileList & fileList, 
							UniqueName const & newName)
{
	Assert (_fileData != 0);
	if (IsFolder ())
		return; // Revisit when implementing folder RENAME/MOVE

	CheckOut (pathFinder, fileList);
	Assert (_state.IsPresentIn (Area::Project));
	XPhysicalFile file (_fileData->GetGlobalId (), pathFinder, fileList);
	CopyOverToProject (Area::Project, newName, file);
	Assert (_dataBase.XGetParentFileData (_fileData)->GetState ().IsPresentIn (Area::Project));
	// Make file under New name read write
	file.MakeReadWriteIn (Area::Project);
	dbg << "Transformer::MoveFile" << std::endl;
	dbg << _fileData->GetState () << std::endl;
	_fileData->DumpAliases ();
}

void Transformer::ChangeFileType (PathFinder & pathFinder, 
								  TransactionFileList & fileList,
								  const FileType & fileType)
{
	Assert (_fileData != 0);
	CheckOut (pathFinder, fileList);
	_fileData->ChangeType (fileType);
}

bool Transformer::UnpackDelete (PathFinder & pathFinder,
								TransactionFileList & fileList,
								Area::Location area,
								bool doDelete)
{
	Assert (_fileData != 0);
	Assert (area == Area::Synch || area == Area::Reference);
	XPhysicalFile file (_fileData->GetGlobalId (), pathFinder, fileList);
	DeleteFrom (area, file);
	_state.SetRelevantIn (area, true);
	if (area == Area::Synch)
	{
		_state.SetSoDelete (doDelete);
		if (!doDelete)
			file.MakeReadWriteIn (Area::Project);
	}
	_fileData->SetState (_state);
	if (doDelete)
		_fileData->SetCheckSum (CheckSum ());

	dbg << "Transformer::UnpackDelete" << std::endl;
	dbg << _fileData->GetState () << std::endl;
	return true;
}

bool Transformer::UnpackNew (Area::Location targetArea)
{
	Assert (_fileData != 0);
	Assert (targetArea == Area::Synch || targetArea == Area::Reference);
	_state.SetRelevantIn (targetArea, true);
	_state.SetPresentIn (targetArea, true);
	_fileData->SetState (_state);
	dbg << "Transformer::UnpackNew" << std::endl;
	dbg << _fileData->GetState () << std::endl;
	return true;
}

bool Transformer::CheckIn (PathFinder & pathFinder, 
						   TransactionFileList & fileList, 
						   CommandList & cmdList)
{
	Assert (_fileData != 0);
	if (!IsCheckedOut ())
		return false;

	FileData const * parentData = _dataBase.XGetParentFileData (_fileData);
	FileState parentState = parentData->GetState ();
	Assume (!parentState.IsNew (), ::ToHexString (parentState.GetValue ()).c_str ());
	if (!parentState.IsPresentIn (Area::Project))
	{
		// Parent is not present in the project -- this is OK only when
		// this file/folder is to deleted or removed from the project.
		// Otherwise something really bad happened.
		if (_state.IsPresentIn (Area::Project))
		{
			GlobalIdPack pack (_fileData->GetGlobalId ());
			std::string info;
			if (_fileData->GetType ().IsFolder ())
				info += "Folder";
			else
				info += "File";
			info += ' ';
			info += _fileData->GetName ();
			info += ' ';
			info += pack.ToBracketedString (); 
			info += " cannot be checked in, because its\nparent folder ";
			GlobalIdPack pack1 (parentData->GetGlobalId ());
			info += parentData->GetName ();
			info += ' ';
			info += pack1.ToBracketedString () ;
			info += " has been removed from the project.";
			throw Win::Exception ("Illegal check-in operation.", info.c_str ());
		}
	}

	if (IsFolder ())
		CheckInFolder (pathFinder, fileList, cmdList);
	else
		CheckInFile (pathFinder, fileList, cmdList);

	return true;
}

void Transformer::AcceptSynch (PathFinder & pathFinder, TransactionFileList & fileList, SynchArea & synchArea)
{
	dbg << "Transformer::AcceptSynch" << std::endl;
	dbg << _fileData->GetState () << std::endl;

	Assert (_fileData != 0);
	if (!_state.IsRelevantIn (Area::Synch))
	{
		dbg << "* Removing irrelevant file form sync area" << std::endl;
		synchArea.XRemoveSynchFile (_fileData->GetGlobalId ());
		return;
	}

	XPhysicalFile file (_fileData->GetGlobalId (), pathFinder, fileList);
	if (_state.IsRelevantIn (Area::Original))
	{
		dbg << "File: " << _fileData->GetName() << " is checked out." << std::endl;
		if (_fileData->GetCheckSum () != GetCheckSum (Area::Synch, file))
		{
			if (!file.ExistsInLocalEdits ())
			{
				// Make a backup copy
				file.Copy (Area::Project, Area::LocalEdits);
			}
			throw Win::Exception ("Checksum mismatch in the sync area.\n\n"
				"Please un-checkout the file\n"
				"and run Repair from the Project menu.",
				file.GetFullPath (Area::Project));
	
		}
		// Synch -> Original
		Copy (Area::Synch, Area::Original, file);
		file.MakeReadOnlyIn (Area::Original);
	}
	else
	{
		dbg << "File: " << _fileData->GetName() << " is not checked out." << std::endl;
		// After accepting synch all files not checked out
		// have to have read-only attribute set
		if (!IsFolder () && _state.IsPresentIn (Area::Project))
			file.MakeReadOnlyIn (Area::Project);
		_fileData->ClearAliases ();
	}
	_state.SetRelevantIn (Area::Reference, false);
	DeleteFrom (Area::Reference, file);
	_state.SetRelevantIn (Area::Synch, false);
	DeleteFrom (Area::Synch, file);
	_state.SetRelevantIn (Area::PreSynch, false);
	DeleteFrom (Area::PreSynch, file);
	_state.SetSoDelete (false);
	_state.SetMerge (false);
	synchArea.XRemoveSynchFile (_fileData->GetGlobalId ());
	_fileData->SetState (_state);

	// If after accepting synch changes the file is in state New
	// it was preemptively checked out during unpacking synch script.
	// Check if the file type is unrecoverable and change it appropriatelly.
	if (!IsFolder () && _state.IsNew ())
	{
		dbg << "The file state is NEW" << std::endl;
		if (!IsRecoverable ())
		{
			_fileData->GetTypeEdit ().SetUnrecoverable (false);
		}
	}
	dbg << _fileData->GetState () << std::endl;
}

void Transformer::RemoveFromReference ()
{
	_state.SetPresentIn (Area::Reference, false);
	_fileData->SetState (_state);
	dbg << "Transformer::RemoveFromReference" << std::endl;
	dbg << _fileData->GetState () << std::endl;
}

void Transformer::CopyToReference (PathFinder & pathFinder, TransactionFileList & fileList)
{
	_state.SetPresentIn (Area::Reference, true);
	// Revisit: do we really need to remeber created file in the Reference Area ?
	char const * refPath = pathFinder.XGetFullPath (_fileData->GetGlobalId (), Area::Reference);
	fileList.RememberCreated (refPath, IsFolder ());
	_fileData->SetState (_state);
	dbg << "Transformer::CopyToReference" << std::endl;
	dbg << _fileData->GetState () << std::endl;
}

void Transformer::UndoRename (UniqueName const & originalName)
{
	_fileData->AddUnameAlias (originalName, Area::Reference);
	dbg << "Transformer::UndoRename" << std::endl;
	dbg << _fileData->GetState () << std::endl;
	_fileData->DumpAliases ();
}

void Transformer::UndoChangeType (FileType const & oldType)
{
	_fileData->AddTypeAlias (oldType, Area::Reference);
	dbg << "Transformer::UndoChangeType" << std::endl;
	dbg << _fileData->GetState () << std::endl;
	_fileData->DumpAliases ();
}

//
// Helper methods
//

void Transformer::MergeFolder (PathFinder & pathFinder, TransactionFileList & fileList)
{
	Assert (_state.IsRelevantIn (Area::Synch));

	XPhysicalFolder folder (_fileData->GetGlobalId (), pathFinder, fileList);
	Copy (Area::Project, Area::PreSynch, folder);
	// Update state with backup bits set, so later rename operation
	// will propagate aliases correctly
	_fileData->SetState (_state);
	if (_state.IsPresentIn (Area::Synch))
	{
		// Script adds folder to the project
		if (!_state.IsRelevantIn (Area::Original))
		{
			// Create folder in Project Area
			folder.CreateFolder ();
			_state.SetPresentIn (Area::Project, true);
		}
		else
		{
			if (!_state.IsPresentIn (Area::Original))
			{
				// Unpacking script caused undo of local folder deletion
				_state.SetPresentIn (Area::Original, true);
				// Creating folder is idempotent, so no need to check if folder already exists
				folder.CreateFolder ();
			}
		}
	}
	else
	{
		// Script deletes or removes folder from the project
		if (_state.IsRelevantIn (Area::Original))
		{
			// Folder is checked out -- Merge
			if (_state.IsPresentIn (Area::Project))
			{
				// User wants it back -- make it a restored new folder in the project
				_state.SetPresentIn (Area::Original, false);
				if (_state.IsPresentIn (Area::Project) && _state.IsPresentIn (Area::Reference))
					_state.SetMerge (true);	// Synch delete contra local changes
			}
			else
			{
				// User also removed/deleted folder -- clear merge
				_state.SetRelevantIn (Area::Original, false);
				_state.SetPresentIn (Area::Original, false);
			}
		}
		else
		{
			// Folder is not checked out -- check if something from its contents is checked out
			if (_dataBase.XFindInFolder (_fileData->GetGlobalId (), ::IsCheckedOut))
			{
				// Folder contains checked out files/folders.
				// Since user wants back something from the removed folder,
				// the folder has to be merged too.
				// Merge this folder and parent path if removed by synch.
				_state.SetPresentIn (Area::Original, false);
				_state.SetRelevantIn (Area::Original, true);
				// Now walk parent path up to the project root and find
				// every folder removed or deleted by synch
				GidList parentPath;
				FileData const * parentData = _dataBase.XGetParentFileData (_fileData);
				FileState parentState = parentData->GetState ();
				while (parentState.IsSynchDelete ())
				{
					parentPath.push_back (parentData->GetGlobalId ());
					parentData = _dataBase.XGetParentFileData (parentData);
					parentState = parentData->GetState ();
				}
				// Merge folders -- don't use transformer, because this
				// it too costly -- we just want to flip couple of state bits
				for (GidList::const_iterator iter = parentPath.begin ();
					 iter != parentPath.end ();
					 ++iter)
				{
					FileData * fd = _dataBase.XGetEdit (*iter);
					FileState state = fd->GetState ();
					state.SetPresentIn (Area::Project, true);	// Back to the project
					state.SetPresentIn (Area::Original, false);	// After accept folder has
					state.SetRelevantIn (Area::Original, true);	// to be in state new
					fd->SetState (_state);
				}
			}
			else
			{
				// Just regular folder-remove or folder-delete
				_state.SetPresentIn (Area::Project, false);
			}
		}
	}
	_fileData->SetState (_state);
	dbg << "Transformer::MergeFolder" << std::endl;
	dbg << _fileData->GetState () << std::endl;
}

void Transformer::MergeFile (PathFinder & pathFinder,
							 TransactionFileList & fileList,
							 SynchArea const & synchArea,
							 Progress::Meter & meter)
{
	Assert (_state.IsRelevantIn (Area::Synch));

	XPhysicalFile file (_fileData->GetGlobalId (), pathFinder, fileList);
	Copy (Area::Project, Area::PreSynch, file);
	if (_state.IsRelevantIn (Area::Original))
		file.MakeReadOnlyIn (Area::PreSynch);	// Make sure that area copy is read-only

	// Update state with backup bits set, so later rename operation
	// will propagate aliases correctly
	_fileData->SetState (_state);
	CheckSum newCheckSum = synchArea.XGetCheckSum (_fileData->GetGlobalId ());
	if (newCheckSum != GetCheckSum (Area::Synch, file))
	{
		throw Win::Exception ("Checksum mismatch in the Synch Area: Run Repair from the Project menu",
			file.GetFullPath (Area::Project));
	}
	if (_state.IsPresentIn (Area::Synch))
	{
		// Script will propagate some edit changes or before accepting
		// script changes we had to undo local file deletion

		if (_state.IsRelevantIn (Area::Original))
		{
			// Checked out
			if (_state.IsPresentIn (Area::Original))
			{
				// File was changed (edited, renamed, moved or deleted) before or during
				// the unpacking of the script
				if (_state.IsPresentIn (Area::Project))
				{
					MergeContents (file, fileList, pathFinder, meter);
				}
				else
				{
					// Merge with local delete
					_state.SetMerge (true);
				}
			}
			else if (_state.IsPresentIn (Area::Reference))
			{
				// Unpacking script caused undo of local file deletion
				// Local deletion wins, but the user can still uncheckout
				Copy (Area::Reference, Area::Original, file);
				if (file.IsDifferent (Area::Reference, Area::Synch))
					_state.SetMerge (true);	// Local delete contra synch changes
			}
			else
			{
				// File was locally restored and synch also restores it.
				Assert (_state.IsNew ());
				if (file.IsContentsDifferent (Area::Project, Area::Synch))
				{
					// Restored file was edited -- localy or by synch or both.
					_state.SetMerge (true);
				}
			}
		}
		else // not checked out
		{
			CopyToProject (Area::Synch, file, pathFinder, fileList);
			file.MakeReadOnlyIn (Area::Project);
		}
	}
	else // absent from Synch
	{
		if (_state.IsRelevantIn (Area::Original)) // merge
		{
			// Script deletes or removes localy changed file
			if (_state.IsPresentIn (Area::Project))
			{
				// User wants it back -- make it a restored new file in the project
				_state.SetPresentIn (Area::Original, false);
			}
			else
			{
				// Synch deletes file that user also deleted
				Assert (!_state.IsPresentIn (Area::PreSynch));
				Copy (Area::Original, Area::PreSynch, file);
				_state.SetRelevantIn (Area::Original, false);
				if (_state.IsCoDelete () && !_state.IsSoDelete ())
				{
					// Synch removes file while localy file was deleted
					// Leave file copy in the project area
					CopyToProject (Area::PreSynch, file, pathFinder, fileList);
					_state.SetPresentIn (Area::Project, false);	// After synch file is still not in the project
				}
				_state.SetCoDelete (false);
			}
			DeleteFrom (Area::Original, file);
			if (_state.IsPresentIn (Area::Project) && _state.IsPresentIn (Area::Reference))
				_state.SetMerge (true);	// Synch delete contra local changes
		}
		else // no merge
		{
			// Script deletes or removes file from the project
			if (_state.IsSoDelete ())
			{
				// Really delete file from project
				file.DeleteFrom (Area::Project);
			}
			_state.SetPresentIn (Area::Project, false);
		}
	}
	// Checksum always describes the Original file
	_fileData->SetCheckSum (newCheckSum);
	_fileData->SetState (_state);

	dbg << "Transformer::MergeFile" << std::endl;
	dbg << _fileData->GetState () << std::endl;
	_fileData->DumpAliases ();
}

void Transformer::CheckInFolder (PathFinder & pathFinder, TransactionFileList & fileList, CommandList & cmdList)
{
	Assert (!_state.IsNone ());
	XPhysicalFolder folder (_fileData->GetGlobalId (), pathFinder, fileList);
	if (_state.IsPresentIn (Area::Project))
	{
		if (!_state.IsPresentIn (Area::Original))
		{
			// New folder has been added to the project
			folder.MakeNewFolderCmd (*_fileData, cmdList);
		}
	}
	else
	{
		Assert (_state.IsRelevantIn (Area::Original));
		folder.MakeDeleteFolderCmd (*_fileData, cmdList);
		_state.SetCoDelete (false);
	}
	_state.SetPresentIn (Area::Original, false);
	_state.SetRelevantIn (Area::Original, false);
	_state.SetResolvedNameConflict (false);
	_fileData->SetState (_state);
	dbg << "Transformer::CheckInFolder" << std::endl;
	dbg << _fileData->GetState () << std::endl;
}

void Transformer::CheckInFile (PathFinder & pathFinder, TransactionFileList & fileList, CommandList & cmdList)
{
	Assert (!_state.IsRelevantIn (Area::Synch));
	XPhysicalFile file (_fileData->GetGlobalId (), pathFinder, fileList);
	if (_state.IsPresentIn (Area::Project))
	{
		if (!_fileData->GetType ().IsRecoverable ())
		{
			throw Win::InternalException ("You cannot check in an unrecoverable file", 
				_fileData->GetName ().c_str ());
		}
		CheckSum checkSum;
		if (_state.IsPresentIn (Area::Original))
		{
			if (file.MakeDiffScriptCmd (*_fileData, cmdList, checkSum))
				_fileData->SetCheckSum (checkSum);
		}
		else
		{
			file.MakeNewScriptCmd (*_fileData, cmdList, checkSum);
			_fileData->SetCheckSum (checkSum);
		}
		file.MakeReadOnlyIn (Area::Project);
	}
	else
	{
		if (_state.IsPresentIn (Area::Original))
		{
			if (_fileData->IsRenamedIn (Area::Original) ||_fileData->IsTypeChangeIn (Area::Original))
			{
				// File has been renamed or moved or its type is chanched
				// When deleting use its original name or location and type
				FileData original (*_fileData);
				original.ClearAliases ();
				if (_fileData->IsRenamedIn (Area::Original))
				{
					UniqueName const & originalName = _fileData->GetUnameIn (Area::Original);
					original.Rename (originalName);
				}
				if (_fileData->IsTypeChangeIn (Area::Original))
				{
					FileType const  orgType = _fileData->GetTypeIn (Area::Original);
					original.ChangeType (orgType);
				}
				
				file.MakeDeletedScriptCmd (original, cmdList);
			}
			else
			{
				file.MakeDeletedScriptCmd (*_fileData, cmdList);
			}
			_state.SetCoDelete (false);
			_fileData->SetCheckSum (CheckSum ());
		}
	}

	_fileData->ClearAliases ();
	DeleteFrom (Area::Original, file);
	_state.SetRelevantIn (Area::Original, false);
	_state.SetResolvedNameConflict (false);
	_fileData->SetState (_state);
	dbg << "Transformer::CheckInFile" << std::endl;
	dbg << *_fileData;
}

UniqueName const & Transformer::GetOriginalUname () const
{
	if (_state.IsRelevantIn (Area::Synch))
	{
		// Changes to local file were undone because of script conflict.
		// If local file has alias in reference area this is its original unique name
		// otherwise check if file has alias in original area and use it if present
		if (_fileData->IsRenamedIn (Area::Reference))
			return _fileData->GetUnameIn (Area::Reference);
		else if (_fileData->IsRenamedIn (Area::Original))
			return _fileData->GetUnameIn (Area::Original);
	}
	else // absent from Synch
	{
		// If local file has alias in original area this is its original unique name
		if (_fileData->IsRenamedIn (Area::Original))
			return _fileData->GetUnameIn (Area::Original);
	}
	// If not renamed anywhere use the current unique file name.
	return _fileData->GetUniqueName ();
}

void Transformer::VerifyRenameFile (FileData const & scriptFileData) const
{
	// File reanme is possible if both parties (the script sender and
	// the script recipient) start the renaming from the same old unique file name.
	Assert (scriptFileData.IsRenamedIn (Area::Original));
	// Get expected old unique name from script sender
	UniqueName const & expectedOldUname = scriptFileData.GetUnameIn (Area::Original);
	// Get old unique name from script recipient
	UniqueName const & currentOldUname = GetOriginalUname ();
	// Make sure that we are refering to the same file
	if (!IsCaseEqual (expectedOldUname.GetName (), currentOldUname.GetName ()))
	{
		// If file names differ only in case then accept rename -- bug in version 3.0
		// didn't propagate case only renames.
		if (!IsNocaseEqual (expectedOldUname.GetName (), currentOldUname.GetName ()))
		{
			std::string info ("Illegal ");
			info += (expectedOldUname.GetParentId () != currentOldUname.GetParentId () ? "MOVE" : "RENAME");
			info += ": original names different\n\n"
					"The synchronization script expects file '";
			info += expectedOldUname.GetName ();
			info += "'\nwhile in this project the same file is named '";
			info += currentOldUname.GetName ();
			info += "'\n\nDefect from the project and enlist again.";
			TheOutput.Display (info.c_str ());
			throw Win::Exception ();
		}
	}
	// Make sure that move starts in the same folder
	if (expectedOldUname.GetParentId () != currentOldUname.GetParentId ())
	{
		FileData const * localFolder = _dataBase.XGetFileDataByGid (currentOldUname.GetParentId ());
		FileData const * scriptFolder = _dataBase.XGetFileDataByGid (expectedOldUname.GetParentId ());
		char const * localFolderName = localFolder->GetType ().IsRoot () ? "Project Root Folder"
																		   : localFolder->GetName ().c_str ();
		char const * scriptFolderName;
		if (scriptFolder == 0)
			scriptFolderName = "Unknown";
		else
			scriptFolderName = scriptFolder->GetType ().IsRoot () ? "Project Root Folder" : scriptFolder->GetName ().c_str ();
		std::string info ("Illegal MOVE: original folders different\n\n"
						  "The synchronization script wants to move the file '");
		info += currentOldUname.GetName ();
		info += "' from folder '";
		info += localFolderName;
		info += "'\nwhile in this project this file is present in the folder: '";
		info += scriptFolderName;
		info += "'\n\nDefect from the project and enlist again.";
		TheOutput.Display (info.c_str ());
		throw Win::Exception ();
	}
	dbg << "Transformer::Verify Rename" << std::endl;
	dbg << _fileData->GetState () << std::endl;
	_fileData->DumpAliases ();
}

