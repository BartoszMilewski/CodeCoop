//-----------------------------------
//  (c) Reliable Software 2001 - 2006
//-----------------------------------

#include "precompiled.h"
#include "FileDropDlg.h"
#include "UserDropPreferences.h"
#include "Prompter.h"

#include <Ctrl/Output.h>
#include <Win/Message.h>
#include <Com/Shell.h>
#include <File/File.h>
#include <File/Path.h>

void QuestionData::Init (char const * projName,
						 char const * projFolder,
						 DropInfo const & fileInfo,
						 bool isMultiFileDrop)
{
	_projName = projName;
	_projFolder = projFolder;
	_fileInfo = &fileInfo;
	_isMultiFileDrop = isMultiFileDrop;
	_yesToAll = false;
	_noToAll = false;
	_no = false;
}

bool DropQuestionCtrl::OnInitDialog () throw (Win::Exception)
{
	_info.Init (GetWindow (), IDC_DROP_INFO);
	_ok.Init (GetWindow (), Out::OK);
	if (_dlgData->IsMultiFileDrop ())
	{
		_yesToAll.Init (GetWindow (), IDC_DROP_YESTOALL);
		_noToAll.Init (GetWindow (), IDC_DROP_NOTOALL);
		_no.Init (GetWindow (), IDC_DROP_NO);
	}
	return true;
}

bool DropQuestionCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	switch (ctrlId)
	{
	case IDC_DROP_YESTOALL:
		_dlgData->SetYesToAll ();
		EndOk ();
		return true;
	case IDC_DROP_NOTOALL:
		_dlgData->SetNoToAll ();
		EndOk ();
		return true;
	case IDC_DROP_NO:
		_dlgData->SetNo ();
		EndOk ();
		return true;
	}
    return false;
}

bool DropQuestionCtrl::OnApply () throw ()
{
	EndOk ();
	return true;
}

FileDropQuestionCtrl::FileDropQuestionCtrl (QuestionData * data, bool isMultiFileDrop)
	: DropQuestionCtrl (data, isMultiFileDrop ? IDD_FILE_DROP : IDD_ONE_FILE_DROP)
{}

bool FileDropQuestionCtrl::OnInitDialog () throw (Win::Exception)
{
	DropQuestionCtrl::OnInitDialog ();
	Win::Dow::Handle dlgWin (GetWindow ());

	_targetIcon.Init (dlgWin, IDC_FILE_DROP_TARGET_ICON);
	_targetSize.Init (dlgWin, IDC_FILE_DROP_TARGET_SIZE);
	_targetDate.Init (dlgWin, IDC_FILE_DROP_TARGET_DATE);
	_sourceIcon.Init (dlgWin, IDC_FILE_DROP_SOURCE_ICON);
	_sourceSize.Init (dlgWin, IDC_FILE_DROP_SOURCE_SIZE);
	_sourceDate.Init (dlgWin, IDC_FILE_DROP_SOURCE_DATE);

	std::string info;
	std::string caption;
	DropInfo const * fileInfo = _dlgData->GetDropInfo ();
	FileControlState const & state = fileInfo->GetCtrlState ();
	if (fileInfo->IsControlled ())
		caption += "Controlled ";
	caption += "File Replace";
	dlgWin.SetText (caption.c_str ());
	// Ask about file overwrite permission
	info += "Folder:\n    ";
	info += _dlgData->GetProjFolder ();
	info += "\nalready contains a file named\n    ";
	info += fileInfo->GetFileName ();
	if (fileInfo->IsControlled () && state.IsCheckedOut ())
	{
		info += "\nThe file has been checked out and edited.";
	}
	_info.SetText (info.c_str ());
	// Display information about target
	{
		ShellMan::FileInfo shellInfo (fileInfo->GetTargetPath ());
		_targetIcon.SetIcon (shellInfo.GetIcon ());
		FileInfo info (fileInfo->GetTargetPath ());
		std::string size (ToString (info.GetSize ().Low ()));
		size += " bytes";
		_targetSize.SetText (size.c_str ());
		FileTime fileTime (info);
		PackedTimeStr modificationTime (fileTime);
		std::string  modified ("modified ");
		modified += modificationTime.c_str (); 
		_targetDate.SetText (modified.c_str ());
	}
	// Display information about source
	{
		ShellMan::FileInfo shellInfo (fileInfo->GetSourcePath ());
		_sourceIcon.SetIcon (shellInfo.GetIcon ());
		FileInfo info (fileInfo->GetSourcePath ());
		std::string size (ToString (info.GetSize ().Low ()));
		size += " bytes";
		_sourceSize.SetText (size.c_str ());
		FileTime fileTime (info);
		PackedTimeStr modificationTime (fileTime);
		std::string modified ("modified ");
		modified += modificationTime.c_str ();
		_sourceDate.SetText (modified.c_str ());
	}
	return true;
}

