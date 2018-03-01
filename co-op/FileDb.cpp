//------------------------------------
//	(c) Reliable Software, 1996 - 2008
//------------------------------------

#include "precompiled.h"
#include "FileDb.h"
#include "ScriptCommandList.h"
#include "PathFind.h"
#include "FeedbackMan.h"
#include "TmpProjectArea.h"
#include "Predicate.h"

#include <Ex/WinEx.h>
#include <File/WildCard.h>
#include <Dbg/Out.h>
#include <StringOp.h>

//
// FileDb
//

class IsRoot : public std::unary_function<FileData const *, bool>
{
public:
	bool operator () (FileData const * fileData) const
	{
		return fileData->GetType ().IsRoot ();
	}
};

GlobalId FileDb::GetRootId () const
{
	FileDataIter iter = std::find_if (_fileData.begin (), _fileData.end (), IsRoot ());
	if (iter != _fileData.end ())
		return (*iter)->GetGlobalId ();
	return gidInvalid;
}

void FileDb::XAddFile2FullSynch (FileData const * fileData,
								char const * path,
								CommandList & initialFileInvnetory,
								bool beLazy)
{
	std::unique_ptr<WholeFileCmd> cmd (new WholeFileCmd (*fileData));
	Assert (cmd->GetOldCheckSum () == CheckSum ());
	CheckSum newCheckSum;
	if (beLazy)
	{
		FileInfo fileInfo (path);
		cmd->SetSource (path, fileInfo.GetSize ());
		newCheckSum = CheckSum (path);
	}
	else
	{
		MemFileReadOnly newFile (path);
		cmd->CopyIn (newFile.GetBuf (), newFile.GetSize ());
		newCheckSum = CheckSum (newFile);
	}
	cmd->SetNewCheckSum (newCheckSum);
	Assert (fileData->GetCheckSum () == newCheckSum);
	initialFileInvnetory.push_back (std::move(cmd));
}

void FileDb::XAddProjectFiles (PathFinder & pathFinder,
							   CommandList & initialFileInventory,
							   TmpProjectArea const & restoredFiles,
							   Progress::Meter & meter) const
{
	meter.SetRange (0, _fileData.XCount (), 1);
	meter.SetActivity ("Adding project files/folders to the full synch script");
	// Prepare a script that contains the whole
	// text of each file in baseline
	// Start with folders
	XAddFolders (initialFileInventory, meter);

	for (FileDataIter iter = xbegin (); iter != xend (); ++iter)
	{
		FileData const * fileData = *iter;
		Assert (fileData != 0);
		FileType type = fileData->GetType ();
		if (type.IsRoot () || type.IsFolder ())
		{
			// Already added by XAddFolders
			continue;
		}
		else
		{
			meter.StepIt ();
			bool beLazy = false; // copy file contents into a buffer (it may disappear from under us!)
			//Assert (type.IsHeader () || type.IsSource () || type.IsText () || type.IsBinary ());
			char const * fullPath;
			GlobalId gid = fileData->GetGlobalId ();
			FileState state = fileData->GetState ();
			if (restoredFiles.IsReconstructed (gid))
			{
				// Restored file -- get it from the temporary area
				fullPath = pathFinder.XGetFullPath (gid, restoredFiles.GetAreaId ());
			}
			else if (state.IsCheckedIn ())
			{
				// Checked-in file -- get it from the project area
				beLazy = true;
				fullPath = pathFinder.XGetFullPath (gid, Area::Project);
			}
			else if (state.IsRelevantIn (Area::Original) && state.IsPresentIn (Area::Original))
			{
				// Checked-out file
				fullPath = pathFinder.XGetFullPath (gid, Area::Original);
				if (fileData->IsRenamedIn (Area::Original) || fileData->IsTypeChangeIn (Area::Original))
				{
					// File has been renamed or moved or its type is change
					// In full synch use its original name or location and type
					FileData original (*fileData);
					original.ClearAliases ();
					if (fileData->IsRenamedIn (Area::Original))
					{
						UniqueName const & originalName = fileData->GetUnameIn (Area::Original);
						original.Rename (originalName);
					}
					if (fileData->IsTypeChangeIn (Area::Original))
					{
						FileType const  orgType = fileData->GetTypeIn (Area::Original);
						original.ChangeType (orgType);
					}
					beLazy = true; // files don't disappear from original 
					// Revisit: files do disappear from original AFTER the transaction!
					// so if we move unicast outside of the transaction, this code will bomb
					XAddFile2FullSynch (&original, fullPath, initialFileInventory, beLazy);
					continue;
				}
			}
			else
			{
				// File not in project -- skip it
				continue;
			}
			dbg << "    " << fileData->GetName () << std::endl;
			XAddFile2FullSynch (fileData, fullPath, initialFileInventory, beLazy);
		}
	}
	meter.Close ();
}

