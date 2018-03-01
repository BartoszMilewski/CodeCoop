#if !defined (PATH_H)
#define PATH_H
//----------------------------------
// (c) Reliable Software 1998 - 2009
//----------------------------------

// has value semantics 
// with copy-on-write behavior of std::string

class FilePath
{
public:
	FilePath ()
	{
		Clear ();
	}
	FilePath (char const * path)
	{
		Change (path);
	}
	FilePath (std::string const & path)
	{
		Change (path);
	}
	void Clear ();
	void Change (char const * newPath);
	void Change (FilePath const & newPath);
	void Change (std::string const & newPath);
	void DirDown (char const * subDir, bool useSlash = false);
	void DirDown (std::string const & subDir, bool useSlash = false)
	{
		DirDown(subDir.c_str(), useSlash);
	}
	bool DirDownMaterialize(char const * subDir, bool quiet = false);
	void DirUp ();
	bool IsDirStrEmpty () const { return _prefix == 0; }

	std::string ToString () const { return std::string (GetDir ()); }
	std::string GetDrive () const;
	char const * GetAllFilesPath () const;
	char const * GetDir () const;
	char const * GetFilePath (char const * fileName, bool useSlash = false) const;
	char const * GetFilePath (std::string const & fileName, bool useSlash = false) const;
	void ResetLen () const { _buf.resize (_prefix); }
	unsigned int  GetLen () const { return _buf.length (); }
	char const * GetRelativePath (unsigned int prefix) const;
	bool Canonicalize (bool quiet = false);
	void ConvertToLongPath ();
	void ExpandEnvironmentStrings ();

	// Static methods
	static inline bool IsSeparator (char c)
	{
		return c == _backSlash || c == _forwardSlash || c == _querySign;
	}
	static bool IsEqualDir (std::string const & dir1, std::string const & dir2);
	static bool IsDirLess (std::string const & dir1, std::string const & dir2);
	static bool IsNetwork (char const * path);
	static bool IsAbsolute (char const * path);
	static bool IsNetwork (std::string const & path)
	{
		return IsNetwork (path.c_str ());
	}
	static bool IsAbsolute (std::string const & path)
	{
		return IsAbsolute (path.c_str ());
	}
	static bool HasValidDriveLetter (char const * path);
	static bool HasValidDriveLetter (std::string const & path)
	{
		return HasValidDriveLetter (path.c_str ());
	}
	static bool IsFileNameOnly (char const * path);
	static bool IsFileNameOnly (std::string const & path)
	{
		return IsFileNameOnly (path.c_str ());
	}
	static int GetLenLimit ()
	{
		return MAX_PATH;
	}
	bool HasPrefix (char const * path) const;
	bool HasPrefix (FilePath const & path) const
	{
		return HasPrefix (path.c_str ());
	}
	bool HasPrefix (std::string const & path) const
	{
		return HasPrefix (path.c_str ());
	}

	bool HasSuffix (char const * path) const;
	bool HasSuffix (FilePath const & path) const
	{
		return HasSuffix (path.c_str ());
	}
	bool HasSuffix (std::string const & path) const
	{
		return HasSuffix (path.c_str ());
	}

	bool IsEqualDir (char const * path) const;
	bool IsEqualDir (FilePath const & path) const
	{
		return IsEqualDir (path.GetDir ());
	}
	bool IsEqualDir (std::string const & path) const
	{
		return IsEqualDir (path.c_str ());
	}
	bool IsDirLess (FilePath const & path) const;
	static bool IsValid (std::string const & path);
	static bool IsValidPattern (char const * path);

	// Constants
	static char const IllegalChars [];

protected:
	void Append (std::string const & subDir, bool useSlash = false);
	char const * c_str () const { return _buf.c_str (); }

	static char const _allFilesName [];
	static char const _pathSeparators [];
	static char const _emptyPath [];

protected:
	mutable std::string _buf;
	size_t				_prefix;

private:
	enum
	{
		_backSlash = '\\',
		_forwardSlash = '/',
		_querySign = '?'
	};
};

