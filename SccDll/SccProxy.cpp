//------------------------------------
//  (c) Reliable Software, 2000 - 2008
//------------------------------------

#include "precompiled.h"
#include "SccProxy.h"
#include "SccDllFunctions.h"
#include "Global.h"

namespace CodeCoop
{
	class RegistryKey
	{
	public:
		RegistryKey (std::string const & subKeyPath, HKEY hRootKey = HKEY_LOCAL_MACHINE);
		~RegistryKey ();

		bool Exists () const { return _exists; }
		std::string GetStringVal (std::string const & valueName) const;

	private:
		HKEY	_hKey;
		bool	_exists;
	};
}

using namespace CodeCoop;

RegistryKey::RegistryKey (std::string const & subKeyPath, HKEY hRootKey)
	: _hKey (0),
	  _exists (false)
{
	long err = ::RegOpenKeyEx (hRootKey,
							   subKeyPath.c_str (),
							   0,
							   KEY_READ,
							   &_hKey);
	_exists = (err == ERROR_SUCCESS);
}

RegistryKey::~RegistryKey ()
{
	if (_hKey != 0)
		::RegCloseKey (_hKey);
	_exists = false;
}

std::string RegistryKey::GetStringVal (std::string const & valueName) const
{
	if (!_exists)
		return std::string ();

	unsigned long bufSize = 0;
	unsigned long type;
	long err = ::RegQueryValueEx (_hKey,
								  valueName.c_str (),
								  0, // reserved
								  &type,
								  0, // no buffer, query size only
								  &bufSize);

	if (err == ERROR_FILE_NOT_FOUND) // Value not found
		return std::string ();
	if (err != ERROR_SUCCESS)
		throw Win::Exception ("Registry operation failed. Cannot query key.");
	if (type != REG_SZ)
		throw Win::Exception ("Registry operation failed. Wrong type of key.");

	std::string buf;
	// bufSize includes the terminating null!
	buf.resize (bufSize);
	if (bufSize != 0)
	{
		err = ::RegQueryValueEx (_hKey,
								 valueName.c_str (),
								 0,
								 &type,
								 reinterpret_cast<unsigned char *> (&buf [0]),
								 &bufSize);
		if (err != ERROR_SUCCESS)
			throw Win::Exception ("Registry operation failed. Cannot retrieve key value."); 
		buf.resize (strlen (&buf [0]));
	}
	return buf;
}

Dll::Dll ()
	: _hDll (0)
{
	std::string dllPath;
	std::string	providerKey ("Software\\Reliable Software\\Code Co-op");
	// Code Co-op is not a registered source code control provider
	// Look at Code Co-op registry entries
	RegistryKey machineKey (providerKey);
	if (machineKey.Exists ())
	{
		dllPath = machineKey.GetStringVal (STR_SCCPROVIDERPATH);
	}
	else
	{
		// HKEY_LOCAL_MACHINE doesn't have Code Co-op installation key -- try HKEY_CURRENT_USER
		RegistryKey userKey (providerKey, HKEY_CURRENT_USER);
		if (userKey.Exists ())
			dllPath = userKey.GetStringVal (STR_SCCPROVIDERPATH);
		else
			throw Win::InternalException ("Code Co-op not installed. Run installation again.");
	}

	if (dllPath.empty ())
		throw Win::InternalException ("Cannot locate Code Co-op dynamic link library.");

	_hDll = ::LoadLibrary (dllPath.c_str ());
	if (_hDll == 0)
		throw Win::Exception ("Cannot load Code Co-op dynamic link library.", dllPath.c_str ());
}

Dll::~Dll ()
{
	if (_hDll != 0)
	{
		::FreeLibrary (_hDll);
	}
}

FunPtr Dll::GetFunction (std::string const & funcName) const
{
	FunPtr pFunction = reinterpret_cast<FunPtr> (::GetProcAddress (_hDll, funcName.c_str ()));
	if (pFunction == 0)
		throw Win::InternalException ("Cannot find function in the SccDll.dll", funcName.c_str ());

	return pFunction;
}

Proxy::Proxy (TextOutProc textOutCallback)
	: _context (0)
{
	SccGetCoopVersionPtr getCoopVersion;
	_dll.GetFunction ("SccGetCoopVersion", getCoopVersion);
	long sccCoopVersion = getCoopVersion ();
	if (sccCoopVersion != TheCurrentMajorVersion)
	{
		throw Win::InternalException ("Command line applets and Code Co-op version mismatch.\n"
				"Please, download appropriate command line applets from ftp://ftp.relisoft.com");
	}

	char providerName [128];
	char auxPath [512];
	long providerCapabilities = 0;
	long checkoutCommentLength = 0;
	long otherCommentLength = 0;
	SccInitializePtr initialize;
	_dll.GetFunction ("SccInitialize", initialize);
	SCCRTN result = initialize (&_context,
								0,			// No parent window
								"Code Co-op Proxy",
								providerName,
								&providerCapabilities,
								auxPath,
								&checkoutCommentLength,
								&otherCommentLength);
	if (result != SCC_OK)
		return;

	SccOpenProjectPtr openProject;
	_dll.GetFunction ("SccOpenProject", openProject);
	openProject (_context,
				 0,				// No parent window
				 0,				// No user
				 0,				// No project name
				 "dummy",		// No local project path
				 0,				// No aux project path
				 0,				// No open comment
				 textOutCallback,// Reporting callback
				 SCC_OP_SILENTOPEN);
}

