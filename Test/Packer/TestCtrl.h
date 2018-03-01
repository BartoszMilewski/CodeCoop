#if !defined (TESTCTRL_H)
#define TESTCTRL_H
//---------------------------------------------------
//	TestCtrl.h
//	(c) Reliable Software, 2001
//---------------------------------------------------

#include "CmdVector.h"

#include <Win/Controller.h>
#include <Com/Shell.h>

#include <memory>

namespace Win
{
	class MessagePrepro;
}

class Model;
class ViewManager;
class Commander;

class TestController : public Win::Controller
{
public:
	TestController (Win::MessagePrepro * msgPrepro);
	~TestController ();

	// Win::Controller
	bool OnCreate (Win::CreateData const * create, bool & success) throw ();
	bool OnDestroy () throw ();
	bool OnFocus (Win::Dow::Handle winPrev) throw ();
	bool OnSize (int width, int height, int flag) throw ();
	bool OnCommand (int cmdId, bool isAccel) throw ();
	bool OnMenuSelect (int id, Menu::State state, Menu::Handle menu) throw ();
	bool OnWheelMouse (int zDelta) throw ();
	bool OnUserMessage (Win::UserMessage & msg) throw ();
	bool OnRegisteredMessage (Win::Message & msg) throw ();

private:
	// Helpers
	bool Initialize ();
	void MenuCommand (int cmdId);
private:
	Com::Use					_comUser;			// Must be first
	std::auto_ptr<Model>		_model;

	// User Interface
	std::auto_ptr<Commander>	_commander;
	std::auto_ptr<CmdVector>	_cmdVector;
	std::auto_ptr<ViewManager>	_viewMan;
	Win::MessagePrepro *		_msgPrepro; 	// Code Co-op message pomp
};

#endif