void FileDb::XAddFolders (CommandList & initialFileInventory, Progress::Meter & meter) const
{
	std::vector<FileData const *> folders;
    GlobalId rootId = gidInvalid;

	// Find all folders in the project
	// Skip New folders; they will be added by a separate script
	for (FileDataIter iter = xbegin (); iter != xend (); ++iter)
	{
		FileData const * fileData = *iter;
		Assert (fileData != 0);
		FileType type = fileData->GetType ();
		if (type.IsRoot ())
		{
			rootId = fileData->GetGlobalId ();
		}
		else if (type.IsFolder ())
		{
			FileState state = fileData->GetState ();
			// Skip folders not in project or just created
			if (state.IsNone () || state.IsNew ())
				continue;
			folders.push_back (fileData);
			meter.StepIt ();
		}
	}
	XAddChildren (rootId, folders, initialFileInventory, meter);
}

void FileDb::XAddChildren (GlobalId curParentId, 
						   std::vector<FileData const *> & folders, 
						   CommandList & initialFileInventory,
						   Progress::Meter & meter) const
{
	for (std::vector<FileData const *>::iterator iter = folders.begin ();
		 iter != folders.end ();
		 ++iter)
	{
		FileData const * folderData = *iter;
		if (folderData != 0)
		{
			if (folderData->IsParent (curParentId))
			{
				// Child folder of current folder -- add it to the initial file invnetory
				Assert (!folderData->GetState ().IsNone () && !folderData->GetState ().IsNew ());
				std::unique_ptr<ScriptCmd> cmd (new NewFolderCmd (*folderData));
				initialFileInventory.push_back (std::move(cmd));
				dbg << "folder: " << folderData->GetName () << std::endl;
				meter.StepIt ();
				// Remove added folder from the folder list
				*iter = 0;
				XAddChildren (folderData->GetGlobalId (), folders, initialFileInventory, meter);
			}
		}
	}
}

void FileDb::XRemoveKnownFilesFrom (GidSet & historicalFiles)
{
	Assert (!historicalFiles.empty ());
	// Remove from historical files set all files present in the file database
	// regardless of their state -- we want to know which global ids are not present
	// in this database
	for (FileDataIter iter = _fileData.xbegin (); iter != _fileData.xend (); ++iter)
	{
		FileData const * fileData = *iter;
		GlobalId fileGid = fileData->GetGlobalId ();
		historicalFiles.erase (fileGid);
		if (historicalFiles.empty ())
			return;
	}
}

void FileDb::XImportFileData (std::vector<FileData>::const_iterator begin,
							  std::vector<FileData>::const_iterator end)
{
	for (std::vector<FileData>::const_iterator iter = begin; iter != end; ++iter)
	{
		std::unique_ptr<FileData> historicalData (new FileData (*iter));
		_fileData.XAppend (std::move(historicalData));
	}
}

class IsEqualId : public std::unary_function<FileData const *, bool>
{
public:
	IsEqualId (GlobalId gid)
		: _gid (gid)
	{}
	bool operator () (FileData const * fileData) const
	{
		return fileData->GetGlobalId () == _gid;
	}
private:
	GlobalId	_gid;
};

