//----------------------------------
// (c) Reliable Software 1997 - 2007
//----------------------------------
#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "Directory.h"
#include "DataBase.h"
#include "OutputSink.h"
#include "Global.h"
#include "CheckoutNotifications.h"

#include <File/Dir.h>
#include <File/FolderWatcher.h>

Directory::Directory (FileIndex const & fileIndex,
					  CheckOut::Db const & checkOutDb,
					  MultiFolderWatcher & watcher)
	: _fileIndex (fileIndex),
	  _checkOutDb (checkOutDb),
	  _watcher (watcher),
	  _curPath (fileIndex),
	  _enableWatching (false),
	  _rootLen (0)
{
	SetRootId (gidInvalid);
}

void Directory::SetProjectRoot (char const * path, 
								GlobalId rootId, 
								std::string const & rootName, 
								bool useFolderWatcher)
{
	_curPath.Change (path);
	_rootLen = _curPath.GetLen ();
	Assert (_rootLen != 0);
	if (useFolderWatcher)
		_enableWatching = !FilePath::IsNetwork (path);
	else
		_enableWatching = false;
	SetRootId (rootId);
	_rootName = rootName;
}

std::string Directory::GetRootName () const
{
	return _rootName;
}

char const * Directory::GetFilePath (std::string const & fname) const
{ 
	return _curPath.GetFilePath (fname);
}

char const * Directory::GetRootRelativePath (char const * fullPath) const
{
	int fullPathLen = strlen (fullPath);
	if (fullPathLen > _rootLen)
		return &fullPath [_rootLen + 1];
	else if (fullPathLen == _rootLen)
		return &fullPath [_rootLen];
	else
		return fullPath;
}

char const * Directory::GetRelativePath (char const * fullPath) const
{
	unsigned int len = _curPath.GetLen ();
	Assert (strlen (fullPath) >= len);
	char const * relPath = &fullPath [len];
	if (FilePath::IsSeparator (*relPath))
		++relPath;
	return relPath;
}

std::string Directory::GetRootRelativePath (GlobalId gid) const
{
	Project::Path project (_fileIndex);
	return std::string (project.MakePath (gid));
}

// May be used to descend down to a file
void Directory::Down (char const * name)
{
	StopWatchingCurrentFolder ();
	DownToSubFolder (name);
	WatchCurrentFolder ();
}

void Directory::GotoRoot ()
{
	StopWatchingCurrentFolder ();
	SetRootFolder ();
	WatchCurrentFolder ();
}

void Directory::Up (int levelsUp)
{
	Assert (levelsUp > 0);
	StopWatchingCurrentFolder ();
	while (_gidPath.size () > 1 && levelsUp > 0)
	{
		_curPath.DirUp ();
		_gidPath.pop_back ();
		--levelsUp;
	}
	WatchCurrentFolder ();
}

void Directory::SetRootId (GlobalId rootId)
{
	_gidPath.clear ();
	_gidPath.push_back (rootId);
}

void Directory::SetCurrentFolderId (GlobalId folderId)
{
	if (folderId == gidInvalid || _gidPath.back () == folderId)
		return;
	GidList::const_iterator iter = std::find (_gidPath.begin (), _gidPath.end (), folderId);
	if (iter != _gidPath.end ())
	{
		for (GidList::reverse_iterator rIter = _gidPath.rbegin (); *rIter != *iter; ++rIter)
		{
			Up ();
		}
		Assert (_gidPath.back () == folderId);
	}
}

// Revisit: we would be much better off using vector paths everywhere
// They can be quickly converted to file paths
bool Directory::Change (std::string const & relativePath)
{
	if (FilePath::IsEqualDir (relativePath, GetCurrentRelativePath ()))
		return false;

	// Change current project folder
	StopWatchingCurrentFolder ();
	SetRootFolder ();
	if (!relativePath.empty ())
	{
		for (PartialPathSeq seq (relativePath.c_str ()); !seq.AtEnd (); seq.Advance ())
		{
			if (File::Exists (GetFilePath (seq.GetSegment ())))
				DownToSubFolder (seq.GetSegment ());
			else
				break;
		}
	}
	WatchCurrentFolder ();
	return true;
}

bool Directory::FolderChange (FilePath const & folder)
{
	if (folder.IsEqualDir (_curPath))
	{
		ExternalNotify ();
		return true;
	}
	return false;
}

//
// Table interface
//

