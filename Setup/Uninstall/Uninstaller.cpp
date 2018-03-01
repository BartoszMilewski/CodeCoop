//------------------------------------
//  (c) Reliable Software, 1997 - 2009
//------------------------------------

#include "precompiled.h"
#include "Uninstaller.h"
#include "UninstallParams.h"
#include "SetupRegistry.h"
#include "SetupParams.h"
#include "DispatcherParams.h"
#include "OutputSink.h"
#include "Proxy.h"
#include "FileOperation.h"
#include "Catalog.h"
#include "RegKeys.h"
#include "PathRegistry.h"
#include "DbHandler.h"

#include <File/File.h>
#include <File/Dir.h>
#include <Sys/WinString.h>
#include <Ex/WinEx.h>
#include <StringOp.h>
#include <Com/Shell.h>

#include <StringOp.h>

class UninstallClone
{
public:
	UninstallClone ();
	~UninstallClone ();
	void Execute ();

private:
	FilePath			_orgPath;
	TmpPath				_clonePath;
	HANDLE				_hCloneFile;
	HANDLE				_orgUninstall;
	PROCESS_INFORMATION	_processInfo;
	bool				_cloneIsRunning;
};

UninstallClone::UninstallClone ()
	: _hCloneFile (INVALID_HANDLE_VALUE),
	  _orgUninstall (INVALID_HANDLE_VALUE),
	  _cloneIsRunning (false)
{
	std::string pgmPath = Registry::GetProgramPath ();
	_orgPath.Change (pgmPath);
}

UninstallClone::~UninstallClone ()
{
	if (_cloneIsRunning)
	{
		// Resume clone thread execution
		::ResumeThread (_processInfo.hThread);
	}
}

void UninstallClone::Execute ()
{
	char const * cloneFilePath = _clonePath.GetFilePath ("Clone.exe");
	char const * orgFilePath = _orgPath.GetFilePath (UninstallExeName);
	if (!File::Exists (orgFilePath))
		return;
	File::Copy (orgFilePath, cloneFilePath);
	SECURITY_ATTRIBUTES security;
	security.nLength = sizeof (SECURITY_ATTRIBUTES);
	security.lpSecurityDescriptor = 0;	// Use default security descriptor of the calling process
	security.bInheritHandle = TRUE;		// Must be inherited when a new process is created
	_hCloneFile = ::CreateFile (cloneFilePath,
	                            GENERIC_READ,
						        FILE_SHARE_READ,
								&security,
								OPEN_EXISTING,
								FILE_FLAG_DELETE_ON_CLOSE,
								0);
	if (_hCloneFile == INVALID_HANDLE_VALUE)
	{
		File::Delete (cloneFilePath);
		throw Win::Exception ("Internal error: Cannot open uninstaller program file");
	}
	DWORD thisProcessId = ::GetCurrentProcessId ();
	_orgUninstall = ::OpenProcess (SYNCHRONIZE, TRUE, thisProcessId);
	if (_orgUninstall == INVALID_HANDLE_VALUE)
	{
		File::Delete (cloneFilePath);
		throw Win::Exception ("Internal error: Cannot open uninstaller process");
	}
	std::string cmdLineStr ("\"");
	cmdLineStr += cloneFilePath;
	cmdLineStr += "\" 0x";
	cmdLineStr += ToHexString (reinterpret_cast<unsigned int>(_orgUninstall)); 
	cmdLineStr += " \"";
	cmdLineStr += _orgPath.GetDir ();
	cmdLineStr += "\"";

	STARTUPINFO startupInfo;
	memset (&startupInfo, 0, sizeof (STARTUPINFO));
	startupInfo.cb = sizeof (STARTUPINFO);
	BOOL cloneExecuted = ::CreateProcess (0,
										  const_cast<char *> (cmdLineStr.c_str ()),
										  0,
									 	  0,
										  TRUE,
										  CREATE_SUSPENDED | NORMAL_PRIORITY_CLASS,
										  0,
										  0,
										  &startupInfo,
										  &_processInfo);
	if (cloneExecuted == 0)
	{
		File::Delete (cloneFilePath);
		throw Win::Exception ("Internal error: Cannot execute uninstaller process");
	}
	_cloneIsRunning = true;
}

