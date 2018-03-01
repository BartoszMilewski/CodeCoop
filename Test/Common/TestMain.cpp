//-------------------------------------------------
// (c) Reliable Software 2003
//-------------------------------------------------
#include "precompiled.h"

#include "TestController.h"
#include <Win/WinMain.h>
#include <Win/WinClass.h>
#include <Win/WinMaker.h>
#include <Win/MsgLoop.h>

#include <Ex/WinEx.h>
#include <Dbg/Memory.h>

int Win::Main (Win::Instance hInst, char const * cmdParam, int cmdShow)
{
	TheOutput.Init ("Test");
	Win::Class::Maker winClass ("testwin", hInst);
	winClass.Register ();
	Win::TopMaker winMaker ("Test Window", "testwin", hInst);
	TestController testCtrl;
	Win::Dow::Handle win = winMaker.Create (&testCtrl, "Test App");
	win.Show (cmdShow);
	Win::MessagePrepro msgPrepro;
	msgPrepro.Pump ();
	return 0;
}