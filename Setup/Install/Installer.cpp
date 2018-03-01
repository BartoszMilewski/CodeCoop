//------------------------------------
//  (c) Reliable Software, 1997 - 2009
//------------------------------------

#include "precompiled.h"
#include "Installer.h"
#include "Catalog.h"
#include "License.h"
#include "SetupDlg.h"
#include "OutputSink.h"
#include "SetupRegistry.h"
#include "PathRegistry.h"
#include "Registry.h"
#include "EmailRegistry.h"
#include "RegKeys.h"
#include "FileOperation.h"
#include "DbHandler.h"
#include "ProjectData.h"
#include "Config.h"
#include "EmailConfig.h"
#include "ScriptProcessorConfig.h"
#include "TransportHeader.h"
#include "FileTypes.h"
#include "MemberDescription.h"
#include "SetupParams.h"
#include "BuildOptions.h"
#include "AltDiffer.h"
#include "VersionXml.h"
#include "Validators.h"
#include "Proxy.h"

#include <Ctrl/ProgressMeter.h>
#include <File/File.h>
#include <File/Dir.h>
#include <File/SafePaths.h>
#include <Com/Shell.h>
#include <Ex/WinEx.h>
#include <Ex/Error.h>
#include <File/Drives.h>
#include <Sys/Security.h>
#include <Net/SharedObject.h>
#include <Sys/Date.h>
#include <Graph/Cursor.h>

Installer::~Installer ()
{
	if (_externalTool.get () != 0 && _externalTool->IsAlive ())
		_externalTool->Terminate ();
	File::DeleteNoEx (_toolOutputFile);
}

bool Installer::PerformInstallation (Progress::Meter & meter)
{
	// Display license agreement
	LicenseDlgData licDlgData (_win.GetInstance ());
    LicenseDlgCtrl licCtrl (&licDlgData);
    Dialog::Modal license (_win, licCtrl);

	if (!license.IsOK ())
		return false;

	VerifySetup ();

	// Ask the user where to install Code Co-op
	SetupDlgCtrl setupCtrl (&_config);
	Dialog::Modal setup (_win, setupCtrl);
	if (!setup.IsOK ())
		return false;

	Cursor::Hourglass hourglass;
	Cursor::Holder	working (hourglass);

	meter.SetRange (0, 12, 1);
	meter.SetActivity ("Preparing for setup.");
	meter.StepIt ();
	Setup::CloseRunningApp ();
	CleanupPreviousInstallation ();

	meter.SetActivity ("Installing program files.");
	meter.StepIt ();
	TransferFiles ();

	meter.SetActivity ("Creating database folders.");
	meter.StepIt ();
	CreateDatabaseFolders (meter);

	meter.SetActivity ("Registering product.");
	meter.StepIt ();
	SetupRegistry (); // must be called before SetupCatalog!
	meter.StepIt ();
	SetupCatalog ();
	meter.StepIt ();
	CreateIcons ();
	meter.StepIt ();
	CreateAssociations ();
	meter.StepIt ();
	UpdateEnvironmentVariables ();

	meter.SetActivity ("Finishing up installation.");
	meter.StepIt ();
	StoreLicense ();
	meter.StepIt ();
	ConfigDefaultDifferMerger ();
	meter.StepIt ();
	
	_undoFileList.Commit ();

	return true;
}

bool Installer::TemporaryUpdate ()
{
	CheckForCurrentCoopInstall ();
	VerifyFiles (ExeFiles,
				 _config.GetInstallPath (), 
				 "The installation cannot continue.\n"
				 "Your prior installation is corrupted. "
				 "Please run Code Co-op Setup to recover. "
				 "The following file is missing:\n\n");
	// Ask the user if he/she really wants to install the temporary update
	char const * msg = "You are about to install a temporary update version\n"
					   "over your current installation.\n\n"
					   "Do you want to continue?";
	if (TheOutput.Prompt (msg, Out::PromptStyle (Out::YesNo)) == Out::No)
		return false;

	Setup::CloseRunningApp ();
	BackupOriginalFiles ();
	CopyUpdateFiles (_config.GetInstallPath ());

	_undoFileList.Commit ();

	return true;
}

bool Installer::PermanentUpdate ()
{
	CheckForCurrentCoopInstall ();
	VerifyFiles (ExeFiles,
				 _config.GetInstallPath (), 
				 "The installation cannot continue.\n"
				 "Your prior installation is corrupted. "
				 "Please run Code Co-op Setup to recover. "
				 "The following file is missing:\n\n");
	// Ask the user if he/she really wants to install the permanent update
	char const * msg = "You are about to update your Code Co-op installation.\n\n"
					   "Do you want to continue?";
	if (TheOutput.Prompt (msg, Out::PromptStyle (Out::YesNo)) == Out::No)
		return false;

	Setup::CloseRunningApp ();
	CleanupTemporaryInstall ();
	CopyUpdateFiles (_config.GetInstallPath ());

	return true;
}

bool Installer::InstallCmdLineTools ()
{
	CurrentFolder curFolder;
	VerifyFiles (CmdLineToolsFiles,
				 curFolder,
				 "Command Line Tools Setup cannot complete "
				 "because the following file is missing:\n\n");

	CheckForCurrentCoopInstall ();

	std::string cmdLineToolsPath = Registry::GetCmdLineToolsPath ();
	if (!cmdLineToolsPath.empty ())
		_config.SetInstallPath (cmdLineToolsPath);

	FilePath dlgPath = _config.GetInstallPath ();
	CmdLineToolsDlgCtrl ctrl (dlgPath);
	Dialog::Modal toolsPathDlg (_win, ctrl);
	if (!toolsPathDlg.IsOK ())
		throw Win::Exception (); // cancel Setup
	_config.SetInstallPath (dlgPath);
	MaterializeFolderPath (_config.GetInstallPath ().GetDir (), true);

	Registry::KeyCoop coopKey (_config.IsUserSetup ());
	coopKey.SetCmdLineToolsPath (_config.GetInstallPath ().GetDir ());
	CopyUpdateFiles (_config.GetInstallPath ());

	// Modify CoopVars.bat to include the actual cmdline tools installation path
	std::string cmd = "set PATH=%PATH%;\"";
	cmd += _config.GetInstallPath ().GetDir ();
	cmd += "\"";
	
	MemFileExisting batFile (_config.GetInstallPath ().GetFilePath (CoopVarsBatFile));
	batFile.ResizeFile (File::Size (cmd.length (), 0));
	memcpy (batFile.GetBuf (), cmd.c_str (), cmd.length ());
	batFile.Flush ();

	return true;
}

