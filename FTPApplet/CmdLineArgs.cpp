//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include "precompiled.h"
#include "CmdLineArgs.h"

#include <CmdLineScanner.h>
#include <StringOp.h>

// Command line arguments:
//	<operation> <local path> <remote path> <server> [<user> <password> [noCleanup]]
//
// Where:
//	<opeartion> = fileupload | folderupload | filedownload | folderdownload
//
// For file upload/download operation source path must be a file path (file name must be present).
// If destination path cannot specify file name.

CmdLineArgs::CmdLineArgs (char const * args)
{
	CmdLineScanner scanner (args);
	while (scanner.Look () != CmdLineScanner::tokEnd)
	{
		if (scanner.Look () == CmdLineScanner::tokString)
		{
			std::string arg = scanner.GetString ();
			_args.push_back (arg);
		}
		scanner.Accept ();
	}
}

bool CmdLineArgs::IsUpload () const
{
	std::string const & arg = GetString (0);
	return IsNocaseEqual (arg, "fileupload") || IsNocaseEqual (arg, "folderupload");
}

bool CmdLineArgs::IsDownload () const
{
	std::string const & arg = GetString (0);
	return IsNocaseEqual (arg, "filedownload") || IsNocaseEqual (arg, "folderdownload");
}

bool CmdLineArgs::IsFileUpload () const
{
	std::string const & arg = GetString (0);
	return IsNocaseEqual (arg, "fileupload");
}

bool CmdLineArgs::IsFileDownload () const
{
	std::string const & arg = GetString (0);
	return IsNocaseEqual (arg, "filedownload");
}

std::string const & CmdLineArgs::GetLocalPath () const
{
	return GetString (1);
}

std::string const & CmdLineArgs::GetRemotePath () const
{
	return GetString (2);
}

std::string const & CmdLineArgs::GetServer () const
{
	return GetString (3);
}

std::string const & CmdLineArgs::GetUser () const
{
	return GetString (4);
}

std::string const & CmdLineArgs::GetPassword () const
{
	return GetString (5);
}

bool CmdLineArgs::IsNoCleanup() const
{
	std::string const & arg = GetString (6);
	return IsNocaseEqual (arg, "noCleanup");
}

std::string const & CmdLineArgs::GetString (unsigned idx) const
{
	if (idx < _args.size ())
		return _args [idx];
	else
		return _empty;
}
