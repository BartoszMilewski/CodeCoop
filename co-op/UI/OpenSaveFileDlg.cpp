//------------------------------------
//  (c) Reliable Software, 2007 - 2008
//------------------------------------

#include "precompiled.h"
#include "OpenSaveFileDlg.h"
#include "Resource.h"
#include "RegFunc.h"
#include "OutputSink.h"
#include "Prompter.h"
#include "BrowseForFolder.h"
#include "FtpOpenFileDlg.h"
#include "CoopBlocker.h"
#include "Global.h"
#include "AppInfo.h"

#include <TimeStamp.h>
#include <File/File.h>
#include <Ctrl/FileGet.h>
#include <Sys/Process.h>
#include <Net/Ftp.h>
#include <Sys/Crypto.h>
#include <Sys/Synchro.h>
#include <Ex/Error.h>

//
// Save File Dialog Controller
//

SaveFileDlgCtrl::SaveFileDlgCtrl (SaveFileRequest & dlgData)
	: Dialog::ControlHandler (dlgData.HasUserNote () ? IDD_SAVE_FILE_NOTE : IDD_SAVE_FILE),
	  _dlgData (dlgData)
{
	Assert (!_dlgData.IsReadRequest ());
}

bool SaveFileDlgCtrl::OnInitDialog () throw (Win::Exception)
{
	Win::Dow::Handle dlgWin = GetWindow ();
	if (_dlgData.HasUserNote ())
		_userNote.Init (dlgWin, IDC_USER_NOTE);

	_fileNameFrame.Init (dlgWin, IDC_FILENAME_FRAME);
	_storeFrame.Init (dlgWin, IDC_STORE_FRAME);
	_fileName.Init (dlgWin, IDC_FILE_NAME);
	_computerFolder.Init (dlgWin, IDC_TARGET_FOLDER);
	_ftpServer.Init (dlgWin, IDC_FTP_SERVER);
	_ftpFolder.Init (dlgWin, IDC_FTP_FOLDER);
	_ftpUser.Init (dlgWin, IDC_FTP_USER);
	_ftpPassword.Init (dlgWin, IDC_FTP_PASSWORD);
	_browseComputer.Init (dlgWin, IDC_MY_COMPUTER_BROWSE);
	_myComputer.Init (dlgWin, IDC_MY_COMPUTER);
	_internet.Init (dlgWin, IDC_INTERNET);
	_overwriteExisting.Init (dlgWin, IDC_OVERWRITE_EXISTING);
	_ftpAnonymousLogin.Init (dlgWin, IDC_ANONYMOUS_LOGIN);

	dlgWin.SetText (_dlgData.GetDialogCaption ());
	_fileNameFrame.SetText (_dlgData.GetFileNameFrameCaption ());
	_storeFrame.SetText (_dlgData.GetStoreFrameCaption ());
	if (_dlgData.HasUserNote ())
		_userNote.SetText (_dlgData.GetUserNote ());

	std::string overwriteExistingCaption (" Overwrite existing ");
	overwriteExistingCaption += _dlgData.GetFileDescription ();
	_overwriteExisting.SetText (overwriteExistingCaption);

	_fileName.SetString (_dlgData.GetFileName ());
	if (_dlgData.IsOverwriteExisting ())
		_overwriteExisting.Check ();

	_computerFolder.SetString (_dlgData.GetPath ());

	if (TheAppInfo.IsFtpSupportEnabled ())
	{
		Ftp::SmartLogin const & ftpLogin = _dlgData.GetFtpLogin ();
		_ftpServer.SetString (ftpLogin.GetServer ());
		_ftpFolder.SetString (ftpLogin.GetFolder ());
		if (ftpLogin.IsAnonymous ())
		{
			_ftpAnonymousLogin.Check ();
			_ftpUser.Disable ();
			_ftpPassword.Disable ();
		}
		else
		{
			_ftpAnonymousLogin.UnCheck ();
			_ftpUser.SetString (ftpLogin.GetUser ());
			_ftpPassword.SetString (ftpLogin.GetPassword ());
		}

		if (_dlgData.IsStoreOnInternet ())
			_internet.Check ();
		else if (_dlgData.IsStoreOnMyComputer () || _dlgData.IsStoreOnLAN ())
			_myComputer.Check ();
	}
	else
	{
		_internet.Disable ();
		_ftpServer.Disable ();
		_ftpFolder.Disable ();
		_ftpAnonymousLogin.Disable ();
		_ftpUser.Disable ();
		_ftpPassword.Disable ();
		_myComputer.Check ();
	}

	return true;
}