FolderDropQuestionCtrl::FolderDropQuestionCtrl (QuestionData * data, bool isMultiFileDrop)
	: DropQuestionCtrl (data, isMultiFileDrop ? IDD_FOLDER_DROP : IDD_ONE_FOLDER_DROP)
{}

bool FolderDropQuestionCtrl::OnInitDialog () throw (Win::Exception)
{
	DropQuestionCtrl::OnInitDialog ();
	std::string info;
	DropInfo const * fileInfo = _dlgData->GetDropInfo ();
	FileControlState const & state = fileInfo->GetCtrlState ();
	std::string caption;
	if (fileInfo->IsControlled ())
		caption += "Controlled ";
	caption += "Folder Replace";
	Win::Dow::Handle dlgWin (GetWindow ());
	dlgWin.SetText (caption.c_str ());
	// Ask about folder overwrite permission
	info += "Folder:\n    ";
	info += _dlgData->GetProjFolder ();
	info += "\nalready contains a folder named\n    ";
	info += fileInfo->GetFileName ();
	info += "\n\nIf the files in the existing folder have the same names as files in the folder you are copying, they will be replaced.";
	if (fileInfo->IsControlled ())
	{
		if (state.IsCheckedOut ())
		{
			info += "\n\nDo you still want to copy the folder and replace its edited contents?";
		}
		else
		{
			info += "\n\nDo you still want to checkout the folder and replace its contents?";
		}
	}
	else
	{
		info += "\n\nDo you still want to copy the folder?";
	}
	_info.SetText (info.c_str ());
	return true;
}

bool AddDropQuestionCtrl::OnInitDialog () throw (Win::Exception)
{
	DropQuestionCtrl::OnInitDialog ();
	std::string info ("Would you like to add ");
	DropInfo const * fileInfo = _dlgData->GetDropInfo ();
	// Ask about add to project permission
	if (fileInfo->IsFolder ())
		info += "folder";
	else
		info += "file";
	PathSplitter splitter (fileInfo->GetTargetPath ());
	std::string tgtName (splitter.GetFileName ());
	tgtName += splitter.GetExtension ();
	info += "\n    ";
	info += tgtName;
	info += "\nto the project '";
	info += _dlgData->GetProjName ();
	info += "'?";
	_info.SetText (info.c_str ());
	return true;
}

void DropPreferences::SetControlledState (bool notControlledSeen, bool controlledSeen)
{
	if (!notControlledSeen && controlledSeen)
	{
		// Only controlled files/folders dropped -- we will never ask about adding to project
		_option.set (NeverAddToProject);
		// We will never ask about not controlled overwrite
		_option.set (NeverOverwrite);
	}
	if (notControlledSeen && !controlledSeen)
	{
		// Only not controlled files/folders dropped -- we will never ask about controlled overwrite
		_option.set (NeverOverwriteControlled);
	}
}