FileData const * FileDb::FindByGid (GlobalId gid) const
{
	FileDataIter iter = std::find_if (_fileData.begin (), _fileData.end (), IsEqualId (gid));
	if (iter != _fileData.end ())
		return *iter;
	return 0;
}

FileData const * FileDb::XFindByGid (GlobalId gid) const
{
	FileDataIter iter = std::find_if (_fileData.xbegin (), _fileData.xend (), IsEqualId (gid));
	if (iter != _fileData.xend ())
		return *iter;
	return 0;
}

class FileTest : public std::unary_function<FileData const *, bool>
{
public:
	FileTest (std::function<bool(long, long)> predicate)
		: _predicate (predicate)
	{}
	bool operator () (FileData const * fileData) const
	{
		return _predicate (fileData->GetState ().GetValue (),
						   fileData->GetType ().GetValue ());
	}
private:
	std::function<bool(long, long)> _predicate;
};

bool FileDb::XAreFilesOut () const
{
	return std::find_if (_fileData.xbegin (), _fileData.xend (), FileTest (IsCheckedOut)) != _fileData.xend ();
}

bool FileDb::AreFilesOut () const
{
	return std::find_if (_fileData.begin (), _fileData.end (), FileTest (IsCheckedOut)) != _fileData.end ();
}

// Returns Gid of a file with the same name or gidInvalid
GlobalId FileDb::XIsUnique (GlobalId thisFileGid, UniqueName const & uname) const
{
	GidList folderContents;
	XListProjectFiles (uname.GetParentId (), folderContents);
	for (GidList::const_iterator gidIter = folderContents.begin ();
		 gidIter != folderContents.end (); ++gidIter)
	{
		if (*gidIter == thisFileGid)
			continue;	// Skip this file
		FileData const * fileData = XSearchByGid (*gidIter);
		FileState state = fileData->GetState ();
		if (state.IsNone () || state.IsToBeDeleted ())
			continue;	// File is or will be removed from the project
		if (IsCaseEqual (uname.GetName (), fileData->GetName ()))
		{
			// Names match -- different global ids
			Assert (thisFileGid != fileData->GetGlobalId ());
			return *gidIter;
		}
		if (fileData->IsRenamedAs (uname))
			return *gidIter;	// Not unique -- has aliases with the same name
	}
	return gidInvalid;
}

class GidPusher
{
public:
	GidPusher (GidList & ids) : _ids (ids) {}
	void operator= (FileData const * fileData)
	{
		_ids.push_back (fileData->GetGlobalId ());
	}
	GidPusher &  operator++ () { return *this; }
	GidPusher &  operator++ (int) { return *this; }
	GidPusher & operator *() { return *this; }
private:
	GidList & _ids;
};

void FileDb::XListCheckedOutFiles (GidList & ids) const
{
	std::copy_if (_fileData.xbegin (), _fileData.xend (), GidPusher (ids), FileTest (IsCheckedOut));
}

void FileDb::ListCheckedOutFiles (GidList & ids) const
{
	std::copy_if (_fileData.begin (), _fileData.end (), GidPusher (ids), FileTest (IsCheckedOut));
}

class IsSynchedOut : public std::unary_function<FileData const *, bool>
{
public:
	bool operator () (FileData const * fileData) const
	{
		return fileData->GetState ().IsRelevantIn (Area::Synch);
	}
};

void FileDb::ListSynchFiles (GidList & ids) const
{
	std::copy_if (_fileData.begin (), _fileData.end (), GidPusher (ids), IsSynchedOut ());
}

class IsInProjectFolder : public std::unary_function<FileData const *, bool>
{
public:
	IsInProjectFolder (GlobalId folderId)
		: _folderId (folderId)
	{}
	bool operator() (FileData const * fileData) const
	{
		return fileData->IsParent (_folderId) && !fileData->GetState ().IsNone ();
	}
private:
	GlobalId	_folderId;
};

void FileDb::XListProjectFiles (GlobalId folderId, GidList & contents) const
{
	IsInProjectFolder isInProjectFolder (folderId);
	std::copy_if (_fileData.xbegin (), _fileData.xend (), GidPusher (contents), isInProjectFolder);
}

