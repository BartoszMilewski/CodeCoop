//---------------------------
// (c) Reliable Software 2000
//---------------------------

#include <Ctrl/Controls.h>

extern int Main (HINSTANCE hInst, char * cmdParam, int cmdShow);

#if defined (_DEBUG)
#include "MemCheck.h"
static HeapCheck heapCheck;
#endif


int PASCAL WinMain
	(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR cmdParam, int cmdShow)
{
	Win::UseCommonControls ();
	return Main (hInst, cmdParam, cmdShow);
}