void Installer::VerifySetup ()
{
	CurrentFolder sourcePath;
	VerifyFiles (ExeFiles,
				 sourcePath,
				 "Code Co-op Setup cannot complete because the following file is missing:\n\n");
	VerifyFiles (AutoUpdateFiles,
				 sourcePath,
				 "Code Co-op Setup cannot complete because the following file is missing:\n\n");
	VerifyFiles (BeyondCompareFiles,
				 sourcePath,
				 "Code Co-op Setup cannot complete because the following file is missing:\n\n");

	VersionXml updatesXml (
					sourcePath.GetFilePath (AutoUpdateInfoFile), 
					0); // bulletin number does not matter here
	if (!updatesXml.IsVersionInfo () || !updatesXml.IsBulletinInfo ())
		throw Win::Exception ("Code Co-op Setup cannot complete because\n"
							  "the update information file is invalid.");
}

void Installer::CheckForCurrentCoopInstall ()
{
	// Is Code Co-op installed ?
	if (_config.GetInstallPath ().IsDirStrEmpty ())
	{
		throw Win::Exception ("Could not find Code Co-op installation on your computer.");
	}
	else if (!File::Exists (_config.GetInstallPath ().GetDir ()))
	{
		throw Win::Exception ("Your Code Co-op installation is corrupted.\n"
						      "Please run the Code Co-op Installer to recover.");
	}
	
	// Is it 4.0 or later version?
	FileVersion coopExeVersion (_config.GetInstallPath ().GetFilePath (CoopExeName));
	// File version number is coded in 8 bytes. 4 MS bytes in hex format: 00 40 00 00
	int majorVersion = coopExeVersion.GetFileVersionMS () >> 16;
	if (majorVersion < 4)
		throw Win::Exception ("Installer cannot continue.\n"
							  "You need to have Code Co-op 4.x or 5.x version installed.");
}

void Installer::BackupOriginalFiles ()
{
	FilePath backupPath (_config.GetInstallPath ());
	backupPath.DirDown (BackupFolder);

	if (!File::Exists (backupPath.GetDir ()))
	{
		if (!File::CreateFolder (backupPath.GetDir ()))
			throw Win::Exception ("The installation failed.\n"
								  "Cannot create backup folder for the original files.");

		_undoFileList.RememberCreated (backupPath.GetDir (), true);
	}
	for (int i = 0; ExeFiles [i] != 0; i++)
	{
		if (!IsFileNameEqual (ExeFiles [i], UninstallExeName))
		{
			char const * src  = _config.GetInstallPath ().GetFilePath (ExeFiles [i]);
			char const * dest = backupPath.GetFilePath (ExeFiles [i]);
			if (File::CopyNoEx (src, dest))
			{
				_undoFileList.RememberCreated (dest, false);
			}
			else
			{
				throw Win::Exception ("The installation failed.\n"
									  "Cannot backup the Code Co-op original files.");
			}
		}
	}
}

void Installer::CopyUpdateFiles (FilePath const & destPath)
{
	for (FileSeq seq ("*.*"); !seq.AtEnd (); seq.Advance ())
	{
		char const * curFile = seq.GetName ();
		if (!IsFileNameEqual (curFile, PermanentUpdateMarker) &&
			!IsFileNameEqual (curFile, CmdLineToolsMarker) &&
			!IsFileNameEqual (curFile, SetupExeName))
		{
			char const * dest = destPath.GetFilePath (curFile);
			if (!File::CopyNoEx (curFile, dest))
			{
				throw Win::Exception ("The installation failed.\n"
									  "Cannot copy the new Code Co-op file to the installation folder.",
									  curFile);
			}
		}
	}
}

void Installer::CleanupTemporaryInstall ()
{
	// Revisit: share this code with Uninstaller
	FilePath backupPath (_config.GetInstallPath ());
	backupPath.DirDown (BackupFolder);

	// Delete the Co-op files from the "Original" folder.
	for (int i = 0; ExeFiles [i] != 0; i++)
	{
		File::DeleteNoEx (backupPath.GetFilePath (ExeFiles [i]));
	}
	// Delete the backup folder if it is empty.
	//  ::RemoveDirectory only succeeds on empty directories
	::RemoveDirectory (backupPath.GetDir ());
	Win::ClearError ();

	// Delete the version marker file.
	if (!File::DeleteNoEx (_config.GetInstallPath ().GetFilePath (TempVersionMarker)))
	{
		throw Win::Exception ("Upgrade failed.\nThe prior installation could not be cleaned up. "
							  "Make sure you have permission to modify Code Co-op installation files.");
	}
}

void Installer::CleanupPreviousInstallation ()
{
	if (!_config.PreviousInstallationDetected ())
		return;

	FilePath const & prevPath = _config.GetPreviousInstallPath ();

	if (File::Exists (prevPath.GetDir ()))
	{
		if (_config.IsInSameFolderAsPrevious ())
		{
			// Delete configuration problems file from previous setup run
			File::DeleteNoEx (prevPath.GetFilePath (ConfigurationProblems));

			Setup::DeleteIntegratorFiles (prevPath);
		}

		CleanupTemporaryInstall ();

		// Upgrading -- delete old obsolete files
		{
			FilePath oldInstall (prevPath);
			// File used only once during installation
			File::DeleteNoEx (oldInstall.GetFilePath (PostInstallExeName));
			Setup::DeleteFiles (OldProjectFiles, oldInstall);
		}

		if (!_config.IsInSameFolderAsPrevious ())
		{
			Setup::DeleteFiles (BeyondCompareFiles, prevPath);
			Setup::DeleteFilesAndFolder (ExeFiles, prevPath);
		}
	}

	// Remove Help link (no longer used)
	ShellMan::CommonProgramsFolder programs;
	FilePath ourProgramGroup;
	if (programs.GetPath (ourProgramGroup))
	{
		ourProgramGroup.DirDown (CompanyName);
		if (File::Exists (ourProgramGroup.GetDir ()))
		{
			// Easy scenario - links installed in the "Reliable Software" folder
			char const * hlpLinkPath = ourProgramGroup.GetFilePath (HlpLink);
			if (File::Exists (hlpLinkPath))
			{
				File::DeleteNoEx (hlpLinkPath);
			}
		}
	}
}

