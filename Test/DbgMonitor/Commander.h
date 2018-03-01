#if !defined (COMMANDER_H)
#define COMMANDER_H
//----------------------------------
// (c) Reliable Software 2003 - 2006
//----------------------------------

#include <Ctrl/Command.h>
#include <Win/Win.h>

class ViewManager;
class RichDumpWindow;

class Commander
{
public:
	Commander (Win::Dow::Handle hwnd, RichDumpWindow & display);

    // Command execs and testers
    void Monitor_Save ();
	Cmd::Status can_Monitor_Save () const { return Cmd::Enabled; }
    void Monitor_ClearAll ();
	Cmd::Status can_Monitor_ClearAll () const { return Cmd::Enabled; }
    void Monitor_Exit ();
	Cmd::Status can_Monitor_Exit () const { return Cmd::Enabled; }

private:
	Win::Dow::Handle	_hwnd;
	RichDumpWindow &	_display;
};

#endif
