//----------------------------------
// (c) Reliable Software 2003 - 2008
//----------------------------------

#include <WinLibBase.h>

#include "Ftp.h"
#include <File/File.h>
#include <File/Vpath.h>
#include <Ex/Error.h>

namespace Ftp
{
	bool Handle::GetFile (	char const * remotePath,
							char const * localPath, 
							::File::Attributes attr,
							Internet::Callback * callback)
	{
		bool success = ::FtpGetFile (ToNative (), 
							remotePath, 
							localPath, 
							FALSE, // fail if exists
							attr.ToNative (),
							FTP_TRANSFER_TYPE_BINARY | INTERNET_FLAG_HYPERLINK,
							reinterpret_cast<DWORD_PTR> (callback)) != FALSE;
		if (!success && callback != 0 && Win::GetError () == ERROR_IO_PENDING)
		{
			Win::ClearError ();
			success = true;
		}
		return success;
	}

	bool Handle::PutFile (	char const * localPath,
							char const * remotePath,
							Internet::Callback * callback)
	{
		bool success =  ::FtpPutFile (ToNative (), 
							localPath, 
							remotePath, 
							FTP_TRANSFER_TYPE_BINARY | INTERNET_FLAG_HYPERLINK,
							reinterpret_cast<DWORD_PTR> (callback)) != FALSE;

		if (!success && callback != 0 && Win::GetError () == ERROR_IO_PENDING)
		{
			Win::ClearError ();
			success = true;
		}
		return success;
	}

	std::string Handle::GetCurrentDirectory ()
	{
		std::string buf;
		buf.reserve (MAX_PATH);
		unsigned long bufLen = buf.capacity ();
		if (::FtpGetCurrentDirectory (ToNative (), writable_string (buf), &bufLen) == FALSE)
			buf.clear ();
		return buf;
	}

	bool Handle::Exists (std::string const & path, bool & isFolder) const
	{
		bool exists = true;
		try
		{
			Ftp::FileSeq seq (*this, path);
			if (seq.AtEnd ())
				return false;
			isFolder = seq.IsFolder ();
		}
		catch (...)
		{
			exists = false;
		}
		return exists;
	}

	class CWDUser
	{
	public:
		CWDUser (Ftp::Handle hFtp)
			: _hFtp (hFtp)
		{
			// Remember current directory on the server
			_currentWorkingDirectory = _hFtp.GetCurrentDirectory ();
			if (_currentWorkingDirectory.empty ())
			{
				LastSysErr lastSysErr;
				if (lastSysErr.IsInternetError ())
				{
					LastInternetError lastInternetError;
					throw Win::InternalException ("Cannot retrieve the current directory on the specified FTP sever.",
												  lastInternetError.Text ().c_str ());
				}
				else
				{
					throw Win::Exception ("Cannot retrieve the current directory on the specified FTP sever.");
				}
			}
		}
		~CWDUser ()
		{
			// Return server to its previous current directory
			_hFtp.SetCurrentDirectory (_currentWorkingDirectory.c_str ());
		}

	private:
		std::string _currentWorkingDirectory;
		Ftp::Handle	_hFtp;
	};

	::File::Size Handle::GetFileSize () const
	{
		unsigned long high, low;
		low = ::FtpGetFileSize (ToNative (), &high);
		return ::File::Size (low, high);
	}

	void Handle::MaterializeFolderPath (std::string const & path)
	{
		CWDUser cwd (*this);
		::File::Vpath workPath(::File::Vpath::UseFwdSlash);
		if (path.empty() || path [0] == '/')
		{
			if (!SetCurrentDirectory ("/"))
			{
				LastSysErr lastSysErr;
				if (lastSysErr.IsInternetError ())
				{
					LastInternetError lastInternetError;
					throw Win::InternalException ("Cannot change the current directory on the specified FTP sever.",
												  lastInternetError.Text ().c_str ());
				}
				else
				{
					throw Win::Exception ("Cannot change the current directory on the specified FTP sever.");
				}
			}
		}

		if (!path.empty())
		{
			if (path [0] == '/')
			{
				if (path.length() > 1)
					workPath.SetPath (&path [1]);
			}
			else
			{
				workPath.SetPath (path);
			}
		}

		for (Vpath::const_iterator iter = workPath.begin (); iter != workPath.end (); ++iter)
		{
			std::string const & segment = *iter;
			if (SetCurrentDirectory (segment.c_str ()))
			{
				// Changed current directory successfully
				continue;
			}
			else
			{
				// Cannot change current directory on the server
				if (CreateDirectory (segment.c_str ()))
				{
					// Created folder on the server
					if (SetCurrentDirectory (segment.c_str ()))
						continue;	// Changed current directory to the newly created folder
				}

				LastSysErr lastSysErr;
				if (lastSysErr.IsInternetError ())
				{
					LastInternetError lastInternetError;
					throw Win::InternalException ("Cannot create folder path on the specified FTP sever.",
												  lastInternetError.Text ().c_str ());
				}
				else
				{
					throw Win::Exception ("Cannot create folder path on the specified FTP sever.", path.c_str ());
				}
			}
		}
		// Done - the current directory on the server is set to the materialized path
		// Destructor of CWDUser will bring server to its previous current directory.
	}