void Installer::TransferFiles ()
{
	FilePath destPath (_config.GetInstallPath ());
	if (!File::Exists (destPath.GetDir ()))
	{
		// Ask the user about creating New folders only if this is NOT default installation folder
		MaterializeFolderPath (destPath.GetDir (), !_config.IsDefaultInstallPath ());
	}
	// Copy files to the installation folder
	CopyFiles (ExeFiles);
	CopyFiles (BeyondCompareFiles);
}

void Installer::CreateDatabaseFolders (Progress::Meter & meter)
{
	// Path could be in registry but not exist if user un-installed manually
	FilePath catPath = _config.GetCatalogPath ();
	bool success = false;
	do
	{
		catPath = QueryDatabasePath (catPath);
		if (MaterializeFolderPath (catPath.GetDir (), false, true)) // don't ask, quiet
		{
			CoopDbHandler dbHandler (catPath);
			success = dbHandler.CreateDb ();
		}

		if (success)
		{
			_config.SetCatalogPath (catPath);
		}
		else
		{
			std::string info ("Cannot create Code Co-op database in the following folder:\n\n");
			info += catPath.GetDir ();
			info += "\n\nPlease, select another folder and try again.";
			TheOutput.Display (info.c_str ());
			catPath.Clear ();
		}
	} while (!success);

	// Copy wiki system files
	FilePath wikiDir (_config.GetWikiDir ());
	MaterializeFolderPath (wikiDir.GetDir (), false, false); // don't ask, not quiet
	for (int i = 0; WikiFolders [i] != 0; ++i)
	{
		MaterializeFolderPath (wikiDir.GetFilePath (WikiFolders [i]), false, false);
	}
	for (int i = 0; WikiFiles [i] != 0; ++i)
	{
		char const * relPath = WikiFiles [i];
		PathSplitter splitter (relPath);
		if (splitter.GetDir () [0] == '\0')
			File::Copy (WikiFiles [i], wikiDir.GetFilePath (WikiFiles [i]));
		else
		{
			std::string fileName = splitter.GetFileName ();
			fileName += splitter.GetExtension ();
			File::Copy (fileName.c_str (), wikiDir.GetFilePath (relPath));
		}
	}

	try
	{
		meter.SetActivity ("Setting Access Control List for CODECOOP share.");
		meter.StepIt ();
		// Make sure everyone can access the database folder
		File::OpenExistingMode mode;
		mode << File::Mode::Access::WriteDac;
		mode << File::Mode::Share::All; // to be able to propagate new permissions to children
		File dir (catPath.GetDir (), mode, File::DirectoryAttributes ());
		Security::Acl oldAcl (dir);
		Security::TrusteeEveryone everyone;
		Security::ExplicitAccess ace (everyone, true); // children inherit new permissions
		std::vector<Security::ExplicitAccess> aces;
		aces.push_back (ace);
		Security::AccessControlList newAcl (aces, oldAcl);
		dir.SetAcl (newAcl);
	}
	catch (...)
	{}
}

void Installer::SetupCatalog ()
{
	Catalog catalog (true); // create if necessary; convert if needed
	try
	{
		ConvertToVersion4 (catalog);
	}
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
		Win::ClearError ();
		throw Win::Exception ("Catalog conversion failed.");
	}
}

