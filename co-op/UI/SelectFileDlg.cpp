//----------------------------------
// (c) Reliable Software 2005 - 2006
//----------------------------------

#include "precompiled.h"
#include "SelectFileDlg.h"
#include "Resource.h"
#include "OutputSink.h"
#include "BrowseForFolder.h"

#include <Com/Shell.h>
#include <File/File.h>

SelectFileCtrl::SelectFileCtrl (std::string const & fileCaption, std::string const & browseCaption)
	: Dialog::ControlHandler (IDD_SELECT_FILE),
	  _fileCaption (fileCaption),
	  _browseCaption (browseCaption)
{}

bool SelectFileCtrl::OnInitDialog () throw (Win::Exception)
{
	_path.Init (GetWindow (), IDC_SELECT_TARGET_PATH);
	_fileName.Init (GetWindow (), IDC_SELECT_TARGET_FILE);
	_overwriteExisting.Init (GetWindow (), IDC_SELECT_OVERWRITE);
	_browse.Init (GetWindow (), IDC_TARGET_PATH_BROWSE);
	_fileFrame.Init (GetWindow (), IDC_SELECT_FILE_FRAME);

	_fileFrame.SetText (_fileCaption.c_str ());
	return true;
}

bool SelectFileCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	if (ctrlId == IDC_TARGET_PATH_BROWSE &&
		Win::SimpleControl::IsClicked (notifyCode))
	{
		std::string path;
		if (BrowseForAnyFolder (path,
								GetWindow (),
								_browseCaption.c_str ()))
		{
			_path.SetString (path);
		}
		return true;
	}
	return false;
}

bool SelectFileCtrl::OnApply () throw ()
{
	FilePath targetFolder (_path.GetString ());
	char const * path = targetFolder.GetFilePath (_fileName.GetString ());
	if (!_overwriteExisting.IsChecked () && File::Exists (path))
	{
		std::string info (path);
		info += " already exists.\nDo you want to replace it?";
		Out::Answer userChoice = TheOutput.Prompt (info.c_str (),
			Out::PromptStyle (Out::YesNo, Out::Yes, Out::Question),
			GetWindow ());
		if (userChoice == Out::No)
			return false;
	}
	return true;
}
