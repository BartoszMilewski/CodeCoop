//------------------------------------
//  (c) Reliable Software, 1997 - 2009
//------------------------------------

#include "precompiled.h"
#include "SetupDlg.h"
#include "SetupParams.h"
#include "OutputSink.h"
#include "Resource.h"
#include "BrowseForFolder.h"

#include <Sys/WinString.h>
#include <Com/Shell.h>
#include <File/Dir.h>
#include <File/Drives.h>
#include <Sys/Process.h>

int const dxDlgOffset = 300;
int const dyDlgOffset = -120;

SetupDlgCtrl::SetupDlgCtrl (InstallConfig * data)
	: Dialog::ControlHandler (IDD_SETUP),
	  _dlgData (data),
	  _canUseInstallationFolder (true),
	  _pgmGroupHandler (IDC_PGM_GROUP_LIST, _pgmGroupList, _pgmGroup)
{}


// Revisit: shouldn't access registry: use dlg data
bool SetupDlgCtrl::OnInitDialog () throw (Win::Exception)
{
	_installPath.Init (GetWindow (), IDC_INSTALL_EDIT);
	_browseInstall.Init (GetWindow (), IDC_INSTALL_BROWSE);
	_pgmGroup.Init (GetWindow (), IDC_PGM_GROUP_EDIT);
	_pgmGroupList.Init (GetWindow (), IDC_PGM_GROUP_LIST);
	_desktopShortcut.Init (GetWindow (), IDC_INSTALL_DESKTOP_SHORTCUT);
	_autoUpdate.Init (GetWindow (), IDC_INSTALL_AUTO_UPDATE);

    _pgmGroupList.AddProportionalColumn (96, "Existing Program Folders");
	GetWindow ().CenterOverScreen (dxDlgOffset, dyDlgOffset);
	// Limit the number of characters the user can type in the install path edit field
	_installPath.LimitText (FilePath::GetLenLimit ());
	// Display default install path
	_installPath.SetString (_dlgData->_newInstallPath.GetDir ());
	_pgmGroup.SetString (CompanyName);
	ShellMan::CommonProgramsFolder programs;
	FilePath pgmsPath;
	if (programs.GetPath (pgmsPath))
	{
		for (DirSeq seq (pgmsPath.GetAllFilesPath ()); !seq.AtEnd (); seq.Advance ())
		{
			char const * pgmGroupName = seq.GetName ();
			if (strcmp (pgmGroupName, "..") == 0)
				continue;
			_pgmGroupList.AppendItem (pgmGroupName);
		}
	}
	else
	{
		throw Win::Exception ("Internal error: Cannot locate system folder Programs");
	}
	if (_dlgData->_isDesktopShortcut)
		_desktopShortcut.Check ();
	else
		_desktopShortcut.UnCheck ();
	if (_dlgData->_isAutoUpdate)
		_autoUpdate.Check ();
	else
		_autoUpdate.UnCheck ();
	return true;
}

bool SetupDlgCtrl::OnDlgControl (unsigned id, unsigned notifyCode) throw (Win::Exception)
{
	if (id == IDC_INSTALL_BROWSE && Win::SimpleControl::IsClicked (notifyCode))
	{
		std::string path;
		if (BrowseForLocalFolder (path, GetWindow (), "Select folder for Code Co-op installation."))
		{
			_dlgData->_isDefaultInstallPath = false;
			_installPath.SetString (path);
		}
		return true;
	}
    return false;
}

bool SetupDlgCtrl::OnApply () throw ()
{
	_dlgData->_newInstallPath = _installPath.GetString ();
	_dlgData->_pgmGroupName   = _pgmGroup.GetString ();
	_dlgData->_isDesktopShortcut     = _desktopShortcut.IsChecked ();
	_dlgData->_isAutoUpdate   = _autoUpdate.IsChecked ();

	if (IsValid ())
		EndOk ();
	else
		DisplayErrors ();

	return true;
}

Notify::Handler * SetupDlgCtrl::GetNotifyHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ()
{
	if (_pgmGroupHandler.IsHandlerFor (idFrom))
		return &_pgmGroupHandler;
	else
		return 0;
}

bool ProgramGroupNotifyHandler::OnItemChanged (Win::ListView::ItemState & state) throw ()
{
	if (state.GainedSelection ())
	{
		std::string buf = _pgmGroupList.RetrieveItemText (state.Idx ());
		_pgmGroup.SetString (buf.c_str ());
	}
	return true;
}