void Installer::SetupRegistry ()
{
	if (!_config.IsUserSetup ())
	{
		// Check if we can write in non-user registry
		_config.SetIsUserSetup (!Registry::CanWriteToMachineRegKey ());
	}

	// Set up program paths
	{
		Registry::KeyCoop keyCoop (_config.IsUserSetup ());
		keyCoop.SetProgramPath (_config.GetInstallPath ().GetDir ());
		keyCoop.SetCatalogPath (_config.GetCatalogPath ().GetDir ());
		keyCoop.SetSccServerName (SccServerName);
		keyCoop.SetSccServerPath (_config.GetInstallPath ().GetFilePath (SccDll));
	}

	// Set up SCC
	if (_config.IsUserSetup ())
	{
		TheOutput.Display ("Because you have no write access to the LOCAL_MACHINE branch of the registry\n"
							"setup will not be able to make Code Co-op the Source Control provider\n"
							"for applications that use the SCC interface (compilers, editors, etc...).\n"
							"If this feature is important to you, after completing the setup, log on as Administrator\n"
							"and double click on the file SCC.reg in the Code Co-op setup directory",
							Out::Information);
	}
	else
	{
		try
		{
			std::string currentProvider;
			Registry::KeySccCheck keySccCheck;
			if (keySccCheck.Exists ())
			{
				if (keySccCheck.IsCurrentProviderSet ())
				{
					if (!keySccCheck.IsCoopCurrentProvider ())
					{
						std::string msg ("Your current default source code control provider is:\n\n");
						msg += keySccCheck.GetCurrentProvider ();
						msg += "\n\nWould you like to change it to Code Co-op?";
						if (TheOutput.Prompt (msg.c_str (), Out::PromptStyle (Out::YesNo), _win) == Out::Yes)
						{
							currentProvider.assign (SccProvider);
						}
					}
				}
				else
				{
					currentProvider.assign (SccProvider);
				}
			}
			else
			{
				currentProvider.assign (SccProvider);
			}

			Registry::KeyScc keyScc;
			keyScc.AddProvider (SccServerName, SccProvider);
			if (!currentProvider.empty ())
				keyScc.SetCurrentProvider (SccProvider);
		}
		catch ( ... )
		{
			Win::ClearError ();
			TheOutput.Display ("Setup could not make Code Co-op the Source Control provider for applications\n"
								"that use the SCC interface (compilers, editors, etc...).\n"
								"If this feature is important to you, after completing the setup, log on as Administrator\n"
								"and double click on the file SCC.reg in the Code Co-op setup directory",
								Out::Information);
		}
	}

	// Set up unistall and system entries
	try
	{
		Registry::KeyUninstall keyUninstall (_config.IsUserSetup ());
		keyUninstall.SetProgramDisplayName ("Reliable Software " COOP_PRODUCT_NAME);
		keyUninstall.SetUninstallString (_config.GetInstallPath ().GetFilePath (UninstallExeName));
		keyUninstall.SetDisplayIcon (_config.GetInstallPath ().GetFilePath (CoopExeName));
		keyUninstall.SetDisplayVersion (COOP_PRODUCT_VERSION);
		keyUninstall.SetCompanyName (CompanyName);
		keyUninstall.SetSupportLink ("http://www.relisoft.com/co_op/support.html");
		keyUninstall.SetCompanyURL ("http://www.relisoft.com");
		keyUninstall.SetProgramURL ("http://www.relisoft.com/co_op");


		Registry::KeyAppPaths keyAppPaths;
		keyAppPaths.SetAppPath (_config.GetInstallPath ().GetFilePath (CoopExeName));
	}
	catch (...)
	{
		// Ignore errors
		Win::ClearError ();
	}

	// Set up preferences
	try
	{
		// Set up user name
		if (Registry::GetUserName ().empty ())
		{
			RegKey::System keySystem;
			std::string userName (keySystem.GetRegisteredOwner ().c_str ());
	
			Registry::SetUserName (userName.c_str ());
		}
		// Start nagging for license after re-installation
		Registry::SetNagging (true);

		// Remove the old preferences if present
		RemovePreferencesIfNecessary ("File View");
		RemovePreferencesIfNecessary ("CheckIn Area View");
		RemovePreferencesIfNecessary ("Mailbox View");
		RemovePreferencesIfNecessary ("Synch Area View");
		RemovePreferencesIfNecessary ("History View");
		RemovePreferencesIfNecessary ("Project View");

		Registry::UserDifferPrefs diffPrefs;
		diffPrefs.SetSplitRatio (25);
	}
	catch ( ... )
	{
		// Ignore errors during setting user defaults
		Win::ClearError ();
	}

	try
	{
		if (_config.PreviousInstallationDetected ())
		{
			// Set version 5.1 or higher registry settings
			Registry::DispatcherUserRoot dispatcher;
			if (!dispatcher.Key ().IsValuePresent ("ConfigurationState"))
			{
				// Delete all values stored in the DispatcherUserRoot key
				std::vector<std::string> values;
				for (RegKey::ValueSeq seq (dispatcher.Key ()); !seq.AtEnd (); seq.Advance ())
				{
					values.push_back (seq.GetName ());
				}
				for (std::vector<std::string>::const_iterator iter = values.begin ();
					 iter != values.end ();
					 ++iter)
				{
					dispatcher.Key ().DeleteValue (*iter);
				}
				dispatcher.Key ().SetValueString ("ConfigurationState", "Configured");
			}

			ConvertEmailRegistry ();
		}

		Registry::UserDispatcherPrefs prefs;
		// Internet updates settings
		if (_config.IsAutoUpdate ())
		{
			int year, month, day;
			if (!prefs.GetUpdateTime (year, month, day) || // not set or
				(year == 0 && month == 0 && day == 0))     // turned off
			{
				// set defaults
				Date nextCheck;
				nextCheck.Now ();
				prefs.SetIsConfirmUpdate (true);
				prefs.SetUpdateTime (nextCheck.Year (), nextCheck.Month (), nextCheck.Day ());
				prefs.SetUpdatePeriod (UpdatePeriodValidator::GetDefault ());
			}
			else
			{
				// preserve existing settings
			}
		}
		else
		{
			prefs.SetUpdateTime (0, 0, 0);
		}
		int lastBulletin = prefs.GetLastBulletin ();
		CurrentFolder curFolder;
		VersionXml update (curFolder.GetFilePath (AutoUpdateInfoFile), lastBulletin);
		int currentBulletin = 0;
		// bulletin info should be there under normal circumstances
		// if it is not, sit quiet
		if (update.IsBulletinInfo ())
			currentBulletin = update.GetBulletinNumber ();
		if (currentBulletin > lastBulletin)
		{
			prefs.SetLastBulletin (currentBulletin);
			FilePath updatesFolder = _config.GetCatalogPath ().GetFilePath ("Updates");
			File::CopyNoEx (curFolder.GetFilePath (AutoUpdateInfoFile),
							updatesFolder.GetFilePath (AutoUpdateInfoFile));
		}
		// Missing script resend settings
		unsigned long delay, repeat;
		bool setDelay = !prefs.GetResendDelay (delay);
		bool setRepeat = !prefs.GetRepeatInterval (repeat);
		if (setDelay || setRepeat)
		{
			prefs.SetAutoResend (setDelay ? 30 : delay,		// Wait 30 minutes before sending the first request
								 setRepeat ? 1440 : repeat);// Wait one day before sending the next request
		}
	}
	catch ( ... )
	{
		Win::ClearError ();
	}

	// Setup alternative differ and merger
	try
	{
		Registry::UserDifferPrefs prefs;
		bool isOn;
		if (prefs.IsAlternativeDiffer (isOn))
		{
			// is in registry
			bool useXml = false;
			std::string altPath = prefs.GetAlternativeDiffer (useXml);
			if (!useXml)
			{
				if (altPath.find ("BCDiffer") != std::string::npos
					|| altPath.find ("Beyond Compare") != std::string::npos)
				{
					prefs.SetAlternativeDiffer (isOn, altPath, BCDIFFER_CMDLINE, BCDIFFER_CMDLINE2, true);
				}
				else if (altPath.find ("BCMerger") != std::string::npos
					|| altPath.find ("BcMerger") != std::string::npos
					|| altPath.find ("Cirrus") != std::string::npos
					|| altPath.find ("BCMerge") != std::string::npos) //<- update anyway
				{
					prefs.SetAlternativeDiffer (isOn, 
						_config.GetInstallPath ().GetFilePath (BcMergerExe), 
						BCDIFFER_CMDLINE, 
						BCDIFFER_CMDLINE2,
						true);
				}
			}
		}

		if (prefs.IsAlternativeMerger (isOn))
		{
			// is in registry
			bool useXml = false;
			std::string altPath = prefs.GetAlternativeMerger (useXml);
			if (!useXml)
			{
				if (altPath.find ("BCDiffer") != std::string::npos
					|| altPath.find ("Beyond Compare") != std::string::npos)
				{
					prefs.SetAlternativeMerger (true, altPath, BCMERGER_CMDLINE, true);
					prefs.SetAlternativeAutoMerger (altPath, BCAUTOMERGER_CMDLINE, true);
				}
				else if (altPath.find ("Cirrus") != std::string::npos
					|| altPath.find ("BCMerger") != std::string::npos
					|| altPath.find ("BcMerger") != std::string::npos
					|| altPath.find ("BCMerge") != std::string::npos) //<- update anyway
				{
					prefs.SetAlternativeMerger (true, 
						_config.GetInstallPath ().GetFilePath (BcMergerExe), 
						BCMERGER_CMDLINE, true);
					prefs.SetAlternativeAutoMerger (
						_config.GetInstallPath ().GetFilePath (BcMergerExe),
						BCAUTOMERGER_CMDLINE, true);
				}
			}
			prefs.ToggleAlternativeMerger (true);
		}
		else
		{
			prefs.SetAlternativeMerger (true, 
				_config.GetInstallPath ().GetFilePath (BcMergerExe), 
				BCMERGER_CMDLINE, true);
			prefs.SetAlternativeAutoMerger (
				_config.GetInstallPath ().GetFilePath (BcMergerExe),
				BCAUTOMERGER_CMDLINE, true);
		}
	}
	catch ( ... )
	{
		Win::ClearError ();
	}
}

