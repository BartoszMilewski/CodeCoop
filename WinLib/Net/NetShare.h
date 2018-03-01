#if !defined (NETSHARE_H)
#define NETSHARE_H
//---------------------------
// (c) Reliable Software 2000
//---------------------------
#include "NetShareImpl.h"
#include <File/Path.h>

// Usage:
// Net::Share share;
// Net::SharedFolder folder (shareName, localPath);
// share.Add (folder);
// ... or
// share.Delete (shareName);

namespace Net
{
	class Path: public FilePath
	{
	public:
		Path (char const * machineName)
		{
			_buf += "\\\\";
			_buf += machineName;
			_prefix = _buf.length ();
		}
	};

	class LocalPath: public FilePath
	{
	public:
		LocalPath ()
		{
			DWORD len = MAX_COMPUTERNAME_LENGTH + 1;
			_buf.resize (len + 2);
			_buf [0] = '\\';
			_buf [1] = '\\';
			BOOL status = ::GetComputerName (&_buf[2], &len);
			if (status == FALSE)
				throw Win::Exception ("Couldn't retrieve local computer name");
			_buf.resize (len + 2);
			_prefix = _buf.length ();
		}
	};

	class Share
	{
	public:
		Share ();
		void Add (Net::SharedObject const & object);
		void Delete (std::string const & netname);
	private:
		std::unique_ptr<Net::ShareImpl> _impl;
	};
}

#endif
