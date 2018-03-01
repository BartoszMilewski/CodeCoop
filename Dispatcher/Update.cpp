// ----------------------------------
// (c) Reliable Software, 2004 - 2008
// ----------------------------------

#include "precompiled.h"
#include "Update.h"
#include "UpdateDlg.h"
#include "DispatcherParams.h"
#include "OutputSink.h"
#include "AlertMan.h"
#include "Prompter.h"
#include "Registry.h"
#include "Global.h"
#include "VersionXml.h"
#include "Validators.h"

#include <Ctrl/ProgressDialog.h>
#include <Net/BitsDownloader.h>
#include <Net/FtpDownloader.h>
#include <File/File.h>
#include <Com/Shell.h>
#include <Ex/WinEx.h>
#include <Ex/Error.h>
#include <Ctrl/ProgressBar.h>
#include <Lock.h>
#include <Xml/XmlTree.h>

const char VERSION_XML_FILE [] = "CoopVersion.xml";
const char LOCAL_EXE_FILE   [] = "CoopUpdate.exe";
char const RELISOFT_WWW  [] = "www.relisoft.com";
char const RELISOFT_FTP  [] = "ftp.relisoft.com";

char const CHECKING_FEEDBACK [] = "Checking for a newer version...";
char const DOWNLOADING_FEEDBACK [] = "Downloading the latest version...";

UpdateSettings::UpdateSettings ()
{
	Registry::UserDispatcherPrefs prefs;
	_checkPeriod = prefs.GetUpdatePeriod ();
	UpdatePeriodValidator period (_checkPeriod);
	if (!period.IsValid ())
		_checkPeriod = period.GetDefault ();
	_lastBulletin = prefs.GetLastBulletin ();
	_lastDownloadedVersion = prefs.GetLastDownloadedVersion ();
	bool hasValidSettings = false;
	int year, day, month;
	if (prefs.GetUpdateTime (year, month, day))
	{
		_isAutoDownload = !prefs.IsConfirmUpdate ();
		Date dateFromReg (month, day, year);
		if (dateFromReg.IsValid ())
		{
			_nextCheck = dateFromReg;
			hasValidSettings = true;
		}
		else
		{
			if (year == 0 && month == 0 && day == 0)
				hasValidSettings = true; // zero means automatic checking is off
		}
	}
	if (!hasValidSettings)
	{
		// defaults
		_nextCheck.Now ();
		_isAutoDownload = false;
	}
}

bool UpdateSettings::IsTimeToCheck () const
{
	if (_nextCheck.IsValid ())
	{
		if (_nextCheck.IsPast ())
			return true;
	}
	return false;
}

void UpdateSettings::UpdateAutoOptions (bool isAuto, bool isAutoDownload, int period)
{
	dbg << "Updating auto options" << std::endl;
	if (isAuto)
	{
		_isAutoDownload = isAutoDownload;
		if (period == 0)
			throw Win::InternalException ("The period of checking for updates is incorrect.\n"
										  "Please contact support@relisoft.com");

		_checkPeriod = period;
		_nextCheck.Now ();
	}
	else
	{
		_nextCheck.Clear ();
	}
	Registry::UserDispatcherPrefs prefs;
	prefs.SetUpdatePeriod (_checkPeriod);
	prefs.SetIsConfirmUpdate (!_isAutoDownload);
	prefs.SetUpdateTime (_nextCheck.Year (), _nextCheck.Month (), _nextCheck.Day ());
}

void UpdateSettings::UpdateLastBulletin (int lastBulletin)
{
	_lastBulletin = lastBulletin;
	Registry::UserDispatcherPrefs prefs;
	prefs.SetLastBulletin (_lastBulletin);
}

void UpdateSettings::UpdateNextCheck ()
{
	if (IsAutoCheck ())
	{
		_nextCheck.Now ();
		_nextCheck.AddDays (_checkPeriod);
		Registry::UserDispatcherPrefs prefs;
		prefs.SetUpdateTime (_nextCheck.Year (), _nextCheck.Month (), _nextCheck.Day ());
	}
}

void UpdateSettings::UpdateLastDownloadedVersion (std::string const & version)
{
	dbg << "Updating last version registry settings" << std::endl;
	_lastDownloadedVersion = version;
	Registry::UserDispatcherPrefs prefs;
	prefs.SetLastDownloadedVersion (_lastDownloadedVersion);
}