void Installer::ConvertEmailRegistry ()
{
	Email::RegConfig emailConfig;
	if (_config.IsVersionLowerThen3 ())
	{
		emailConfig.SetIsUsingPop3 (false);
		emailConfig.SetIsUsingSmtp (false);
		emailConfig.SetDefaults ();
		return;
	}
	
	Registry::UserDispatcherPrefs prefs;
	if (!emailConfig.IsValuePresent (Email::RegConfig::EMAIL_IN_NAME) ||
		!emailConfig.IsValuePresent (Email::RegConfig::EMAIL_OUT_NAME))
	{
		if (prefs.IsSimpleMapiForcedObsolete ())
		{
			emailConfig.ForceSimpleMapi ();
			emailConfig.SetIsUsingPop3 (false);
			emailConfig.SetIsUsingSmtp (false);
		}
		else
		{
			emailConfig.SetIsUsingPop3 (prefs.IsUsePop3Obsolete ());
			emailConfig.SetIsUsingSmtp (prefs.IsUseSmtpObsolete ());
		}
	}

	if (!emailConfig.IsValuePresent (Email::RegConfig::EMAIL_SIZE_NAME))
	{
		unsigned long size = 0;
		prefs.GetMaxEmailSizeObsolete (size);
		emailConfig.SetMaxEmailSize (size);
	}
	
	if (!emailConfig.IsValuePresent (Email::RegConfig::AUTO_RECEIVE_NAME))
	{
		unsigned long period = 0;
		prefs.GetAutoReceivePeriodObsolete (period);
		emailConfig.SetAutoReceive (period);
	}

	if (!emailConfig.IsValuePresent (Email::RegConfig::EMAIL_TEST_NAME))
	{
		emailConfig.SetEmailStatus (static_cast<Email::Status>(prefs.GetEmailStatusObsolete ()));
	}

	if (!emailConfig.IsScriptProcessorPresent ())
	{
		ScriptProcessorConfig cfg;
		prefs.ReadScriptProcessorConfigObsolete (cfg);
		emailConfig.SaveScriptProcessorConfig (cfg);
	}
}

void Installer::RemovePreferencesIfNecessary (std::string const & keyName)
{
	Registry::Preferences settings;
	RegKey::Check keyCheck (settings.Key (), keyName);
	if (keyCheck.Exists ())
	{
		RegKey::DeleteTree (keyCheck);
		settings.Key ().DeleteSubKey (keyName);
	}
}

void Installer::ConvertToVersion4 (Catalog & catalog)
{
	// conversion to 4.0
	// force serialization in 4.0 format
	catalog.Flush ();
	// delete old cluster recipient log
	File::DeleteNoEx (_config.GetCatalogPath ().GetFilePath ("Database/ClusterLog.bin"));
	// Move all scripts pending in project outboxes to Public Inbox
	FilePath outbox (_config.GetCatalogPath ());
	outbox.DirDown ("Outbox");
	FilePath publicInbox (_config.GetCatalogPath ());
	publicInbox.DirDown ("PublicInbox");
	if (!File::Exists (outbox.GetDir ()))
		return;
	
	TmpPath tmpFolder;
	for (::ProjectSeq seq (catalog); !seq.AtEnd (); seq.Advance ())
	{
		outbox.DirDown (ToString (seq.GetProjectId ()).c_str ());
		if (File::Exists (outbox.GetDir ()))
		{
			SafePaths tempScriptCopies;
			ShellMan::QuietDelete (0, tmpFolder.GetFilePath ("*.snc"));
			for (FileSeq fileSeq (outbox.GetFilePath ("*.snc"));
				 !fileSeq.AtEnd ();
				 fileSeq.Advance ())
			{
				std::string tmpFile = tmpFolder.GetFilePath (fileSeq.GetName ());
				// set forward flag
				// do it on temporary copy (in case of any failure we stay with untouched script)
				tempScriptCopies.MakeTmpCopy (outbox.GetFilePath (fileSeq.GetName ()),
												tmpFile.c_str ());
				MemMappedHeader tmpScript (tmpFile.c_str ());
				tmpScript.StampForwardFlag (true);
			}
			if (!tempScriptCopies.IsEmpty ())
			{
				std::string allScripts (tmpFolder.GetFilePath ("*.snc"));
				allScripts += '\0';
				allScripts += '\0';
				// Throws upon failure
				ShellMan::CopyFiles (_win, allScripts.c_str (), publicInbox.GetDir (), ShellMan::Silent);
				ShellMan::QuietDelete (_win, allScripts.c_str ());
				ShellMan::QuietDelete (_win, outbox.GetFilePath ("*.snc"));
			}
		}
		ShellMan::DeleteContents (_win, outbox.GetDir (), ShellMan::SilentAllowUndo);
		ShellMan::Delete (_win, outbox.GetDir (), ShellMan::SilentAllowUndo);
		outbox.DirUp ();
	}
	ShellMan::DeleteContents (_win, outbox.GetDir (), ShellMan::SilentAllowUndo);
	ShellMan::Delete (_win, outbox.GetDir (), ShellMan::SilentAllowUndo);
}


