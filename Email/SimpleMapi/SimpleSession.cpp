//----------------------------------
// (c) Reliable Software 1998 - 2009
//----------------------------------

#include "precompiled.h"
#include "SimpleSession.h"
#include "EmailRegistry.h"
#include "SimpleMapiEx.h"

#include <Ex/WinEx.h>
#include <File/File.h>
#include <File/Path.h>

using namespace SimpleMapi;

Session::Session ()
{
	Registry::DefaultEmailClient defaultClient;
	std::string defaultClientName = defaultClient.GetName ();
	Registry::EmailClientCheck defaultClientKeyCheck (defaultClientName);
	if (defaultClientKeyCheck.Exists ())
	{
		Registry::EmailClient defaultClientKey (defaultClientName);
		FilePath dllPath (defaultClientKey.GetDllPath ());
		if (!dllPath.IsDirStrEmpty ())
		{
			dllPath.ExpandEnvironmentStrings ();
			if (File::Exists (dllPath.GetDir ()))
				_simpleMapiDll.reset (new Dll (dllPath.GetDir ()));
		}
	}

	if (_simpleMapiDll.get () == 0)
		_simpleMapiDll.reset (new Dll ("mapi32.dll"));

	typedef ULONG (FAR PASCAL *Logon) (ULONG ulUIParam,
									   LPTSTR lpszProfileName,
									   LPTSTR lpszPassword,
									   FLAGS flFlags,
									   ULONG ulReserved,
									   LPLHANDLE lplhSession);

	Logon logon;
	_simpleMapiDll->GetFunction ("MAPILogon", logon);
	ULONG rCode = logon (0,			// Handle to window displaying Simple MAPI dialogs
						 0,			// Use default profile
						 0,			// No password
						 0,			// No flags
						 0,			// Reserved -- must be zero
						 &_session);// Session handle
	if (rCode != SUCCESS_SUCCESS)
	{
		dbg << "Trying to log on in a new session" << std::endl;
		// Logon with default profile failed.
		// Try again displaying logon dialog.
		rCode = logon (0,			// Handle to window displaying Simple MAPI dialogs
					   0,			// Use default profile
					   0,			// No password
					   MAPI_NEW_SESSION |
									// Create New session
					   MAPI_LOGON_UI,
									// Display logon dialog
					   0,			// Reserved -- must be zero
					   &_session);	// Session handle
		if (rCode != SUCCESS_SUCCESS)
			throw SimpleMapi::Exception ("MAPI -- Cannot logon", rCode);
	}
};

Session::~Session ()
{
	typedef ULONG (FAR PASCAL *Logoff) (LHANDLE session,
										ULONG ulUIParam,
										FLAGS flFlags,
										ULONG reserved);
	try
	{
		Logoff logoff;
		_simpleMapiDll->GetFunction ("MAPILogoff", logoff);
		logoff (_session, 0, 0, 0);
	}
	catch ( ... )
	{
		// Ignore all exceptions
		Win::ClearError ();
	}
}
