//------------------------------------
//  (c) Reliable Software, 1998 - 2008
//------------------------------------

#include "precompiled.h"
#include "ExportProject.h"
#include "OutputSink.h"
#include "Registry.h"
#include "resource.h"
#include "BrowseForFolder.h"
#include "Catalog.h"
#include "AppInfo.h"

#include <TimeStamp.h>
#include <Com/Shell.h>

char const ExportProjectData::_preferencesCopyTypeValueName [] = "Recent project export";
char const ExportProjectData::_preferencesFTPKeyName [] = "Project Export FTP site";
char const ExportProjectData::_preferencesLocalFolderValueName [] = "Project Export";

ExportProjectData::ExportProjectData (VersionInfo & info, Catalog & catalog)
	: _versionInfo (info),
	  _catalog (catalog)	  
{
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
}

ExportProjectData::~ExportProjectData ()
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
	}
	catch (...)
	{
		// Ignore all exceptions
	}
}

std::string ExportProjectData::GetVersionComment () const
{
	StrTime timeStamp (_versionInfo.GetTimeStamp ());
	std::string comment;
	if (_versionInfo.IsCurrent ())
	{
		comment += _versionInfo.GetComment ();
		comment += " -- from ";
		comment += timeStamp.GetString ();
		comment += "\r\n\r\nHint: To export any historical version, switch to history view, ";
		comment += "\r\nselect a script and use menu Selection>Export Version.";
	}
	else
	{
		comment = GlobalIdPack (_versionInfo.GetVersionId ()).ToBracketedString ();
		comment += " -- ";
		comment += _versionInfo.GetComment ();
		comment += "\r\nFrom ";
		comment += timeStamp.GetString ();
	}
	return comment;
}

bool ExportProjectData::IsValid ()
{
	if (!_copyRequest.IsInternet ())
	{
		Assert (_copyRequest.IsLAN () || _copyRequest.IsMyComputer ());
		int projectId = -1;
		if (_catalog.IsSourcePathUsed (_copyRequest.GetLocalFolder (), projectId))
		{
			_options.set (TargetIsValid, false);
			return false;
		}
	}

	bool result = _copyRequest.IsValid (true); // test write
	_options.set (TargetIsValid, result);
	return result;
}

void ExportProjectData::DisplayErrors (Win::Dow::Handle hwndOwner) const
{
	int projectId = -1;
	if (!_copyRequest.IsInternet ())
	{
		Assert (_copyRequest.IsLAN () || _copyRequest.IsMyComputer ());
		if (_copyRequest.GetLocalFolder ().empty ())
		{
			TheOutput.Display ("Please, specify the target folder for exported project files.",
							   Out::Information, hwndOwner);
			return;
		}
		else if (_catalog.IsSourcePathUsed (_copyRequest.GetLocalFolder (), projectId))
		{
			std::string info ("The following folder:\n\n");
			info += _copyRequest.GetLocalFolder ();
			info += "\n\nalready contains Code Co-op project and cannot be overwritten.";
			TheOutput.Display (info.c_str (), Out::Information, hwndOwner);
			return;
		}
	}
	_copyRequest.DisplayErrors (hwndOwner);
}

ExportProjectCtrl::ExportProjectCtrl (ExportProjectData & exportData)
	: Dialog::ControlHandler (IDD_EXPORT_PROJECT),
	  _exportData (exportData)
{}

// command line
// -Selection_VersionExport version:"'current' | <script id>" overwrite:"yes | no"
//							localedits: "yes | no" target:"<export file path>"
//							[server:"<ftp server>"] [ftpuser:"<ftp user>"]
//							[password:"<ftp password>"]