std::string Installer::GetShareName (std::string const & publicInboxShare)
{
	if (!publicInboxShare.empty ())
	{
		std::string::size_type i = publicInboxShare.rfind ('\\');
		if (i != std::string::npos)
		{
			return publicInboxShare.substr (i + 1);
		}
	}
	return std::string ();
}

FilePath Installer::QueryDatabasePath (FilePath const & dbPath)
{
	DatabaseDlgData data (dbPath);
	if (data.Validate (true))	// Quiet
		return dbPath;

	DriveSeq drives;
	std::string largestDisk;
	int largestMbFree = 0;
	for (DriveSeq drives; !drives.AtEnd (); drives.Advance ())
	{
		DriveInfo drive (drives.GetDriveString ());

		if (drive.IsFixed ())
		{
			int mbFree = drive.MbytesFree ();
			if (mbFree > largestMbFree)
			{
				largestDisk = drives.GetDriveString ();
				largestMbFree = mbFree;
			}
		}
	}
	largestDisk += "co-op";
	data.SetPath (largestDisk);
	DatabaseDlgCtrl ctrl (&data);
	Dialog::Modal database (_win, ctrl);
	if (!database.IsOK ())
		throw Win::Exception (); // cancel setup
	return data.GetPath ();
}

bool Installer::ValidateProjectRegistryData (Project::Data const & projData,
											 MemberDescription const & member) const
{
	return ValidatePath (projData.GetDataPath ()) &&
		   ValidatePath (projData.GetRootDir ()) &&
		   !projData.GetProjectName ().empty () &&
		   !member.GetHubId ().empty ();
		   // user id label may be empty in old projects
}

bool Installer::ValidatePath (FilePath const & path) const
{
	return !path.IsDirStrEmpty () && 
			FilePath::IsValid (path.GetDir ()) &&
			FilePath::IsAbsolute (path.GetDir ());
}

void Installer::ConfirmInstallation ()
{
	if (_config.PreviousInstallationDetected ())
	{
		// Start Dispatcher
		FilePath programPath (Registry::GetProgramPath ());
		char const * dispatcherPath = programPath.GetFilePath (DispatcherExeName);
		if (File::Exists (dispatcherPath))
		{
			Win::ChildProcess dispatcher (dispatcherPath);
			dispatcher.SetCurrentFolder (programPath.GetDir ());
			dispatcher.ShowNormal ();
			Win::ClearError ();
			if (dispatcher.Create (5000)) // Give the Dispatcher 5 sec to launch
			{
				DispatcherProxy dispatcherWin;
				if (dispatcherWin.GetWin ().IsNull ())
				{
					throw Win::Exception ("Cannot communicate with Code Co-op Dispatcher.");
				}
			}
			else
			{
				throw Win::Exception ("Cannot execute Code Co-op Dispatcher.");
			}
		}
		else
		{
			Win::ClearError ();
			throw Win::Exception ("Cannot find Code Co-op Dispatcher.  Run Code Co-op installation again.", dispatcherPath);
		}
	}

	if (PostInstallTask ())
		return;

	// Display installation confirmation dialog
	FilePath installationFolder (_config.GetInstallPath ());
	ConfirmDlgData dlgData (_infoFile, installationFolder);
    ConfirmDlgCtrl ctrl (&dlgData);
    Dialog::Modal confirm (_win, ctrl);
	if (dlgData.RunTutorial ())
	{
		FilePath installPath (_config.GetInstallPath ());
		CurrentFolder::Set (installPath.GetDir ());
		// We can't use the help subsystem to start the tutorial,
		// The help window would close as soon as setup finishes
		int errCode = ShellMan::Open (_win, installPath.GetFilePath (HelpModuleName));
		if (errCode != -1)
		{
			std::string msg = ShellMan::HtmlOpenError (errCode, "Tutorial", installPath.GetFilePath(HelpModuleName));
			TheOutput.Display (msg.c_str (), Out::Error);
		}
	}
}

bool Installer::PostInstallTask ()
{
	char const * path = _config.GetInstallPath ().GetFilePath (PostInstallExeName);
	if (File::Exists (path))
	{
		int errCode = ShellMan::Execute (_win, path, 0);
		return errCode == ShellMan::Success;
	}
	return false;
}

