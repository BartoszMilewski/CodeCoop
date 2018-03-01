#if !defined PHYSICALFILE_H
#define PHYSICALFILE_H

//----------------------------------
// (c) Reliable Software 1997 - 2006
//----------------------------------

#include "GlobalId.h"
#include "Area.h"
#include "CheckSum.h"
#include "FileState.h"

class FileData;
class TransactionFileList;
class SynchArea;
class UniqueName;
class PathFinder;
class FilePath;
class CommandList;

class PhysicalFile
{
public:
    PhysicalFile (FileData const & fileData, PathFinder & pathFinder)
        : _fileData (fileData),
          _pathFinder (pathFinder)
    {}

	bool IsTextual () const;
	bool IsBinary () const;
    bool IsDifferent (Area::Location oldLoc, Area::Location newLoc) const;
    bool IsContentsDifferent (Area::Location oldLoc, Area::Location newLoc) const;
    bool ExistsIn (Area::Location loc) const;
	bool IsReadOnlyInProject () const;
	bool ExistsInProjectAs (UniqueName const & alias) const;
	bool IsRenamedIn (Area::Location loc) const;
	UniqueName const & GetUnameIn (Area::Location loc) const;
	UniqueName const & GetUniqueName () const;
    void MakeReadWriteIn (Area::Location loc) const;
	void MakeReadOnlyInProject () const;
	void OverwriteInProject (std::vector<char> & buf);
    void CopyToProject (Area::Location from) const;
	void CreateEmptyIn (Area::Location loc) const;
    CheckSum GetCheckSum (Area::Location loc) const;
	FileState GetState () const;
	GlobalId GetGlobalId () const;
	char const * GetFullPath (Area::Location loc) const;

	void CopyToLocalEdits () const;

private:
    FileData const & _fileData;
    PathFinder &	 _pathFinder;
};

class XPhysicalFile
{
public:
    XPhysicalFile (GlobalId gid, PathFinder & pathFinder, TransactionFileList & fileList)
        : _fileGid (gid),
          _pathFinder (pathFinder),
          _isFolder (false),
		  _fileList (fileList)
    {}

    bool MakeDiffScriptCmd (FileData const & fileData, CommandList & cmdList, CheckSum & newCheckSum) const;
    void MakeDeletedScriptCmd (FileData const & fileData, CommandList & cmdList) const;
    void MakeNewScriptCmd (FileData const & fileData, CommandList & cmdList, CheckSum & newCheckSum) const;
    void MakeNewFolderCmd (FileData const & fileData, CommandList & cmdList) const;
    void MakeDeleteFolderCmd (FileData const & fileData, CommandList & cmdList) const;

    bool IsDifferent (Area::Location oldLoc, Area::Location newLoc) const;
	bool IsMissingInProject () const;
	bool IsContentsDifferent (Area::Location oldLoc, Area::Location newLoc) const;
    void MakeReadWriteIn (Area::Location loc) const;
    void MakeReadOnlyIn (Area::Location loc) const;
    void TouchIn (Area::Location loc) const;
	void ForceReadWriteIn (Area::Location loc) const;
    void CopyRemember (Area::Location from, Area::Location to) const;
	void CopyRemember (UniqueName const & srcUname, Area::Location to) const;
    void DeleteFrom (Area::Location loc) const;
    void Copy (Area::Location from, Area::Location to) const;
    void CopyOverToProject (Area::Location from, bool needTouch = true) const;
    void CopyOverToProject (Area::Location from, UniqueName const & uname) const;
	void OverwriteInProject (std::vector<char> & buf);
	void OverwriteInProject (std::vector<char> & buf, UniqueName const & uname);
    void CreateFolder ();
    CheckSum GetCheckSum (Area::Location loc) const;
	char const * GetFullPath (Area::Location loc) const;
	std::string GetName () const;
	bool ExistsInLocalEdits () const
	{
		char const * fullPath = GetFullPath (Area::LocalEdits);
		return File::Exists (fullPath);
	}

protected:
    XPhysicalFile (GlobalId gid, PathFinder & pathFinder, TransactionFileList & fileList, bool isFolder)
        : _fileGid (gid),
          _pathFinder (pathFinder),
          _isFolder (isFolder),
		  _fileList (fileList)
    {}

private:
    GlobalId				_fileGid;
    PathFinder &			_pathFinder;
	TransactionFileList &	_fileList;
    bool					_isFolder;
};

class XPhysicalFolder : public XPhysicalFile
{
public:
    XPhysicalFolder (GlobalId folderGid, PathFinder & pathFinder, TransactionFileList & fileList)
        : XPhysicalFile (folderGid, pathFinder, fileList, true)
    {}
};

#endif
