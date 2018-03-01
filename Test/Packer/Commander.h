#if !defined (COMMANDER_H)
#define COMMANDER_H
//-----------------------------------------
// (c) Reliable Software 2001
//-----------------------------------------

#include <Ctrl/Command.h>
#include <Win/Win.h>

class Model;
class ViewManager;

namespace Win
{
	class MessagePrepro;
}

class Commander
{
public:
	Commander (Model & model,
			   Win::MessagePrepro * msgPrepro,
			   Win::Dow::Handle hwnd);

    // Command execs and testers
    void Test_Run ();
	Cmd::Status can_Test_Run () const { return Cmd::Enabled; }
    void Test_Exit ();
	Cmd::Status can_Test_Exit () const { return Cmd::Enabled; }

private:
	Model &				_model;
	ViewManager *		_displayMan;
	Win::MessagePrepro *_msgPrepro;
	Win::Dow::Handle	_hwnd;	// Needed by progress meter
};

#endif
