//---------------------------
// (c) Reliable Software 2000
//---------------------------
#include "precompiled.h"
#include "SetupMain.h"

#include "OutputSink.h"
#include <Sys/SysVer.h>
#include <Ctrl/Controls.h>

int PASCAL WinMain (HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR cmdParam, int cmdShow)
{
	Win::UseCommonControls ();
	SystemVersion osVer;
	if (osVer.IsOK ())
	{
		bool supportedVersion = osVer.IsWin32Windows () || (osVer.IsWinNT () && osVer.MajorVer () >= 4);
		if (!supportedVersion)
		{
			TheOutput.Display ("Unsupported Windows version", Out::Error);
			return 0;
		}
	}
	else
		return 0;	// If we can't get system version can we run ?
	return SetupMain (hInst, cmdParam, cmdShow);
}

