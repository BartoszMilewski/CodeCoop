//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------

#include "precompiled.h"
#include "FileDropInfo.h"
#include "FileDropInfoCollector.h"
#include "UserDropPreferences.h"
#include "OutputSink.h"

#include <File/File.h>

bool ControlledFileDropInfo::RecordDropInfo (FileDropInfoCollector & collector, DropPreferences & userPrefs) const
{
	if (_targetState.IsCheckedOut ())
	{
		if (_targetState.IsDeleted ())
		{
			if (_targetState.TargetExists ())
			{
				// Ask the user if we can overwrite the target
				if (!userPrefs.AskCanOverwrite (collector.GetCurrentPath (), *this))
					return false;	// User canceled file drop

				if (userPrefs.CanOverwrite (*this))
					collector.RememberCopyRequest (_sourcePath, _targetPath);
				else
					return true;	// User doesn't want to overwrite deleted file
			}
			else
			{
				collector.RememberCopyRequest (_sourcePath, _targetPath);
			}

			// Ask the user if he/she wants to add dropped file/folder to the project
			if (!userPrefs.AskAddToProject (*this))
				return false; // User canceled file drop

			if (userPrefs.CanAddToProject ())
				collector.RememberFileToAdd (_targetPath);
		}
		else
		{
			// Ask the user if we can overwrite the target
			if (!userPrefs.AskCanOverwrite (collector.GetCurrentPath (), *this))
				return false;	// User canceled file drop

			if (userPrefs.CanOverwrite (*this))
				collector.RememberCopyRequest (_sourcePath, _targetPath);
		}
	}
	else
	{
		// Checked in
		if (_targetState.TargetExists ())
		{
			// Ask the user if we can overwrite the target
			if (!userPrefs.AskCanOverwrite (collector.GetCurrentPath (), *this))
				return false;	// User canceled file drop

			if (userPrefs.CanOverwrite (*this))
			{
				collector.RememberFileToCheckout (_gid);
				collector.RememberCopyRequest (_sourcePath, _targetPath);
			}
		}
		else
		{
			std::string info ("The following project file does not exist on disk:\n\n");
			info += _targetPath;
			info += "\n\nPlease, execute Project>Repair first and then try file copy.";
			TheOutput.Display (info.c_str ());
			return false;
		}
	}
	return true;
}

bool ControlledFolderDropInfo::RecordDropInfo (FileDropInfoCollector & collector, DropPreferences & userPrefs) const
{
	if (!_targetState.TargetExists ())
	{
		File::CreateFolder (_targetPath.c_str (), false);	// Not quiet
		collector.RememberCreatedFolder (_targetPath);
	}
	// Recursive project folder copy
	DirectoryListing dirList (_sourcePath);
	collector.FolderDown (_fileName);
	DirectoryListing::Sequencer seq (dirList);
	bool result = collector.GatherFileInfo (seq,
											userPrefs.GetAllControlledFilesOverride (),
											userPrefs.GetAllControlledFoldersOverride ());	
	collector.FolderUp ();
	return result;
}

bool NotControlledFileDropInfo::RecordDropInfo (FileDropInfoCollector & collector, DropPreferences & userPrefs) const
{
	if (_targetState.TargetExists ())
	{
		// Ask the user if we can overwrite the target
		if (!userPrefs.AskCanOverwrite (collector.GetCurrentPath (), *this))
			return false;	// User canceled file drop

		if (userPrefs.CanOverwrite (*this))
			collector.RememberCopyRequest (_sourcePath, _targetPath);
		else
			return true;	// User doesn't want to overwrite not controlled file
	}
	else
	{
		collector.RememberCopyRequest (_sourcePath, _targetPath);
	}

	// Ask the user if he/she wants to add dropped file/folder to the project
	if (!userPrefs.AskAddToProject (*this))
		return false; // User canceled file drop

	if (userPrefs.CanAddToProject ())
		collector.RememberFileToAdd (_targetPath);
	return true;
}

bool NotControlledFolderDropInfo::RecordDropInfo (FileDropInfoCollector & collector, DropPreferences & userPrefs) const
{
	if (!_targetState.TargetExists ())
	{
		File::CreateFolder (_targetPath.c_str (), false);	// Not quiet
		collector.RememberCreatedFolder (_targetPath);
	}
	DirectoryListing dirList (_sourcePath);
	DirectoryListing::Sequencer seq (dirList);
	bool result = true;
	if (seq.AtEnd ())
	{
		// Empty folder
		// Ask the user if he/she wants to add dropped folder to the project
		if (!userPrefs.AskAddToProject (*this))
			return false; // User canceled file drop

		if (userPrefs.CanAddToProject ())
			collector.RememberFileToAdd (_targetPath);
	}
	else
	{
		// Folder copy
		collector.FolderDown (_fileName);
		result = collector.GatherFileInfo (seq,
										   userPrefs.GetAllControlledFilesOverride (),
										   userPrefs.GetAllControlledFoldersOverride ());		
		collector.FolderUp ();
	}
	return result;
}