Proxy::~Proxy ()
{
	SccCloseProjectPtr closeProject;
	_dll.GetFunction ("SccCloseProject", closeProject);
	closeProject (_context);

	SccUninitializePtr uninitialize;
	_dll.GetFunction ("SccUninitialize", uninitialize);
	uninitialize (_context);
}

bool Proxy::StartCodeCoop ()
{
	SccRunSccPtr run;
	_dll.GetFunction ("SccRunScc", run);
	SCCRTN result = (*run) (_context,	// SCC context
							0,		// No IDE hwnd
							0,		// No files
							0);		// No file paths
	return result == SCC_OK;
}

bool Proxy::CheckOut (int fileCount, char const ** paths, SccOptions cmdOptions)
{
	SccCheckoutPtr checkout;
	_dll.GetFunction ("SccCheckout", checkout);
	SCCRTN result = (*checkout) (_context,		// SCC context
								 0,				// No IDE hwnd
								 fileCount,		// Number of checked out files
								 paths,			// File paths
								 0,				// No checkout comment
								 0,				// No command flags
								 reinterpret_cast<void *>(cmdOptions.GetValue ()));	// Code Co-op options
	return result == SCC_OK;
}

bool Proxy::UncheckOut (int fileCount, char const ** paths, SccOptions cmdOptions)
{
	SccUncheckoutPtr uncheckout;
	_dll.GetFunction ("SccUncheckout", uncheckout);
	SCCRTN result = (*uncheckout) (_context,	// SCC context
								   0,			// No IDE hwnd
								   fileCount,	// File count
								   paths,		// File paths
								   0,			// No command flags
								   reinterpret_cast<void *>(cmdOptions.GetValue ()));	// Code Co-op options
	return result == SCC_OK;
}

bool Proxy::CheckIn (int fileCount, char const ** paths, char const * comment, SccOptions cmdOptions)
{
	SccCheckinPtr checkin;
	_dll.GetFunction ("SccCheckin", checkin);
	SCCRTN result = (*checkin) (_context,		// SCC context
								0,				// No IDE hwnd
								fileCount,		// File count
								paths,			// File paths
								comment,		// Checkin comment
								0,				// Command flags
								reinterpret_cast<void *>(cmdOptions.GetValue ()));	// Code Co-op options
	return result == SCC_OK;
}

bool Proxy::AddFile (int fileCount, char const ** paths, std::string const & comment, SccOptions cmdOptions)
{
	SccAddPtr addFile;
	_dll.GetFunction ("SccAdd", addFile);
	SCCRTN result = (*addFile) (_context,	// SCC context
								0,			// No IDE hwnd
								fileCount,	// File count
								paths,		// File paths
								comment.c_str (),	// Check-in comment
								0,			// No command flags
								reinterpret_cast<void *>(cmdOptions.GetValue ()));	// Code Co-op options
	return result == SCC_OK;
}

bool Proxy::RemoveFile (int fileCount, char const ** paths, SccOptions cmdOptions)
{
	SccRemovePtr removeFile;
	_dll.GetFunction ("SccRemove", removeFile);
	SCCRTN result = (*removeFile) (_context,	// SCC context
								   0,			// No IDE hwnd
								   fileCount,	// File count
								   paths,		// File paths
								   0,			// No remove comment
								   0,			// No command flags
								   reinterpret_cast<void *>(cmdOptions.GetValue ()));	// Code Co-op options
	return result == SCC_OK;
}

bool Proxy::Status (int fileCount, char const ** paths, long * status)
{
	SccQueryInfoPtr queryInfo;
	_dll.GetFunction ("SccQueryInfo", queryInfo);
	SCCRTN result = (*queryInfo) (_context,		// SCC context
								  fileCount,	// File count
								  paths,		// File paths
								  status);		// Status vector
	return result == SCC_OK;
}

// Returns true when SCC call was successful
bool Proxy::IsControlledFile (char const * path, bool & isControlled, bool & isCheckedout)
{
	isCheckedout = false;
	isControlled = false;
	long sccStatus = -1;
	if (Status (1, &path, &sccStatus))
	{
		if (sccStatus != -1)
		{
			if ((sccStatus & SCC_STATUS_CONTROLLED) != 0)
			{
				isControlled = true;
				isCheckedout = (sccStatus & SCC_STATUS_CHECKEDOUT) != 0;
			}
			return true;
		}
	}
	return false;
}
