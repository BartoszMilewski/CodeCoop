//------------------------------------
//  (c) Reliable Software, 2007 - 2008
//------------------------------------

#include "precompiled.h"
#include "FtpSite.h"
#include "OutputSink.h"
#include "Registry.h"
#include "TestFile.h"

#include <Ctrl/ProgressMeter.h>
#include <File/File.h>
#include <File/Path.h>
#include <File/Dir.h>
#include <Com/ShellRequest.h>
#include <Ex/Error.h>

void Ftp::SmartLogin::RetrieveUserPrefs (std::string const & keyName)
{
	Registry::UserPreferences prefs;
	RegKey::Check userPrefsKey (prefs.GetRootKey (), keyName);
	if (userPrefsKey.Exists ())
	{
		RegKey::Existing ftpPrefsKey (prefs.GetRootKey (), keyName);
		_server = ftpPrefsKey.GetStringVal ("Server");
		_folder = ftpPrefsKey.GetStringVal ("Folder");
		_user = ftpPrefsKey.GetStringVal ("User");
		if (!IsAnonymous ())
			ftpPrefsKey.GetStringDecrypt ("Password", _password);
	}
}

void Ftp::SmartLogin::RememberUserPrefs (std::string const & keyName) const
{
	Registry::UserPreferences prefs;
	RegKey::New ftpPrefsKey (prefs.GetRootKey (), keyName);
	ftpPrefsKey.SetValueString ("Server", GetServer ());
	ftpPrefsKey.SetValueString ("Folder", GetFolder ());
	if (!IsAnonymous ())
	{
		ftpPrefsKey.SetValueString ("User", GetUser ());
		ftpPrefsKey.SetStringEncrypt ("Password", GetPassword (), true); // Quiet
	}
}

void Ftp::SmartLogin::Clear ()
{
	Ftp::Login::Clear ();
	_folder.clear ();
}

namespace Ftp
{
	class TmpConnection
	{
	public:
		TmpConnection (Ftp::Login const & login, BitSet<Ftp::Site::Errors> & errors);
		Ftp::Handle GetHandle () const { return _hFtp; }
	private:
		Ftp::Login const &			_login;
		BitSet<Ftp::Site::Errors> & _errors;
		Internet::Access			_access;
		Internet::AutoHandle		_hInternet;
		Ftp::Session				_session;
		Ftp::AutoHandle				_hFtp;
	};
}

Ftp::TmpConnection::TmpConnection (Ftp::Login const & login, 
								   BitSet<Ftp::Site::Errors> & errors)
	: _login (login)
	, _errors (errors)
	, _access ("Code Co-op")
{
	if (!_access.AttemptConnect ())
	{
		_errors.set (Ftp::Site::NoInternet, true);
		return;
	}
	std::string url ("ftp://");
	url += _login.GetServer ();
	if (!_access.CheckConnection (url.c_str (), true))
	{
		_errors.set (Ftp::Site::CannotConnect, true);
		return;
	}

	_hInternet = _access.Open ();
	_session.SetInternetHandle (_hInternet);
	_session.SetServer (_login.GetServer ());
	if (!_login.IsAnonymous ())
	{
		_session.SetUser (_login.GetUser ());
		_session.SetPassword (_login.GetPassword ());
	}
	_hFtp = _session.Connect ();
}

bool Ftp::Site::Exists (std::string const & path, bool & isFolder)
{
	_errors.init (0);
	Ftp::TmpConnection connection (_login, _errors);
	if (!_errors.empty ())
		return false;
	return connection.GetHandle ().Exists (path, isFolder);
}