	Ftp::AutoHandle Session::Connect (Internet::Protocol protocol, 
									  Internet::Callback * callback)
	{
		INTERNET_PORT port;
		DWORD service;
		switch (protocol)
		{
		case Internet::FTP:
			port = INTERNET_DEFAULT_FTP_PORT;
			service = INTERNET_SERVICE_FTP;
			break;
		case Internet::HTTP:
			port = INTERNET_DEFAULT_HTTP_PORT;
			service = INTERNET_SERVICE_HTTP;
			break;
		case Internet::GOPHER:
			port = INTERNET_DEFAULT_GOPHER_PORT;
			service = INTERNET_SERVICE_GOPHER;
			break;
        default:
            throw Win::Exception("Unknown internet protocol.");
		};

		Ftp::AutoHandle h (::InternetConnect (  _internetHandle.ToNative (),
												_server.c_str (),
												port,
												_user.c_str (),
												_passwd.c_str (),
												service,
												0,
												reinterpret_cast<DWORD_PTR> (callback)));
		if (!h.IsNull())
			return h;

		if (callback != 0 && Win::GetError() == ERROR_IO_PENDING)
			return h; // empty

		// Real error
		unsigned long error = 0;
		char buffer[256];
		unsigned long len = sizeof(buffer) - 1;
		::InternetGetLastResponseInfo(&error, buffer, &len);
		throw Internet::Exception("Cannot connect to FTP server", buffer, error);
	}

	// Couldn't figure out how to work it asynchronously
	FileSeq::FileSeq (Ftp::Handle hFtp, std::string const & pattern, Internet::Callback * callback)
		: _atEnd (false),
		  _callback (callback),
		  _handle (::FtpFindFirstFile ( hFtp.ToNative (),
										pattern.c_str (),
										&_data,
										0,
										reinterpret_cast<DWORD_PTR> (callback)))
	{
		Assert (callback == 0); // not tested
		if (_handle == INVALID_HANDLE_VALUE || _handle.IsNull ())
		{
			LastSysErr lastSysErr;
			if (_callback != 0 && lastSysErr.IsIOPending ())
				return;

			if (lastSysErr.IsFileNotFound () || lastSysErr.IsNoMoreFiles ())
			{
				_atEnd = true;
				Win::ClearError ();
			}
			else if (lastSysErr.IsInternetError ())
			{
				LastInternetError lastError;
				throw Win::Exception ("FTP: Cannot list files.", lastError.Text ().c_str ());
			}
			else
			{
				throw Internet::Exception ("FTP: Cannot list files.", pattern.c_str ());
			}
		}
		SkipPseudoDirs ();
	}

	void FileSeq::SkipPseudoDirs ()
	{
		// skip "." and ".." directories
		while (!AtEnd () && IsFolder () && IsDots ())
			Advance ();
	}

	void FileSeq::Advance ()
	{
		_atEnd = !::InternetFindNextFile (_handle.ToNative (), &_data);
		if (_atEnd)
			Win::ClearError ();
	}

	char const * FileSeq::GetName () const
	{
		Assert (!_atEnd);
		return _data.cFileName;
	}

	RemoteFile::RemoteFile (Ftp::Handle hFtp,
							char const * path,
							Access access,
							Internet::Callback * callback)
		: _h (::FtpOpenFile (hFtp.ToNative (), 
							 path, 
							 access,
							 FTP_TRANSFER_TYPE_BINARY | INTERNET_FLAG_HYPERLINK, 
							 reinterpret_cast<DWORD_PTR> (callback)))
	{
		Assert (callback == 0); // not tested
		if (_h.IsNull ())
		{
			LastSysErr lastSysErr;
			if (lastSysErr.IsInternetError ())
			{
				LastInternetError lastError;
				throw Win::Exception ("Cannot open file.", lastError.Text ().c_str ());
			}
			else
			{
				throw Internet::Exception ("Cannot open file.", path);
			}
		}
		// Note: only one file can be opened in an FTP session
	}

	FileReadable::FileReadable (Ftp::Handle ftpSessionHandle, char const * path, Internet::Callback * callback)
		: Ftp::RemoteFile (ftpSessionHandle, path, Ftp::RemoteFile::GenericRead, callback)
	{}

	FileWritable::FileWritable (Ftp::Handle ftpSessionHandle, char const * path, Internet::Callback * callback)
		: Ftp::RemoteFile (ftpSessionHandle, path, Ftp::RemoteFile::GenericWrite, callback)
	{}

	File::File (Ftp::Handle ftpSessionHandle, std::string const & path)
		: _ftpSessionHandle (ftpSessionHandle),
		  _path (path)
	{
	}

	void File::OpenRead ()
	{
		_file.reset (new Ftp::FileReadable (_ftpSessionHandle, _path.c_str ()));
	}

	void File::OpenWrite ()
	{
		_file.reset (new Ftp::FileWritable (_ftpSessionHandle, _path.c_str ()));
	}

	::File::Size File::GetSize ()
	{
		Assume (_file.get () == 0, "Ftp::File::GetSize called during transfer");
		Ftp::FileReadable file (_ftpSessionHandle, _path.c_str ());
		return file.GetSize ();
	}

	void File::Commit ()
	{
		_file.reset (); // close file
	}

	void File::Abort ()
	{
		_file.reset (); // close file
		_ftpSessionHandle.DeleteFile (_path.c_str ());
	}
}
