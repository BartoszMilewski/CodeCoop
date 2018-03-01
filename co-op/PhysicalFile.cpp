//----------------------------------
// (c) Reliable Software 1997 - 2007
//----------------------------------

#include "precompiled.h"
#include "PhysicalFile.h"
#include "FileData.h"
#include "ScriptCommandList.h"
#include "PathFind.h"
#include "FileList.h"
#include "RefFile.h"
#include "Diff.h"

#include <Ex/WinEx.h>
#include <File/File.h>
#include <Dbg/Out.h>

bool PhysicalFile::IsTextual () const
{
	return _fileData.GetType ().IsTextual ();
}

bool PhysicalFile::IsBinary () const
{
	return _fileData.GetType ().IsBinary ();
}

bool PhysicalFile::IsDifferent (Area::Location oldLoc, Area::Location newLoc) const
{
	std::string oldName (GetFullPath (oldLoc));
	std::string newName (GetFullPath (newLoc));
	return !File::IsEqualQuick (oldName.c_str (), newName.c_str ());
}

bool PhysicalFile::IsContentsDifferent (Area::Location oldLoc, Area::Location newLoc) const
{
	std::string oldName (GetFullPath (oldLoc));
	std::string newName (GetFullPath (newLoc));
	return !File::IsContentsEqual (oldName.c_str (), newName.c_str ());
}

bool PhysicalFile::ExistsIn (Area::Location loc) const
{
	char const * fullPath = GetFullPath (loc);
	return File::Exists (fullPath);
}

bool PhysicalFile::IsReadOnlyInProject () const
{
	char const * fullPath = GetFullPath (Area::Project);
	return File::IsReadOnly (fullPath);
}

bool PhysicalFile::ExistsInProjectAs (UniqueName const & alias) const
{
	char const * fullPath = _pathFinder.GetFullPath (alias);
	return File::Exists (fullPath);
}

bool PhysicalFile::IsRenamedIn (Area::Location loc) const
{
	return _fileData.IsRenamedIn (loc);
}

UniqueName const & PhysicalFile::GetUnameIn (Area::Location loc) const
{
	return _fileData.GetUnameIn (loc);
}

UniqueName const & PhysicalFile::GetUniqueName () const
{
	return _fileData.GetUniqueName ();
}

void PhysicalFile::MakeReadWriteIn (Area::Location loc) const
{
	char const * fullPath = GetFullPath (loc);
	if (File::Exists (fullPath))
		File::MakeReadWrite (fullPath);
}

void PhysicalFile::MakeReadOnlyInProject () const
{
	char const * fullPath = GetFullPath (Area::Project);
	if (File::Exists (fullPath))
		File::MakeReadOnly (fullPath);
}

void PhysicalFile::OverwriteInProject (std::vector<char> & buf)
{
	char const * projectPath = GetFullPath (Area::Project);
	std::string preMergePath = File::CreateUniqueName (projectPath, "pre-merge");
	// Copy project file to 'pre-merge' file
	File::Copy (projectPath, preMergePath.c_str ());

	{
		MemFileAlways file (projectPath);
		File::Size newSize (buf.size (), 0);
		file.ResizeFile (newSize);
		std::copy (buf.begin (), buf.end (), file.GetBuf ());
	}

	// Overwrite successfull - delete 'pre-merge' file
	File::DeleteNoEx (preMergePath.c_str ());
}

void PhysicalFile::CopyToProject (Area::Location from) const
{
	std::string fromPath (GetFullPath (from));
	File::Copy (fromPath.c_str (), GetFullPath (Area::Project));
}

void PhysicalFile::CreateEmptyIn (Area::Location loc) const
{
	char const * path = GetFullPath (loc);
	File emptyFile (path, File::CreateAlwaysMode ());
}

CheckSum PhysicalFile::GetCheckSum (Area::Location loc) const
{
	char const * fullPath = GetFullPath (loc);
	return CheckSum (fullPath);
}

FileState PhysicalFile::GetState () const
{
	return _fileData.GetState ();
}

GlobalId PhysicalFile::GetGlobalId () const
{
	return _fileData.GetGlobalId ();
}

char const * PhysicalFile::GetFullPath (Area::Location loc) const
{
	return _pathFinder.GetFullPath (_fileData.GetGlobalId (), loc);
}

void PhysicalFile::CopyToLocalEdits () const
{
	char const * pathFrom = GetFullPath (Area::Project);
	char const * pathTo = GetFullPath (Area::LocalEdits);
	File::CopyNoEx (pathFrom, pathTo);
}