void Installer::CreateIcons ()
{
	std::unique_ptr<ShellMan::Folder> programs;
	if (_config.IsUserSetup ())
		programs.reset (new ShellMan::UserProgramsFolder ());
	else
		programs.reset (new ShellMan::CommonProgramsFolder ());

	FilePath ourProgramGroup;
	if (programs->GetPath (ourProgramGroup))
	{
		ourProgramGroup.DirDown (_config.GetPgmGroupName ().c_str ());
		if (!File::Exists (ourProgramGroup.GetDir ()))
		{
			// Do not ask user about anything
			MaterializeFolderPath (ourProgramGroup.GetDir (), false);
		}
		FilePath programPath (_config.GetInstallPath ());
		ShellMan::ShortCut codeCoop;
		codeCoop.SetObject (programPath.GetFilePath (CoopExeName));
		codeCoop.SetDescription ("Code Co-op");
		codeCoop.Save (ourProgramGroup.GetFilePath (CoopLink));
		if (_config.IsDesktopShortcut ())
		{
			std::unique_ptr<ShellMan::Folder> desktop;
			if (_config.IsUserSetup ())
				desktop.reset (new ShellMan::UserDesktopFolder ());
			else
				desktop.reset (new ShellMan::CommonDesktopFolder ());

			FilePath desktopPath;
			if (desktop->GetPath (desktopPath))
			{
				codeCoop.Save (desktopPath.GetFilePath (CoopLink));
			}
			else
			{
				throw Win::Exception ("Setup error: Cannot locate Desktop folder");
			}
		}
		ShellMan::ShortCut dispatcher;
		dispatcher.SetObject (programPath.GetFilePath (DispatcherExeName));
		dispatcher.SetDescription ("Code Co-op Script Dispatcher");
		dispatcher.Save (ourProgramGroup.GetFilePath (DispatcherLink));
		ShellMan::ShortCut uninstall;
		uninstall.SetObject (programPath.GetFilePath (UninstallExeName));
		uninstall.SetDescription ("Code Co-op Uninstaller");
		uninstall.Save (ourProgramGroup.GetFilePath (UninstallLink));

		// Dispatcher shortcut
		FilePath startupPath;
		ShellMan::UserStartupFolder userStartupFolder;
		if (!userStartupFolder.GetPath (startupPath))
			throw Win::Exception ("Setup error: Cannot locate User Startup folder.");
		bool isShortcutInUser = File::Exists (startupPath.GetFilePath (DispatcherLink));
		
		if (!isShortcutInUser && !_config.IsUserSetup ())
		{
			// Admin setup and there is no shortcut in User folder
			ShellMan::CommonStartupFolder commonStartupFolder;
			if (!commonStartupFolder.GetPath (startupPath))
				throw Win::Exception ("Setup error: Cannot locate Common Startup folder.");
		}
		dispatcher.Save (startupPath.GetFilePath (DispatcherLink));
	}
	else
	{
		throw Win::Exception ("Setup error: Cannot locate Programs folder");
	}
}

void Installer::AssociateOpenCommand (std::string const & extension,
									  std::string const & extensionClass,
									  std::string const & associationDescription,
									  std::string const & programName)
{
	Registry::ClassRoot extensionRoot (extension.c_str ());
	extensionRoot.Key ().SetValueString ("", extensionClass);
	Registry::ClassRoot zncClass (extensionClass.c_str ());
	zncClass.Key ().SetValueString ("", associationDescription);
	FilePath installPath (_config.GetInstallPath ());
	Registry::ClassShellOpenCmd shellOpen (extensionClass.c_str ());
	std::string cmd;
	cmd = installPath.GetFilePath (programName);
	cmd += " \"%1\"";
	shellOpen.Key ().SetValueString ("", cmd.c_str ());
}

void Installer::CreateAssociations ()
{
	// creating associations is minor --
	// do not interrupt the install process
	try
	{
		Registry::ClassRootCheck zncKey (".znc");
		if (!zncKey.Exists ())
		{
			AssociateOpenCommand (".znc",
								  ZncClass,
								  "Code Co-op Compressed Synchronization Scripts",
								  "unznc.exe");

			Registry::ClassIcon icon (ZncClass);
			FilePath installPath (_config.GetInstallPath ());
			icon.Key ().SetValueString ("", installPath.GetFilePath ("znc.ico"));
		}

		Registry::ClassRootCheck sncKey (".snc");
		if (!sncKey.Exists ())
		{
			AssociateOpenCommand (".snc",
								  SncClass,
								  "Code Co-op Uncompressed Synchronization Scripts",
								  "unznc.exe");

			Registry::ClassIcon icon (SncClass);
			FilePath installPath (_config.GetInstallPath ());
			icon.Key ().SetValueString ("", installPath.GetFilePath ("znc.ico"));
		}

		Registry::ClassRootCheck cnkKey (".cnk");
		if (!cnkKey.Exists ())
		{
			AssociateOpenCommand (".cnk",
								  CnkClass,
								  "Code Co-op Compressed Synchronization Script Chunk",
								  "unznc.exe");

			Registry::ClassIcon icon (SncClass);
			FilePath installPath (_config.GetInstallPath ());
			icon.Key ().SetValueString ("", installPath.GetFilePath ("znc.ico"));
		}

		// Revisit: abstract
		Registry::ClassRootCheck wikiKey (".wiki");
		bool updateWiki = true;
		if (wikiKey.Exists ())
		{
			Registry::ClassRoot extension (".wiki");
			if (extension.Key ().GetStringVal ("") != WikiClass)
				updateWiki = false;
		}
		if (updateWiki)
		{
			AssociateOpenCommand (".wiki",
								  WikiClass,
								  "Code Co-op Wiki Files",
								  "differ.exe");

			Registry::ClassRoot extension (".wiki");
			extension.Key ().SetValueString ("Content Type", "text/plain");
			extension.Key ().SetValueString ("PerceivedType", "text");
		}
	}
	catch (...)
	{}
}

void Installer::CopyFiles (char const * const fileList [])
{
	FilePath const & dst = _config.GetInstallPath ();
	bool isCreating = !_config.IsInSameFolderAsPrevious ();
	std::string sourceList;
	for (int i = 0; fileList [i] != 0; i++)
	{
		char const * source = fileList [i];
		sourceList.append (source, strlen (source));
		sourceList.push_back ('\0');
		if (isCreating)
		{
			// remember only newly created files -- already existing files should be preserved in case of error
			_undoFileList.RememberCreated (dst.GetFilePath (fileList [i]), false);
		}
	}
	if (File::Exists (PostInstallExeName))
	{
		sourceList.append (PostInstallExeName, strlen (PostInstallExeName));
		sourceList.push_back ('\0');
		if (isCreating)
			_undoFileList.RememberCreated (dst.GetFilePath (PostInstallExeName), false);
	}
	ShellMan::CopyFiles (_win, sourceList.c_str (), dst.GetDir ());
}

