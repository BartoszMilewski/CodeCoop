//-----------------------------------
// (c) Reliable Software 2001 -- 2005
//-----------------------------------

#include <WinLibBase.h>
#include "GlobalUniqueName.h"

#include <StringOp.h>

GlobalUniqueName::GlobalUniqueName ()
{
	GUID guid;
	if (::CoCreateGuid (&guid) != S_OK)
		throw Win::Exception ("Internal error: Cannot create globaly unique name.");

	Init (guid);
}

GlobalUniqueName::GlobalUniqueName (GUID const & guid)
{
	Init (guid);
}

void GlobalUniqueName::Init (GUID const & guid)
{
	WCHAR buf [64];
	int len = ::StringFromGUID2 (guid, buf, sizeof (buf) / sizeof (WCHAR));
	Assert (len != 0);
	// Convert wide-char string (UNICODE) to single char string
	_name = ToMBString (buf);
}