bool SaveFileDlgCtrl::OnDlgControl (unsigned id, unsigned notifyCode) throw (Win::Exception)
{
	switch (id)
	{
	case IDC_MY_COMPUTER:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			_computerFolder.SetFocus ();
		}
		return true;

	case IDC_INTERNET:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			_ftpServer.SetFocus ();
		}
		return true;

	case IDC_TARGET_FOLDER:
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
			std::string browserHint ("Select folder for the ");
			browserHint += _dlgData.GetFileDescription ();
			std::string path;
			if (BrowseForAnyFolder (path, GetWindow (), browserHint))
			{
				_computerFolder.SetString (path);
			}
		}
		return true;

	case IDC_ANONYMOUS_LOGIN:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			if (_ftpAnonymousLogin.IsChecked ())
			{
				_ftpUser.Disable ();
				_ftpPassword.Disable ();
			}
			else
			{
				_ftpUser.Enable ();
				_ftpPassword.Enable ();
			}
		}
		return true;
	}
	return false;
}

bool SaveFileDlgCtrl::OnApply () throw ()
{
	_dlgData.SetFileName (_fileName.GetString ());
	_dlgData.SetOverwriteExisting (_overwriteExisting.IsChecked ());
	if (_myComputer.IsChecked ())
	{
		std::string path = _computerFolder.GetString ();
		_dlgData.SetPath (path);
		if (FilePath::IsNetwork (path))
			_dlgData.SetLAN ();
		else
			_dlgData.SetMyComputer ();
	}
	else if (_internet.IsChecked ())
	{
		_dlgData.SetInternet ();
		Ftp::SmartLogin & ftpLogin = _dlgData.GetFtpLogin ();
		ftpLogin.SetServer (_ftpServer.GetString ());
		ftpLogin.SetFolder (_ftpFolder.GetString ());
		if (_ftpAnonymousLogin.IsChecked ())
		{
			ftpLogin.SetUser ("");
			ftpLogin.SetPassword ("");
		}
		else
		{
			ftpLogin.SetUser (_ftpUser.GetTrimmedString ());
			ftpLogin.SetPassword (_ftpPassword.GetTrimmedString ());
		}
	}

	if (_myComputer.IsChecked () && !_dlgData.IsOverwriteExisting ())
	{
		FilePath archivePath (_dlgData.GetPath ());
		char const * path = archivePath.GetFilePath (_dlgData.GetFileName ());
		if (File::Exists (path))
		{
			std::string info (path);
			info += " already exists.\nDo you want to replace it?";
			Out::Answer userChoice = TheOutput.Prompt (info.c_str (),
													   Out::PromptStyle (Out::YesNo, Out::Yes, Out::Question),
													   GetWindow ());
			if (userChoice == Out::No)
				return false;

			_dlgData.SetOverwriteExisting (true);
		}
	}

	if (!_dlgData.IsValid ())
	{
		_dlgData.DisplayErrors (GetWindow ());
		return false;
	}

	_dlgData.RememberUserPrefs ();

	if (!_dlgData.OnApply ())
		return false;

	return Dialog::ControlHandler::OnApply ();
}