void FileDb::ListProjectFiles (GlobalId folderId, GidList & contents) const
{
	IsInProjectFolder isInProjectFolder (folderId);
	std::copy_if (_fileData.begin (), _fileData.end (), GidPusher (contents), isInProjectFolder);
}

class IsMissingFromDisk : public std::unary_function<FileData const *, bool>
{
public:
	IsMissingFromDisk (PathFinder & pathFinder, Progress::Meter & meter)
		: _pathFinder (pathFinder),
		  _meter (meter)
	{}

	bool operator() (FileData const * fileData) const
	{
		_meter.StepIt ();
		FileState state = fileData->GetState ();
		if (state.IsPresentIn (Area::Project) && !fileData->GetType ().IsRoot ())
		{
			// Database tells us that file/folder is present in the project
			return !File::Exists (_pathFinder.GetFullPath (fileData->GetGlobalId (), Area::Project));
		}
		return false;	// Not in the project -- cannot be missing from disk
	}

private:
	PathFinder &	_pathFinder;
	Progress::Meter &	_meter;
};

void FileDb::FindMissingFromDisk (PathFinder & pathFinder, GidList & missing, Progress::Meter & meter) const
{
	IsMissingFromDisk isMissingFromDisk (pathFinder, meter);
	std::copy_if (_fileData.begin (), _fileData.end (), GidPusher (missing), isMissingFromDisk);
}

class IsInFolder : public std::unary_function<FileData const *, bool>
{
public:
	IsInFolder (GlobalId folderId)
		: _folderId (folderId)
	{}
	bool operator() (FileData const * fileData) const
	{
		return fileData->IsParent (_folderId);
	}
private:
	GlobalId	_folderId;
};

void FileDb::XListAllFiles (GlobalId folderId, GidList & files) const
{
	IsInFolder isInFolder (folderId);
	std::copy_if (_fileData.xbegin (), _fileData.xend (), GidPusher (files), isInFolder);
}

void FileDb::FindAllDescendants (GlobalId gidFolder, GidList & gidList) const
{
	for (FileDataIter it = begin (); it != end (); ++it)
	{
		FileData const * data = *it;
		if (data->IsParent (gidFolder))
		{
			gidList.push_back (data->GetGlobalId ());
			if (data->GetType ().IsFolder ())
			{
				FindAllDescendants (data->GetGlobalId (), gidList);
			}
		}
	}
}

class FolderPredicate : public std::unary_function<FileData const *, bool>
{
public:
	FolderPredicate (GlobalId folderId, std::function<bool(long, long)> predicate)
		: _folderId (folderId),
		  _predicate (predicate)
	{}
	bool operator() (FileData const * fileData) const
	{
		return fileData->IsParent (_folderId) &&
			   _predicate (fileData->GetState ().GetValue (),
						   fileData->GetType ().GetValue ());
	}
private:
	GlobalId						_folderId;
	std::function<bool(long, long)>	_predicate;
};

bool FileDb::XFindInFolder (GlobalId gidFolder, std::function<bool(long, long)> predicate) const
{
	Assert (gidFolder != gidInvalid);
	FolderPredicate folderPredicate (gidFolder, predicate);
	return std::find_if (_fileData.xbegin (), _fileData.xend (), folderPredicate) != _fileData.xend ();
}

//
// Check in operations
//

FileData * FileDb::XAppend (UniqueName const & uname, 
							GlobalId gid, 
							FileState state, 
							FileType type)
{
	std::unique_ptr<FileData> newFile (new FileData (uname, gid, state, type));
	int i = _fileData.XAppend (std::move(newFile));
	return _fileData.XGetEdit (i);
}

FileData * FileDb::XAppendForeign (FileData const & fileData)
{
	std::unique_ptr<FileData> newFileData (new FileData (fileData));
	int i = _fileData.XAppend (std::move(newFileData));
	return _fileData.XGetEdit (i);
}