//
// Physical file used during transactions
//

bool XPhysicalFile::MakeDiffScriptCmd (FileData const & fileData,
									   CommandList & cmdList,
									   CheckSum & newCheckSum) const
{
	Assert (_fileGid == fileData.GetGlobalId ());
	ReferenceFile reference (GetFullPath (Area::Original));
	char const * path = GetFullPath (Area::Project);
	// Fill the command with diff data
	if (reference.Diff (path, fileData ))
	{
		// Edit changes
		cmdList.push_back (reference.GetCmd ());
		newCheckSum = reference.GetNewCheckSum ();
		return true;
	}
	if (fileData.IsRenamedIn (Area::Original) || fileData.IsTypeChangeIn (Area::Original))
	{
		// Rename or move or change type - store in the script command the old file checksum
		cmdList.push_back (reference.GetCmd ());
	}
	return false;
}

void XPhysicalFile::MakeDeletedScriptCmd (FileData const & fileData, CommandList & cmdList) const
{
	Assert (_fileGid == fileData.GetGlobalId ());
	std::unique_ptr<DeleteCmd> cmd (new DeleteCmd (fileData));

	// Copy the deleted file from the Original to the Temporary Area
	// so we can avoid storing file contents in the memory.
	// Don't remember the Temporary Area file copy on the transaction file list
	// because we want to have this file on disk after the check-in transaction
	// commits.
	Copy (Area::Original, Area::Temporary);
	char const * filePath = GetFullPath (Area::Temporary);
	FileInfo fileInfo (filePath);
	cmd->SetSource (filePath, fileInfo.GetSize (), true); // delete tmp copy
	CheckSum onDiskChecksum = CheckSum (filePath);
	if (fileData.GetCheckSum () != onDiskChecksum && fileData.GetType ().IsRecoverable ())
		throw Win::InternalException ("The file original copy has been modified during the check-in.",
									  GetFullPath (Area::Project));

	cmd->SetNewCheckSum (CheckSum ());

	cmdList.push_back (std::move(cmd));
}

void XPhysicalFile::MakeNewScriptCmd (FileData const & fileData,
									  CommandList & cmdList,
									  CheckSum & checkSum) const
{
	Assert (_fileGid == fileData.GetGlobalId ());
	// Prepare a script that contains the whole text of the file
	char const * filePath = GetFullPath (Area::Project);
	checkSum = CheckSum (filePath);

	FileInfo fileInfo (filePath);
	std::unique_ptr<WholeFileCmd> cmd (new WholeFileCmd (fileData));
	Assert (cmd->GetOldCheckSum () == CheckSum ());
	cmd->SetSource (filePath, fileInfo.GetSize ());
	cmd->SetNewCheckSum (checkSum);

	cmdList.push_back (std::move(cmd));
}

void XPhysicalFile::MakeNewFolderCmd (FileData const & fileData, CommandList & cmdList) const
{
	Assert (_fileGid == fileData.GetGlobalId ());
	// prepare a script that adds New folder to the project
	std::unique_ptr<ScriptCmd> cmd (new NewFolderCmd (fileData));
	cmdList.push_back (std::move(cmd));
}

void XPhysicalFile::MakeDeleteFolderCmd (FileData const & fileData, CommandList & cmdList) const
{
	Assert (_fileGid == fileData.GetGlobalId ());
	// prepare a script that deletes folder from the project
	std::unique_ptr<DeleteFolderCmd> cmd (new DeleteFolderCmd (fileData));
	cmdList.push_back (std::move(cmd));
}

bool XPhysicalFile::IsMissingInProject () const
{
	char const * path = GetFullPath (Area::Project);
	return !File::Exists (path);
}

bool XPhysicalFile::IsDifferent (Area::Location oldLoc, Area::Location newLoc) const
{
	std::string oldName (GetFullPath (oldLoc));
	std::string newName (GetFullPath (newLoc));
	return !File::IsEqualQuick (oldName.c_str (), newName.c_str ());
}

bool XPhysicalFile::IsContentsDifferent (Area::Location oldLoc, Area::Location newLoc) const
{
	std::string oldName (GetFullPath (oldLoc));
	std::string newName (GetFullPath (newLoc));
	return !File::IsContentsEqual (oldName.c_str (), newName.c_str ());
}


void XPhysicalFile::DeleteFrom (Area::Location loc) const
{
	_fileList.RememberDeleted (GetFullPath (loc), _isFolder);
}

void XPhysicalFile::CopyRemember (Area::Location from, Area::Location to) const
{
	Assert (!_isFolder);
	Copy (from, to);
	_fileList.RememberCreated (GetFullPath (to), _isFolder);
}