// aux
void Display (char const * msg, bool onUserCmd)
{
	if (onUserCmd)
		TheOutput.Display (msg);
	else
		TheAlertMan.PostInfoAlert (msg);
}

// UpdateManger 

UpdateManager::UpdateManager (
				   Win::Dow::Handle winParent,
				   Win::MessagePrepro & msgPrepro,
				   FilePath const & updatesPath)
	: _winParent (winParent),
	  _msgPrepro (msgPrepro),
	  _updatesFolder (updatesPath),
	  _progressDialog (new Progress::BlindDialog (msgPrepro)),
	  _downloadWhat (Nothing),
	  _downloadState (Idle),
	  _isForeground (false),
	  _excitement (None)
{
	dbg << "Update manager: create." << std::endl;
}

void UpdateManager::OnStartup ()
{
	Assert (!Registry::IsFirstRun ());
	try
	{
		dbg << "Update manager: Restoring state from previous session." << std::endl;
		// Examine downloads from previous sessions
		CreateDownloader ();
		unsigned int count = _downloader->CountDownloads ();
		if (count > 1)
		{
			_downloader->CancelAll ();
			Assert (_downloader->CountDownloads () == 0);
			count = 0;
		}
		// no downloads or exactly one download
		char const * xmlPath = _updatesFolder.GetFilePath (VERSION_XML_FILE);
		if (count == 0)
		{
			dbg << "Updates: no downloads to continue." << std::endl;
			Reset ();
			if (_settings.IsAutoCheck () && File::Exists (xmlPath))
			{
				_versionXml.reset (new VersionXml (xmlPath,	_settings.GetLastBulletinNumber ()));
				ActUponXml (false);
			}
			else
			{
				dbg << "Updates: xml does not exist on disk or auto checking is off." << std::endl;
			}
		}
		else
		{
#if 0
			// for debugging purposes
			_downloader->CancelAll ();
			Reset ();
			return;
#endif

			dbg << "Updates: Found one existing download." << std::endl;
			Assert (count == 1);
			if (!_settings.IsAutoCheck ())
			{
				dbg << "Updates: Cancel the download -- auto checking is off." << std::endl;
				_downloader->CancelAll ();
				Reset ();
				return;
			}
			// auto checking is on.
			if (_downloader->Continue (_updatesFolder.GetFilePath (VERSION_XML_FILE)))
			{
				_downloadWhat = Xml;
				_downloadState = Starting;
				_downloader->SetNormalPriority ();
				_downloader->SetTransientErrorIsFatal (false);
				dbg << "Continuing download of " << VERSION_XML_FILE << std::endl;
			}
			else
			{
				if (File::Exists (xmlPath))
				{
					dbg << "Updates: xml exists on disk." << std::endl;
					_versionXml.reset (new VersionXml (xmlPath,	_settings.GetLastBulletinNumber ()));
					if (_versionXml->IsVersionInfo () &&
						_downloader->Continue (_updatesFolder.GetFilePath (LOCAL_EXE_FILE)))
					{
						_downloadWhat = Exe;
						_downloadState = Starting;
						_downloader->SetNormalPriority ();
						_downloader->SetTransientErrorIsFatal (false);
						dbg << "Continuing download of " << LOCAL_EXE_FILE << std::endl;
					}
					else
					{
						dbg << "Updates: Cannot continue exe download -- cancel the download and examine xml." << std::endl;
						_downloader->CancelAll ();
						Reset ();
						ActUponXml (false);
					}
				}
				else
				{
					dbg << "Updates: xml not found on disk, cannot continue exe download -- cancel the download." << std::endl;
					_downloader->CancelAll ();
					Reset ();
				}
			}
		}
	}
	catch (XML::Exception e)
	{
		OnXmlException (e);
	}
	catch (Win::Exception e)
	{
		OnException (e);
	}
	catch (...)
	{
		OnUnknownException ();
	}
}

UpdateManager::~UpdateManager () {}