bool Ftp::Site::TestConnection (bool testWrite)
{
	_errors.init (0);
	if (_login.GetServer ().empty ())
	{
		_errors.set (NoServerName, true);
		return false;
	}

	try
	{
		Ftp::TmpConnection connection (_login, _errors);
		if (!_errors.empty ())
			return false;
		if (testWrite)
		{
			Ftp::Handle hFtp = connection.GetHandle ();
			hFtp.MaterializeFolderPath (_remoteFolder.GetDir ());
			LocalTestFile localTestFile;
			Ftp::Site::TestFile ftpTestFile (hFtp,
											 _remoteFolder.GetFilePath ("FtpTest.txt", true)); // Use slash
			ftpTestFile.CreateFrom (localTestFile);
			Assert (hFtp.GetCurrentDirectory () == "/");
		}
	}
	catch (Win::Exception e)
	{
		_errors.set (ExceptionThrown, true);
		_errorMsg = Out::Sink::FormatExceptionMsg (e);
		return false;
	}
	catch ( ... )
	{
		_errors.set (ExceptionThrown, true);
		_errorMsg = "Unknown exception during Internet access.";
		return false;
	}
	return true;
}

void Ftp::Site::DisplayErrors () const
{
	if (_errors.test (NoServerName))
	{
		TheOutput.Display ("Please, specify the FTP server name.");
	}
	else if (_errors.test (ExceptionThrown) || _errors.test (FileNotFound))
	{
		TheOutput.Display (_errorMsg.c_str ());
	}
	else if (_errors.test (NoInternet))
	{
		TheOutput.Display ("You are not connected to the Internet.");
	}
	else if (_errors.test (CannotConnect))
	{
		std::string url ("ftp://");
		url += _login.GetServer ();
		std::string info ("Cannot connect to ");
		info += url;
		info += "\n Error: ";
		info += ToString(::GetLastError());
		TheOutput.Display (info.c_str ());
	}
	else if (_errors.test (FileSizeDontMatch))
	{
		TheOutput.Display ("Operation failed - source file size different from the destination file size.");
	}
	else if (_errors.test (OperationCanceled))
	{
		TheOutput.Display ("Operation canceled by the user.");
	}
}

// Returns true when upload successful
bool Ftp::Site::Upload (std::string const & localPath,
						std::string const & remoteFileName,
						Progress::Meter & meter) throw (Win::Exception)
{
	CopyProgressMeter copyProgressMeter (meter, _login);
	Ftp::Uploader uploader (copyProgressMeter, _login);
	uploader.MaterializeFolderPath (_remoteFolder.GetDir ());
	uploader.StartPutFile (localPath, _remoteFolder.GetFilePath (remoteFileName, true));// Use slash
	bool wasCancelled = false;
	while (uploader.IsWorking ())
	{
		wasCancelled = meter.WasCanceled (); // pump message loop
		::Sleep (500);
	}

	if (wasCancelled)
	{
		_errors.set (OperationCanceled, true);
		uploader.Abort ();
	}
	else
	{
		_errors.set (ExceptionThrown, copyProgressMeter.IsException ());
		if (_errors.test (ExceptionThrown))
			_errorMsg = copyProgressMeter.GetErrorMsg ();

		try
		{
			::File::Size uploadedFileSize = uploader.GetUploadedFilesize ();
			::File sourceFile (localPath, ::File::ReadOnlyMode ());
			::File::Size sourceFileSize = sourceFile.GetSize ();
			if (sourceFileSize != uploadedFileSize)
				_errors.set (FileSizeDontMatch, true);
		}
		catch ( ... )
		{
			_errors.set (FileSizeDontMatch, true);
		}
	}

	return !wasCancelled && !_errors.test (ExceptionThrown) && !_errors.test (FileSizeDontMatch);
}

