//----------------------------------
// (c) Reliable Software 1997 - 2008
//----------------------------------

#include "precompiled.h"
#include "DataBase.h"
#include "History.h"
#include "PathFind.h"
#include "PhysicalFile.h"

#include <Ex/WinEx.h>

//
// FileIndex
//

void FileIndex::Validate (FileDb::FileDataIter beg, FileDb::FileDataIter end) const
{
	if (!_isValid)
	{
		for (iterator it = _ids.begin (); it != _ids.end (); ++it)
		{
			it->second = gidInvalid;
		}

		for (FileDb::FileDataIter fit = beg; fit != end; ++fit)
		{
			FileState state = (*fit)->GetState ();
			UniqueName const & uname = (*fit)->GetUniqueName ();
			GlobalId gid = (*fit)->GetGlobalId ();
			if (state.IsPresentIn (Area::Project))
			{
				_ids [uname] = gid;
			}
			else if (state.IsRelevantIn (Area::Original) ||
					 state.IsRelevantIn (Area::Synch))
			{
				iterator it = _ids.find (uname);
				if (it == _ids.end ())
					_ids [uname] = gid;
				else if (it->second == gidInvalid)
					it->second = gid;
			}
		}
		_isValid = true;
	}
}

FileData const * DataBase::FindProjectFileByName (UniqueName const & uname) const
{
	Assert (uname.IsValid ());
	Validate (_fileDb.begin (), _fileDb.end ());
	GlobalId gid = GetGlobalId (uname);
	if (gid == gidInvalid)
		return 0;
	else
		return _fileDb.SearchByGid (gid);
}

// Is there an old record of a folder with the same path in the database?
// We could just revive it, instead of creating a new one
bool DataBase::CanReviveFolder (UniqueName const & uname) const
{
	Assert (!uname.IsNormalized ());
	GlobalId currentParentGid = uname.GetParentId ();
	for (PartialPathSeq pathSeq (uname.GetName ().c_str ()); !pathSeq.IsLastSegment (); pathSeq.Advance ())
	{
		UniqueName segmentUname (currentParentGid, pathSeq.GetSegment ());
		FileData const * fd = _fileDb.FindAbsentFolderByName (segmentUname);
		if (fd == 0)
			return false;
		currentParentGid = fd->GetGlobalId ();
	}
	return true;
}

//
// Database
//

void DataBase::InitPaths (PathFinder & pathFinder)
{
	_projectDb.InitPaths (pathFinder);
}

void DataBase::ListFolderContents (GlobalId folderId, GidList & contents, bool recursive) const
{
	GidList currentContents;
	_fileDb.ListProjectFiles (folderId, currentContents);
	std::copy (currentContents.begin (), currentContents.end (), std::back_inserter (contents));

	if (recursive)
	{
		// Recursively list project folder contents
		for (GidList::const_iterator iter = currentContents.begin ();
			 iter != currentContents.end ();
			 ++iter)
		{
			FileData const * fd = GetFileDataByGid (*iter);
			if (fd->GetType ().IsFolder ())
				ListFolderContents (*iter, contents, recursive);
		}
	}
}

//
// Project operations. Revisit: most, if not all, of them
// should go to Model
//

void DataBase::XCreateProjectRoot () 
{
	GlobalId gid = _projectDb.XMakeGlobalId (true);	// Make global id for the project root file data
	Assert (gid == gidRoot);
    FileState state;
    state.SetPresentIn (Area::Project, true);
	UniqueName uname;
	RootFolder root;
    _fileDb.XAppend (uname, gid, state, root); 
}

void DataBase::XDefect ()
{
	_projectDb.XDefect (); 
}

std::unique_ptr<VerificationReport> DataBase::Verify (PathFinder & pathFinder, Progress::Meter & meter) const
{
	// Check if all project files/folders are still on disk
	std::unique_ptr<VerificationReport> report (new VerificationReport ());
	GidList missing;
	_fileDb.FindMissingFromDisk (pathFinder, missing, meter);
	for (GidList::const_iterator iter = missing.begin (); iter != missing.end (); ++iter)
	{
		GlobalId gid = *iter;
		FileData const * fileData = GetFileDataByGid (gid);
		FileState state = fileData->GetState ();
		Assert (state.IsPresentIn (Area::Project));
		if (fileData->GetType ().IsFolder ())
			report->Remember (VerificationReport::MissingFolder, gid);
		else if (state.IsCheckedIn ())
			report->Remember (VerificationReport::Corrupted, gid);
		else if (state.IsNew ())
			report->Remember (VerificationReport::MissingNew, gid);
		else
		{
			Assert (state.IsRelevantIn (Area::Original));
			if (File::Exists (pathFinder.GetFullPath (gid, Area::Original)))
			{
				// Exists in the original area
				if (File::Exists (pathFinder.GetFullPath (gid, Area::LocalEdits)))
					report->Remember (VerificationReport::PreservedLocalEdits, gid);	// Can restore local edits
				else
					report->Remember (VerificationReport::MissingCheckedout, gid);	// Can only un-checkout
			}
			else
			{
				// Doesn't exist in the original area -- can be only repaired.
				report->Remember (VerificationReport::Corrupted, gid);
			}
		}
	}
	return report;
}

//
// File operations
//

FileData * DataBase::XAddFile (UniqueName const & uname, FileType type)
{
	GlobalId gid = _projectDb.XMakeGlobalId ();
	return XAddForeignFile (uname, gid, type);
}

FileData * DataBase::XAddForeignFile (FileData const & fileData)
{
	return _fileDb.XAppendForeign (fileData);
}

FileData * DataBase::XAddForeignFile (UniqueName const & uname, GlobalId gid, FileType type)
{
	FileState state;
	return _fileDb.XAppend (uname, gid, state, type);
}

void DataBase::XAddForeignFolder (GlobalId gid, UniqueName const & uname)
{
	FileState stateNone;
	FolderType typeFolder;
	_fileDb.XAppend (uname, gid, stateNone, typeFolder);
}

//
// Full Synch Script
//

void DataBase::XAddProjectFiles (PathFinder & pathFinder, 
								 CommandList & initialFileInventory, 
								 TmpProjectArea const & restoredFiles, 
								 Progress::Meter & meter)
{
	_fileDb.XAddProjectFiles (pathFinder, initialFileInventory, restoredFiles, meter);
}

//
// Transaction support
//

void DataBase::CommitTransaction () throw ()
{
	TransactableContainer::CommitTransaction ();
	Invalidate ();
}

void DataBase::Clear () throw ()
{
	TransactableContainer::Clear ();
	Invalidate ();
}

void DataBase::Serialize (Serializer& out) const
{
	_fileDb.Save (out);
	_projectDb.Save (out);
	_orgId.Serialize (out);
}

void DataBase::Deserialize (Deserializer& in, int version)
{
	CheckVersion (version, VersionNo ());
	_fileDb.Read (in);
	_projectDb.Read (in);
	if (version >= 11)
		_orgId.Deserialize (in, version);
}

void DataBase::MissingFile (char const * fullPath)
{
	throw Win::Exception ("Corrupted database; Missing file!\n", fullPath);
}
