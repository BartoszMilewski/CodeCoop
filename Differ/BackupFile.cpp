//----------------------------------
// Reliable Software (c) 1998 - 2007
//----------------------------------

#include "precompiled.h"
#include "BackupFile.h"

#include <File/File.h>
#include <Ex/WinEx.h>
#include <Dbg/Assert.h>

BackupFile::BackupFile (std::string const & originalPath)
	: _commit (false),
	  _backup ("~")
{
	Assert (!File::IsReadOnly (originalPath));

	PathSplitter orgPath (originalPath);
	_original = orgPath.GetFileName ();
	_original += orgPath.GetExtension ();
	_backup += orgPath.GetFileName ();
	_backup += orgPath.GetExtension ();

	PathMaker projPath (orgPath.GetDrive (), orgPath.GetDir (), "", "");
	_path.Change (projPath);

	std::string backupPath (_path.GetFilePath (_backup));
	// Just in case, delete file from backupPath, otherwise move might fail
	File::Delete (backupPath.c_str ());
	File::Move (_path.GetFilePath (_original.c_str ()), backupPath.c_str ());
}

BackupFile::~BackupFile ()
{
	if (!_commit)
	{
		std::string originalPath (_path.GetFilePath (_original));
		// Copy first, then delete backup
		File::Copy (_path.GetFilePath (_backup.c_str ()), originalPath.c_str ());
		File::Delete (_path.GetFilePath (_backup.c_str ()));
	}
}

void BackupFile::Commit ()
{
	_commit = true;
	File::Delete (_path.GetFilePath (_backup.c_str ()));
}
