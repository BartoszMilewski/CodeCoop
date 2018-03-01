//------------------------------------
//  (c) Reliable Software, 2006 - 2009
//------------------------------------

#include "precompiled.h"
#include "SaveHtml.h"
#include "Registry.h"
#include "OutputSink.h"
#include "BrowseForFolder.h"

#include <Com/Shell.h>

SaveHtmlData::SaveHtmlData (std::string const & curWikiDir)
: _currentWikiFolder (curWikiDir)
{
	Registry::UserPreferences prefs;
	_targetFolder.Change (prefs.GetFilePath ("Export HTML"));
}

SaveHtmlData::~SaveHtmlData ()
{
	if (IsValid ())
	{
		Registry::UserPreferences prefs;
		prefs.SaveFilePath ("Export HTML", _targetFolder.GetDir ());
	}
}

bool SaveHtmlData::IsValid () const
{
	if (_targetFolder.IsDirStrEmpty ())
		return false;

	char const * path = _targetFolder.GetDir ();
	if (!FilePath::IsValid (path) ||
		!FilePath::IsAbsolute (path) ||
		!FilePath::HasValidDriveLetter (path))
	{
		return false;
	}
	if (_targetFolder.HasPrefix (_currentWikiFolder))
		return false;
	return true;
}

void SaveHtmlData::DisplayErrors (Win::Dow::Handle hwndOwner) const
{
	if (_targetFolder.IsDirStrEmpty ())
	{
		TheOutput.Display ("Select the folder where the files are to be saved.",
			Out::Information, hwndOwner);
	}
	else if (!FilePath::IsValid (_targetFolder.GetDir ()))
	{
		TheOutput.Display ("The target folder path contains illegal characters.",
			Out::Information, hwndOwner);
	}
	else if (!FilePath::IsAbsolute (_targetFolder.GetDir ()))
	{
		TheOutput.Display ("Please, select a drive (or a UNC path) for the target folder.",
			Out::Information, hwndOwner);
	}
	else if (!FilePath::HasValidDriveLetter (_targetFolder.GetDir ()))
	{
		TheOutput.Display ("Please, select a valid drive (or a UNC path) for the target folder.",
			Out::Information, hwndOwner);
	}
	else if (_targetFolder.HasPrefix (_currentWikiFolder))
	{
		TheOutput.Display ("The target folder cannot be inside the exported wiki site.",
			Out::Information, hwndOwner);
	}
}

bool SaveHtmlCtrl::OnInitDialog () throw (Win::Exception)
{
	Win::Dow::Handle dlgWin = GetWindow ();
	_targetFolder.Init (dlgWin, IDC_EDIT_PATH);
	_targetFolderBrowse.Init (dlgWin, IDC_BUTTON_BROWSE);

	FilePath const & targetFolder = _dlgData.GetTargetFolder ();
	_targetFolder.SetString (targetFolder.GetDir ());
	return true;
}

bool SaveHtmlCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	switch (ctrlId)
	{
	case IDC_BUTTON_BROWSE:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			std::string path;
			if (BrowseForAnyFolder (path,
									GetWindow (),
									"Select where to save files."))
			{
				_dlgData.SetTargetFolder (path);
				_targetFolder.SetString (path);
			}
			return true;
		}
		break;
	}
	return false;
}

bool SaveHtmlCtrl::OnApply () throw ()
{
	_dlgData.SetTargetFolder (_targetFolder.GetString ());
	if (_dlgData.IsValid ())
		EndOk ();
	else
		_dlgData.DisplayErrors (GetWindow ());
	return true;
}
