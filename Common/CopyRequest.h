#if !defined (FILECOPYREQUEST_H)
#define FILECOPYREQUEST_H
//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include "FtpSite.h"

#include <Bit.h>

class FileCopyRequest
{
public:

	void SetLocalFolder (std::string const & folder)
	{
		_localFolder = folder;
	}

	void SetMyComputer ()
	{
		_targetType.set (MyComputer, true);
		_targetType.set (LAN, false);
		_targetType.set (Internet, false);
	}
	void SetLAN ()
	{
		_targetType.set (MyComputer, false);
		_targetType.set (LAN, true);
		_targetType.set (Internet, false);
	}
	void SetInternet ()
	{
		_targetType.set (MyComputer, false);
		_targetType.set (LAN, false);
		_targetType.set (Internet, true);
	}
	void SetOverwriteExisting (bool flag)
	{
		_targetType.set (OverwiteExisting, flag);
	}
	void SetUseFolderNames (bool flag)
	{
		_targetType.set (UseFolderNames, flag);
	}
	void Clear ()
	{
		_ftpLogin.Clear ();
		_localFolder.clear ();
		_targetType.init (0);
	}

	std::string GetTargetFolder () const;
	std::string const & GetLocalFolder () const { return _localFolder; }
	Ftp::SmartLogin & GetFtpLogin () { return _ftpLogin; }
	Ftp::SmartLogin const & GetFtpLogin () const { return _ftpLogin; }

	bool IsMyComputer () const { return _targetType.test (MyComputer); }
	bool IsLAN () const { return _targetType.test (LAN); }
	bool IsInternet () const { return _targetType.test (Internet); }
	bool IsOverwriteExisting () const { return _targetType.test (OverwiteExisting); }
	bool IsUseFolderNames () const { return _targetType.test (UseFolderNames); }
	bool IsAnonymousLogin () const { return _ftpLogin.IsAnonymous (); }

	bool IsValid (bool testFtpWrite = false);
	void DisplayErrors (Win::Dow::Handle owner) const;

private:
	enum Options
	{
		MyComputer,			// Store on the local computer
		LAN,				// Store on the local area network
		Internet,			// Store on the internet
		OverwiteExisting,
		UseFolderNames
	};

private:
	Ftp::SmartLogin				_ftpLogin;
	std::unique_ptr<Ftp::Site>	_ftpSite;
	std::string					_localFolder;
	BitSet<Options>				_targetType;	
};

#endif