void Directory::QueryUniqueNames (Restriction const & restrict, 
								  std::vector<std::string> & names, 
								  GidList & parentIds) const
{
	Assert (GetRootId () != gidInvalid);
	Assert (_fileCache.empty ());
	if (restrict.HasFiles ())
	{
		// Special restriction pre-filled with unique names
		Restriction::UnameIter it;
		for (it = restrict.BeginFiles (); it != restrict.EndFiles (); ++it)
		{
			UniqueName const & uname = *it;
			names.push_back (uname.GetName ().c_str ());
			parentIds.push_back (uname.GetParentId ());
		}
		return;
	}

	GlobalId currentFolderId;
	FilePath folderPath;
	if (restrict.IsOn ("SpecificFolder"))
	{
		// Listing some other folder, possibly different from the current one
		folderPath.Change (std::string (_curPath.ToString (), 0, _rootLen));
		File::Vpath const & vpath = restrict.GetFolderPath ();
		if (vpath.Depth () != 0)
		{
			folderPath.DirDown (vpath.ToString ().c_str ());
		}
		if (!File::Exists (folderPath.GetDir ()))
			return;
		currentFolderId = restrict.GetFolderGid ();
	}
	else
	{
		// Listing only current folder
		RefreshGidPath ();
		folderPath.Change (_curPath.ToString ());
		currentFolderId = GetCurrentId ();
	}

	try
	{
		// list folder
		bool foldersOnly = restrict.IsOn ("FoldersOnly");
		bool filterOn = restrict.IsOn ("FileFiltering");
		bool hideNonProject = restrict.IsOn ("HideNonProject");

		for (FileSeq seq (folderPath.GetAllFilesPath ()); !seq.AtEnd (); seq.Advance ())
		{
			// skip files is only folders requested
			if (foldersOnly && !seq.IsFolder ())
				continue;

			char const * fileName = seq.GetName ();

			// skip files with filtered extensions
			if (filterOn && restrict.HasExtensionFilter ())
			{
				PathSplitter splitter (fileName);
				
				if (splitter.HasExtension () &&
					!seq.IsFolder () &&
					restrict.IsFilteredOut (splitter.GetExtension ()))
					continue;
			}
			FileData const * fd = 0;
			if (currentFolderId != gidInvalid)
			{
				UniqueName uname (currentFolderId, fileName);
				fd = _fileIndex.FindProjectFileByName (uname);
			}

			if (fd != 0 && !fd->GetState ().IsNone () && !fd->GetState ().IsToBeDeleted ())
			{
				// Controlled file
				FileInfo info (seq.GetSize (), seq.GetWriteTime (),
							   fd->GetGlobalId (), fd->GetState (), fd->GetType (),
							   seq.IsReadOnly ());
				_fileCache.insert (std::make_pair (fileName, info));
			}
			else
			{
				// Skip non-project files/folder if only project files requested
				if (filterOn && hideNonProject)
					continue;
				FileInfo info (seq.GetSize (), seq.GetWriteTime (), seq.IsReadOnly ());
				if (seq.IsFolder ())
					info._type = FolderType ();
				_fileCache.insert (std::make_pair (fileName, info));
			}
			names.push_back (fileName);
			parentIds.push_back (currentFolderId);
		}
	} // Revisit: Why are we displaying exceptions here???
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
	}
	catch ( ... )
	{
		std::string info ("Unknown error: Cannot list folder contents.\n\n");
		info += folderPath.ToString ();
		TheOutput.Display (info.c_str ());
	}
}

void Directory::RefreshGidPath () const
{
	if (GetCurrentId () == gidInvalid)
	{
		Assert (_gidPath.size () > 1);
		// Go from the project root down and check which folder
		// global ids are still invalid. Stop at first invalid id.
		char const * relativePath = _curPath.GetRelativePath (_rootLen + 1);
		// Now go down
		PartialPathSeq pathSeq (relativePath);
		for (unsigned int i = 1; i < _gidPath.size (); ++i)
		{
			GlobalId curParentId = _gidPath [i - 1];
			Assert (curParentId != gidInvalid);
			Assert (!pathSeq.AtEnd ());
			char const * curSegment = pathSeq.GetSegment ();
			pathSeq.Advance ();
			UniqueName uname (curParentId, curSegment);
			FileData const * fd = _fileIndex.FindProjectFileByName (uname);
			if (fd == 0)
			{
				// Folder no longer in the project -- set all global ids below it to invalid
				for (unsigned int j = i; j < _gidPath.size (); ++j)
					_gidPath [j] = gidInvalid;
				break;
			}
			_gidPath [i] = fd->GetGlobalId ();
		}
	}
	else
	{
		// Go from the current project folder up to the root and
		// check which folder global ids are still valid. Stop at first valid id.
		for (unsigned int i = _gidPath.size () - 1; i != 0; --i)
		{
			Assert (_gidPath [i] != gidInvalid);
			FileData const * fd = _fileIndex.GetFileDataByGid (_gidPath [i]);
			if (fd->GetState ().IsNone ())
			{
				_gidPath [i] = gidInvalid;
			}
			else
			{
				// Folder global id is valid -- all the ids above are also valid.
				break;
			}
		}
	}
}

