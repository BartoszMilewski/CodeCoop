#if !defined (FILEDATASEQUENCER_H)
#define FILEDATASEQUENCER_H
//------------------------------------
//  (c) Reliable Software, 1998 - 2008
//------------------------------------

#include "FileFilter.h"
#include "DataBase.h"
#include "ProjectPath.h"

class FileFilter;

class FileDataSequencer
{
public:
	virtual ~FileDataSequencer () {}

    virtual bool AtEnd () const = 0;
    virtual void Advance () = 0;

	virtual FileData const * GetFileData () const = 0;
	virtual unsigned int size () const = 0;
	virtual char const * GetProjectRelativePath () = 0; 
};

class FilteredSequencer : public FileDataSequencer
{
public:
	FilteredSequencer (GidList const & gids, DataBase const & dataBase)
		: _cur (gids.begin ()),
		  _end (gids.end ()),
		  _dataBase (dataBase),
		  _projectRelativePath (dataBase),
		  _size (gids.size ())
	{}

	bool AtEnd () const { return _cur == _end; }
    void Advance () { ++_cur; }

	FileData const * GetFileData () const
	{
		GlobalId gid = *_cur;
		return _dataBase.GetFileDataByGid (gid);
	}
	unsigned int size () const { return _size; }
	char const * GetProjectRelativePath () { return _projectRelativePath.MakePath (GetFileData ()->GetUniqueName ()); }

private:
	GidList::const_iterator _cur;
	GidList::const_iterator	_end;
	DataBase const &		_dataBase;
	Project::Path			_projectRelativePath;
	unsigned int			_size;
};

class IsProjectFile : public std::unary_function<FileData const *, bool>
{
public:
	bool operator() (FileData const * fileData) const
	{
		return !fileData->GetState ().IsNone () &&
			   !fileData->GetType ().IsRoot ()  &&
			   !fileData->GetType ().IsFolder ();
	}
};

class XAllProjectFilesSequencer : public FileDataSequencer
{
public:
	XAllProjectFilesSequencer (DataBase const & dataBase)
		: _cur (std::find_if (dataBase.xbegin (), dataBase.xend (), IsProjectFile ())),
		  _end (dataBase.xend ()),
		  _size (dataBase.FileCount ()),
		  _projectRelativePath (dataBase)
	{}

    bool AtEnd () const { return _cur == _end; }
    void Advance ()
	{
		++_cur;
		_cur = std::find_if (_cur, _end, IsProjectFile ());
	}

	FileData const * GetFileData () const { return *_cur; }
	unsigned int size () const { return _size; }
	char const * GetProjectRelativePath () { return _projectRelativePath.XMakePath ((*_cur)->GetUniqueName ()); }

private:
	DataBase::FileIter	_cur;
	DataBase::FileIter	_end;
	int					_size;
	Project::XPath		_projectRelativePath;
};

class AllProjectFilesSequencer : public FileDataSequencer
{
public:
	AllProjectFilesSequencer (DataBase const & dataBase)
		: _cur (std::find_if (dataBase.begin (), dataBase.end (), IsProjectFile ())),
		  _end (dataBase.end ()),
		  _size (dataBase.FileCount ()),
		  _projectRelativePath (dataBase)
	{}

    bool AtEnd () const { return _cur == _end; }
    void Advance ()
	{
		++_cur;
		_cur = std::find_if (_cur, _end, IsProjectFile ());
	}

	FileData const * GetFileData () const { return *_cur; }
	unsigned int size () const { return _size; }
	char const * GetProjectRelativePath () { return _projectRelativePath.MakePath ((*_cur)->GetUniqueName ()); }

private:
	DataBase::FileIter	_cur;
	DataBase::FileIter	_end;
	int					_size;
	Project::Path		_projectRelativePath;
};

class IsProjectFolder : public std::unary_function<FileData const *, bool>
{
public:
	bool operator() (FileData const * fileData) const
	{
		return !fileData->GetState ().IsNone () &&
			   !fileData->GetType ().IsRoot ()  &&
			   fileData->GetType ().IsFolder ();
	}
};

class XAllProjectFoldersSequencer : public FileDataSequencer
{
public:
	XAllProjectFoldersSequencer (DataBase const & dataBase)
		: _cur (std::find_if (dataBase.xbegin (), dataBase.xend (), IsProjectFolder ())),
		  _end (dataBase.xend ()),
		  _size (dataBase.FileCount ()),
		  _projectRelativePath (dataBase)
	{}

    bool AtEnd () const { return _cur == _end; }
    void Advance ()
	{
		++_cur;
		_cur = std::find_if (_cur, _end, IsProjectFolder ());
	}

	FileData const * GetFileData () const { return *_cur; }
	unsigned int size () const { return _size; }
	char const * GetProjectRelativePath () { return _projectRelativePath.XMakePath ((*_cur)->GetUniqueName ()); }

private:
	DataBase::FileIter	_cur;
	DataBase::FileIter	_end;
	int					_size;
	Project::XPath		_projectRelativePath;
};

class AllProjectFoldersSequencer : public FileDataSequencer
{
public:
	AllProjectFoldersSequencer (DataBase const & dataBase)
		: _cur (std::find_if (dataBase.begin (), dataBase.end (), IsProjectFolder ())),
		  _end (dataBase.end ()),
		  _size (dataBase.FileCount ()),
		  _projectRelativePath (dataBase)
	{}

    bool AtEnd () const { return _cur == _end; }
    void Advance ()
	{
		++_cur;
		_cur = std::find_if (_cur, _end, IsProjectFolder ());
	}

	FileData const * GetFileData () const { return *_cur; }
	unsigned int size () const { return _size; }
	char const * GetProjectRelativePath () { return _projectRelativePath.MakePath ((*_cur)->GetUniqueName ()); }

private:
	DataBase::FileIter	_cur;
	DataBase::FileIter	_end;
	int					_size;
	Project::Path		_projectRelativePath;
};

#endif
