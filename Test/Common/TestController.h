#if !defined (TESTCONTROLLER_H)
#define TESTCONTROLLER_H
// (c) Reliable Software 2003
#include "WinOut.h"
#include <Win/Controller.h>
#include <Sys/Timer.h>
#include <Ctrl/Edit.h>

// This is the main test routine
extern int RunTest (WinOut & output);

class TestController: public Win::Controller, public WinOut
{
public:
	TestController () : _timer (0) {}
	bool OnCreate (Win::CreateData const * create, bool & success) throw ();
	bool OnDestroy () throw ()
	{
		Win::Quit ();
		return true;
	}
	bool OnTimer (int id) throw ();
	bool OnSize (int width, int height, int flag) throw ()
	{
		_edit.Move (0, 0, width, height);
		return true;
	}
	// WinOut
	void PutLine (char const * text);
	void PutBoldLine (char const * text);
	void PutMultiLine (std::string const & text);
private:
	Win::Timer _timer;
	Win::Edit  _edit;
};

#endif