bool SetupDlgCtrl::IsValid ()
{
    if (_dlgData->_newInstallPath.IsDirStrEmpty ())
        return false;
	char const * path = _dlgData->_newInstallPath.GetDir ();
    if (!FilePath::IsAbsolute (path))
        return false;
	if (!FilePath::IsValid (path))
		return false;
	if (File::Exists (path))
	{
		// Installation folder exists - can we write to it?
		try
		{
			FilePath testPath (path);

			{
				File::CreateAlwaysMode mode;
				File::NormalAttributes attrib;
				File test (testPath.GetFilePath ("test.tmp"), mode, attrib);
			}

			File::DeleteNoEx (testPath.GetFilePath ("test.tmp"));
		}
		catch ( ... )
		{
			_canUseInstallationFolder = false;
			return false;
		}
	}
	else
	{
		// Installation folder desn't exists - can we create it?
		try
		{
			FullPathSeq folderSeq (path);
			FilePath workPath (folderSeq.GetHead ());
			// Skip existing path segments
			for (; !folderSeq.AtEnd (); folderSeq.Advance ())
			{
				workPath.DirDown (folderSeq.GetSegment ().c_str ());
				if (!File::Exists (workPath.GetDir ()))
					break;
			}
			// Create first missing path segment
			if (!folderSeq.AtEnd ())
			{
				if (File::CreateNewFolder (workPath.GetDir ()))	// Quiet create
				{
					// Missing segment created
					File::RemoveEmptyFolder (workPath.GetDir ());	// Quiet deletion
				}
				else
				{
					_canUseInstallationFolder = false;
					return false;
				}
			} 
		}
		catch ( ... )
		{
			_canUseInstallationFolder = false;
			return false;
		}
	}

	if (_dlgData->_pgmGroupName.empty ())
        return false;

	return true;
}

void SetupDlgCtrl::DisplayErrors () const
{
    if (_dlgData->_newInstallPath.IsDirStrEmpty ())
    {
        TheOutput.Display ("Please select an installation folder");
    }
	else
	{
		char const * path = _dlgData->_newInstallPath.GetDir ();
		if (!FilePath::IsAbsolute (path))
		{
			TheOutput.Display ("Please select a drive for the program installation");
		}
		if (!FilePath::IsValid (path))
		{
			TheOutput.Display ("The installation folder path contains some illegal characters.\n"
							   "Please, specify a valid path.");
		}
		if (!_canUseInstallationFolder)
		{
			TheOutput.Display ("Code Co-op Setup cannot create or write to the installation folder.\n"
				"Please, specify another installation folder.");
		}
	}
    if (_dlgData->_pgmGroupName.empty ())
    {
        TheOutput.Display ("Please choose a program group on the system Start menu");
    }
}

// Database dialog

bool DatabaseDlgData::Validate (bool quiet) const
{
	Out::Muter muteOutput (TheOutput, quiet);
    if (_path.IsDirStrEmpty ())
	{
		TheOutput.Display ("Please select a directory.");
        return false;
	}

	if (!FilePath::IsValid (_path.GetDir ()) ||
		!FilePath::IsAbsolute (_path.GetDir ()) ||
		!FilePath::HasValidDriveLetter (_path.GetDir ()))
	{
		TheOutput.Display ("Please select a valid directory.");
		return false;
	}

	PathSplitter splitter (_path.GetDir ());
	std::string root (splitter.GetDrive ());
	root += '\\';
	DriveInfo drive (root.c_str ());
	if (!drive.IsFixed () && !quiet)
	{
		if (drive.IsRemovable ())
		{
			std::string msg = "You are placing Code Co-op database folder on a removable drive.\n";
			msg += "Make sure this drive is alwyas present when you run Code Co-op.\n";
			msg += "It will also have to be mounted under the same drive letter: ";
			msg += splitter.GetDrive ();
			msg += ".\n\n";
			msg += "Do you want to continue?";
			Out::Answer userChoice =
				TheOutput.Prompt (msg.c_str (), Out::PromptStyle (Out::YesNo, Out::No, Out::Warning));
			if (userChoice == Out::No)
				return false;
		}
		else
		{
			TheOutput.Display ("The database directory must be on a local drive");
			return false;
		}
	}

	ProgramFilesPath progFilesPath;
	if (_path.HasPrefix (progFilesPath))
	{
		TheOutput.Display ("The atabase directory cannot be in the Program Files directory");
		return false;
	}
	SystemFilesPath sysFilesPath;
	if (_path.HasPrefix (sysFilesPath))
	{
		TheOutput.Display ("The database directory cannot be in the System directory");
		return false;
	}
	return true;
}

DatabaseDlgCtrl::DatabaseDlgCtrl (DatabaseDlgData * data)
	: Dialog::ControlHandler (IDD_DATABASE_PATH),
	  _dlgData (data)
{}

bool DatabaseDlgCtrl::OnInitDialog () throw (Win::Exception)
{
	_browse.Init (GetWindow (), IDB_BROWSE);
	_path.Init (GetWindow (), IDC_EDIT);
	_path.SetString (_dlgData->GetPath ().GetDir ());
	GetWindow ().CenterOverScreen (dxDlgOffset, dyDlgOffset);
	return true;
}

bool DatabaseDlgCtrl::OnDlgControl (unsigned id, unsigned notifyCode) throw (Win::Exception)
{
	if (id == IDB_BROWSE && Win::SimpleControl::IsClicked (notifyCode))
	{
		std::string path;
		if (BrowseForLocalFolder (path, GetWindow (), "Select folder for Code Co-op database."))
			_path.SetString (path);

		return true;
	}
    return false;
}