Uninstaller::Uninstaller ()
	: _coopLinkDeleted (false),
	  _dispatcherLinkDeleted (false),
	  _uninstallLinkDeleted (false)
{
	std::string pgmPath = Registry::GetProgramPath ();
	_installPath.Change (pgmPath.c_str ());
	_backupPath.Change (pgmPath);
	_backupPath.DirDown (BackupFolder);
}

void Uninstaller::PerformFullUninstall ()
{
#ifndef DRY_RUN
	Setup::CloseRunningApp ();
	Setup::DeleteIntegratorFiles (_installPath);
	UninstallClone clone;
	clone.Execute ();
	CleanupTemporaryInstall (false);
	CleanupCmdLineToolsFiles ();
	DeleteFiles ();
	DeleteIcons ();
	DeleteAssociations ();
	CleanupCatalog ();
#endif
}

void Uninstaller::UninstallUpdate ()
{
	CheckBackupFiles ();
	Setup::CloseRunningApp ();
	RestoreOriginalFiles ();
	CleanupTemporaryInstall ();
}

void Uninstaller::CheckBackupFiles ()
{
	// Check for executable files in backup folder
	// Remember that Uninstall.exe is not backed up.
	for (int i = 0; ExeFiles [i] != 0; i++)
	{
		if (!IsFileNameEqual (ExeFiles [i], UninstallExeName))
		{
			char const * backupFile = _backupPath.GetFilePath (ExeFiles [i]);
			if (!File::Exists (backupFile))
				throw Win::Exception ("Cannot restore the original version. The backup copy is not complete.\n"
									  "Please run the Code Co-op Installer to restore the original version.");
		}
	}
}

void Uninstaller::RestoreOriginalFiles ()
{
	for (int i = 0; ExeFiles [i] != 0; i++)
	{
		if (!IsFileNameEqual (ExeFiles [i], UninstallExeName))
		{
			char const * src  = _backupPath.GetFilePath (ExeFiles [i]);
			char const * dest = _installPath.GetFilePath (ExeFiles [i]);
			if (!File::CopyNoEx (src, dest))
			{
				  throw Win::Exception ("Cannot restore backup files. "
										"Make sure you have permission to modify "
									    "Code Co-op installation files." );
			}
		}
	}
}

void Uninstaller::CleanupTemporaryInstall (bool careForVersionMarker)
{
	// Delete the Co-op files from the backup folder.
	Setup::DeleteFilesAndFolder (ExeFiles, _backupPath);
	// Delete the version marker file.
	if (!File::DeleteNoEx (_installPath.GetFilePath (TempVersionMarker)))
	{
		if (careForVersionMarker)
		{
			throw Win::Exception ("Cannot restore the original version.\n"
								  "Failed to replace some files in the installation folder.\n"
								  "Please run the Code Co-op Installer to restore the original version.");
		}
	}
}

void Uninstaller::CleanupCmdLineToolsFiles ()
{
	FilePath cmdLineToolsPath (Registry::GetCmdLineToolsPath ());
	if (cmdLineToolsPath.IsDirStrEmpty ())
		return; // Nothing to do.

	Setup::DeleteFilesAndFolder (CmdLineToolsFiles, cmdLineToolsPath);

	// Registry will be cleaned up in CleanupCatalog
}

