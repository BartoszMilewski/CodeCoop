#if !defined (FTPDOWNLOADER_H)
#define FTPDOWNLOADER_H
// ----------------------------------
// (c) Reliable Software, 2003 - 2007
// ----------------------------------

#include "Download.h"
#include <File/ActiveCopy.h>
#include "Ftp.h"

namespace Ftp
{
	class Downloader : public ::Downloader, public ActiveCopy
	{
	public:
		Downloader (CopyProgressSink & sink, Ftp::Login const & login)
			: ActiveCopy (sink),
			  _access ("Code Co-op"),
			  _internet (_access.Open ())
		{
			_session.SetInternetHandle (_internet);
			_session.SetServer (login.GetServer ());
			_session.SetUser (login.GetUser ());
			_session.SetPassword (login.GetPassword ());
			_ftpSessionHandle = _session.Connect ();
		}

		bool IsAvailable () const { return true; }
		unsigned int CountDownloads () 
		{ 
			return IsWorking () ? 1: 0; 
		}
		void StartGetFile (std::string const & sourcePath, std::string const & targetPath);
		bool Continue (std::string const & destFile) { return false; }

		void CancelCurrent () { StopCopy (); }
		void CancelAll () { CancelCurrent (); }
		void SetTransientErrorIsFatal (bool isFatal) {}
		void SetForegroundPriority () {}
		void SetNormalPriority () {}

		bool Exists (std::string const & path)
		{
			bool isFolder;
			return _ftpSessionHandle.Exists (path, isFolder);
		}
		Ftp::Handle GetFtpSessionHandle () const { return _ftpSessionHandle; }

	private:
		Internet::Access			_access;
		Internet::AutoHandle		_internet;
		Ftp::Session				_session;
		Ftp::AutoHandle				_ftpSessionHandle;
		std::unique_ptr<Ftp::File>	_srcFile;
		std::unique_ptr<LocalFile>	_destFile;
	};
}
#endif
