//----------------------------------
// Reliable Software (c) 1998 - 2007
//----------------------------------

#include "precompiled.h"
#include "Transformer.h"
#include "PhysicalFile.h"
#include "Merge.h"
#include "Diff.h"
#include "DataBase.h"
#include "PathFind.h"
#include "FileList.h"
#include "OutputSink.h"
#include "FileChanges.h"

#include <Ctrl/ProgressMeter.h>
#include <Ex/WinEx.h>
#include <File/MemFile.h>

// When we have real merge conflict?
//
//                              Local Change
//				Edit    Move    Rename    New    Delete    Type
//            +------+-------+---------+------+---------+-------+
//   S Edit   |   M  |       |         |      |    M    |       |
//   y        +------+-------+---------+------+---------+-------+
//   n Move   |      |   M   |    M    |      |    M    |       |
//   c        +------+-------+---------+------+---------+-------+
//   h Rename |      |   M   |    M    |      |    M    |       |
//            +------+-------+---------+------+---------+-------+
//   C New    |      |       |         |   M  |         |       |
//   h        +------+-------+---------+------+---------+-------+
//   a Delete |   M  |   M   |    M    |      |         |   M   |
//   n        +------+-------+---------+------+---------+-------+
//   g  Type  |      |       |         |      |    M    |   M   |
//   e        +-------------------------------------------------+
//

void Transformer::MergeContents (XPhysicalFile & file,
								 TransactionFileList & fileList,
								 PathFinder & pathFinder,
								 Progress::Meter & meter)
{
	Assert (_state.IsRelevantIn (Area::Original));
	Assert (_state.IsPresentIn (Area::Original));
	Assert (_state.IsPresentIn (Area::Project));
	Assert (_state.IsPresentIn (Area::Synch));
	FileChanges fileChanges;
	_fileData->GetNonContentsFileChanges (fileChanges);
	bool onlySynchRename = (fileChanges.IsSynchMove () || fileChanges.IsSynchRename ()) &&
						   !(fileChanges.IsLocalMove () || fileChanges.IsLocalRename ());
	bool onlySynchTypeChange = fileChanges.IsSynchTypeChange () && !fileChanges.IsLocalTypeChange ();
	if (!file.IsContentsDifferent (Area::Synch, Area::Reference))
	{
		// Synch doesn't edit file
		if (onlySynchRename)
		{
			// Note: we are copying file from the project area under old name
			// back to the project area under new name, because file is locally
			// edited by the user and we don't want to lose local changes during
			// synch rename.
			UniqueName newUname (_fileData->GetUnameIn (Area::Synch));
			CopyOverToProject (Area::Project, newUname, file);
		}
	}
	else if (!file.IsContentsDifferent (Area::Project, Area::Reference))
	{
		// User just checked out, but didn't edit it. Sync edited contents.
		if (onlySynchRename)
		{
			UniqueName newUname (_fileData->GetUnameIn (Area::Synch));
			CopyOverToProject (Area::Synch, newUname, file);
		}
		else
		{
			// Sync didn't rename file OR both renamed file
			// Propagate synch edit changes but keep user name
			file.DeleteFrom (Area::Project);
			file.CopyOverToProject (Area::Synch, true);
		}
	}
	else
	{
		// Both user and synch edited the file or user didn't edit the file
		// but there was a script conflict that rolled back changes already recorded.
		_state.SetMerge (true);
		_state.SetMergeConflict (true);
		if (_fileData->GetType ().IsTextual ())
		{
			// For textual files we have to merge changes - this is done after transaction either by our
			// own automatic merger or by the alternative (external) merger.
			if (onlySynchRename)
			{
				// Note: we are copying file from the project area under old name
				// back to the project area under new name, because file is locally
				// edited by the user and we don't want to lose local changes during
				// synch rename.
				UniqueName newUname (_fileData->GetUnameIn (Area::Synch));
				CopyOverToProject (Area::Project, newUname, file);
			}
			// Else (synch and locally renamed) do nothing - we want to keep user name
		}
		else
		{
			Assert (_fileData->GetType ().IsBinary ());
			// We don't know how to merge binary files, so we
			// just preserve local user edits by making a copy
			// of the binary file in the project area.
			// Remember current project path - can change if sync remanes file.
			std::string originalProjectPath (file.GetFullPath (Area::Project));
			if (onlySynchRename)
			{
				// Copy synch version to the project under a new name
				UniqueName newUname (_fileData->GetUnameIn (Area::Synch));
				CopyOverToProject (Area::Synch, newUname, file);
			}
			else
			{
				// User also renamed file or no renames -- propagate synch edit changes
				// but keep user name.
				file.DeleteFrom (Area::Project);
				file.CopyOverToProject (Area::Synch, true);
			}
			// Always create pre-merge using official file name.
			// Note: if file was renamed by sync then the target path will reflect this 
			char const * targetPath = file.GetFullPath (Area::Project);
			std::string toPath = File::CreateUniqueName (targetPath, "pre-merge");
			Assert (!toPath.empty ());
			File::Copy (originalProjectPath.c_str (), toPath.c_str ());
			fileList.RememberCreated (toPath.c_str (), false);
		}
	}

	if (onlySynchTypeChange)
	{
		FileType const newType = _fileData->GetTypeIn (Area::Synch);
		_fileData->ChangeType (newType);
	}

	if (!_state.IsMerge ())
	{
		// Check for non-contents merge conflicts
		bool localNameChange = fileChanges.IsLocalMove () || fileChanges.IsLocalRename ();
		bool synchNameChange = fileChanges.IsSynchMove () || fileChanges.IsSynchRename ();
		bool merge = (localNameChange && synchNameChange) ||
					 (fileChanges.IsLocalTypeChange () && fileChanges.IsSynchTypeChange ());
		_state.SetMerge (merge);
	}
}

