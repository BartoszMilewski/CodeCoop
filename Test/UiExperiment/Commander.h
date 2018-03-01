#if !defined (COMMANDER_H)
#define COMMANDER_H
//----------------------------------
// (c) Reliable Software 2007-09
//----------------------------------

#include <Ctrl/Command.h>
#include <Win/Win.h>

class Commander
{
public:
	Commander (Win::Dow::Handle hwnd);

    // Command execs and testers
    void Browser_Exit ();

private:
	Win::Dow::Handle	_hwnd;
};

#endif