// command line parameters
// overwrite:"yes | no" target:"<file path>" server:"<ftp server>" user:"<ftp user>" password:"<ftp password>"
bool SaveFileDlgCtrl::GetDataFrom (NamedValues const & source)
{
	_dlgData.ReadNamedValues(source);

	if (!_dlgData.IsValid ())
		return false;

	return _dlgData.OnApply ();
}

//
// Open File Dialog Controller
//

OpenFileDlgCtrl::OpenFileDlgCtrl (OpenFileRequest & dlgData, Win::Dow::Handle topWin)
	: Dialog::ControlHandler (IDD_OPEN_FILE),
	  _topWin (topWin),
	  _dlgData (dlgData)
{
	Assert (_dlgData.IsReadRequest ());
}

bool OpenFileDlgCtrl::OnInitDialog () throw (Win::Exception)
{
	Win::Dow::Handle dlgWin = GetWindow ();
	_fileFrame.Init (dlgWin, IDC_FILE_FRAME);
	_filePath.Init (dlgWin, IDC_LOCAL_SOURCE_PATH);
	_ftpServer.Init (dlgWin, IDC_FTP_SERVER);
	_ftpFolder.Init (dlgWin, IDC_FTP_FOLDER);
	_ftpFilename.Init (dlgWin, IDC_FTP_FILENAME);
	_ftpUser.Init (dlgWin, IDC_FTP_USER);
	_ftpPassword.Init (dlgWin, IDC_FTP_PASSWORD);
	_browseComputer.Init (dlgWin, IDC_MY_COMPUTER_BROWSE);
	_myComputer.Init (dlgWin, IDC_MY_COMPUTER);
	_internet.Init (dlgWin, IDC_INTERNET);
	_ftpAnonymousLogin.Init (dlgWin, IDC_ANONYMOUS_LOGIN);
	_browseFtp.Init (dlgWin, IDC_FTP_BROWSE);

	dlgWin.SetText (_dlgData.GetDialogCaption ());
	_fileFrame.SetText (_dlgData.GetFrameCaption ());

	_filePath.SetString (_dlgData.GetPath ());

	if (TheAppInfo.IsFtpSupportEnabled ())
	{
		Ftp::SmartLogin const & ftpLogin = _dlgData.GetFtpLogin ();
		_ftpServer.SetString (ftpLogin.GetServer ());
		_ftpFolder.SetString (ftpLogin.GetFolder ());
		if (ftpLogin.IsAnonymous ())
		{
			_ftpAnonymousLogin.Check ();
			_ftpUser.Disable ();
			_ftpPassword.Disable ();
		}
		else
		{
			_ftpAnonymousLogin.UnCheck ();
			_ftpUser.SetString (ftpLogin.GetUser ());
			_ftpPassword.SetString (ftpLogin.GetPassword ());
		}

		if (_dlgData.IsStoreOnInternet ())
			_internet.Check ();
		else if (_dlgData.IsStoreOnMyComputer () || _dlgData.IsStoreOnLAN ())
			_myComputer.Check ();
	}
	else
	{
		_internet.Disable ();
		_ftpServer.Disable ();
		_ftpFolder.Disable ();
		_ftpFilename.Disable ();
		_ftpAnonymousLogin.Disable ();
		_ftpUser.Disable ();
		_ftpPassword.Disable ();
		_browseFtp.Disable ();
		_myComputer.Check ();
	}

	return true;
}