void UpdateManager::Reset ()
{
	// do not reset neither _versionXml nor _excitement here! 
	// they are preserved between down loader sessions
	dbg << "Resetting the current download." << std::endl;
	_downloadWhat = Nothing;
	_downloadState = Idle;
	if (_downloader.get () != 0)
		_downloader->CancelCurrent ();
	_downloader.reset ();
	dbg << "Downloader destroyed." << std::endl;
	_progressDialog.reset (new Progress::BlindDialog (_msgPrepro));
	_isForeground = false;
}

bool UpdateManager::IsDownloading () const
{
	Assert (_downloader.get () == 0 && _downloadWhat == Nothing && _downloadState == Idle ||
			_downloader.get () != 0 && _downloadWhat != Nothing && _downloadState != Idle);
	return _downloadWhat != Nothing;
}

void UpdateManager::Configure ()
{
	try
	{
		// Revisit: Displaying dialogs here must be synchronized with DispatcherController::_isBusy.
		// handling of isBusy case must be generalized up there in DispatcherController
		// no need to have separate the isBusy flag on this level
		static bool isBusy = false;
		if (isBusy)
			return;
		ReentranceLock lock (isBusy);

		if (IsDownloading () && _isForeground) // Gui download
			return;

		UpdateOptionsDlgData updateOptionsData (
				_settings.IsAutoCheck (),
				_settings.IsAutoDownload (),
				_settings.GetCheckPeriod ());
		UpdateOptionsCtrl ctrl (updateOptionsData);
		if (ThePrompter.GetData (ctrl))
		{
			if (updateOptionsData.IsAutoCheck ())
			{
				if (!updateOptionsData.IsAutoDownload ())
				{
					// cancel current exe background download
					if (_downloadWhat == Exe && !_isForeground)
					{
						Assert (_downloader.get () != 0);
						Reset ();
					}
				}
			}
			else
			{
				_excitement = None;
				// cancel current background download
				if (!_isForeground)
				{
					Reset ();
				}
			}

			_settings.UpdateAutoOptions (
				updateOptionsData.IsAutoCheck (),
				updateOptionsData.IsAutoDownload (),
				updateOptionsData.GetPeriod ());
		}
	}
	catch (XML::Exception e)
	{
		OnXmlException (e);
	}
	catch (Win::Exception e)
	{
		OnException (e);
	}
	catch (...)
	{
		OnUnknownException ();
	}
}

void UpdateManager::CheckForUpdate (bool onUserCommand)
{
	try
	{
		if (onUserCommand)
		{
			dbg << "Updates: Checking for update on user command." << std::endl;
			if (IsDownloading ())
			{
				SwitchToGui (false); // existing download
				return;
			}
		}
		else
		{
			// automatic check
			dbg << "Updates: Checking for update automatically." << std::endl;
			if (!_settings.IsTimeToCheck ())
			{
				dbg << "Updates: nothing to do -- it's not a check time." << std::endl;
				return;
			}
			if (IsDownloading ())
			{
				dbg << "Updates: there already is a background download." << std::endl;
				return;
			}
		}
		// new request:
		StartDownload (Xml, onUserCommand);
	}
	catch (XML::Exception e)
	{
		OnXmlException (e);
	}
	catch (Win::Exception e)
	{
		OnException (e);
	}
	catch (...)
	{
		OnUnknownException ();
	}
}

void UpdateManager::OnDownloadProgress (int bytesTotal, int bytesTransferred)
{
	try
	{
		dbg << "Updates: Processing update downloader progress notification." << std::endl;
		// is this notification obsolete? (no longer downloading)
		if (!IsDownloading ())
		{
			dbg << "Updates: No longer downloading." << std::endl;
			return; 
		}
		Assert (_isForeground);
		Assert (_progressDialog.get () != 0);
		Progress::Meter & meter = _progressDialog->GetProgressMeter ();
		if (meter.WasCanceled ())
		{
			dbg << "OnDownload: job cancelled by user." << std::endl;
			Reset ();
		}
		else
		{
			if (_downloadState != Downloading)
			{
				_downloadState = Downloading;
				meter.SetActivity ("Downloading...");
			}
			// convert to KB to avoid operations on large int (may overflow)
			bytesTotal = bytesTotal / 1024 + 1; // + 1 -- in case of fileSize < 1024 (div by 0)
			bytesTransferred /= 1024;
			unsigned int progressPercent = 100 * bytesTransferred / bytesTotal;
			meter.StepTo (progressPercent);
		}
	}
	catch (XML::Exception e)
	{
		OnXmlException (e);
	}
	catch (Win::Exception e)
	{
		OnException (e);
	}
	catch (...)
	{
		OnUnknownException ();
	}
}

