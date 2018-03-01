//
// (c) Reliable Software 1998
//

#include "MapiSession.h"
#include "MapiEx.h"

#include <mapix.h>

Session::Session ()
{
	HRESULT hRes = MAPIInitialize (0);
	if (FAILED (hRes))
		throw WinException ("Cannot initialize MAPI subsystem");
	hRes = MAPILogonEx (0,		// [in] Handle to the window to which the logon dialog box is modal.
								// If no dialog box is displayed during the call the parameter is ignored.
								// This parameter can be zero.
						0,		// [in] Pointer to a string containing the name of the profile to use when logging on.
								// This string is limited to 64 characters.
						0,		// [in] Pointer to a string containing the password of the profile.
								// The parameter can be NULL whether or not the lpszProfileName parameter is NULL.
								// This string is limited to 64 characters.
						MAPI_USE_DEFAULT | MAPI_LOGON_UI,
								// Requesting explicit profile without giving its name means default profile.
						&_p);
	if (FAILED (hRes))
		throw MapiException ("Cannot logon using default MAPI profile", hRes);

}

Session::~Session ()
{
	Free ();
	MAPIUninitialize ();
}

