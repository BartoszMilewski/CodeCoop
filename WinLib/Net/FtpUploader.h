#if !defined (FTPUPLOADER_H)
#define FTPUPLOADER_H
//------------------------------------
//  (c) Reliable Software, 2007 - 2008
//------------------------------------

#include <File/ActiveCopy.h>
#include "Ftp.h"

namespace Ftp
{
	class Uploader : public ActiveCopy
	{
	public:
		Uploader (CopyProgressSink & sink, Ftp::Login const & login)
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

		::File::Size GetUploadedFilesize () const;
		void MaterializeFolderPath (std::string const & path)
		{
			_ftpSessionHandle.MaterializeFolderPath (path);
		}
		void StartPutFile (std::string const & sourcePath, std::string const & targetPath);
		void Abort () { _targetFile->Abort (); }

	private:
		Internet::Access			_access;
		Internet::AutoHandle		_internet;
		Ftp::Session				_session;
		Ftp::AutoHandle				_ftpSessionHandle;
		std::unique_ptr<LocalFile>	_sourceFile;
		std::unique_ptr<Ftp::File>	_targetFile;
	};
}

#endif