// Primitives

void Transformer::CopyOverToProject (Area::Location fromArea, UniqueName const & newName, XPhysicalFile & file)
{
	Assert (_fileData != 0);
	// Remove file copy under old name from project area
	file.DeleteFrom (Area::Project);
	file.CopyOverToProject (fromArea, newName);
	_fileData->Rename (newName);
}

void Transformer::RenameIn (Area::Location loc, UniqueName const & uname)
{
	// Revisit: add special case for Area::Project
	Assert (loc != Area::Project);
	Assert (_fileData->GetState ().IsRelevantIn (loc));
	Assert (_fileData->GetState ().IsPresentIn (loc));
	_fileData->AddUnameAlias (uname, loc);
}
void Transformer::ChangeTypeIn (Area::Location loc, FileType const & type)
{
	Assert (loc != Area::Project);
	Assert (_fileData->GetState ().IsRelevantIn (loc));
	Assert (_fileData->GetState ().IsPresentIn (loc));
	_fileData->AddTypeAlias (type, loc);
}

void Transformer::DeleteFrom (Area::Location loc, XPhysicalFile & file)
{
	Assert (loc != Area::Project);
	if (!IsFolder ())
		file.DeleteFrom (loc);
	_fileData->RemoveAlias (loc);
	_state.SetPresentIn (loc, false);
}

CheckSum Transformer::GetCheckSum (Area::Location loc, XPhysicalFile & file)
{
	if (_state.IsPresentIn (loc))
		return file.GetCheckSum (loc);
	else
		return CheckSum ();
}

void Transformer::Copy (Area::Location fromArea, Area::Location toArea, XPhysicalFile & file)
{
	Assert (toArea != Area::Project);

	_state.SetRelevantIn (toArea, true);

	if (_state.IsPresentIn (fromArea))
	{
		_state.SetPresentIn (toArea, true);
		if (!IsFolder ())
		{
			if (_state.IsResolvedNameConflict () && fromArea == Area::Project)
			{
				Assert (_state.IsPresentIn (Area::Original) && _fileData->IsRenamedIn (Area::Original));
				UniqueName const & originalUname = _fileData->GetUnameIn (Area::Original);
				file.CopyRemember (originalUname, toArea);
			}
			else
			{
				file.CopyRemember (fromArea, toArea);
			}
			CopyAlias (fromArea, toArea);
		}
	}
	else
	{
		if (_state.IsPresentIn (toArea))
		{
			if (!IsFolder ())
			{
				CopyAlias (fromArea, toArea);
				file.DeleteFrom (toArea);
			}
		}
		_state.SetPresentIn (toArea, false);
	}
}