FileData * FileDb::XGetEdit (GlobalId gid)
{
	for (unsigned int i = 0; i < _fileData.XCount (); i++)
	{
		if (_fileData.XGet (i)->GetGlobalId () == gid)
		{
			return _fileData.XGetEdit (i);
		}
	}
	return 0;
}

FileData const * FileDb::SearchByGid (GlobalId gid) const
{
	Assert (gid != gidInvalid);
	FileData const * fileFound = FindByGid (gid);
	if (fileFound != 0)
		return fileFound;
	GlobalIdPack pack (gid);
	throw Win::Exception ("Internal error: database corrupted -- invalid global file id", pack.ToString ().c_str ());
	return 0;
}

class IsFileNameMatch : public std::unary_function<FileData const *, bool>
{
public:
	IsFileNameMatch (Matcher const & matcher)
		: _matcher (matcher)
	{}
	bool operator() (FileData const * fileData) const
	{
		return _matcher.IsMatch (fileData->GetName ().c_str ());
	}
private:
	Matcher const & _matcher;
};

void FileDb::FindAllByName (Matcher const & matcher, GidList & foundFile) const
{
	IsFileNameMatch isFileNameMatch (matcher);
	std::copy_if (_fileData.begin (), _fileData.end (), GidPusher (foundFile), isFileNameMatch);
}

class IsProjectFileWithEqualUname : public std::unary_function<FileData const *, bool>
{
public:
	IsProjectFileWithEqualUname (UniqueName const & uname)
		: _uname (uname)
	{}
	bool operator() (FileData const * fileData) const
	{
		return !fileData->GetState ().IsNone () && _uname.IsEqual (fileData->GetUniqueName ());
	}
private:
	UniqueName const &	_uname;
};

FileData const * FileDb::XFindProjectFileByName (UniqueName const & uname) const
{
	Assert (uname.IsValid ());
	FileDataIter iter = std::find_if (_fileData.xbegin (),
									  _fileData.xend (),
									  IsProjectFileWithEqualUname (uname));
	if (iter != _fileData.xend ())
		return *iter;

	return 0;
}

class IsAbsentFolderWithEqualUname : public std::unary_function<FileData const *, bool>
{
public:
	IsAbsentFolderWithEqualUname (UniqueName const & uname)
		: _uname (uname)
	{}

	bool operator() (FileData const * fileData) const
	{
		return fileData->GetType ().IsFolder () &&
			   fileData->GetState ().IsNone ()  &&
			   _uname.IsEqual (fileData->GetUniqueName ());
	}

private:
	UniqueName const &	_uname;
};

FileData const * FileDb::FindAbsentFolderByName (UniqueName const & uname) const
{
	Assert (uname.IsValid ());
	FileDataIter iter = std::find_if (_fileData.begin (),
									  _fileData.end (),
									  IsAbsentFolderWithEqualUname (uname));
	if (iter != _fileData.end ())
		return *iter;

	return 0;
}

FileData * FileDb::XFindAbsentFolderByName (UniqueName const & uname)
{
	Assert (uname.IsValid ());
	FileDataIter iter = std::find_if (_fileData.xbegin (),
									  _fileData.xend (),
									  IsAbsentFolderWithEqualUname (uname));
	if (iter != _fileData.xend ())
		return _fileData.XGetEdit (_fileData.XtoIndex (iter));

	return 0;
}

FileData const * FileDb::XSearchByGid (GlobalId gid) const
{
	Assert (gid != gidInvalid);
	FileDataIter iter = std::find_if (_fileData.xbegin (), _fileData.xend (), IsEqualId (gid));
	if (iter != _fileData.xend ())
		return *iter;
	GlobalIdPack pack (gid);
	throw Win::Exception ("Internal error: database corrupted -- invalid global file id", pack.ToString ().c_str ());
	return 0;
}

//
// Transaction support
//

void FileDb::Serialize (Serializer& out) const
{
	_fileData.Serialize (out);
}

void FileDb::Deserialize (Deserializer& in, int version)
{
	CheckVersion (version, VersionNo ());
	_fileData.Deserialize (in, version);
}

