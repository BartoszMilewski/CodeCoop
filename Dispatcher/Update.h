#if !defined (UPDATE_H)
#define UPDATE_H
// ----------------------------------
// (c) Reliable Software, 2003 - 2007
// ----------------------------------

#include <Net/Download.h>
#include <File/Path.h>
#include <Sys/Date.h>
#include <Win/Win.h>
#include <File/ActiveCopy.h>

namespace Win { class MessagePrepro; }
class VersionXml;
namespace Progress
{
	class Dialog;
	class Meter;
}
namespace XML { class Exception; }

// registry settings
class UpdateSettings
{
public:
	UpdateSettings ();

	int GetLastBulletinNumber () const { return _lastBulletin; }
	int GetCheckPeriod () { return _checkPeriod; }
	std::string const & GetLastDownloadedVersion () const { return _lastDownloadedVersion; }

	bool IsAutoCheck () const { return _nextCheck.IsValid (); }
	bool IsAutoDownload () const { return _isAutoDownload; }
	bool IsTimeToCheck () const;

	void UpdateAutoOptions (bool isAuto, bool isAutoDownload, int period);
	void UpdateNextCheck ();
	void UpdateLastBulletin (int lastBulletin);
	void UpdateLastDownloadedVersion (std::string const & version);
private:
	Date							_nextCheck;   // empty date means that checking is NOT automatic
	unsigned int					_checkPeriod; // in days
	bool							_isAutoDownload;
	unsigned int					_lastBulletin;
	std::string						_lastDownloadedVersion;
};

class UpdateManager : public CopyProgressSink
{
public:
	// UM_DOWNLOAD_EVENT wParam:
	enum DownloadEvent
	{
		ConnectingEvent,
		CompletionEvent,
		FatalErrorEvent
	};
public:
	UpdateManager (Win::Dow::Handle winParent,
				   Win::MessagePrepro & msgPrepro,
				   FilePath const & updatesPath);
	~UpdateManager ();
	void OnStartup ();

	void CheckForUpdate (bool onUserCommand);
	void Configure ();
	bool OnDownloadEvent (DownloadEvent event, bool & isNewUpdate, bool & wasUserNotified);
	void OnDownloadProgress (int bytesTotal, int bytesTransferred);
	bool OnAlertOpen ();
	bool UsesAlerts () const { return _settings.IsAutoCheck (); }
	bool IsAlerting () const { return UsesAlerts () && IsExcited (); }
	char const * GetNewUpdateInfo () const;
	
	// CopyProgressSink interface
	// all methods returning boolean value:
	// return false, if the download should be terminated
	bool OnConnecting ();
	bool OnProgress (int bytesTotal, int bytesTransferred);
	bool OnCompleted (CompletionStatus status);
	void OnTransientError ();
	void OnException (Win::Exception & ex);
	void OnFatalError ();

private:
	enum DownloadWhat
	{
		Nothing,
		Xml,
		Exe
	};
private:
	void CreateDownloader ();
	void Reset ();
	void OnXmlDownloaded (bool onUserCmd);
	void OnExeDownloaded (bool onUserCmd);
	void ActUponXml (bool onUserCmd);
	void OnExeOnDisk (bool onUserCmd);
	void OnExeOnServer (bool onUserCmd);
	void OnNewBulletin ();
	void OnNoExeNoBulletin ();
	void OnAlertExeOnServer ();
	void OnAlertExeDownloaded ();
	void OnAlertNewBulletin ();
	void DisplayNoExeDlg ();
	bool AskToInstallExe ();
	bool IsLatestExeOnDisk () const;
	bool IsExcited () const { return _excitement != None; }
	bool IsDownloading () const;
	void RunSetup ();
	
	void StartDownload (DownloadWhat what, bool isOnUserCommand);
	void OnDownloadError ();
	void OnException (Win::Exception const & e);
	void OnXmlException (XML::Exception const & e);
	void OnUnknownException ();

	void SwitchToGui (bool isNewDownload);
private:
	enum DownloadState
	{
		Idle,
		Starting,
		Connecting,
		Downloading
	};
	enum Excitement // waits for user interaction
	{
		None = 0,
		ExeOnServer,
		ExeDownloaded,
		NewBulletin
	};

	Win::Dow::Handle  				_winParent;
	Win::MessagePrepro &			_msgPrepro;
	
	UpdateSettings					_settings;
	FilePath						_updatesFolder;

	std::unique_ptr<Downloader>		_downloader;
	std::unique_ptr<Progress::Dialog>	_progressDialog;
	
	// current download status
	DownloadWhat					_downloadWhat;
	DownloadState					_downloadState;
	bool							_isForeground;
	// result of download (preserved between downloads)
	std::unique_ptr<VersionXml>		_versionXml;
	Excitement						_excitement;
};

#endif
