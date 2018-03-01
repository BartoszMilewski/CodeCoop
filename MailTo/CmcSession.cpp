//
// (c) Reliable Software 1998
//

#include "CmcSession.h"
#include "CmcEx.h"
#include "LightString.h"

#include <Sys\RegKey.h>

// HKEY_LOCAL_MACHINE
//      Software
//          Microsoft
//              Windows Messaging Subsystem
class RegKeyCMC
{
public:
    RegKeyCMC ()
       :_keyMain (HKEY_LOCAL_MACHINE, "Software"),
        _keyMicrosoft (_keyMain, "Microsoft"),
        _keyWMS (_keyMicrosoft, "Windows Messaging Subsystem")
    {}

    bool IsAvailable ()
	{
		auto_array<char> cmc = _keyWMS.RetrieveString ("CMC");
		return cmc [0] == '1';
	}

	auto_array<char> RetrieveCMCDllName ()
	{
		return _keyWMS.RetrieveString ("CMCDLLNAME32");
	}

private:
    RegKeyExisting	_keyMain;
    RegKeyExisting	_keyMicrosoft;
    RegKeyExisting	_keyWMS;
};

Session::Session ()
{
	RegKeyCMC regCMC;
	if (!regCMC.IsAvailable ())
		throw WinException ("CMC messaging subsysten is not available on this machine");
	auto_array<char> cmcDllName = regCMC.RetrieveCMCDllName ();
	_CMCLib = ::LoadLibrary (cmcDllName);
	if (_CMCLib == 0)
		throw WinException ("Cannot load CMC library", cmcDllName);

	typedef CMC_return_code (*QueryCfg) (CMC_session_id, CMC_enum term, CMC_buffer buf, CMC_extension * cfgExt);
	typedef CMC_return_code (*Logon) (CMC_string service,
									  CMC_string user,
									  CMC_string password,
									  CMC_object_identifier char_set,
									  CMC_ui_id ui_id,
									  CMC_uint16 caller_cmc_version,
									  CMC_flags logon_flags,
									  CMC_session_id * session,
									  CMC_extension * logon_extension);

	QueryCfg getConfig = reinterpret_cast<QueryCfg>(GetCmcFunction ("cmc_query_configuration"));
	Logon cmcLogon = reinterpret_cast<Logon>(GetCmcFunction ("cmc_logon"));
	CMC_return_code	rCode;
	CMC_boolean		UIAvail;

	rCode = getConfig (0,
					   CMC_CONFIG_UI_AVAIL,
					   reinterpret_cast<void*>(&UIAvail),
					   0);
	if (rCode != CMC_SUCCESS)
		throw CmcException ("Cannot get messaging service configuration details", rCode);

	CMC_flags logonFlags = UIAvail == TRUE ? CMC_LOGON_UI_ALLOWED | CMC_ERROR_UI_ALLOWED : 0;
	rCode = cmcLogon (0,			// Default service
					  0,			// Default user name
					  0,			// Default password
					  0,			// Default character set
					  0,			// Default UI Id
					  CMC_VERSION,	// This CMC version
					  logonFlags,	// If UI available use it when necessary
					  &_session,	// Returned CMC session id
					  0);			// No Microsoft CMC extensions used
	if (rCode != CMC_SUCCESS)
		throw CmcException ("Cannot logon to messaging service", rCode);
};

Session::~Session ()
{
	if (_CMCLib != 0)
	{
		typedef CMC_return_code (*Logoff) (CMC_session_id session,
										   CMC_ui_id ui_id,
										   CMC_flags logoff_flags,
										   CMC_extension * logoff_extension);
		Logoff cmcLogoff = reinterpret_cast<Logoff>(GetCmcFunction ("cmc_logoff"));
		CMC_return_code	rCode;
		rCode = cmcLogoff (_session,
						   0,
						   0,
						   0);
		if (rCode != CMC_SUCCESS)
			throw CmcException ("Cannot logoff from messaging service", rCode);
		::FreeLibrary (_CMCLib);
	}
}

void * Session::GetCmcFunction (char const * funcName) const
{
	void * pFunction = ::GetProcAddress (_CMCLib, funcName);
	if (pFunction == 0)
		throw WinException ("Cannot find function in the CMC library", funcName);
	return pFunction;
}