bool ExportProjectCtrl::GetDataFrom (NamedValues const & source)
{
	std::string version (source.GetValue ("version"));
	std::string targetPath (source.GetValue ("target"));
	std::string overwrite (source.GetValue ("overwrite"));
	std::string localedits (source.GetValue ("localedits"));

	std::string server;
	if (TheAppInfo.IsFtpSupportEnabled ())
		server = source.GetValue ("server");

	if (version == "current")
	{
		_exportData.SetVersionId (gidInvalid);
	}
	else
	{
		GlobalIdPack pack (version);
		_exportData.SetVersionId (pack);
	}

	FileCopyRequest & copyRequest = _exportData.GetFileCopyRequest ();
	if (!server.empty ())
	{
		copyRequest.SetInternet ();
		Ftp::SmartLogin & ftpLogin = copyRequest.GetFtpLogin ();
		ftpLogin.SetServer (server);
		ftpLogin.SetFolder (targetPath);
		ftpLogin.SetUser (source.GetValue ("user"));
		ftpLogin.SetPassword (source.GetValue ("password"));
	}
	else
	{
		copyRequest.SetLocalFolder (targetPath);
		if (FilePath::IsNetwork (targetPath))
			copyRequest.SetLAN ();
		else
			copyRequest.SetMyComputer ();
	}

	if (overwrite == "yes")
		copyRequest.SetOverwriteExisting (true);

	if (_exportData.IsCurrentVersion ())
	{
		if (localedits == "yes")
			_exportData.SetIncludeLocalEdits (true);
	}
	return _exportData.IsValid ();
}

bool ExportProjectCtrl::OnInitDialog () throw (Win::Exception)
{
	Win::Dow::Handle dlgWin = GetWindow ();
	_versionFrame.Init (dlgWin, IDC_EXPORT_PROJECT_FRAME);
	_version.Init (dlgWin, IDC_EXPORT_PROJECT_VERSION);
	_myComputer.Init (dlgWin, IDC_MY_COMPUTER);
	_localFolder.Init (dlgWin, IDC_LOCAL_FOLDER);
	_browseMyComputer.Init (dlgWin, IDC_MY_COMPUTER_BROWSE);
	_internet.Init (dlgWin, IDC_INTERNET);
	_server.Init (dlgWin, IDC_FTP_SERVER);
	_serverFolder.Init (dlgWin, IDC_FTP_FOLDER);
	_anonymousLogin.Init (dlgWin, IDC_ANONYMOUS_LOGIN);
	_user.Init (dlgWin, IDC_FTP_USER);
	_password.Init (dlgWin, IDC_FTP_PASSWORD);
	_localEdits.Init (dlgWin, IDC_EXPORT_LOCAL_EDITS);
	_overwriteExisting.Init (dlgWin, IDC_EXPORT_OVERWRITE);

	if (_exportData.IsCurrentVersion ())
		_versionFrame.SetText ("Copy all project files and folders as of: ");

	_version.SetText (_exportData.GetVersionComment ());

	FileCopyRequest const & copyRequest = _exportData.GetFileCopyRequest ();
	if (TheAppInfo.IsFtpSupportEnabled ())
	{
		if (copyRequest.IsInternet ())
		{
			_internet.Check ();
		}
		else if (copyRequest.IsLAN () || copyRequest.IsMyComputer ())
		{
			_myComputer.Check ();
		}

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
		_myComputer.Check ();
		_internet.Disable ();
		_server.Disable ();
		_serverFolder.Disable ();
		_anonymousLogin.Disable ();
		_user.Disable ();
		_password.Disable ();
	}

	_localFolder.SetString (copyRequest.GetLocalFolder ());

	if (copyRequest.IsOverwriteExisting ())
		_overwriteExisting.Check ();
	else
		_overwriteExisting.UnCheck ();

	if (_exportData.IsCurrentVersion ())
	{
		_localEdits.Enable ();
		if (_exportData.IsIncludeLocalEdits ())
			_localEdits.Check ();
		else
			_localEdits.UnCheck ();
	}
	else
		_localEdits.Disable ();
	return true;
}

bool ExportProjectCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
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
			std::string currentTargetPath = _localFolder.GetString ();
			std::string path;
			if (BrowseForAnyFolder (path,
									GetWindow (),
									"Select where to save file(s).",
									currentTargetPath.c_str ()))
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

bool ExportProjectCtrl::OnApply () throw ()
{
	FileCopyRequest & copyRequest = _exportData.GetFileCopyRequest ();
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
	copyRequest.SetUseFolderNames (true);
	if (_exportData.IsCurrentVersion ())
		_exportData.SetIncludeLocalEdits (_localEdits.IsChecked ());

	if (_exportData.IsValid ())
		EndOk ();
	else
		_exportData.DisplayErrors (GetWindow ());
	return true;
}
