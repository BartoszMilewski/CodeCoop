// (c) Reliable Software 2002
#include <Win/WinMain.h>

// Windows entry point: calls clent entry point

int WINAPI WinMain
	(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR cmdParam, int cmdShow)
{
	return Win::Main (hInst, cmdParam, cmdShow);
}