void Uninstaller::DeleteFiles ()
{
	if (!_installPath.IsDirStrEmpty () && File::Exists (_installPath.GetDir ()))
	{
		// Delete executable files
		for (int i = 0; ExeFiles [i] != 0; i++)
		{
			if (!IsFileNameEqual (ExeFiles [i], UninstallExeName))
			{
				char const * coopFile = _installPath.GetFilePath (ExeFiles [i]);
				File::DeleteNoEx (coopFile);
			}
		}
		// Delete Beyond Compare files
		for (int i = 0; BeyondCompareFiles [i] != 0; i++)
		{
			char const * coopFile = _installPath.GetFilePath (BeyondCompareFiles [i]);
			File::DeleteNoEx (coopFile);
		}
		// Delete Wiki files
		for (int i = 0; WikiFiles [i] != 0; ++i)
		{
			char const * file = _installPath.GetFilePath (WikiFiles [i]);
			File::DeleteNoEx (file);
		}
		for (int i = 0; WikiFolders [i] != 0; ++i)
			::RemoveDirectory (_installPath.GetFilePath (WikiFolders [i]));
	}
	// Delete catalog directory
	std::string catPath = Registry::GetCatalogPath ();
	if (!catPath.empty () && File::Exists (catPath.c_str ()))
	{
		CoopDbHandler dbHandler (catPath);
		dbHandler.DeleteDb ();
	}
}

void Uninstaller::DeleteIcons ()
{
	// Remove Desktop shortcuts (Co-op & Public inbox) if present
	std::unique_ptr<ShellMan::Folder> desktop;
	if (Registry::IsUserSetup ())
		desktop.reset (new ShellMan::UserDesktopFolder ());
	else
		desktop.reset (new ShellMan::CommonDesktopFolder ());

	FilePath desktopPath;
	if (desktop->GetPath (desktopPath))
	{
		File::DeleteNoEx (desktopPath.GetFilePath (CoopLink));
		File::DeleteNoEx (desktopPath.GetFilePath (InboxLinkName));
	}
	// Remove Dispatcher Startup shortcut(s)
	ShellMan::CommonStartupFolder commonStartupFolder;
	FilePath startupPath;
	if (commonStartupFolder.GetPath (startupPath))
	{
		File::DeleteNoEx (startupPath.GetFilePath (DispatcherLink));
	}
	ShellMan::UserStartupFolder userStartupFolder;
	if (userStartupFolder.GetPath (startupPath))
	{
		File::DeleteNoEx (startupPath.GetFilePath (DispatcherLink));
	}

	// Remove Code Co-op shortcuts
	std::unique_ptr<ShellMan::Folder> programs;
	if (Registry::IsUserSetup ())
		programs.reset (new ShellMan::UserProgramsFolder ());
	else
		programs.reset (new ShellMan::CommonProgramsFolder ());

	FilePath ourProgramGroup;
	if (programs->GetPath (ourProgramGroup))
	{
		ourProgramGroup.DirDown (CompanyName);
		if (File::Exists (ourProgramGroup.GetDir ()))
		{
			// Easy scenario - links installed in the "Reliable Software" folder
			char const * appLinkPath = ourProgramGroup.GetFilePath (CoopLink);
			if (File::Exists (appLinkPath))
			{
				File::DeleteNoEx (appLinkPath);
				_coopLinkDeleted = true;
			}
			char const * dispatcherLinkPath = ourProgramGroup.GetFilePath (DispatcherLink);
			if (File::Exists (dispatcherLinkPath))
			{
				File::DeleteNoEx (dispatcherLinkPath);
				_dispatcherLinkDeleted = true;
			}
			char const * uninstallLinkPath = ourProgramGroup.GetFilePath (UninstallLink);
			if (File::Exists (uninstallLinkPath))
			{
				File::DeleteNoEx (uninstallLinkPath);
				_uninstallLinkDeleted = true;
			}
			// If "Reliable Software" folder is empty remove it
			// ::RemoveDirectory only succeeds on empty directories
			if (::RemoveDirectory (ourProgramGroup.GetDir ()) != 0)
			{
				// Notify the shell that one entry from start.programs has been removed
				SHChangeNotify (SHCNE_RMDIR, SHCNF_PATH | SHCNF_FLUSH, ourProgramGroup.GetDir (), 0);
			}
		}

		if (!_coopLinkDeleted || !_dispatcherLinkDeleted || !_uninstallLinkDeleted)
		{
			ourProgramGroup.DirUp ();
			// Find and delete our links
			FindAndDelete (ourProgramGroup);
		}
	}
	else
	{
		throw Win::Exception ("Internal error: Cannot locate system folder Programs");
	}
}