// return true if 
// - download was successful and
// - auto checking is ON
// under these conditions taskbar icon must be refreshed
// AXIOM: only called in response to download events in the downloader thread
bool UpdateManager::OnDownloadEvent (DownloadEvent event, bool & isNewUpdate, bool & wasUserNotified)
{
	try
	{
		dbg << "Updates: Processing update downloader notification" << std::endl;
		// is this notification obsolete? (no longer downloading)
		if (!IsDownloading ())
		{
			dbg << "Updates: No longer downloading." << std::endl;
			return false; 
		}

		DownloadWhat what = _downloadWhat;
		bool wasOnUserCmd = _isForeground;
		switch (event)
		{		
		case ConnectingEvent:
			{
				dbg << "OnDownloadEvent: Connecting." << std::endl;
				Assert (_progressDialog.get () != 0);
				Progress::Meter & meter = _progressDialog->GetProgressMeter ();
				_downloadState = Connecting;
				if (meter.WasCanceled ())
				{
					Assert (_isForeground);
					dbg << "OnDownloadEvent: job cancelled by user." << std::endl;
					Reset ();
				}
				else
				{
					meter.SetActivity ("Connecting...");
				}
			}
			break;
		case CompletionEvent:
			dbg << "OnDownloadEvent: Completed." << std::endl;
			Reset ();
			if (what == Xml)
			{
				OnXmlDownloaded (wasOnUserCmd);
			}
			else
			{
				Assert (what == Exe);
				OnExeDownloaded (wasOnUserCmd);
			}
			isNewUpdate = IsExcited ();
			wasUserNotified = wasOnUserCmd;
			return _settings.IsAutoCheck ();
			break;
		case FatalErrorEvent:
			dbg << "OnDownloadEvent: fatal error." << std::endl;
			Reset ();
			if (wasOnUserCmd)
				OnDownloadError ();
			break;
		default:
			Assert (!"Invalid downloader state.");
		};
	}
	catch (XML::Exception e)
	{
		OnXmlException (e);
	}
	catch (Win::Exception e)
	{
		OnException (e);
	}
	catch (...)
	{
		OnUnknownException ();
	}
	return false;
}

void UpdateManager::OnXmlDownloaded (bool onUserCmd)
{
	dbg << "Finished downloading " << VERSION_XML_FILE << std::endl;
	Assert (!IsDownloading ());
	_settings.UpdateNextCheck ();
	if (!File::Exists (_updatesFolder.GetFilePath (VERSION_XML_FILE)))
	{
		dbg << "Updates: job transferred, but " << VERSION_XML_FILE << " file DOES NOT exist." << std::endl;
		Display ("Automatic updates error:\nCannot find the downloaded update "
				 "information file on disk.", onUserCmd);
		return;
	}
	// download succeeded!
	dbg << "Updates: job transferred, the XML file exists." << std::endl;
	_versionXml.reset (new VersionXml (_updatesFolder.GetFilePath (VERSION_XML_FILE),
									   _settings.GetLastBulletinNumber ()));
	ActUponXml (onUserCmd);
}

void UpdateManager::OnExeDownloaded (bool onUserCmd)
{
	dbg << "Finished downloading " << LOCAL_EXE_FILE << std::endl;
	Assert (!IsDownloading ());
	
	// Check coherence of data. Defensive approach. Be careful when interacting with external systems.
	if (_versionXml.get () == 0 || !_versionXml->IsVersionInfo ())
	{
		dbg << "Updates: exe downloaded, but info file has no information about it." << std::endl;
		Display ("Cannot proceed with update.\nUpdate information is missing."
				 "\n\nPlease try again.", onUserCmd);
		return;
	}

	if (!File::Exists (_updatesFolder.GetFilePath (LOCAL_EXE_FILE)))
	{
		dbg << "Updates: job transferred, but " << LOCAL_EXE_FILE << " DOES NOT exist." << std::endl;
		Display ("Automatic updates error: "
				"Cannot find the downloaded setup file on disk.",
				onUserCmd);
		return;
	}
	// download succeeded!
	dbg << "Updates: job transferred, the setup file exists." << std::endl;
	_settings.UpdateLastDownloadedVersion (_versionXml->GetVersionNumber ());

	if (_settings.IsAutoCheck ())
		_excitement = ExeDownloaded;

	if (onUserCmd)
	{
		if (!AskToInstallExe ())
			return;
		
		RunSetup ();
	}
}

