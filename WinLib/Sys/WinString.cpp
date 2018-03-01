//
// (c) Reliable Software 1997-2000
//
#include <WinLibBase.h>
#include "WinString.h"

ResString::ResString (Win::Instance hInst, int resId)
	: _buf (64, ' ')
{
	Load (hInst, resId);
}

ResString::ResString (Win::Dow::Handle mainWnd, int resId)
	: _buf (64, ' ')
{
	Load (mainWnd.GetInstance (), resId);
}

void ResString::Load (Win::Instance hInst, int resId)
{
	// Win doc says string table resource string legnth <= 4096
	unsigned int charsCopied = ::LoadString (hInst, resId, &_buf [0], _buf.length ());
    while (charsCopied == _buf.length () - 1)
	{
		_buf.resize (2 * _buf.length ());
		charsCopied = ::LoadString (hInst, resId, &_buf [0], _buf.length ());
	}
	if (charsCopied == 0)
        throw Win::Exception ("Load String failed");
}