void Uninstaller::DeleteAssociations ()
{
	// deleting associations is minor --
	// do not interrupt the uninstall process
	try
	{
		RegKey::Root classesRoot (HKEY_CLASSES_ROOT);
		// znc extension
		Registry::ClassRoot zncExtension (".znc");
		std::string associatedClass = zncExtension.Key ().GetStringVal ("");
		if (IsNocaseEqual (associatedClass, ZncClass))
		{
			// delete only if associated with Code Co-op
			DeleteKey (zncExtension.Key (), ".znc", classesRoot);
		}
		// Znc associated Class
		Registry::ClassRoot zncClass (ZncClass);
		DeleteKey (zncClass.Key (), ZncClass, classesRoot);
	}
	catch (...)
	{}
}

void Uninstaller::DeleteKey (RegKey::Handle & thisKey, char const * thisKeyName, RegKey::Handle & parentKey)
{
	RegKey::DeleteTree (thisKey);
	parentKey.DeleteSubKey (thisKeyName);
}

void Uninstaller::CleanupLocalMachine ()
{
	// HKEY_LOCAL_MACHINE
	//      Software
	//          Reliable Software
	//              Code Co-op
	RegKey::Root      keyRoot (HKEY_LOCAL_MACHINE);
	RegKey::Existing  keyMain (keyRoot, "Software");
	RegKey::Existing  keyReliSoft (keyMain, "Reliable Software");
	RegKey::Existing	keyCoop (keyReliSoft, ApplicationName);
	_prevProvider = keyCoop.GetStringVal ("PreviousSccProvider");
	DeleteKey (keyCoop, ApplicationName, keyReliSoft);

	// HKEY_LOCAL_MACHINE
	//      Software
	//          Reliable Software
	//              Dispatcher
	RegKey::Check dispatcher (keyReliSoft, DispatcherSection);
	if (dispatcher.Exists ())
	{
		RegKey::Existing keyDispatcher (keyReliSoft, DispatcherSection);
		DeleteKey (keyDispatcher, DispatcherSection, keyReliSoft);
	}

	if (keyReliSoft.IsKeyEmpty ())
	{
		keyMain.DeleteSubKey ("Reliable Software");
	}
}

void Uninstaller::CleanupCurrentUser ()
{
	// HKEY_CURRENT_USER
	//      Software
	//          Reliable Software
	//              Code Co-op
	//					Preferences
	//					State
	//                  User
	RegKey::Root      keyRoot (HKEY_CURRENT_USER);
    RegKey::Existing  keyMain (keyRoot, "Software");
    RegKey::Existing  keyReliSoft (keyMain, "Reliable Software");
	RegKey::Existing	keyCoop (keyReliSoft, ApplicationName);

	// Delete Code Co-op key
	DeleteKey (keyCoop, ApplicationName, keyReliSoft);

	// HKEY_CURRENT_USER
	//      Software
	//          Reliable Software
	//              Dispatcher
	RegKey::Check dispatcher (keyReliSoft, DispatcherSection);
	if (dispatcher.Exists ())
	{
		RegKey::Existing keyDispatcher (keyReliSoft, DispatcherSection);
		DeleteKey (keyDispatcher, DispatcherSection, keyReliSoft);
	}

	if (keyReliSoft.IsKeyEmpty ())
	{
		keyMain.DeleteSubKey ("Reliable Software");
	}
}