void UpdateManager::ActUponXml (bool onUserCmd)
{
	dbg << "Analyzing XML" << std::endl;
	Assert (_versionXml.get () != 0);
	Assert (!IsDownloading ());

	if (_versionXml->IsVersionInfo () && _versionXml->IsNewerExe ())
	{
		if (IsLatestExeOnDisk ())
			OnExeOnDisk (onUserCmd);
		else
			OnExeOnServer (onUserCmd);
		
		return;
	}

	// no newer exe
	if (_versionXml->IsBulletinInfo () && _versionXml->IsNewerBulletin ())
	{
		dbg << "New bulletin issue" << std::endl;
		if (_settings.IsAutoCheck ())
			_excitement = NewBulletin;

		if (onUserCmd) // prevent alert
		{
			int lastBulletin = _versionXml->GetBulletinNumber ();
			Assert (lastBulletin > 0);
			_settings.UpdateLastBulletin (lastBulletin);
		}
	}
	else
	{
		dbg << "No newer version" << std::endl;
		_excitement = None;
	}
	
	if (onUserCmd)
		DisplayNoExeDlg ();
}

void UpdateManager::OnExeOnDisk (bool onUserCmd)
{
	dbg << "Setup program already on disk." << std::endl;
	Assert (!IsDownloading ());
	Assert (_versionXml.get () != 0);
	Assert (_versionXml->IsVersionInfo ());

	if (_settings.IsAutoCheck ())
		_excitement = ExeDownloaded;

	if (onUserCmd)
	{
		if (AskToInstallExe ())
			RunSetup ();
	}
}

void UpdateManager::OnExeOnServer (bool onUserCmd)
{
	dbg << "New version is available on server." << std::endl;
	Assert (!IsDownloading ());
	Assert (_versionXml.get () != 0);
	Assert (_versionXml->IsVersionInfo ());

	if (_settings.IsAutoCheck () && !_settings.IsAutoDownload ())
		_excitement = ExeOnServer;

	if (onUserCmd)
	{
		if (AskToInstallExe ())
		{
			StartDownload (Exe, onUserCmd);
		}
	}
	else
	{
		if (_settings.IsAutoDownload ())
		{
			StartDownload (Exe, onUserCmd);
			// new bulletin issue may be also available
			// wait with notifying about new bulletin till new version is downloaded
		}
	}
}

// returns true, if no longer excited after user intervention
bool UpdateManager::OnAlertOpen ()
{
	try
	{
		dbg << "Handling version update alert" << std::endl;
		// Revisit: Displaying dialogs here must be synchronized with DispatcherController::_isBusy.
		// handling of isBusy case must be generalized up there in DispatcherController
		// no need to have separate the isBusy flag on this level
		static bool isBusy = false;
		if (isBusy)
			return !IsExcited ();

		ReentranceLock lock (isBusy);

		if (IsDownloading ())
		{
			SwitchToGui (false); // not a new download
			return !IsExcited ();
		}

		switch (_excitement)
		{
		case ExeOnServer:
			OnAlertExeOnServer ();
			break;
		case ExeDownloaded:
			OnAlertExeDownloaded ();
			break;
		case NewBulletin:
			OnAlertNewBulletin ();
			break;
		default:
			Assert (!"Invalid state");
		};
	}
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
	}
	catch (...)
	{
		TheOutput.Display ("An unknown error.", Out::Error);
	}
	return !IsExcited ();
}

void UpdateManager::OnAlertExeOnServer ()
{
	Assert (!IsDownloading ());
	Assert (_versionXml.get () != 0);
	Assert (_versionXml->IsVersionInfo ());

	if (AskToInstallExe ())
	{
		try
		{
			StartDownload (Exe, true);
		}
		catch (XML::Exception e)
		{
			OnXmlException (e);
		}
		catch (Win::Exception e)
		{
			OnException (e);
		}
		catch (...)
		{
			OnUnknownException ();
		}
	}
}

