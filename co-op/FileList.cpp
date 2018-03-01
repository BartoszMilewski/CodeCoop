//---------------------------------------------
// (c) Reliable Software 1997 -- 2003
//---------------------------------------------

#include "precompiled.h"
#include "FileList.h"

#include <File/File.h>
#include <Com/Shell.h>
#include <Dbg/Assert.h>
#include <Ex/WinEx.h>

TransactionFileList::~TransactionFileList()
{
	if (!_commit)
	{
		// Clean up files created during transaction
		std::for_each (_created.begin (), _created.end (), DeletePath);
	}
}

void TransactionFileList::RememberDeleted (char const * fullPath, bool isFolder)
{
	Assert (!_commit);
	PathInfo info (fullPath, isFolder);
	_deleted.push_back (info);
}

void TransactionFileList::RememberCreated (char const * fullPath, bool isFolder)
{
	Assert (!_commit);
	PathInfo info (fullPath, isFolder);
	_created.push_back (info);
}

void TransactionFileList::RememberToMakeReadOnly (GlobalId gid, char const * path) 
{
	// If file is moved from staging area set its read-only bit, otherwise
	// remember file's path. Not every read-only file is moved from the staging
	// area, for example during check-in file in the project area is made read-only
	MoveIterator iter = std::find_if (_moved.begin (), _moved.end (), IsEqualGid (gid));
	if (iter != _moved.end ())
		iter->_readOnly = true;	
	else
		_makeReadOnly.push_back (path); 
}

void TransactionFileList::RememberToMove (GlobalId gid, char const * fromPath, char const * toPath, bool touch)
{
	Assert (!_commit);
	Assert (!File::IsFolder (fromPath));
	Assert (!File::IsFolder (toPath));
	MoveInfo info (gid, true, touch, fromPath, toPath);
	_moved.push_back (info);
}

void TransactionFileList::CommitPhaseOne ()
{
	// Perepare for delayed move
	std::for_each (_moved.begin (), _moved.end (), PrepareMove);
	_commit = true;
}

void TransactionFileList::CommitOrDie () throw (Win::ExitException)
{
	Assert (_commit);
	std::for_each (_deleted.begin (), _deleted.end (), DeletePath);
	// Perfrom delayed move
	std::for_each (_moved.begin (), _moved.end (), Move);
	// Set ReadOnly attribute
	std::for_each (_makeReadOnly.begin (), _makeReadOnly.end (), File::MakeReadOnlyNoEx);
	// Set ReadWrite attribute
	std::for_each (_makeReadWrite.begin (), _makeReadWrite.end (), File::MakeReadWriteNoEx);
}

void TransactionFileList::SetOverwrite (GlobalId gid, bool flag)
{
	MoveIterator iter = std::find_if (_moved.begin (), _moved.end (), IsEqualGid (gid));
	if (iter != _moved.end ())
		iter->_overwrite = flag;
}

void TransactionFileList::DeletePath (PathInfo const & path)
{
	char const * p = path._path.c_str ();
	if (path._isFolder)
	{
		// We do not pass here the parent hwnd to the Shell
		// for quiet operation
		ShellMan::QuietDelete (0, p);
	}
	else
	{
		File::DeleteNoEx (p);
	}
}

void TransactionFileList::PrepareMove (MoveInfo const & info)
{
	// "from" is staging area
	char const * from = info._from.c_str ();
	if (info._touch)
	{
		File::MakeReadWrite (from);
		File::Touch (from);
	}
	if (info._readOnly)
		File::MakeReadOnly (from);
}

void TransactionFileList::Move (MoveInfo const & info) throw (Win::ExitException)
{
	// "from" is staging area; "to" is project area
	char const * from = info._from.c_str ();
	char const * to = info._to.c_str ();
	if (!info._overwrite && !File::IsContentsEqual (from, to))
		File::Rename2PreviousOrDie (to);

	File::CopyOrDie (from, to);
	File::DeleteNoEx (from);
}
