#if !defined (TESTCONTROLLER_H)
#define TESTCONTROLLER_H
// (c) Reliable Software 2003
#include "Tester.h"
#include "WinOut.h"
#include <Win/Controller.h>
#include <Win/Timer.h>
#include <Win/Synchro.h>
#include <Win/Edit.h>
#include <deque>

class TestController: public Win::Controller, public WinOut
{
public:
	TestController ();
	bool OnCreate (Win::CreateData const * create, bool & success) throw ();
	bool OnDestroy () throw ()
	{
		Win::Quit ();
		return true;
	}
	bool OnTimer (int id) throw ();
	bool OnSize (int width, int height, int flag) throw ()
	{
		// Resize the edit control
		_edit.Move (0, 0, width, height);
		return true;
	}
	bool OnUserMessage (Win::UserMessage & msg) throw ();
	// WinOut interface
	void PutLine (char const * text);
	void PostLine (char const * text);
private:
	Win::Timer _timer;
	Win::Edit  _edit;

	Win::UserMessage _lineReadyMsg;	// User-defined Windows message
	Win::CritSection _critSect;		// To serialize posting of lines
	std::deque<std::string> _lineQueue;	// Queue of lines to be displayed

	std::auto_ptr<Tester> _tester;
};

#endif
