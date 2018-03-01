#if !defined (WINMAIN_H)
#define WINMAIN_H
// (c) Reliable Software 2002
#include <Win/Instance.h>

namespace Win
{
	// Client must implement this entry point in every Windows program
	Main (Win::Instance inst, char const * cmdParam, int cmdShow);
}

#endif
