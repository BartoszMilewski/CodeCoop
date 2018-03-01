//-----------------------------------
// (c) Reliable Software 2001 -- 2003
//-----------------------------------

#include "precompiled.h"
#include "TestMain.h"

#include <Ctrl/Controls.h>


int PASCAL WinMain
	(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR cmdParam, int cmdShow)
{
	Win::UseCommonControls ();
	return TestMain (hInst, cmdParam, cmdShow);
}