bool Directory::IsValid () const
{
	return GetRootId () != gidInvalid;
}

bool Directory::IsWiki () const
{
	return File::Exists (_curPath.GetFilePath ("index.wiki"));
}

bool Directory::IsCurrentFolder (std::string const & path) const
{
	return IsNocaseEqual (GetCurrentPath (), path);
}

std::string Directory::GetRootPath() const
{
	return std::string (_curPath.ToString (), 0, _rootLen);
}

std::string Directory::GetStringField (Column col, UniqueName const & uname) const
{
	Assert (col == colStateName);
	if (_fileCache.empty ())
	{
		FileData const * file = _fileIndex.FindProjectFileByName (uname);
		if (file != 0)
		{
			FileState state = file->GetState ();
			return state.GetName ();
		}
	}
	else
	{
		NocaseMap<FileInfo>::const_iterator iter = _fileCache.find (uname.GetName ());
		if (iter != _fileCache.end ())
		{
			FileInfo const & info = iter->second;
			return info._state.GetName ();
		}
	}
	std::string empty ("----");
	return empty;
}

GlobalId Directory::GetIdField (Column col, UniqueName const & uname) const
{
	Assert (col == colId);
	if (_fileCache.empty ())
	{
		FileData const * file = _fileIndex.FindProjectFileByName (uname);
		if (file != 0)
			return file->GetGlobalId ();
	}
	else
	{
		NocaseMap<FileInfo>::const_iterator iter = _fileCache.find (uname.GetName ());
		if (iter != _fileCache.end ())
		{
			FileInfo const & info = iter->second;
			return info._gid;
		}
	}
	return gidInvalid;
}

std::string Directory::GetStringField (Column col, GlobalId gid) const
{
	if (col == colParentPath)
	{
		return GetCurrentPath ();
	}
	else
	{
		FileData const * file = _fileIndex.GetFileDataByGid (gid);
		Assert (file != 0);
		if (col == colName)
		{
			return file->GetName ();
		}
		else
		{
			Assert (col == colStateName);
			FileState state = file->GetState ();
			return state.GetName ();
		}
	}
}

GlobalId Directory::GetIdField (Column col, GlobalId gid) const
{
	Assert (col == colParentId);
	return GetCurrentId ();
}

unsigned long Directory::GetNumericField (Column col, UniqueName const & uname) const
{
	if (_fileCache.empty ())
	{
		std::string filePath = GetProjectPath (uname);
		FileSeq seq (filePath.c_str ());
		if (col == colSize)
			return seq.GetSize ();
		else if (col == colReadOnly)
			return seq.IsReadOnly () ? 1 : 0;

		FileData const * file = _fileIndex.FindProjectFileByName (uname);
		if (file == 0)
		{
			// File not controlled
			if (col == colType)
			{
				if (seq.IsFolder ())
					return FolderType ().GetValue ();
				else
					return TextFile ().GetValue ();	// Assume text file
			}
			else if (col == colState)
				return 0;	// State none
		}
		else
		{
			// File controlled
			if (col == colState)
				return file->GetState ().GetValue ();
			else if (col == colType)
				return file->GetType ().GetValue ();
		}
	}
	else
	{
		NocaseMap<FileInfo>::const_iterator iter = _fileCache.find (uname.GetName ());
		Assert (iter != _fileCache.end ());
		FileInfo const & info = iter->second;
		if (col == colType)
			return info._type.GetValue ();
		else if (col == colState)
		{
			FileState state =  info._state;
			state.SetCheckedOutByOthers (_checkOutDb.IsCheckedOut (info._gid));
			return state.GetValue ();
		}
		else if (col == colSize)
			return info._size;
		else if (col == colReadOnly)
			return info._isReadOnly ? 1 : 0;
	}

	Assert (!"Invalid numeric column");
	return 0;
}

unsigned long Directory::GetNumericField (Column col, GlobalId gid) const
{
	FileData const * file = _fileIndex.GetFileDataByGid (gid);
	return GetNumericField (col, file->GetUniqueName ());
}

bool Directory::GetBinaryField (Column col, void * buf, unsigned int bufLen, GlobalId gid) const
{
	if (col == colTimeStamp && bufLen >= sizeof (FileTime))
	{
		FileData const * file = _fileIndex.GetFileDataByGid (gid);
		Assert (file != 0);
		FileSeq seq (_curPath.GetFilePath (file->GetName ().c_str ()));
		memcpy (buf, &seq.GetWriteTime (), sizeof (FileTime));
		return true;
	}
	return false;
}

