#if !defined FOLDER_WATCHER_H
#define FOLDER_WATCHER_H
//----------------------------------
//  (c) Reliable Software, 1997-2005
//----------------------------------

#include <Sys/Active.h>
#include <Sys/Synchro.h>
#include <File/Path.h>
#include <auto_vector.h>

class FileChangeEvent
{
public:
	FileChangeEvent (std::string const & dir, bool recursive, DWORD notifyFlags)
		:_dir (dir)
    {
		Open (recursive, notifyFlags);
    }
    ~FileChangeEvent ()
    {
		Close ();
    }
	void Close () throw ()
	{
        if (IsValid ())
		{
            ::FindCloseChangeNotification (_handle);
			_handle = INVALID_HANDLE_VALUE;
		}
	}
	void Open (bool recursive, DWORD notifyFlags)
	{
		_handle = ::FindFirstChangeNotification (_dir.c_str (), 
			recursive ? TRUE : FALSE, 
			notifyFlags);
		if (INVALID_HANDLE_VALUE == _handle)
			throw Win::Exception ("Cannot create notification event for the folder", _dir.c_str ());
	}
	std::string const & GetDir () const { return _dir; }
	bool IsEqualDir (std::string const dir) const { return dir == _dir; }
    HANDLE ToNative () const { return _handle; }
	bool IsValid () const { return _handle != INVALID_HANDLE_VALUE; }
    bool ContinueNotification ()
    {
        if (!::FindNextChangeNotification (_handle))
		{
			// Cannot continue notifications
			::SetLastError (0);
			return false;
		}
		return true;
    }
private:
    HANDLE		_handle;
	std::string _dir;
};

class FolderChangeEvent : public FileChangeEvent
{
public:
	FolderChangeEvent (std::string const & folder, bool recursive)
		: _recursive (recursive),
          FileChangeEvent (folder,
						   recursive,
						   FILE_NOTIFY_CHANGE_FILE_NAME |	// Renaming, creating, or deleting a file
						   FILE_NOTIFY_CHANGE_DIR_NAME  |	// Creating or deleting a directory
						   FILE_NOTIFY_CHANGE_ATTRIBUTES)	// Attribute change
    {}
	void Open ()
	{
		FileChangeEvent::Open (_recursive, 
			FILE_NOTIFY_CHANGE_FILE_NAME |	// Renaming, creating, or deleting a file
			FILE_NOTIFY_CHANGE_DIR_NAME  |	// Creating or deleting a directory
			FILE_NOTIFY_CHANGE_ATTRIBUTES);	// Attribute change
	}
private:
	bool _recursive;
};

class TreeChangeEvent : public FileChangeEvent
{
public:
    TreeChangeEvent (std::string const & root)
        : FileChangeEvent (root,
						   true,							// Recursive
						   FILE_NOTIFY_CHANGE_FILE_NAME |	// Renaming, creating, or deleting file
						   FILE_NOTIFY_CHANGE_DIR_NAME)		// Creating or deleting directory
    {}
	void Open ()
	{
		FileChangeEvent::Open (true,		// Recursive
			FILE_NOTIFY_CHANGE_FILE_NAME |	// Renaming, creating, or deleting file
			FILE_NOTIFY_CHANGE_DIR_NAME);	// Creating or deleting directory
	}
};

class FWatcher: public ActiveObject
{
public:
	FWatcher (Win::Dow::Handle win)
		: _win (win)
	{}
	void LokPostChange (std::string const & change);
	bool RetrieveChange (std::string & change);
	void Detach () { _win.Reset (); }
	void FlushThread () { _event.Release (); }
protected:
	Win::Dow::Handle		_win;
	std::set<std::string>	_changes;
	Win::CritSection		_critSect;
	Win::Event				_event;
};

class FolderWatcher : public FWatcher
{
public:
    FolderWatcher (std::string const & dir, Win::Dow::Handle win, bool recursive)
        : FWatcher (win),
		  _folderEvent (dir, recursive)
    {}
private:
    void Run ();
private:
    FolderChangeEvent	_folderEvent;
};

class MultiFolderWatcher : public FWatcher
{
public:
	class WatchedFolder;
	class IsEqualDir;
	typedef auto_vector<WatchedFolder>::iterator FolderIter;
	typedef auto_vector<WatchedFolder>::const_iterator ConstFolderIter;
public:
	MultiFolderWatcher (std::vector<std::string> const & folder, Win::Dow::Handle win);
    MultiFolderWatcher (Win::Dow::Handle win);
//	void Init (std::vector<std::string> const & folder);
	void AddFolder (std::string const & folder, bool recursive);
	void AddFolders (std::vector<std::string> const & folder);
	void StopWatching (std::string const & folder) throw ();
private:
    void Run ();
private:
    auto_vector<WatchedFolder>	_folders;
	bool						_refresh;
};

class MultiFolderWatcher::WatchedFolder
{
public:
	WatchedFolder (std::string const & folder, bool recursive)
		: _event (folder, recursive), _stopWatching (false)
	{}
	~WatchedFolder ()
	{
		if (_event.IsValid ())
			_event.Close ();
	}

	bool ContinueNotification () { return _event.ContinueNotification (); }
	bool IsMarkedStopWatching () const { return _stopWatching; }
	void StopWatching () { _event.Close (); }
	void MarkStopWatching () { _stopWatching = true; }
	void RestartWatching ()
	{
		_stopWatching = false;
		if (!_event.IsValid ())
			_event.Open (); 
	}
	HANDLE GetEvent () const { return _event.ToNative (); }
	std::string GetDir () const { return _event.GetDir (); }
	bool IsEqual (std::string const & folder) const { return _event.IsEqualDir (folder); }
private:
	FolderChangeEvent	_event;
	bool				_stopWatching;
};

class MultiFolderWatcher::IsEqualDir
	: public std::unary_function<MultiFolderWatcher::WatchedFolder const *, bool>
{
public:
	IsEqualDir (std::string const & folder)
		: _folder (folder)
	{}
	bool operator () (MultiFolderWatcher::WatchedFolder const * folder) const
	{
		return folder->IsEqual (_folder);
	}
private:
	std::string _folder;
};

#endif
