//------------------------------------
//  (c) Reliable Software, 2006 - 2008
//------------------------------------

#include "precompiled.h"
#include "SaveFilesDlg.h"
#include "Registry.h"
#include "OutputSink.h"
#include "BrowseForFolder.h"
#include "AppInfo.h"

char const SaveFilesData::_preferencesCopyTypeValueName [] = "Recent save files";
char const SaveFilesData::_preferencesFTPKeyName [] = "Save Files FTP site";
char const SaveFilesData::_preferencesLocalFolderValueName [] = "Save Files";
char const SaveFilesData::_preferencesUseFolderNames [] = "Use Folder Names";

SaveFilesData::SaveFilesData ()
{
	_options.set (AfterChange, true);
	Registry::UserPreferences prefs;
	if (TheAppInfo.IsFtpSupportEnabled ())
	{
		std::string copyType = prefs.GetOption (_preferencesCopyTypeValueName);
		if (copyType == "internet")
		{
			_copyRequest.SetInternet ();
		}
		else if (copyType == "local")
		{
			_copyRequest.SetMyComputer ();
		}
		Ftp::SmartLogin & login = _copyRequest.GetFtpLogin ();
		login.RetrieveUserPrefs (_preferencesFTPKeyName);
	}
	else
	{
		_copyRequest.SetMyComputer ();
	}
	_copyRequest.SetLocalFolder (prefs.GetFilePath (_preferencesLocalFolderValueName));
	std::string useFolderNames = prefs.GetOption (_preferencesUseFolderNames);
	_copyRequest.SetUseFolderNames (useFolderNames == "yes");
}

SaveFilesData::~SaveFilesData ()
{
	if (!_options.test (TargetIsValid))
		return;

	try
	{
		Registry::UserPreferences prefs;
		if (_copyRequest.IsInternet ())
		{
			prefs.SaveOption (_preferencesCopyTypeValueName, "internet");
			Ftp::SmartLogin & login = _copyRequest.GetFtpLogin ();
			login.RememberUserPrefs (_preferencesFTPKeyName);
		}
		else
		{
			Assert (_copyRequest.IsLAN () || _copyRequest.IsMyComputer ());
			prefs.SaveOption (_preferencesCopyTypeValueName, "local");
			prefs.SaveFilePath (_preferencesLocalFolderValueName, _copyRequest.GetLocalFolder ());
		}
		
		prefs.SaveOption (_preferencesUseFolderNames, (_copyRequest.IsUseFolderNames () ? "yes" : "no"));
	}
	catch (...)
	{
		// Ignore all exceptions
	}
}

bool SaveFilesData::IsValid ()
{
	if (_copyRequest.IsValid ())
	{
		_options.set (TargetIsValid, true);
		return true;
	}
	return false;
}

void SaveFilesData::DisplayErrors (Win::Dow::Handle hwndOwner) const
{
	if (!_copyRequest.IsInternet () && !_copyRequest.IsMyComputer () && !_copyRequest.IsLAN ())
	{
		TheOutput.Display ("Please, specify where to save the selected file(s).",
						   Out::Information,
						   hwndOwner);
		return;
	}

	if (_copyRequest.IsInternet ())
	{
		_copyRequest.DisplayErrors (hwndOwner);
	}
	else
	{
		Assert (_copyRequest.IsMyComputer () || _copyRequest.IsLAN ());
		std::string const & localFolder = _copyRequest.GetLocalFolder ();
		if (localFolder.empty ())
		{
			std::string info ("Select the folder where the files are to be saved.");
			TheOutput.Display (info.c_str (), Out::Information, hwndOwner);
		}
		else
		{
			_copyRequest.DisplayErrors (hwndOwner);
		}
	}
}

