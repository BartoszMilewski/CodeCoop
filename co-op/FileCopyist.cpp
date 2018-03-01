//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include "precompiled.h"
#include "FileCopyist.h"
#include "CopyRequest.h"
#include "FtpProxy.h"
#include "AppInfo.h"
#include "OutputSink.h"

#include <File/Path.h>
#include <File/Dir.h>
#include <Com/ShellRequest.h>

FileCopyist::FileCopyist (FileCopyRequest const & request)
	: _overwrite (request.IsOverwriteExisting ()),
	  _completeCleanupPossible (false),
	  _root (request.GetTargetFolder ())
{
	if (request.IsInternet ())
		return;

	Assert (request.IsLAN () || request.IsMyComputer ());
	if (File::Exists (request.GetTargetFolder ()))
	{
		// Complete cleanup possible only if target folder is empty
		FilePath path (request.GetTargetFolder ());
		FileSeq seq (path.GetAllFilesPath ());
		_completeCleanupPossible = seq.AtEnd ();
	}
	else
	{
		// Target folder doesn't exists on disk - complete cleanup possible
		_completeCleanupPossible = true;
	}
}

void FileCopyist::RememberFile (std::string const & srcFullPath, std::string const & tgtRelPath)
{
	PartialPathSeq seq (tgtRelPath.c_str ());
	_root.AddCopyItem (srcFullPath, seq);
}

bool FileCopyist::DoCopy ()
{
	ShellMan::CopyRequest shellCopyRequest;
	if (IsOverwriteExisting ())
		shellCopyRequest.OverwriteExisting ();

	try
	{
		File::Vpath workFolder;
		_root.BuildCopyList (workFolder, shellCopyRequest);
		std::string title ("Copying file(s) to ");
		title += _root.GetFolderName ();
		shellCopyRequest.DoCopy (TheAppInfo.GetWindow (), title.c_str ());
		shellCopyRequest.MakeDestinationReadWrite ();
		// Check if all folders made to the target location.
		// Empty folders will not be created during file copy,
		// so we have to created them explicitly
		FilePath targetFolder (_root.GetFolderName ());
		for (std::vector<std::string>::const_iterator iter = _sourceFolders.begin ();
			 iter != _sourceFolders.end ();
			 ++iter)
		{
			std::string const & folder = *iter;
			char const * targetPath = targetFolder.GetFilePath (folder);
			File::CreateFolder (targetPath);
		}
		return true;
	}
	catch (Win::Exception ex)
	{
		if (ex.GetMessage () != 0)
			throw;
	}
	catch ( ... )
	{
		Win::ClearError ();
		throw Win::Exception ("Unknown exception during file copying.");
	}

	// User aborted copy operation - perform cleanup
	if (File::Exists (_root.GetFolderName ()))
	{
		try
		{
			if (_completeCleanupPossible)
			{
				// Delete full tree
				File::DeleteTree (_root.GetFolderName ());
			}
			else
			{
				// Delete only copied files
				shellCopyRequest.Cleanup ();
				// Remove empty folders that were created at target.
				FilePath targetRoot (_root.GetFolderName ());
				for (std::vector<std::string>::const_iterator iter = _sourceFolders.begin ();
					 iter != _sourceFolders.end ();
					 ++iter)
				{
					std::string const & folder = *iter;
					char const *  targetFolderPath = targetRoot.GetFilePath (folder);
					if (File::IsTreeEmpty (targetFolderPath))
						File::RemoveEmptyFolder (targetFolderPath);
				}
			}
		}
		catch ( ... )
		{
			// Ignore all exceptions during cleanup
		}
	}
	TheOutput.Display ("Copy operation aborted by the user.");
	return false;
}

void FileCopyist::DoUpload (Ftp::Login const & login,
							std::string const & projectName,
							std::string const & targetFolder)
{
	// Make a local copy of exproted project files
	TmpPath tmpPath;
	char const * tmpSourceFolder = tmpPath.GetFilePath (projectName);
	if (File::Exists (tmpSourceFolder))
		File::DeleteTree (tmpSourceFolder);
	_root.SetFolderName (tmpSourceFolder);
	_completeCleanupPossible = true;
	if (DoCopy ())
	{
		// Upload local copy to the server - long operation
		ExecuteFtpUpload (tmpSourceFolder,
						  targetFolder,
						  login);
	}
}

void FileCopyist::CopyTreeNode::AddCopyItem (std::string const & srcFullPath,
											 PartialPathSeq & tgtSeq)
{
	if (tgtSeq.IsLastSegment ())
	{
		// Remember file to copy
		_files.push_back (std::make_pair (srcFullPath, tgtSeq.GetSegment ()));
	}
	else
	{
		// Add file in the appropriate sub-folder
		auto_vector<CopyTreeNode>::iterator iter =
			std::find_if (_subFolders.begin (),
						  _subFolders.end (),
						  IsEqualFolderName (tgtSeq.GetSegment ()));
		CopyTreeNode * subFolder = 0;
		if (iter == _subFolders.end ())
		{
			// Sub-folder seen for the first time
			std::unique_ptr<CopyTreeNode> newSubFolder (new CopyTreeNode (tgtSeq.GetSegment ()));
			_subFolders.push_back (std::move(newSubFolder));
			subFolder = _subFolders.back ();
		}
		else
		{
			subFolder = *iter;
		}
		tgtSeq.Advance ();
		subFolder->AddCopyItem (srcFullPath, tgtSeq);
	}
}

void FileCopyist::CopyTreeNode::BuildCopyList (File::Vpath & currentPath,
											   ShellMan::CopyRequest & request) const
{
	Assert (!IsEmpty ());
	currentPath.DirDown (_folderName);
	for (std::vector<CopyItem>::const_iterator iter = _files.begin (); iter != _files.end (); ++iter)
	{
		CopyItem const & copyItem = *iter;
		request.AddCopyRequest (copyItem.first.c_str (),
								currentPath.GetFilePath (copyItem.second).c_str ());
	}
	for (auto_vector<CopyTreeNode>::const_iterator iter = _subFolders.begin ();
		 iter != _subFolders.end ();
		 ++iter)
	{
		CopyTreeNode const * subFolder = *iter;
		subFolder->BuildCopyList (currentPath, request);
	}
	currentPath.DirUp ();
}
