#if !defined (DIRECTORY_H)
#define DIRECTORY_H
//----------------------------------
// (c) Reliable Software 1997 - 2008
//----------------------------------

#include "Table.h"
#include "GlobalId.h"
#include "FileState.h"
#include "FileTypes.h"
#include "PathFind.h"
#include "Global.h"

#include <StringOp.h>

class FileIndex;
class MultiFolderWatcher;
class FileTime;
namespace CheckOut
{
	class Db;
}

class Directory : public Table
{
public:
	Directory (FileIndex const & fileIndex,
			   CheckOut::Db const & checkOutDb,
			   MultiFolderWatcher & watcher);
	void SetProjectRoot (char const * path, GlobalId rootId, std::string const & rootName, bool useFolderWatcher = true);
	void Down (char const * name);
	void Down (std::string const & name) { Down (name.c_str ()); }
	void Up (int levelsUp = 1);
	void GotoRoot ();
	GlobalId GetCurrentId () const 
	{
		return _gidPath.back (); 
	}
	void SetRootId (GlobalId rootId);
	void SetCurrentFolderId (GlobalId folderId);
	void SetWatchDuty (bool flag) { _enableWatching = flag; }
	GlobalId GetRootId () const { return _gidPath [0]; }
	std::string GetRootPath() const;
	bool IsRootFolder () const { return _gidPath.size () == 1; }
	bool IsCurrentFolder (std::string const & path) const;
	bool IsWiki () const;
	bool Change (File::Vpath const & path);
	bool Change (std::string const & path);
	bool FolderChange (FilePath const & folder);
	char const * GetCurrentPath () const { return _curPath.GetDir (); }
	std::string GetCurrentRelativePath () const;
	char const * GetFilePath (std::string const & fname) const;
	std::string GetProjectPath (UniqueName const & uname) const;
	char const * GetRootRelativePath (char const * fullPath) const;
	char const * GetRelativePath (char const * fullPath) const;
	std::string GetRootRelativePath (GlobalId gid) const;
	void WatchCurrentFolder ();

	// Table interface
	std::string GetRootName () const;
	void QueryUniqueNames  (Restriction const & restrict, 
							std::vector<std::string> & names, 
							GidList & parentIds) const;
	Table::Id GetId () const { return Table::folderTableId; }
	bool IsValid () const;
	std::string GetStringField (Column col, GlobalId gid) const;
	std::string GetStringField (Column col, UniqueName const & uname) const;
	GlobalId	GetIdField (Column col, UniqueName const & uname) const;
	GlobalId	GetIdField (Column col, GlobalId gid) const;
	unsigned long GetNumericField (Column col, UniqueName const & uname) const;
	unsigned long GetNumericField (Column col, GlobalId gid) const; 
    bool GetBinaryField (Column col, void * buf, unsigned int bufLen, GlobalId gid) const;
    bool GetBinaryField (Column col, void * buf, unsigned int bufLen, UniqueName const & uname) const;
	std::string GetCaption (Restriction const & restrict) const;
	void ClearFileCache () const { _fileCache.clear (); }

	void Clear () throw ();

public:
	// Sequencer allows for up and down traversal on the current project path
	// without changing current position in the project tree.
	class Sequencer
	{
	public:
		Sequencer (Directory const & projectDir);

		GlobalId GetCurrentId () const { return _gidPath.back (); }
		char const * GetCurrentPath () const { return _curPath.GetDir (); }
		char const * GetRelativePath (char const * fullPath) const;
		void Down (char const * name);
		void Up ();
		void GotoRoot ();

	private:
		Project::Path	_curPath;	// current path as string
		GidList			_gidPath;	// current path as a chain of global ids
		int 			_rootLen;
		FileIndex const &_fileIndex;
	};

	friend class Sequencer;

private:
	class FileInfo
	{
	public:
		FileInfo (unsigned long size, FileTime dateModified, bool isReadOnly)
			: _size (size),
			  _dateModified (dateModified),
			  _gid (gidInvalid),
			  _isReadOnly (isReadOnly)
		{}

		FileInfo (unsigned long size, FileTime dateModified,
				  GlobalId gid, FileState state, FileType type,
				  bool isReadOnly)
			: _size (size),
			  _dateModified (dateModified),
			  _gid (gid),
			  _state (state),
			  _type (type),
			  _isReadOnly (isReadOnly)
		{}

		unsigned int	_size;
		FileTime		_dateModified;
		GlobalId		_gid;
		FileState		_state;
		FileType		_type;
		bool			_isReadOnly;
	};

private:
	void SetRootFolder ();
	void DownToSubFolder (char const * name);
	void StopWatchingCurrentFolder ();
	void RefreshGidPath () const;

private:
	Project::Path mutable		_curPath;	// current path as string
	GidList mutable				_gidPath;	// current path as a chain of global ids
	std::string					_rootName;
	NocaseMap<FileInfo> mutable	_fileCache;
	FileIndex const &			_fileIndex;
	CheckOut::Db const &		_checkOutDb;
	MultiFolderWatcher &		_watcher;
	bool						_enableWatching;	// Watch current project folder
	int 						_rootLen;
};	
#endif