void UpdateManager::OnAlertExeDownloaded ()
{
	Assert (_versionXml.get () != 0);
	Assert (_versionXml->IsVersionInfo ());

	if (AskToInstallExe ())
	{
		RunSetup ();
	}
}

void UpdateManager::OnAlertNewBulletin ()
{
	Assert (_versionXml.get () != 0);
	Assert (_versionXml->IsBulletinInfo ());
	Assert (_versionXml->GetBulletinNumber () > 0);
	Assert (!_versionXml->GetBulletinLink ().empty ());

	DisplayNoExeDlg ();

	_excitement = None;
	_settings.UpdateLastBulletin (_versionXml->GetBulletinNumber ());
}

void UpdateManager::DisplayNoExeDlg ()
{
	// There is no newer exe, but there may be a new bulletin
	std::string status, headline, bulletinLink;
	if (_versionXml->IsBulletinInfo () && _versionXml->IsNewerBulletin ())
	{
		status = "A new bulletin issue is available.";
		headline = _versionXml->GetBulletinHeadline ();
		if (headline.empty ())
			headline = "No additional information available.";
		bulletinLink = _versionXml->GetBulletinLink ();
	}
	else
	{
		status = "You are using the most up-to-date version of Code Co-op.";
		headline = _versionXml->GetBulletinHeadline ();
		if (headline.empty ())
			headline = "No information available.";
		if (_versionXml->IsBulletinInfo ())
			bulletinLink = _versionXml->GetBulletinLink ();
	}

	UpToDateDlgData upToDateData (status, headline, bulletinLink);
	UpToDateCtrl ctrl (upToDateData);
	ThePrompter.GetData (ctrl);
}

// Return true, if user wants to install/download exe
bool UpdateManager::AskToInstallExe ()
{
	Assert (!IsDownloading ());
	Assert (_versionXml.get () != 0);
	Assert (_versionXml->IsVersionInfo ());
	Assert (_versionXml->IsNewerExe ());

	std::string versionHeadline = _versionXml->GetVersionHeadline ();
	if (versionHeadline.empty ())
		versionHeadline = "No additional version information available.";
	std::string bulletinHeadline = _versionXml->GetBulletinHeadline ();
	if (bulletinHeadline.empty ())
		bulletinHeadline = "No information available.";

	UpdateDlgData updateData (_settings.IsAutoCheck (),
							  _settings.IsAutoDownload (),
							  versionHeadline,
							  _versionXml->GetReleaseNotesLink (),
							  bulletinHeadline,
							  _versionXml->GetBulletinLink (),
							  IsLatestExeOnDisk ());
	UpdateCtrl ctrl (updateData);
	bool isOk = ThePrompter.GetData (ctrl);
	
	if (isOk && updateData.IsTurnOnAutoDownload ())
	{
		int period = _settings.GetCheckPeriod (); // preserve period
		_settings.UpdateAutoOptions (true, true, period);
	}

	if (_versionXml->IsBulletinInfo ())
		_settings.UpdateLastBulletin (_versionXml->GetBulletinNumber ());
	
	if (updateData.IsRemindMeLater ())
	{
		// the simplest "Remind Me Later" functionality implementation
		_excitement = None;
		_settings.UpdateNextCheck ();
	}

	return isOk && !updateData.IsRemindMeLater ();
}

bool UpdateManager::IsLatestExeOnDisk () const
{
	Assert (_versionXml.get () != 0);
	Assert (_versionXml->IsVersionInfo ());
	Assert (!_versionXml->GetVersionNumber ().empty ());

	if (IsNocaseEqual (_settings.GetLastDownloadedVersion (), _versionXml->GetVersionNumber ()))
	{
		return File::Exists (_updatesFolder.GetFilePath (LOCAL_EXE_FILE));
	}
	return false;
}