void Transformer::CopyAlias (Area::Location fromArea, Area::Location toArea)
{
	Assert (toArea != Area::Project);

	if (_state.IsPresentIn (fromArea))
	{
		if (_fileData->IsRenamedIn (fromArea))
		{
			_fileData->AddUnameAlias (_fileData->GetUnameIn (fromArea), toArea);
		}
		else if (_fileData->IsRenamedIn (toArea))
		{
			_fileData->RemoveUnameAlias (toArea);
		}

		if (_fileData->IsTypeChangeIn (fromArea))
		{
			_fileData->AddTypeAlias (_fileData->GetTypeIn (fromArea), toArea);
		}
		else if (_fileData-> IsTypeChangeIn(toArea))
		{
			_fileData->RemoveTypeAlias (toArea);
		}
	}
	else
	{
		if (_state.IsPresentIn (toArea))
		{
			_fileData->RemoveAlias (toArea);
			_fileData->RemoveTypeAlias (toArea);
		}
	}
}

GlobalId Transformer::IsPotentialNameConflict (Area::Location areaFrom) const
{
	UniqueName targetName;
	if (_fileData->IsRenamedIn (areaFrom))
	{
		Assert (!IsFolder ()); // Revisit: when implementing folder MOVE/RENAME
		// make a copy of uname
		targetName = _fileData->GetUnameIn (areaFrom);
	}
	else // no renaming -- folders are OK
	{
		// make a copy of uname
		targetName = _fileData->GetUniqueName ();
	}
	GlobalId thisFileGid = _fileData->GetGlobalId ();
	if (_state.IsNew ())
	{
		// For new files check if any of the checked-out
		// files was using targetName before
		GidList checkedOut;
		_dataBase.XListCheckedOutFiles (checkedOut);
		for (GidList::const_iterator gidIter = checkedOut.begin ();
			 gidIter != checkedOut.end (); ++gidIter)
		{
			if (*gidIter == thisFileGid)
				continue;	// Skip this file
			FileData const * fd = _dataBase.XGetFileDataByGid (*gidIter);
			if (fd->IsRenamedIn (Area::Original))
			{
				UniqueName const & oldName = fd->GetUnameIn (Area::Original);
				if (oldName.IsEqual (targetName))
				{
					// Names match -- different global ids
					Assert (thisFileGid != fd->GetGlobalId ());
					return *gidIter;
				}
			}
			else if (!fd->GetState ().IsPresentIn (Area::Project))
			{
				// deleted or removed: will conflict when doing uncheckout
				if (targetName.IsEqual (fd->GetUniqueName ()))
					return *gidIter;
			}
		}
		return gidInvalid;
	}
	else
	{
		return _dataBase.XIsUnique (thisFileGid, targetName);
	}
}

GlobalId Transformer::IsNameConflict (Area::Location areaFrom) const
{
	// If this file is not present in the areaFrom, the conflict is bogus
	if (!_state.IsPresentIn (areaFrom))
		return gidInvalid;
	GlobalId gidConflict = IsPotentialNameConflict (areaFrom);
	if (gidConflict != gidInvalid)
	{
		FileData const * conflictFile = _dataBase.XGetFileDataByGid (gidConflict);
		FileState conflictFileState = conflictFile->GetState ();

		if (conflictFileState.IsNew ())
			return gidConflict;	// Conflict with localy new file/folder
		if (conflictFileState.IsRelevantIn (areaFrom) && !conflictFileState.IsPresentIn (areaFrom))
			return gidInvalid;	// Conflict file is is not present in the areaFrom, the conflict is bogus
		UniqueName const & thisFileTargetName = _fileData->IsRenamedIn (areaFrom) ?
			_fileData->GetUnameIn (areaFrom) : _fileData->GetUniqueName ();
		if (conflictFile->IsRenamedIn (areaFrom))
		{
			if (!conflictFile->GetUnameIn (areaFrom).IsEqual (thisFileTargetName))
				return gidInvalid;	// Conflict file is renamed in areaFrom
									// and its new name is different from thisFileTargetName.
		}
		else
		{
			// Not renamed in areaFrom -- check why there is potential name conflict
			if (!thisFileTargetName.IsEqual (conflictFile->GetUniqueName ()))
			{
				return gidInvalid;	// Conflict file has still alias in some other area
									// different from areaFrom -- no conflict
			}
		}
	}
	return gidConflict;
}