bool OpenFileDlgCtrl::OnDlgControl (unsigned id, unsigned notifyCode) throw (Win::Exception)
{
	switch (id)
	{
	case IDC_MY_COMPUTER:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			_filePath.SetFocus ();
		}
		return true;

	case IDC_INTERNET:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			_ftpServer.SetFocus ();
		}
		return true;

	case IDC_LOCAL_SOURCE_PATH:
		if (Win::Edit::GotFocus (notifyCode))
		{
			_myComputer.Check ();
			_internet.UnCheck ();
		}
		return true;

	case IDC_FTP_SERVER:
	case IDC_FTP_FOLDER:
	case IDC_FTP_FILENAME:
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
			std::string currentUserDir (_filePath.GetString ());
			FileGetter fileDlg;
			fileDlg.SetInitDir (currentUserDir.c_str ());
			fileDlg.SetFilter (_dlgData.GetFileFilter ());
			if (fileDlg.GetExistingFile (GetWindow (), _dlgData.GetBrowseCaption ().c_str ()))
			{
				_filePath.SetString (fileDlg.GetPath ());
				_filePath.SetFocus ();
			}
		}
		return true;

	case IDC_ANONYMOUS_LOGIN:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			if (_ftpAnonymousLogin.IsChecked ())
			{
				_ftpUser.Disable ();
				_ftpPassword.Disable ();
			}
			else
			{
				_ftpUser.Enable ();
				_ftpPassword.Enable ();
			}
		}
		return true;

	case IDC_FTP_BROWSE:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			_myComputer.UnCheck ();
			_internet.Check ();
			FtpFileOpenData dlgData (_dlgData.GetBrowseCaption (),
									 _ftpServer.GetString (),
									 _ftpFolder.GetString ());
			if (_ftpAnonymousLogin.IsChecked ())
			{
				dlgData.SetAnonymousLogin (true);
			}
			else
			{
				dlgData.SetAnonymousLogin (false);
				dlgData.SetUser (_ftpUser.GetString ());
				dlgData.SetPassword (_ftpPassword.GetString ());
			}

			FtpFileOpenCtrl dlgCtrl (dlgData, _topWin);
			if (ThePrompter.GetData (dlgCtrl))
			{
				_ftpFolder.SetString (dlgData.GetFolder ());
				_ftpFilename.SetString (dlgData.GetFileName ());
				_ftpFilename.SetFocus ();
			}
		}
		return true;
	}
	return false;
}

bool OpenFileDlgCtrl::OnApply () throw ()
{
	if (_myComputer.IsChecked ())
	{
		std::string path = _filePath.GetString ();
		if (FilePath::IsNetwork (path))
			_dlgData.SetLAN ();
		else
			_dlgData.SetMyComputer ();
		if (File::IsFolder (path.c_str ()))
		{
			_dlgData.SetPath (path);
		}
		else
		{
			PathSplitter splitter (path);
			std::string folder (splitter.GetDrive ());
			folder += splitter.GetDir ();
			std::string fileName (splitter.GetFileName ());
			fileName += splitter.GetExtension ();
			_dlgData.SetFileName (fileName);
			_dlgData.SetPath (folder);
		}
	}
	else
	{
		_dlgData.SetInternet ();
		Ftp::SmartLogin & ftpLogin = _dlgData.GetFtpLogin ();
		ftpLogin.SetServer (_ftpServer.GetString ());
		ftpLogin.SetFolder (_ftpFolder.GetString ());
		_dlgData.SetFileName (_ftpFilename.GetString ());
		if (_ftpAnonymousLogin.IsChecked ())
		{
			ftpLogin.SetUser ("");
			ftpLogin.SetPassword ("");
		}
		else
		{
			ftpLogin.SetUser (_ftpUser.GetString ());
			ftpLogin.SetPassword (_ftpPassword.GetString ());
		}
	}

	if (!_dlgData.IsValid ())
	{
		_dlgData.DisplayErrors (GetWindow ());
		return false;
	}

	if (!_dlgData.OnApply ())
		return false;

	_dlgData.RememberUserPrefs ();

	return Dialog::ControlHandler::OnApply ();
}

// command line arguments
// target:"<file path>" server:"<ftp server>" user:"<ftp user>" password:"<ftp password>"
bool OpenFileDlgCtrl::GetDataFrom (NamedValues const & source)
{
	_dlgData.ReadNamedValues(source);

	if (!_dlgData.IsValid ())
		return false;

	_dlgData.SetQuiet (true);
	return _dlgData.OnApply ();
}
