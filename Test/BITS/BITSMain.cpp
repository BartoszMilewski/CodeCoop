//-------------------------------------------------
// (c) Reliable Software 2003
//-------------------------------------------------
#include "precompiled.h"
#include "TestController.h"

#include <Win/WinMain.h>
#include <Win/WinClass.h>
#include <Win/WinMaker.h>
#include <Win/MsgLoop.h>
#include <File/ErrorLog.h>

#include <Ex/WinEx.h>
#include <Dbg/Memory.h>

#include <Handlers.h>

//	Misc declarations

extern Out::Sink TheOutput;

void UnexpectedHandler ();

int Win::Main (Win::Instance hInst, char const * cmdParam, int cmdShow)
{
	TheOutput.Init ("BITS Test");

	Win::Class::Maker winClass ("bitstest", hInst);
	winClass.Register ();
	Win::TopMaker winMaker ("BITS Test Window", "bitstest", hInst);
	TestController testCtrl;
	Win::Dow::Handle win = winMaker.Create (&testCtrl, "BITS Test");
	win.Show (cmdShow);
	Win::MessagePrepro msgPrepro;
	msgPrepro.Pump ();
	return 0;
}

void UnexpectedHandler ()
{
	TheErrorLog << "Unexpected system exception occurred during program execution." << std::endl;
	TheErrorLog << "The program will now exit." << std::endl;
	TheOutput.Display ("Unexpected system exception occurred during program execution.\n"
					   "The program will now exit.");
	terminate ();
}