void Transformer::ListMovedFiles (GidList & files) const
{
	Assert (IsFolder ());
	// Check if among checked out files there are files moved out from deleted folder.
	GlobalId deletedFolderGid = _fileData->GetGlobalId ();
	GidList checkedout;
	_dataBase.XListCheckedOutFiles (checkedout);
	for (GidList::const_iterator iter = checkedout.begin (); iter != checkedout.end (); ++iter)
	{
		FileData const * fd = _dataBase.XGetFileDataByGid (*iter);
		if (fd->IsRenamedIn (Area::Original))
		{
			UniqueName const & orgUname = fd->GetUnameIn (Area::Original);
			if (orgUname.GetParentId () == deletedFolderGid)
			{
				// Moved out file
				files.push_back (fd->GetGlobalId ());
			}
		}
	}
}

void Transformer::CopyToProject (Area::Location areaFrom, 
								 XPhysicalFile & file, 
								 PathFinder & pathFinder,
								 TransactionFileList & fileList)
{
	dbg << "Copy file from " << areaFrom << " to project" << std::endl;
	// Before copying to project make sure that the copied
	// file will be unique in the project area
	if (_state.IsPresentIn (areaFrom))
	{
		if (_fileData->IsRenamedIn (areaFrom))
		{
			Assert (!IsFolder ()); // Revisit: when implementing folder MOVE/RENAME
			// make a copy of uname
			UniqueName newUname (_fileData->GetUnameIn (areaFrom));
			dbg << "Renamed to " << newUname.GetName() << std::endl;
			CopyOverToProject (areaFrom, newUname, file);
			if (_fileData->IsTypeChangeIn (areaFrom))
			{
				FileType const  newType = _fileData->GetTypeIn (areaFrom);
				_fileData->ChangeType (newType);
			}
		}		
		else // no renaming -- folders are OK
		{
			if (IsFolder ())
			{
				file.CreateFolder ();
			}
			else
			{
				bool needTouch = true;
				if (areaFrom == Area::Original)
				{
					try
					{
						// Copying file from Original to Project -- performing un-checkout.
						// If the file in Original is identical with the file in the Project
						// don't touch it, so we don't force unnecessary recompilation.

						needTouch = file.IsDifferent (Area::Original, Area::Project);
					}
					catch ( ... )
					{
						Win::ClearError ();
						needTouch = true;
					}
				}				
				file.CopyOverToProject (areaFrom, needTouch);
			    if (_fileData->IsTypeChangeIn (areaFrom))
				{
					dbg << "Change of type" << std::endl;
					FileType const newType = _fileData->GetTypeIn (areaFrom);
					_fileData->ChangeType (newType);
				}
			}
		}
		_state.SetPresentIn (Area::Project, true);
	}
	else // not present in fromArea
	{
		dbg << "Not present in the source area" << std::endl;
		if (!IsFolder ())
		{
			if (areaFrom == Area::Original)
			{
				// Uncheckout
				if (_state.IsCoDelete ())
				{
					// CoDelete is set by CheckOutIfNecessary, when the file
					// was no longer in project
					file.DeleteFrom (Area::Project);
				}
				else
				{
					// If file physically exist in project
					// create a backup name for it (for instance foo.cpp.bak1) and
					// remember to rename the project file to this name
					char const * currentProjectPath = file.GetFullPath (Area::Project);
					if (File::Exists (currentProjectPath))
					{
						std::string newProjectPath = File::CreateUniqueName (currentProjectPath,
																			 "backup");
						fileList.RememberToMove (_fileData->GetGlobalId (),
												 currentProjectPath,
												 newProjectPath.c_str (),
												 false);	// Don't touch
					}
				}
			}
			else if (areaFrom == Area::Synch)
			{
				if (_state.IsSoDelete ())
					file.DeleteFrom (Area::Project);
			}
			else if (areaFrom == Area::Reference)
			{
				// Undo - file was removed from the project
				file.DeleteFrom (Area::Project);
			}
		}
		_state.SetPresentIn (Area::Project, false);
	}
}
