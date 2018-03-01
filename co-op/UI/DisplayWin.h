#if !defined (DSIPLAYWIN_H)
#define DISPLAYWIN_H
//------------------------------------------------
// DisplayWin.h
// (c) Reliable Software 2002
// -----------------------------------------------

#include "RichDumpWin.h"

#include <Win/Win.h>

class DumpWindow;

class DisplayWindow
{
public:
	DisplayWindow (char const * caption, Win::Dow::Handle appWnd);

	void SetPlacement (Win::Placement const & placement) { _frame.SetPlacement (placement); }
	void Show () { _frame.Show (); }
	DumpWindow & GetDumpWindow () { return *_dumpWin; }
	operator Win::Dow::Handle () { return _frame; }

private:
	Win::Dow::Handle	_frame;		// Frame window controlling display pane
	DumpWindow *		_dumpWin;
};

#endif