// Returns true when download successful
bool Ftp::Site::Download (std::string const & remoteFileName,
						  std::string const & localFilePath,
						  Progress::Meter & meter) throw (Win::Exception)
{
	CopyProgressMeter copyProgressMeter (meter, _login);
	Ftp::Downloader downloader (copyProgressMeter, _login);
	if (!downloader.Exists (_remoteFolder.GetFilePath (remoteFileName, true)))	// Use slash
	{
		_errors.set (FileNotFound, true);
		_errorMsg = "The following file:\n\n";
		_errorMsg += _remoteFolder.GetFilePath (remoteFileName, true);	// Use slash
		_errorMsg += "\n\nwas not found on the server ";
		_errorMsg += _login.GetServer ();
		return false;
	}

	downloader.StartGetFile (_remoteFolder.GetFilePath (remoteFileName), localFilePath);
	bool wasCancelled = false;
	while (downloader.IsWorking ())
	{
		wasCancelled = meter.WasCanceled (); // pump message loop
		::Sleep (500);
	}
	_errors.set (ExceptionThrown, copyProgressMeter.IsException ());
	if (_errors.test (ExceptionThrown))
		_errorMsg = copyProgressMeter.GetErrorMsg ();

	if (wasCancelled)
	{
		_errors.set (OperationCanceled, true);
		downloader.CancelCurrent ();
	}

	return !wasCancelled && !_errors.test (ExceptionThrown);
}

bool Ftp::Site::DeleteFile (std::string const & fileName)
{
	_errors.init (0);
	if (_login.GetServer ().empty ())
	{
		_errors.set (NoServerName, true);
		return false;
	}

	try
	{
		Ftp::TmpConnection connection (_login, _errors);
		if (!_errors.empty ())
			return false;

		Ftp::Handle hFtp = connection.GetHandle ();
		hFtp.DeleteFile (_remoteFolder.GetFilePath (fileName));
	}
	catch (Win::Exception e)
	{
		_errors.set (ExceptionThrown, true);
		_errorMsg = Out::Sink::FormatExceptionMsg (e);
		return false;
	}
	catch ( ... )
	{
		_errors.set (ExceptionThrown, true);
		_errorMsg = "Unknown exception during internet access.";
		return false;
	}
	return true;
}

bool Ftp::Site::DeleteFiles (ShellMan::FileRequest const & request)
{
	_errors.init (0);
	if (_login.GetServer ().empty ())
	{
		_errors.set (NoServerName, true);
		return false;
	}

	try
	{
		Ftp::TmpConnection connection (_login, _errors);
		if (!_errors.empty ())
			return false;

		Ftp::Handle hFtp = connection.GetHandle ();
		for (ShellMan::FileRequest::Sequencer seq (request);
			 !seq.AtEnd ();
			 seq.Advance ())
		{
			hFtp.DeleteFile (_remoteFolder.GetFilePath (seq.GetFilePath ()));
		}
	}
	catch (Win::Exception e)
	{
		_errors.set (ExceptionThrown, true);
		_errorMsg = Out::Sink::FormatExceptionMsg (e);
		return false;
	}
	catch ( ... )
	{
		_errors.set (ExceptionThrown, true);
		_errorMsg = "Unknown exception during internet access.";
		return false;
	}
	return true;
}

Ftp::Site::TestFile::TestFile (Ftp::Handle hFtp, std::string const & path)
	: _hFtp (hFtp),
	  _targetPath (path),
	  _copied (false)
{}

Ftp::Site::TestFile::~TestFile ()
{
	if (_copied)
		_hFtp.DeleteFile (_targetPath.c_str ());
}

void Ftp::Site::TestFile::CreateFrom (LocalTestFile const & localTestFile)
{
	std::string const & localFilePath = localTestFile.GetPath ();
	if (_hFtp.PutFile (localFilePath.c_str (), _targetPath.c_str ()))
	{
		_copied = true;
	}
	else
	{
		LastSysErr lastSysErr;
		if (lastSysErr.IsInternetError ())
		{
			LastInternetError lastInternetError;
			throw Win::InternalException ("Cannot copy test file to the server.",
										  lastInternetError.Text ().c_str ());
		}
		else
		{
			throw Win::Exception ("Cannot copy test file to the server.");
		}
	}
}