void Uninstaller::CleanupSystem ()
{
	// HKEY_CURRENT_USER or HKEY_LOCAL_MACHINE
	//      Software
	//          Microsoft
	//              Windows
	//					CurrentVersion
	//						Uninstall
	//							Code Co-op
	Registry::KeyUninstall keyUninstall (Registry::IsUserSetup ());
	DeleteKey (keyUninstall.Key (), keyUninstall.GetCoopUninstallSubkeyName (), keyUninstall.GetParentKey ());

	// HKEY_LOCAL_MACHINE
	//      Software
	//          SourceCodeControlProvider
	Registry::KeySccCheck keySccCheck;
	if (keySccCheck.IsCoopCurrentProvider ())
	{
		// Check if we can restore previous source code control provider
		std::string newProvider;
		Registry::KeyScc keyScc;
		if (!_prevProvider.empty ())
		{
			// Restore old provider only if it is still on provider list
			RegKey::New providers (keyScc.Key (), "InstalledSCCProviders");
			for (RegKey::ValueSeq prov (providers); !prov.AtEnd (); prov.Advance ())
			{
				if (prov.IsString () && IsNocaseEqual (prov.GetString (), _prevProvider.c_str ()))
				{
					newProvider = _prevProvider;
					break;
				}
			}
		}
		keyScc.SetCurrentProvider (newProvider.c_str ());
	}
	if (keySccCheck.IsCoopRegisteredProvider ())
	{
		// Remove Code Co-op from the registered source code control provider list
		Registry::KeyScc keyScc;
		keyScc.RemoveProvider (SccServerName);
	}

	// HKEY_LOCAL_MACHINE
	//      Software
	//          Microsoft
	//              Windows
	//					CurrentVersion
	//						AppPaths
	//							Co-op.exe
	Registry::KeyAppPaths keyCoopPath;
	DeleteKey (keyCoopPath.Key (), keyCoopPath.GetCoopAppPathsSubkeyName (), keyCoopPath.GetParentKey ());
}

void Uninstaller::CleanupCatalog ()
{
	try
	{
		if (!Registry::IsUserSetup ())
		{
			CleanupLocalMachine ();
		}
		CleanupCurrentUser ();
		CleanupSystem ();
	}
    catch (Win::Exception ex)
    {
#if !defined (NDEBUG)
        TheOutput.Display (ex);
#endif
    }
    catch (...)
    {
		Win::ClearError ();
#if !defined (NDEBUG)
        TheOutput.Display ("Unknown Error -- Registry Cleanup", Out::Error);
#endif
    }
}

bool Uninstaller::FindAndDelete (FilePath & curFolder)
{
	if (!_coopLinkDeleted)
	{
		FileSeq appSeq (curFolder.GetFilePath (CoopLink));
		if (!appSeq.AtEnd ())
		{
			// Application link file found
			File::DeleteNoEx (curFolder.GetFilePath (CoopLink));
			_coopLinkDeleted = true;
		}
	}
	if (!_dispatcherLinkDeleted)
	{
		FileSeq dispatcherSeq (curFolder.GetFilePath (DispatcherLink));
		if (!dispatcherSeq.AtEnd ())
		{
			// Dispatcher link file found
			File::DeleteNoEx (curFolder.GetFilePath (DispatcherLink));
			_dispatcherLinkDeleted = true;
		}
	}
	if (!_uninstallLinkDeleted)
	{
		FileSeq uninstallSeq (curFolder.GetFilePath (UninstallLink));
		if (!uninstallSeq.AtEnd ())
		{
			// Uninstall link file found
			File::DeleteNoEx (curFolder.GetFilePath (UninstallLink));
			_uninstallLinkDeleted = true;
		}
	}
	if (_coopLinkDeleted && _dispatcherLinkDeleted && _uninstallLinkDeleted)
		return true;
	// Keep searching folders below
	for (DirSeq seq (curFolder.GetAllFilesPath ()); !seq.AtEnd (); seq.Advance ())
	{
		char const * subDir = seq.GetName ();
		if (subDir [0] == '.' && subDir [1] == '.')
			continue;
		curFolder.DirDown (subDir);
		if (FindAndDelete (curFolder))
			return true;
		curFolder.DirUp ();
	}
	return false;
}