void UpdateManager::StartDownload (DownloadWhat what, bool isOnUserCommand)
{
	dbg << "Start download." << std::endl;
	Assert (!IsDownloading ());
	Assert (what == Xml || what == Exe);

	std::string remoteFile;
	std::string localFile;
	if (what == Exe)
	{
		Assert (_versionXml.get () != 0);
		Assert (_versionXml->IsVersionInfo ());

		remoteFile = _versionXml->GetUpdateExeName ();
		localFile  = LOCAL_EXE_FILE;
	}
	else
	{
		_settings.UpdateNextCheck ();
		remoteFile = VERSION_XML_FILE;
		localFile  = VERSION_XML_FILE;
	}

	File::CreateFolder (_updatesFolder.GetDir (), false);
	CreateDownloader ();
	// Examine current downloads of the downloader
	unsigned int count = _downloader->CountDownloads ();
	dbg << "Start download: found " << count << " existing downloads." << std::endl;
	if (count > 1)
		Assert (!"More than one downloads at the same time.");
	else if (count == 1)
		Assert (!"Found an existing download when trying to start a new download.");
	
	if (count > 0)
	{
		_downloader->CancelAll ();
		Assert (_downloader->CountDownloads () == 0);
		count = 0;
	}
	// Start new download
	_downloadWhat = what;
	_downloadState = Starting;
	dbg << "Start downloading " << remoteFile << std::endl;
	_downloader->StartGetFile (remoteFile, _updatesFolder.GetFilePath (localFile));
	dbg << "Download started (" << remoteFile << ")" << std::endl;

	if (isOnUserCommand)
		SwitchToGui (true); // new download

	Assert (IsDownloading ());
}

void UpdateManager::CreateDownloader ()
{
	Assert (!IsDownloading ());
	_downloader.reset (new BITS::Downloader (DispatcherTitle, *this, RELISOFT_WWW));
	if (!_downloader->IsAvailable ())
	{
		Ftp::Login login (RELISOFT_FTP);
		_downloader.reset (new Ftp::Downloader (*this, login));

		dbg << "Created FTP downloader" << std::endl;
	}
	else
	{
		dbg << "Created BITS downloader" << std::endl;
	}
}

void UpdateManager::RunSetup ()
{
	dbg << "Launching setup program" << std::endl;
	int errCode = ShellMan::Execute (0, _updatesFolder.GetFilePath (LOCAL_EXE_FILE), 0);
	if (errCode != -1)
	{
		SysMsg errInfo (errCode);
		std::string info = "Problem executing Setup program.\nSystem tells us:\n";
		info += errInfo.Text ();
		TheOutput.Display (info.c_str (), Out::Error);
	}
}

void UpdateManager::OnDownloadError ()
{
	if (_settings.IsAutoCheck ())
		TheOutput.Display ("Code Co-op Update failed due to network problems.\n\n"
					       "We will retry later.");
	else
		TheOutput.Display ("Code Co-op Update failed due to network problems.\n\n"
					       "Try again later.");
}

void UpdateManager::OnXmlException (XML::Exception const & e)
{
	bool wasOnUserCmd = _isForeground;
	Reset ();
#if defined NDEBUG
	if (wasOnUserCmd)
		TheOutput.Display ("No update information available.");
#else
	TheOutput.Display (e);
#endif
	File::DeleteNoEx (_updatesFolder.GetFilePath (VERSION_XML_FILE)); 
}

void UpdateManager::OnException (Win::Exception const & e)
{
	bool wasOnUserCmd = _isForeground;
	Reset ();
#if defined NDEBUG
	if (wasOnUserCmd)
		TheOutput.Display ("The update process failed.\n"
				           "An error occurred when calling the operating system.");
#else
	TheOutput.Display (e);
#endif
}

void UpdateManager::OnUnknownException ()
{
	dbg << "Unexpected error" << std::endl;
	bool wasOnUserCmd = _isForeground;
	Reset ();
	Win::ClearError ();
#if defined NDEBUG
	if (wasOnUserCmd)
		TheOutput.Display ("The update process failed.\n"
		                   "An unknown error occurred.");
#else
	TheOutput.Display ("Unknown error in automatic Co-op updates.");
#endif
}