bool SaveFilesCtrl::OnInitDialog () throw (Win::Exception)
{
	Win::Dow::Handle dlgWin = GetWindow ();
	_myComputer.Init (dlgWin, IDC_MY_COMPUTER);
	_localFolder.Init (dlgWin, IDC_LOCAL_FOLDER);
	_browseMyComputer.Init (dlgWin, IDC_MY_COMPUTER_BROWSE);
	_internet.Init (dlgWin, IDC_INTERNET);
	_server.Init (dlgWin, IDC_FTP_SERVER);
	_serverFolder.Init (dlgWin, IDC_FTP_FOLDER);
	_anonymousLogin.Init (dlgWin, IDC_ANONYMOUS_LOGIN);
	_user.Init (dlgWin, IDC_FTP_USER);
	_password.Init (dlgWin, IDC_FTP_PASSWORD);
	_overwriteExisting.Init (dlgWin, IDC_SAVE_OLD_FILES_OVERWRITE);
	_useFolderNames.Init (dlgWin, IDC_SAVE_OLD_FILES_FOLDERS);
	_performDeletes.Init (dlgWin, IDC_SAVE_OLD_PERFORM_DELETES);
	_versionBefore.Init (dlgWin, IDC_SAVE_OLD_FILES_VERSION_BEFORE);
	_versionAfter.Init (dlgWin, IDC_SAVE_OLD_FILES_VERSION_AFTER);

	FileCopyRequest const & copyRequest = _dlgData.GetFileCopyRequest ();
	if (TheAppInfo.IsFtpSupportEnabled ())
	{
		Ftp::SmartLogin const & ftpLogin = copyRequest.GetFtpLogin ();
		_server.SetString (ftpLogin.GetServer ());
		_serverFolder.SetString (ftpLogin.GetFolder ());
		if (ftpLogin.IsAnonymous ())
		{
			_anonymousLogin.Check ();
			_user.Disable ();
			_password.Disable ();
		}
		else
		{
			_anonymousLogin.UnCheck ();
			_user.SetString (ftpLogin.GetUser ());
			_password.SetString (ftpLogin.GetPassword ());
		}
	}
	else
	{
		_internet.Disable ();
		_server.Disable ();
		_serverFolder.Disable ();
		_anonymousLogin.Disable ();
		_user.Disable ();
		_password.Disable ();
	}

	if (copyRequest.IsLAN () || copyRequest.IsMyComputer ())
		_myComputer.Check ();

	_localFolder.SetString (copyRequest.GetLocalFolder ());

	if (copyRequest.IsOverwriteExisting ())
		_overwriteExisting.Check ();
	else
		_overwriteExisting.UnCheck ();
	if (copyRequest.IsUseFolderNames ())
		_useFolderNames.Check ();
	else
		_useFolderNames.UnCheck ();
	if (_dlgData.IsPerformDeletes ())
		_performDeletes.Check ();
	else
		_performDeletes.UnCheck ();
	if (_dlgData.IsSaveAfterChange ())
		_versionAfter.Check ();
	else
		_versionBefore.Check ();
	
	if (TheAppInfo.IsFtpSupportEnabled ())
	{
		if (copyRequest.IsInternet ())
		{
			_internet.Check ();
			_server.SetFocus ();
		}
	}
	return true;
}

bool SaveFilesCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	switch (ctrlId)
	{
	case IDC_MY_COMPUTER:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			_localFolder.SetFocus ();
		}
		return true;

	case IDC_INTERNET:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			_server.SetFocus ();
		}
		return true;

	case IDC_LOCAL_FOLDER:
		if (Win::Edit::GotFocus (notifyCode))
		{
			_myComputer.Check ();
			_internet.UnCheck ();
		}
		return true;

	case IDC_FTP_SERVER:
	case IDC_FTP_FOLDER:
		if (Win::Edit::GotFocus (notifyCode))
		{
			_myComputer.UnCheck ();
			_internet.Check ();
		}
		return true;

	case IDC_MY_COMPUTER_BROWSE:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			_myComputer.Check ();
			_internet.UnCheck ();
			std::string path;
			FileCopyRequest const & copyRequest = _dlgData.GetFileCopyRequest ();
			if (BrowseForAnyFolder (path,
									GetWindow (),
									"Select where to save file(s).",
									copyRequest.GetLocalFolder ().c_str ()))
			{
				_localFolder.SetString (path);
			}
		}
		return true;

	case IDC_ANONYMOUS_LOGIN:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			if (_anonymousLogin.IsChecked ())
			{
				_user.Disable ();
				_password.Disable ();
			}
			else
			{
				_user.Enable ();
				_password.Enable ();
			}
		}
		return true;
	}
	return false;
}

bool SaveFilesCtrl::OnApply () throw ()
{
	FileCopyRequest & copyRequest = _dlgData.GetFileCopyRequest ();
	if (_myComputer.IsChecked ())
	{
		std::string path = _localFolder.GetString ();
		copyRequest.SetLocalFolder (path);
		if (FilePath::IsNetwork (path))
			copyRequest.SetLAN ();
		else
			copyRequest.SetMyComputer ();
	}
	else if (_internet.IsChecked ())
	{
		copyRequest.SetInternet ();
		Ftp::SmartLogin & ftpLogin = copyRequest.GetFtpLogin ();
		ftpLogin.SetServer (_server.GetString ());
		ftpLogin.SetFolder (_serverFolder.GetString ());
		if (_anonymousLogin.IsChecked ())
		{
			ftpLogin.SetUser ("");
			ftpLogin.SetPassword ("");
		}
		else
		{
			ftpLogin.SetUser (_user.GetString ());
			ftpLogin.SetPassword (_password.GetString ());
		}
	}

	copyRequest.SetOverwriteExisting (_overwriteExisting.IsChecked ());
	copyRequest.SetUseFolderNames (_useFolderNames.IsChecked ());
	_dlgData.SaveAfterChange (_versionAfter.IsChecked ());
	_dlgData.SetPerformDeletes (_performDeletes.IsChecked ());
	
	if (_dlgData.IsValid ())
		EndOk ();
	else
		_dlgData.DisplayErrors (GetWindow ());
	return true;
}