// Predicate for path comparison
class DirPathLess
{
public:
	bool operator () (FilePath const & path1, FilePath const & path2) const
	{
		return path1.IsDirLess (path2);
	}
};

class TmpPath : public FilePath
{
public:
	TmpPath ();
};

// get or set current folder
class CurrentFolder : public FilePath
{
public:
	CurrentFolder ();
	static void Set (char const * path);
};

class CurrentFolderSwitch
{
public:
	CurrentFolderSwitch (char const * path);
	~CurrentFolderSwitch ();
private:
	CurrentFolder _previousCurrentFolder;
};

class ProgramFilesPath: public FilePath
{
public:
	ProgramFilesPath ();
};

class SystemFilesPath: public FilePath
{
public:
	SystemFilesPath ();
};

class FilePathSeq
{
public:
	FilePathSeq  (char const * path);
	bool HasDrive () const;
	bool IsUNC () const;
	bool IsDriveOnly () const;
	// bool IsShareOnly () const; not implemented
protected:
	std::string _path;
};

class PartialPathSeq: public FilePathSeq
{
public:
	PartialPathSeq (char const * path);

	bool AtEnd () const;
	bool IsLastSegment ();
	void Advance ();

	char const * GetSegment ();

protected:
	size_t	_cur;
	size_t	_start;
};

class FullPathSeq: public FilePathSeq
{
public:
	FullPathSeq (char const * path);
	std::string const & GetHead () const { return _head; }
	std::string GetServerName () const;

	bool AtEnd () const { return _start == _path.length (); }
	void Advance ();
	std::string GetSegment () const;

protected:
	size_t		_start; // idx of current segment start
	size_t		_end;   // idx of current segment end
	std::string _head;
protected:
	size_t FindNextSlash (size_t off, bool quiet = false) const;
};

class PathSplitter
{
public:
	PathSplitter (char const * path);
	PathSplitter (std::string const & path);
	char const * GetDrive () const { return _drive; }
	char const * GetDir () const { return _dir; }
	char const * GetFileName () const { return _fname; }
	char const * GetExtension () const { return _ext; }

	bool HasOnlyFileName () const { return _drive [0] == '\0' &&
										   _dir   [0] == '\0' &&
										   _fname [0] != '\0';}
	bool HasFileName  () const { return _fname [0] != '\0'; }
	bool HasExtension () const { return _ext [0] != '\0'; }
	bool IsValidShare () const;

private:
	char _drive [_MAX_DRIVE];
	char _dir	[_MAX_DIR];
	char _fname [_MAX_FNAME];
	char _ext	[_MAX_EXT];
};

class PathMaker
{
public:
	PathMaker (char const * drive, char const * dir, char const * fname, char const * ext);
	operator char const * () const { return _path; }

private:
	char _path [MAX_PATH];
};

class PathSequencer
{
public:
	virtual ~PathSequencer () {}

    virtual bool AtEnd () const = 0;
    virtual void Advance () = 0;
	virtual void Rewind () {}
	virtual unsigned int GetCount () const { return 0; }

	virtual char const * GetFilePath () const = 0;
};

class DirectoryListing
{
public:
	DirectoryListing () {}
	DirectoryListing (std::string const & path);

	void InitWithPath (std::string const & path);
	void AddName (std::string const & name) { _names.push_back (name); }

	unsigned int size () const { return _names.size (); }

	class Sequencer : public PathSequencer
	{
	public:
		Sequencer (DirectoryListing const & dirList)
			: _sourcePath (dirList._sourcePath),
			  _cur (dirList._names.begin ()),
			  _end (dirList._names.end ()),
			  _count (dirList._names.size ())
		{}

		bool AtEnd () const { return _cur == _end; }
		void Advance () { ++_cur; }
		unsigned int GetCount () const { return _count; }

		char const * GetFilePath () const { return _sourcePath.GetFilePath (*_cur); }

	private:
		FilePath const &							_sourcePath;
		std::vector<std::string>::const_iterator	_cur;
		std::vector<std::string>::const_iterator	_end;
		unsigned									_count;
	};

	friend class Sequencer;

private:
	FilePath					_sourcePath;
	std::vector<std::string>	_names;
};

#endif