Ftp::CopyProgressMeter::CopyProgressMeter (Progress::Meter & meter, Ftp::Login const & login)
	: _meter (meter),
	  _login (login),
	  _isCopyInProgress (false),
	  _exception (false)
{
	_meter.SetActivity ("");
	_meter.SetRange (0, 100, 1);	// 0 - 100%
}

bool Ftp::CopyProgressMeter::IsException () const
{
	if (_isCopyInProgress)
		throw Win::InternalException ("Ftp::CopyProgressMeter::IsException - illegal call.");
	return _exception;
}

std::string const & Ftp::CopyProgressMeter::GetErrorMsg () const
{
	if (_isCopyInProgress)
		throw Win::InternalException ("Ftp::CopyProgressMeter::GetErrorMsg - illegal call.");
	return _errorMsg;
}

bool Ftp::CopyProgressMeter::OnConnecting ()
{
	_isCopyInProgress = true;
	_meter.SetActivity ("0% completed.");
	_meter.StepTo (0);
	return true;
}

bool Ftp::CopyProgressMeter::OnProgress (int bytesTotal, int bytesTransferred)
{
	try
	{
		// convert to KB to avoid operations on large int (may overflow)
		bytesTotal = (bytesTotal / 1024) + 1; // + 1 -- in case of fileSize < 1024 (div by 0)
		bytesTransferred /= 1024;
		unsigned int progressPercent = 100 * bytesTransferred / bytesTotal;
		std::string activity;
		activity = ::ToString (progressPercent);
		activity += "% completed.";
		_meter.SetActivity (activity.c_str ());
		_meter.StepTo (progressPercent);
		_meter.StepAndCheck ();
	}
	catch (Win::Exception ex)
	{
		Assert (ex.GetMessage () == 0);
		// User canceled upload
		_isCopyInProgress = false;
		return false;
	}
	return true;
}

bool Ftp::CopyProgressMeter::OnCompleted (CompletionStatus status)
{
	_isCopyInProgress = false;
	return true;
}

void Ftp::CopyProgressMeter::OnTransientError ()
{
}

void Ftp::CopyProgressMeter::OnException (Win::Exception & ex)
{
	_exception = true;
	_errorMsg = Out::Sink::FormatExceptionMsg (ex);
	_isCopyInProgress = false;
}

// take two progress meters: overall and specific
Ftp::UploadTraversal::UploadTraversal (std::string const & sourcePath,
									   std::string const & targetPath,
									   Ftp::Login const & login,
									   Progress::Meter & overallMeter,
									   Progress::Meter & specificMeter)
	: _overallMeter (overallMeter),
	  _copyProgressMeter (specificMeter, login), // <- specific
	  _uploader (_copyProgressMeter, login),
	  _sourcePath (sourcePath),
	  _targetPath (targetPath),
	  _fileCount (0)
{
	FilePath localPath (_sourcePath);
	CountFiles (localPath, _fileCount);
}

void Ftp::UploadTraversal::MaterializeFoldersAtTarget ()
{
	FilePath localPath (_sourcePath);
	FilePath remotePath (_targetPath);
	BuildTree (localPath, remotePath);
}

void Ftp::UploadTraversal::CopyFiles ()
{
	FilePath localPath (_sourcePath);
	FilePath remotePath (_targetPath);
	_overallMeter.SetRange (1, _fileCount);
	unsigned copiedSoFar = 0;
	CopyFiles (localPath, remotePath, copiedSoFar);
}

void Ftp::UploadTraversal::CountFiles (FilePath & localPath, unsigned & count)
{
	for (::FileSeq seq (localPath.GetAllFilesPath ()); !seq.AtEnd (); seq.Advance ())
	{
		if (seq.IsFolder ())
		{
			char const * name = seq.GetName ();
			localPath.DirDown (name);
			CountFiles (localPath, count);
			localPath.DirUp ();
		}
		else
		{
			++count;
		}
	}
}