bool Installer::MaterializeFolderPath (char const * path, bool askUser, bool quiet)
{
    // Count how many levels we have to create
    // If more then one ask the user for confirmation
	unsigned depth = File::CountMissingFolders (path);
    if (depth > 1 && askUser)
    {
		std::string info;
        info = "The path: '";
		info += path;
		info += "'\n\ndoes not exists. Do you want to create it?";
		Out::Answer userChoice = TheOutput.Prompt (info.c_str ());
        if (userChoice != Out::Yes)
		{
			throw Win::Exception ("Setup aborted. Code Co-op has not been installed.");
		}
    }

	if (depth != 0)
	{
		// Now materialize folder path
		return File::MaterializePath (path, quiet);
	}
	return true;
}

void Installer::VerifyFiles (char const * const fileList [],
							 FilePath const & folder,
							 char const * errMsg)
{
	for (int i = 0; fileList [i] != 0; i++)
	{
		char const * sourceFile = folder.GetFilePath (fileList [i]);
		if (!File::Exists (sourceFile))
		{
			throw Win::Exception (errMsg, fileList [i]);
		}
	}
}

void Installer::StoreLicense ()
{
	// REVISIT:!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//		Student license should be stored in the global database
	try
	{
		if (File::Exists ("co-op-setup.bin"))
		{
			MemFileExisting file ("co-op-setup.bin");
			char const * buf = file.GetBuf ();
			std::string license;
			unsigned len = file.GetSize ().Low ();
			license.resize (len);

			char key [] = "Our product is best!";

			unsigned keyLen = strlen (key);
			for (unsigned i = 0; i < len; ++i)
			{
				license [i] = buf [i] - 13 - key [i % keyLen];
			}

			MemberDescription user ("Enter your name",
									"Enter your email address",
									"Comment?",
									license,
									""); // user id label

			Registry::StoreUserDescription (user);
		}
	}
	catch (...)
	{
		Win::ClearError ();
	}
}

void Installer::ConfigDefaultDifferMerger ()
{
	if (_config.GetFullBeyondCompareDifferPath ().IsDirStrEmpty () || !_config.BcSupportsMerge())
	{
		SetUpOurAltDiffer (_config.GetInstallPath ().GetFilePath (BcMergerExe), true); // quiet
	}
//  else: nothing to do, Co-op will detect BCDiffer
}

// Returns true when PATH environment variable updated with our installation path
bool Installer::UpdatePathVariable (std::string & pathVar, std::string const & installPath)
{
	unsigned beg = 0;
	unsigned end = pathVar.find (';');
	do
	{
		std::string dir;
		if (end == std::string::npos)
		{
			dir = pathVar.substr (beg);
			beg = std::string::npos;
		}
		else
		{
			dir = pathVar.substr (beg, end - beg);
			beg = end + 1;
			end = pathVar.find (';', beg);
		}
		if (IsFileNameEqual (dir, installPath))
			return false;
	} while (beg != std::string::npos);
	if (!pathVar.empty ())
		pathVar += ';';
	pathVar += installPath;
	return true;
}

void Installer::UpdateEnvironmentVariables ()
{
	// Add installation path to the PATH variable - required by our help system when invoking
	// Dispatcher and Co-op
	// Windows 2000, XP, and Windows 2003 Server, environment variables are stored in the Registry
	// under the following key:
	//
	//	HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\Session Manager\Environment
	//
	// or for user setup
	//
	//	HKEY_CURRENT_USER\Environment

	try
	{
		std::string installPath (_config.GetInstallPath ().GetDir ());

		if (_config.IsUserSetup ())
		{
			Registry::UserEnvironmentVariables envVars;
			std::string pathVar = envVars.Key ().GetStringVal ("PATH");
			if (UpdatePathVariable (pathVar, installPath))
				envVars.Key ().SetValueString ("PATH", pathVar);
		}
		else
		{
			Registry::EnvironmentVariables envVars;
			std::string pathVar = envVars.Key ().GetStringVal ("Path");
			if (UpdatePathVariable (pathVar, installPath))
				envVars.Key ().SetValueExpandedString ("Path", pathVar);
		}
	}
	catch ( ... )
	{
		// Ignore all errors
	}
}

// Returns calculated timeout (in milliseconds) for the external tool
unsigned Installer::SelectGlobalLicense (Progress::Meter & meter)
{
	FilePath installationFolder (_config.GetInstallPath ());
	std::string appletPath (installationFolder.GetFilePath ("Diagnostics.exe"));
	if (!File::Exists (appletPath))
		return 0;

	Catalog catalog;
	int projectCount = 0;
	for (ProjectSeq seq (catalog); !seq.AtEnd (); seq.Advance ())
		++projectCount;

	if (projectCount == 0)
		return 0;

	unsigned timeout = projectCount * 3000;
	if (timeout == 0)
		return 0;	// Too many projects

	meter.SetRange (0, projectCount + 1);
	meter.SetActivity ("Analyzing project databases");
	meter.StepIt ();
	TmpPath tmpPath;
	_toolOutputFile = tmpPath.GetFilePath ("co-op.xml");
	_infoFile = installationFolder.GetFilePath (ConfigurationProblems);
	std::string cmdLine ("\"");
	cmdLine += appletPath;
	cmdLine += "\" -d:membership -d:catalog -d:pick_license -d:\"";
	cmdLine += ToHexString (reinterpret_cast<unsigned>(_win.ToNative ()));
	cmdLine += "\" \"";
	cmdLine += _toolOutputFile;
	cmdLine += "\" \"";
	cmdLine += _infoFile;
	cmdLine += '"';
	_externalTool.reset (new Win::ChildProcess (cmdLine));
	_externalTool->SetNoFeedbackCursor ();
	_externalTool->ShowMinimizedNotActive ();
	_externalTool->Create ();
	return timeout;
}
