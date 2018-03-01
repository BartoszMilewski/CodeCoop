//------------------------------------
//  (c) Reliable Software, 2006 - 2007
//------------------------------------

#include "precompiled.h"
#include "FileDropInfoCollector.h"
#include "FileDropInfo.h"
#include "Directory.h"
#include "Database.h"
#include "UserDropPreferences.h"

#include <File/File.h>

bool FileDropInfoCollector::GatherFileInfo (PathSequencer & pathSeq,
											bool allControlledFileOverride,
											bool allControlledFolderOverride)
{
	auto_vector<DropInfo> files;
	DropPreferences userPrefs (_parentWin,
							   _projectName,
							   _canMakeChanges,
							   allControlledFileOverride,
							   allControlledFolderOverride);
	InspectFiles (pathSeq, files, userPrefs);
	unsigned dropFilesLeft = files.size ();
	for (auto_vector<DropInfo>::const_iterator iter = files.begin (); iter != files.end (); ++iter)
	{
		userPrefs.SetMultiFileDrop (dropFilesLeft > 1);
		DropInfo const * dropInfo = *iter;
		if (!dropInfo->RecordDropInfo (*this, userPrefs))
			return false;	// User canceled getting file information
		--dropFilesLeft;
	}

	return true;
}

void FileDropInfoCollector::InspectFiles (PathSequencer & pathSeq,
										  auto_vector<DropInfo> & files,
										  DropPreferences & userPrefs)
{
	// Find out what we know about dropped files
	bool notControlledSeen = false;
	bool controlledSeen = false;
	for (unsigned i = 0; !pathSeq.AtEnd (); pathSeq.Advance (), ++i)
	{
		std::string const & sourcePath = pathSeq.GetFilePath ();
		std::unique_ptr<DropInfo> dropInfo = CreateDropInfo (sourcePath);
		if (dropInfo.get () != 0)
		{
			if (!notControlledSeen)
				notControlledSeen = !dropInfo->IsControlled ();
			if (!controlledSeen)
			{
				FileControlState targetState = dropInfo->GetCtrlState ();
				controlledSeen = dropInfo->IsControlled () && !targetState.IsDeleted ();
			}

			files.push_back (std::move(dropInfo));
		}
	}
	userPrefs.SetControlledState (notControlledSeen, controlledSeen);
}

std::unique_ptr<DropInfo> FileDropInfoCollector::CreateDropInfo (std::string const & sourcePath)
{
	FilePath currentFolder (_currentDir.GetCurrentPath ());
	if (currentFolder.HasPrefix (sourcePath))
	{
		// Ignore recursive source path
		return std::unique_ptr<DropInfo> ();
	}

	PathSplitter splitter (sourcePath);
	std::string fileName (splitter.GetFileName ());
	fileName += splitter.GetExtension ();
	std::string targetPath (_currentDir.GetFilePath (fileName.c_str ()));
	if (IsFileNameEqual (sourcePath, targetPath))
	{
		if (_isPaste)
		{
			targetPath = File::CreateUniqueName (sourcePath.c_str (), "copy");
			PathSplitter splitter (targetPath);
			fileName = splitter.GetFileName ();
			fileName += splitter.GetExtension ();
		}
		else
			return std::unique_ptr<DropInfo> ();	// Ignore source path
	}

	FileControlState ctrlState;
	if (File::Exists (targetPath.c_str ()))
		ctrlState.SetTargetExists ();

	GlobalId folderId = _currentDir.GetCurrentId ();
	if (folderId != gidInvalid)
	{
		UniqueName uname (folderId, fileName);
		FileData const * fd = _fileIndex.FindProjectFileByName (uname);
		if (fd != 0)
		{
			FileState fileState = fd->GetState ();
			if (!fileState.IsCheckedIn ())
				ctrlState.SetCheckedOut ();
			if (fileState.IsToBeDeleted () || fileState.IsNone ())
				ctrlState.SetDeleted ();
			if (fd->GetType ().IsFolder ())
			{
				return std::unique_ptr<DropInfo> (new ControlledFolderDropInfo (sourcePath,
																			  targetPath,
																			  fd->GetGlobalId (),
																			  ctrlState));
			}
			else
			{
				return std::unique_ptr<DropInfo> (new ControlledFileDropInfo (sourcePath,
																			targetPath,
																			fd->GetGlobalId (),
																			ctrlState));
			}
		}
	}

	if (File::IsFolder (sourcePath.c_str ()))
	{
		return std::unique_ptr<DropInfo> (new NotControlledFolderDropInfo (sourcePath,
																		 targetPath,
																		 ctrlState));
	}

	return std::unique_ptr<DropInfo> (new NotControlledFileDropInfo (sourcePath,
																   targetPath,
																   ctrlState));
}

char const * FileDropInfoCollector::GetCurrentPath () const
{
	return _currentDir.GetCurrentPath ();
}

void FileDropInfoCollector::FolderDown (std::string const & folderName)
{
	_currentDir.Down (folderName);
}

void FileDropInfoCollector::FolderUp ()
{
	_currentDir.Up ();
}

void FileDropInfoCollector::Cleanup ()
{
	_copyRequest.Cleanup ();
	for (std::vector<std::string>::const_iterator iter = _createdFolders.begin ();
		 iter != _createdFolders.end ();
		 ++iter)
	{
		std::string const & folder = *iter;
		::RemoveDirectory (folder.c_str ());
	}
}