void XPhysicalFile::CopyRemember (UniqueName const & srcUname, Area::Location to) const
{
	Assert (!_isFolder);
	std::string fromPath (_pathFinder.GetFullPath (srcUname));
	std::string toPath (GetFullPath (to));
	File::Copy (fromPath.c_str (),	toPath.c_str ());
	_fileList.RememberCreated (toPath.c_str (), _isFolder);
}

void XPhysicalFile::MakeReadWriteIn (Area::Location loc) const
{
	char const * path = GetFullPath (loc);
	_fileList.RememberToMakeReadWrite (path);
}

void XPhysicalFile::MakeReadOnlyIn (Area::Location loc) const
{
	char const * path = GetFullPath (loc);
	_fileList.RememberToMakeReadOnly (_fileGid, path);
}

void XPhysicalFile::ForceReadWriteIn (Area::Location loc) const
{
	char const * path = GetFullPath (loc);
	File::MakeReadWrite (path);
}

void XPhysicalFile::TouchIn (Area::Location loc) const
{
	char const * fullPath = GetFullPath (loc);
	File::ForceTouchNoEx (fullPath);
}

void XPhysicalFile::Copy (Area::Location from, Area::Location to) const
{
	Assert (!_isFolder);
	std::string fromPath (GetFullPath (from));
	std::string toPath (GetFullPath (to));
	File::Copy (fromPath.c_str (),	toPath.c_str ());
}

std::string XPhysicalFile::GetName () const
{
	char const * projectPath = GetFullPath (Area::Project);
	PathSplitter splitter (projectPath);
	std::string name (splitter.GetFileName ());
	name += splitter.GetExtension ();
	return name;
}

void XPhysicalFile::CopyOverToProject (Area::Location from, bool needTouch) const
{
	dbg << "Copy physical file from " << from << " to the staging area" << std::endl;
	Assert (!_isFolder);
	// Make copy in the Project Staging Area
	CopyRemember (from, Area::Staging);
	// And remember to move it later to the Project Area
	char const * toPath = GetFullPath (Area::Project);
	_fileList.RememberToMove (_fileGid, GetFullPath (Area::Staging), toPath, needTouch);
}

void XPhysicalFile::OverwriteInProject (std::vector<char> & buf)
{
	// Create a New file in the staging area, write into it 
	// and remember to copy it to project
	char const * fromPath = GetFullPath (Area::Staging);
	if (File::Exists (fromPath))
		File::Delete (fromPath);

	MemFileNew file (fromPath, File::Size (buf.size (), 0));
	std::copy (buf.begin (), buf.end (), file.GetBuf ());

	char const * toPath = GetFullPath (Area::Project);
	_fileList.RememberToMove (_fileGid, fromPath, toPath, true);	// touch!
}

void XPhysicalFile::CopyOverToProject (Area::Location from, UniqueName const & uname) const
{
	Assert (!_isFolder);
	// Make copy in the Project Staging Area
	CopyRemember (from, Area::Staging);
	ForceReadWriteIn (Area::Staging);
	// And remember to move it later to the Project Area
	char const * toPath = _pathFinder.XGetFullPath (uname);
	_fileList.RememberToMove (_fileGid, GetFullPath (Area::Staging), toPath, true);	// touch!
}

void XPhysicalFile::OverwriteInProject (std::vector<char> & buf, UniqueName const & uname)
{
	// Create a New file in the staging area, write into it 
	// and remember to copy it to project
	char const * fromPath = GetFullPath (Area::Staging);
	if (File::Exists (fromPath))
		File::Delete (fromPath);

	MemFileNew file (fromPath, File::Size (buf.size (), 0));
	std::copy (buf.begin (), buf.end (), file.GetBuf ());

	char const * toPath = _pathFinder.XGetFullPath (uname);
	_fileList.RememberToMove (_fileGid, fromPath, toPath, true);	// touch!
}

void XPhysicalFile::CreateFolder ()
{
	char const * newFolderPath = GetFullPath (Area::Project);
	if (File::CreateNewFolder (newFolderPath))
	{
		_fileList.RememberCreated (newFolderPath, true);
	}
}

CheckSum XPhysicalFile::GetCheckSum (Area::Location loc) const
{
	char const * filePath = GetFullPath (loc);
	return CheckSum (filePath);
}

char const * XPhysicalFile::GetFullPath (Area::Location loc) const
{
	return _pathFinder.XGetFullPath (_fileGid, loc);
}

