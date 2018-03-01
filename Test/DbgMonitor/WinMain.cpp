//-----------------------------------
// (c) Reliable Software 2003
//-----------------------------------

#include "precompiled.h"
#include "DbgMonitor.h"

#include <Ctrl/Controls.h>


int PASCAL WinMain (HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR cmdParam, int cmdShow)
{
	Win::UseCommonControls ();
	return RunMonitor (hInst, cmdParam, cmdShow);
}