bool Directory::GetBinaryField (Column col, void * buf, unsigned int bufLen, UniqueName const & uname) const
{
	if (col == colTimeStamp && bufLen >= sizeof (FileTime))
	{
		if (_fileCache.empty ())
		{
			std::string filePath = GetProjectPath (uname);
			FileSeq seq (filePath.c_str ());
			memcpy (buf, &seq.GetWriteTime (), sizeof (FileTime));
		}
		else
		{
			NocaseMap<FileInfo>::const_iterator iter = _fileCache.find (uname.GetName ());
			Assert (iter != _fileCache.end ());
			FileInfo const & info = iter->second;
			memcpy (buf, &info._dateModified, sizeof (FileTime));
		}
		return true;
	}
	return false;
}

std::string Directory::GetCaption (Restriction const & restrict) const
{
	// File Area View caption -- current relative project path
	std::string currentFolder (GetCurrentRelativePath ());
	if (currentFolder.empty ())
	{
		return " Project Root";
	}
	else
	{
		return currentFolder;
	}
}

std::string Directory::GetCurrentRelativePath () const
{
	std::string path;
	if (_rootLen != 0)
	{
		char const * curRelativePath = _curPath.GetRelativePath (_rootLen + 1);
		if (curRelativePath != 0)
			path.assign (curRelativePath);
	}
	return path;
}

std::string Directory::GetProjectPath (UniqueName const & uname) const
{
	std::string projectPath;
	if (GetCurrentId () == uname.GetParentId ())
	{
		projectPath.assign (_curPath.GetFilePath (uname.GetName ()));
	}
	else
	{
		FilePath rootPath (_curPath.GetDir ());
		for (unsigned int i = _gidPath.size (); i > 1; --i)
			rootPath.DirUp ();
		if (uname.IsRootName ())
		{
			projectPath = rootPath.GetDir ();
		}
		else
		{
			Project::Path pathMaker (_fileIndex);
			pathMaker.Change (rootPath);
			projectPath = pathMaker.MakePath (uname);
		}
	}
	return projectPath;
}

void Directory::SetRootFolder ()
{
	while (_gidPath.size () > 1)
	{
		_curPath.DirUp ();
		_gidPath.pop_back ();
	}
}

void Directory::Clear () throw ()
{
	StopWatchingCurrentFolder ();
	_curPath.Clear ();
	_rootLen = 0;
	_enableWatching = false;
	_gidPath.clear ();
	SetRootId (gidInvalid);
}

void Directory::DownToSubFolder (char const * name)
{
	GlobalId curFolderId = GetCurrentId ();
	_curPath.DirDown (name);
	if (curFolderId == gidInvalid)
	{
		_gidPath.push_back (gidInvalid);
	}
	else
	{
		UniqueName uname (curFolderId, name);
		FileData const * fileData = _fileIndex.FindProjectFileByName (uname);
		if (fileData != 0)
			_gidPath.push_back (fileData->GetGlobalId ());
		else
			_gidPath.push_back (gidInvalid);
	}
}

void Directory::StopWatchingCurrentFolder ()
{
	// Wiki folders continue to be watched until the browser is closed
	if (_enableWatching && !IsWiki ())
		_watcher.StopWatching (GetCurrentPath ());
}

void Directory::WatchCurrentFolder ()
{
	if (_enableWatching)
	{
		bool isWiki = IsWiki ();
		// watch recursively in wiki folders
		_watcher.AddFolder (GetCurrentPath (), isWiki);
	}
	CurrentFolder::Set (GetCurrentPath ());
}

//
// Directory::Sequencer
//

Directory::Sequencer::Sequencer (Directory const & projectDir)
	: _curPath (projectDir._curPath),
	  _fileIndex (projectDir._fileIndex)
{
	_gidPath = projectDir._gidPath;
	_rootLen = projectDir._rootLen;
}

char const * Directory::Sequencer::GetRelativePath (char const * fullPath) const
{
	unsigned int len = _curPath.GetLen ();
	Assert (strlen (fullPath) >= len);
	char const * relPath = &fullPath [len];
	if (FilePath::IsSeparator (*relPath))
		++relPath;
	return relPath;
}

void Directory::Sequencer::Down (char const * name)
{
	GlobalId curFolderId = GetCurrentId ();
	_curPath.DirDown (name);
	if (curFolderId == gidInvalid)
	{
		_gidPath.push_back (gidInvalid);
	}
	else
	{
		UniqueName uname (curFolderId, name);
		FileData const * fileData = _fileIndex.FindProjectFileByName (uname);
		if (fileData != 0)
			_gidPath.push_back (fileData->GetGlobalId ());
		else
			_gidPath.push_back (gidInvalid);
	}
}

void Directory::Sequencer::Up ()
{
	if (_gidPath.size () > 1)
	{
		_curPath.DirUp ();
		_gidPath.pop_back ();
	}
}

void Directory::Sequencer::GotoRoot ()
{
	while (_gidPath.size () > 1)
	{
		_curPath.DirUp ();
		_gidPath.pop_back ();
	}
}
