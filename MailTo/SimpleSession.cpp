//
// (c) Reliable Software 1998
//

#include "SimpleSession.h"
#include "MapiEx.h"

Session::Session ()
{
	_SimpleMapiLib = ::LoadLibrary ("mapi32.dll");
	if (_SimpleMapiLib == 0)
		throw Win::Exception ("Cannot load Simple MAPI library", "mapi32.dll");

	typedef ULONG (FAR PASCAL *Logon) (ULONG ulUIParam,
									   LPTSTR lpszProfileName,
									   LPTSTR lpszPassword,
									   FLAGS flFlags,
									   ULONG ulReserved,
									   LPLHANDLE lplhSession);
	
	Logon logon = reinterpret_cast<Logon>(GetFunction ("MAPILogon"));
	ULONG rCode = logon (0,			// Handle to window displaying Simple MAPI dialogs
						 0,			// Use default profile
						 0,			// No password
						 0,			// No flags
						 0,			// Reserved -- must be zero
						 &_session);// Session handle
	if (rCode != SUCCESS_SUCCESS)
		throw MapiException ("Cannot logon to Simple MAPI", rCode);
};

Session::~Session ()
{
	if (_SimpleMapiLib != 0)
	{
		typedef ULONG (FAR PASCAL *Logoff) (LHANDLE session,
										    ULONG ulUIParam,
										    FLAGS flFlags,
										    ULONG reserved);
		Logoff logoff = reinterpret_cast<Logoff>(GetFunction ("MAPILogoff"));
		ULONG rCode = logoff (_session,
							  0,
							  0,
							  0);
		if (rCode != SUCCESS_SUCCESS)
			throw MapiException ("Cannot logoff from messaging service", rCode);
		::FreeLibrary (_SimpleMapiLib);
	}
}

void * Session::GetFunction (char const * funcName) const
{
	void * pFunction = ::GetProcAddress (_SimpleMapiLib, funcName);
	if (pFunction == 0)
		throw Win::Exception ("Cannot find function in the Simple MAPI library", funcName);
	return pFunction;
}