void UpdateManager::SwitchToGui (bool isNewDownload)
{
	dbg << "Updates: switch to UI." << std::endl;
	Assert (IsDownloading ());

	if (_isForeground)
		return; // already showing UI

	char const * activity = 0;
	if (isNewDownload)
	{
		activity = "Initiating a new download...";
	}
	else
	{
		// already downloading in background
		activity = "Continuing automatic background download...";
	}

	_isForeground = true;

	_downloader->SetForegroundPriority ();
	_downloader->SetTransientErrorIsFatal (true);
	
	_progressDialog.reset (new Progress::MeterDialog ("Code Co-op - Checking for Update",
													 _winParent,
													 _msgPrepro));
	_progressDialog->SetCaption (_downloadWhat == Exe ? DOWNLOADING_FEEDBACK : CHECKING_FEEDBACK);
	Progress::Meter & meter = _progressDialog->GetProgressMeter ();
	meter.SetRange (1, 100);
	meter.SetActivity (activity);
}

char const * UpdateManager::GetNewUpdateInfo () const
{
	Assert (IsExcited ());
	switch (_excitement)
	{
		// max of 64 characters on Win95, 98, Me
		// max of 128 characters on Win2000, XP
		// Notice: ViewMan adds "\nDouble click to see details." of 30 characters
	case ExeOnServer:
		return "A new version is available for download!"; // 31 + 30 < 64
	case ExeDownloaded:
		return "A new version is ready to install!";
	case NewBulletin:
		return "A new bulletin issue is available!";
	default:
		// not reachable
		return "Bug";
	};
}

// CopyProgressSink interface

// returns false, if the process needs to be terminated
bool UpdateManager::OnConnecting ()
{
	dbg << "Updates: connecting." << std::endl;
	Win::RegisteredMessage msg (UM_DOWNLOAD_EVENT);
	msg.SetWParam (ConnectingEvent);
	_winParent.PostMsg (msg);
	return true;
}

// returns false, if the process needs to be terminated
bool UpdateManager::OnProgress (int bytesTotal, int bytesTransferred)
{
	dbg << "Updates: job progress." << std::endl;
	if (_isForeground)
	{
		Win::RegisteredMessage msgProgress (UM_DOWNLOAD_PROGRESS);
		msgProgress.SetWParam (bytesTotal);
		msgProgress.SetLParam (bytesTransferred);
		_winParent.PostMsg (msgProgress);
	}
	return true;
}

bool UpdateManager::OnCompleted (CompletionStatus status)
{
	switch (status)
	{
	case Success:
		{
			dbg << "Update completion: success." << std::endl;
			Win::RegisteredMessage msg (UM_DOWNLOAD_EVENT);
			msg.SetWParam (CompletionEvent);
			_winParent.PostMsg (msg);
			return true;
		}
		break;
	case PartialSuccess:
		{
			// act as in a case of success
			dbg << "Update completion: partial success." << std::endl;
			Win::RegisteredMessage msg (UM_DOWNLOAD_EVENT);
			msg.SetWParam (CompletionEvent);
			_winParent.PostMsg (msg);
			return true;
		}
		break;
	case CannotDeleteTempFiles:
		{
			// act as in a case of success, but cancel the download;
			dbg << "Update completion: cannot delete temp files." << std::endl;
			Win::RegisteredMessage msg (UM_DOWNLOAD_EVENT);
			msg.SetWParam (CompletionEvent);
			_winParent.PostMsg (msg);
			return false;
		}
		break;
	case AlreadyHandled:
		dbg << "Update completion: already handled." << std::endl;
		// nothing to do
		return true;
		break;
	case Unrecognized:
		dbg << "Update completion: unrecognized status." << std::endl;
		// Cancel the download.
		return false;
		break;
	default:
		Assert (!"Update completion: invalid completion status.");
	};
	return false;
}

void UpdateManager::OnTransientError ()
{
	// ignore transient errors
	dbg << "Updates: transient error." << std::endl;
}

void UpdateManager::OnException (Win::Exception & ex)
{
	dbg << "Updates: exception - " << Out::Sink::FormatExceptionMsg (ex) << std::endl;
	Win::RegisteredMessage msg (UM_DOWNLOAD_EVENT);
	msg.SetWParam (FatalErrorEvent);
	_winParent.PostMsg (msg);
}

void UpdateManager::OnFatalError ()
{
	dbg << "Updates: fatal error." << std::endl;
	Win::RegisteredMessage msg (UM_DOWNLOAD_EVENT);
	msg.SetWParam (FatalErrorEvent);
	_winParent.PostMsg (msg);
}