bool DropPreferences::AskCanOverwrite (char const * projFolder, DropInfo const & fileInfo)
{
	bool canOverwrite;
	if (GetAllOverwritePreferences (canOverwrite, fileInfo))
		return canOverwrite;

	// Ask the user
	_dlgData.Init (0, projFolder, fileInfo, _isMultiFileDrop);
	bool dlgOk;
	if (fileInfo.IsFolder ())
	{
		FolderDropQuestionCtrl ctrl (&_dlgData, _isMultiFileDrop);
		dlgOk = ThePrompter.GetData (ctrl);
	}
	else
	{
		FileDropQuestionCtrl ctrl (&_dlgData, _isMultiFileDrop);
		dlgOk = ThePrompter.GetData (ctrl);
	}
    if (dlgOk)
	{
		if (fileInfo.IsControlled ())
		{
			if (_dlgData.IsYesToAll ())
			{
				_option.set (OverwriteAllControlledFiles);
				if (fileInfo.IsFolder ())
					_option.set (OverwriteAllControlledFolders);
				return true;
			}
			if (_dlgData.IsNoToAll ())
			{
				_option.set (NeverOverwriteControlled);
				return true;
			}
		}
		else
		{
			if (_dlgData.IsYesToAll ())
			{
				_option.set (OverwriteAllFiles);
				if (fileInfo.IsFolder ())
					_option.set (OverwriteAllFolders);
				return true;
			}
			if (_dlgData.IsNoToAll ())
			{
				_option.set (NeverOverwrite);
				return true;
			}
		}
		if (_dlgData.IsNo ())
			_option.reset (LastQuestionAnswerWasYes);
		else
			_option.set (LastQuestionAnswerWasYes);
		if (_option.test (LastQuestionAnswerWasYes))
		{
			if (fileInfo.IsFolder ())
			{
				// Overwrite all folder files, but still ask for subfolders
				if (fileInfo.IsControlled ())
					_option.set (OverwriteAllControlledFiles);
				else
					_option.set (OverwriteAllFiles);
			}
		}
		return true;
	}
	return false; // User canceled operation
}

bool DropPreferences::AskAddToProject (DropInfo const & fileInfo)
{
	if (_option.test (AddAllToProject) || _option.test (NeverAddToProject))
	{
		return true;
	}

	// Ask the user
	_dlgData.Init (_projectName.c_str (), 0, fileInfo, _isMultiFileDrop);
	AddDropQuestionCtrl ctrl (&_dlgData, _isMultiFileDrop);
	if (ThePrompter.GetData (ctrl))
	{
		if (_dlgData.IsYesToAll ())
		{
			_option.set (AddAllToProject);
			return true;
		}
		if (_dlgData.IsNoToAll ())
		{
			_option.set (NeverAddToProject);
			return true;
		}
		if (_dlgData.IsNo ())
			_option.reset (LastQuestionAnswerWasYes);
		else
			_option.set (LastQuestionAnswerWasYes);
		return true;
	}
	return false; // User canceled operation
}

bool DropPreferences::CanOverwrite (DropInfo const & fileInfo)
{
	bool canOverwrite;
	if (!GetAllOverwritePreferences (canOverwrite, fileInfo))
	{
		canOverwrite = _option.test (LastQuestionAnswerWasYes);
	}
	return canOverwrite;
}

bool DropPreferences::CanAddToProject ()
{
	if (_option.test (AddAllToProject))
	{
		return true;
	}
	if (_option.test (NeverAddToProject))
	{
		return false;
	}
	return _option.test (LastQuestionAnswerWasYes);
}

// Returns true if overwrite preferences set for all
bool DropPreferences::GetAllOverwritePreferences (bool & canOverwrite, DropInfo const & fileInfo) const
{
	canOverwrite = false;	// Assume no overwrite
	if (fileInfo.IsControlled ())
	{
		if (fileInfo.IsFolder ())
		{
			if (_option.test (OverwriteAllControlledFolders))
			{
				canOverwrite = true;
				return true;
			}
		}
		else
		{
			if (_option.test (OverwriteAllControlledFiles))
			{
				canOverwrite = true;
				return true;
			}
		}
		if (_option.test (NeverOverwriteControlled))
		{
			return true;
		}
	}
	else
	{
		if (fileInfo.IsFolder ())
		{
			if (_option.test (OverwriteAllFolders))
			{
				canOverwrite = true;
				return true;
			}
		}
		else
		{
			if (_option.test (OverwriteAllFiles))
			{
				canOverwrite = true;
				return true;
			}
		}
		if (_option.test (NeverOverwrite))
		{
			return true;
		}
	}
	return false;
}

// Returns true if we have keep asking questions about overwriting or adding
bool DropPreferences::KeepAsking () const
{
	bool askAboutControlledOverwrite = (!_option.test (OverwriteAllControlledFolders) ||
										!_option.test (OverwriteAllControlledFiles))  &&
										!_option.test (NeverOverwriteControlled);
	bool askAboutOverwrite = (!_option.test (OverwriteAllFolders) ||
							  !_option.test (OverwriteAllFiles))  &&
							  !_option.test (NeverOverwrite);
	bool askAboutAddingToProject = !_option.test (AddAllToProject) && !_option.test (NeverAddToProject);
	return askAboutControlledOverwrite || askAboutOverwrite || askAboutAddingToProject;
}