void Ftp::UploadTraversal::BuildTree (FilePath & localPath, FilePath & remotePath)
{
	_uploader.MaterializeFolderPath (remotePath.GetDir ());

	for (DirSeq seq (localPath.GetAllFilesPath ()); !seq.AtEnd (); seq.Advance ())
	{
		char const * name = seq.GetName ();
		localPath.DirDown (name);
		remotePath.DirDown (name, true);	// Use slash
		BuildTree (localPath, remotePath);
		localPath.DirUp ();
		remotePath.DirUp ();
	}
}

void Ftp::UploadTraversal::CopyFiles (FilePath & localPath,
									  FilePath & remotePath,
									  unsigned & copiedSoFar)
{
	for (::FileSeq seq (localPath.GetAllFilesPath ()); !seq.AtEnd (); seq.Advance ())
	{
		char const * name = seq.GetName ();
		if (seq.IsFolder ())
		{
			localPath.DirDown (name);
			remotePath.DirDown (name, true);	// Use slash
			CopyFiles (localPath, remotePath, copiedSoFar);
			localPath.DirUp ();
			remotePath.DirUp ();
		}
		else
		{
			++copiedSoFar;
			std::string overallActivity ("Copying file ");
			overallActivity += ::ToString (copiedSoFar);
			overallActivity += '/';
			overallActivity += ::ToString (_fileCount);
			_overallMeter.SetActivity (overallActivity);
			_uploader.StartPutFile (localPath.GetFilePath (name),
									remotePath.GetFilePath (name, true)); // Use slash
			while (_uploader.IsWorking ())
			{
				::Sleep (500);
			}
			_overallMeter.StepAndCheck ();
		}
	}
}

Ftp::DownloadTraversal::DownloadTraversal (std::string const & sourcePath,
										   std::string const & targetPath,
										   Ftp::Login const & login,
										   Progress::Meter & meter)
	: _meter (meter),
	  _copyProgressMeter (meter, login),
	  _downloader (_copyProgressMeter, login),
	  _sourcePath (sourcePath),
	  _targetPath (targetPath)
{
}

bool Ftp::DownloadTraversal::CopyFiles ()
{
	if (!_downloader.Exists (_sourcePath))
		throw Win::InternalException ("Folder not found on the server.", _targetPath.c_str ());

	FilePath remotePath (_sourcePath);
	FilePath localPath (_targetPath);
	return CopyFiles (remotePath, localPath);
}

// Returns true when files copied successfully
bool Ftp::DownloadTraversal::CopyFiles (FilePath & remotePath, FilePath & localPath)
{
	std::vector<std::string> remoteSubFolders;
	::File::MaterializePath (localPath.GetDir ());
	bool wasCancelled = false;
	for (Ftp::FileSeq seq (_downloader.GetFtpSessionHandle (), remotePath.GetAllFilesPath ());
		 !seq.AtEnd () && !wasCancelled;
		 seq.Advance ())
	{
		char const * name = seq.GetName ();
		if (seq.IsFolder ())
		{
			remoteSubFolders.push_back (name);
		}
		else
		{
			_downloader.StartGetFile (remotePath.GetFilePath (name, true), // Use slash
									  localPath.GetFilePath (name));
			while (_downloader.IsWorking ())
			{
				wasCancelled = _meter.WasCanceled (); // pump message loop
				::Sleep (500);
			}
		}
	}

	if (!wasCancelled)
	{
		// Copy files from sub-folders
		for (std::vector<std::string>::const_iterator iter = remoteSubFolders.begin ();
			 iter != remoteSubFolders.end ();
			 ++iter)
		{
			std::string const & folder = *iter;
			localPath.DirDown (folder.c_str ());
			remotePath.DirDown (folder.c_str (), true);	// Use slash
			if (!CopyFiles (remotePath, localPath))
				return false;
			localPath.DirUp ();
			remotePath.DirUp ();
		}
	}

	return !wasCancelled;
}