bool DatabaseDlgCtrl::OnApply () throw ()
{
	_dlgData->SetPath (_path.GetString ());

	if (_dlgData->Validate ())
		EndOk ();

	return true;
}

bool CmdLineToolsDlgCtrl::OnInitDialog () throw (Win::Exception)
{
	_browse.Init (GetWindow (), IDB_BROWSE);
	_path.Init (GetWindow (), IDC_EDIT);
	_path.SetString (_installPath.GetDir ());
	GetWindow ().CenterOverScreen ();
	return true;
}

CmdLineToolsDlgCtrl::CmdLineToolsDlgCtrl (FilePath & installPath)
	: Dialog::ControlHandler (IDD_CMDLINE_TOOLS_SETUP),
	  _installPath (installPath)
{}

bool CmdLineToolsDlgCtrl::OnDlgControl (unsigned id, unsigned notifyCode) throw (Win::Exception)
{
	if (id == IDB_BROWSE && Win::SimpleControl::IsClicked (notifyCode))
	{
		std::string path;
		if (BrowseForLocalFolder (path, GetWindow (), "Select folder for Code Co-op Command Line Tools."))
			_path.SetString  (path);

		return true;
	}
    return false;
}

bool CmdLineToolsDlgCtrl::OnApply () throw ()
{
	_installPath.Change (_path.GetString ());
	if (_installPath.IsDirStrEmpty ())
		TheOutput.Display ("Pease, specify the installation path.");
	else if (!FilePath::IsAbsolute (_installPath.GetDir ()))
		TheOutput.Display ("Please, specify an absolute installation path.");
	else if (!FilePath::IsValid (_installPath.GetDir ()))
		TheOutput.Display ("Please, specify a valid installation path.");
	else
		EndOk ();
	
	return true;
}

LicenseDlgData::LicenseDlgData (Win::Instance inst)
{
	HRSRC hRes = FindResource (inst, MAKEINTRESOURCE (ID_LICENSE_TEXT), "TEXT");
	if (hRes == 0)
		throw Win::Exception ("Internal Error: cannot find license text");
	HGLOBAL hGlob = LoadResource (inst, hRes);
	if (hGlob == 0)
		throw Win::Exception ("Internal Error: cannot load license text");
	_text = reinterpret_cast<char const *> (LockResource (hGlob));
}

LicenseDlgCtrl::LicenseDlgCtrl (LicenseDlgData * data)
	: Dialog::ControlHandler (IDD_LICENSE_AGREEMENT),
	  _dlgData (data)
{}

bool LicenseDlgCtrl::OnInitDialog () throw (Win::Exception)
{
	_licenseText.Init (GetWindow (), IDC_LICENSE_AGREEMENT_TEXT);
	
	GetWindow ().CenterOverScreen (dxDlgOffset, dyDlgOffset);
	_licenseText.SetString (_dlgData->GetText ());
	_licenseText.SetFocus ();
	return true;
}

bool LicenseDlgCtrl::OnApply () throw ()
{
	EndOk ();
	return true;
}

void ConfirmDlgData::ShowInfo () const
{
	std::string cmdLine ("\"");
	cmdLine += _installationFolder.GetFilePath (DifferExeName);
	cmdLine += "\" \"";
	cmdLine += _infoFile;
	cmdLine += '"';
	Win::ChildProcess	differ (cmdLine);
	differ.ShowNormal ();
	differ.Create ();
}

ConfirmDlgCtrl::ConfirmDlgCtrl (ConfirmDlgData * data)
	: Dialog::ControlHandler (IDD_CONFIRM),
	  _dlgData (data)
{}

bool ConfirmDlgCtrl::OnInitDialog () throw (Win::Exception)
{
	Win::Dow::Handle hDlg (GetWindow ());
	_caption.Init (hDlg, IDC_CAPTION);
    _tutorial.Init (hDlg, IDC_TUTORIAL);
	_showInfo.Init (hDlg, IDC_SHOW_INFO);
	hDlg.CenterOverScreen ();
	if (_dlgData->RunTutorial ())
		_tutorial.Check ();
	if (_dlgData->HasInfoFile ())
	{
		_caption.SetText ("Code Co-op has been successfully installed on your computer.\n"
						  "Setup has detected some Dispatcher configuration problems.\n"
						  "To view more details click on the button below.");
	}
	else
	{
		_caption.SetText ("Code Co-op has been successfully installed on your computer.");
		_showInfo.Hide ();
	}
	return true;
}

bool ConfirmDlgCtrl::OnDlgControl (unsigned id, unsigned notifyCode) throw (Win::Exception)
{
	if (id == IDC_SHOW_INFO && Win::SimpleControl::IsClicked (notifyCode))
	{
		_dlgData->ShowInfo ();
		return true;
	}
	return false;
}

bool ConfirmDlgCtrl::OnApply () throw ()
{
	_dlgData->SetTutorial (_tutorial.IsChecked ());
	EndOk ();
	return true;
}
