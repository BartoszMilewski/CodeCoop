#if !defined (FtpSite_H)
#define FtpSite_H
//------------------------------------
//  (c) Reliable Software, 2007 - 2008
//------------------------------------

#include <File/Path.h>
#include <Net/Ftp.h>
#include <Net/FtpUploader.h>
#include <Net/FtpDownloader.h>
#include <File/ActiveCopy.h>
#include <Bit.h>

class LocalTestFile;
namespace Progress { class Meter; }
namespace ShellMan { class FileRequest; }

namespace Ftp
{
	class SmartLogin: public Ftp::Login
	{
	public:
		SmartLogin ()
		{}
		SmartLogin (std::string const & server,
					 std::string const & folder,
					 std::string const & user = std::string (),
					 std::string const & password = std::string ())
			: Login (server, user, password),
			  _folder (folder)
		{}
		std::string const & GetFolder () const { return _folder; }
		void SetFolder (std::string const & folder) { _folder = folder; }

		void RetrieveUserPrefs (std::string const & keyName);
		void RememberUserPrefs (std::string const & keyName) const;
		void Clear ();
	private:
		std::string	_folder;
	};

	class CopyProgressMeter : public CopyProgressSink
	{
	public:
		CopyProgressMeter (Progress::Meter & meter, Ftp::Login const & login);

		bool IsException () const;
		std::string const & GetErrorMsg () const;

		// CopyProgressSink
		bool OnConnecting ();
		bool OnProgress (int bytesTotal, int bytesTransferred);
		bool OnCompleted (CompletionStatus status);
		void OnTransientError ();
		void OnException (Win::Exception & ex);

	private:
		Progress::Meter &	_meter;
		Ftp::Login const &	_login;
		bool				_isCopyInProgress;
		bool				_exception;
		std::string			_errorMsg;
	};

	class Site
	{
	public:
		Site (Ftp::Login const & login, std::string const & remoteFolder = std::string ())
			: _login (login),
			  _remoteFolder (remoteFolder)
		{}
		bool Exists (std::string const & path, bool & isFolder);
		void SetRemoteDirectory (std::string const & remoteFolder)
		{
			_remoteFolder.Change (remoteFolder);
		}
		bool TestConnection (bool testWrite = true);
		void DisplayErrors () const;
		bool Upload (std::string const & localPath,
					 std::string const & remoteFileName,
					 Progress::Meter & meter) throw (Win::Exception);
		bool Download (std::string const & remoteFileName,
					   std::string const & localFilePath,
					   Progress::Meter & meter) throw (Win::Exception);
		bool DeleteFile (std::string const & fileName);
		bool DeleteFiles (ShellMan::FileRequest const & request);

		bool IsException () const { return _errors.test (ExceptionThrown); }
		std::string const & GetErrorMsg () const { return _errorMsg; }

	public:
		enum Errors
		{
			NoServerName,
			NoInternet,
			CannotConnect,
			ExceptionThrown,
			FileNotFound,
			FileSizeDontMatch,
			OperationCanceled
		};

	private:
		class TestFile
		{
		public:
			TestFile (Ftp::Handle hFtp, std::string const & path);
			~TestFile ();

			void CreateFrom (LocalTestFile const & localTestFile);

		private:
			Ftp::Handle	_hFtp;
			std::string	_targetPath;
			std::string _targetFilePath;
			bool		_copied;
		};

	private:
		Ftp::Login		_login;
		FilePath		_remoteFolder;
		BitSet<Errors>	_errors;
		std::string		_errorMsg;
	};

	class UploadTraversal
	{
	public:
		UploadTraversal (std::string const & sourcePath,
						 std::string const & targetPath,
						 Ftp::Login const & login,
						 Progress::Meter & overallMeter,
						 Progress::Meter & specificMeter);

		void MaterializeFoldersAtTarget ();
		void CopyFiles ();

	private:
		void CountFiles (FilePath & localPath, unsigned & count);
		void BuildTree (FilePath & localPath, FilePath & remotePath);
		void CopyFiles (FilePath & localPath, FilePath & remotePath, unsigned & copiedSoFar);

	private:
		Progress::Meter &		_overallMeter;
		Ftp::CopyProgressMeter	_copyProgressMeter;
		Ftp::Uploader			_uploader;
		std::string const &		_sourcePath;
		std::string const &		_targetPath;
		unsigned				_fileCount;
	};

	class DownloadTraversal
	{
	public:
		DownloadTraversal (std::string const & sourcePath,
						   std::string const & targetPath,
						   Ftp::Login const & login,
						   Progress::Meter & meter);

		bool CopyFiles ();

	private:
		bool CopyFiles (FilePath & remotePath, FilePath & localPath);

	private:
		Progress::Meter &		_meter;
		Ftp::CopyProgressMeter	_copyProgressMeter;
		Ftp::Downloader			_downloader;
		std::string const &		_sourcePath;
		std::string const &		_targetPath;
	};
}

#endif
