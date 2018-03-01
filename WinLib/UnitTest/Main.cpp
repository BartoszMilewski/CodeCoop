//-----------------------------
//  (c) Reliable Software, 2005
//-----------------------------
#include "precompiled.h"
#include "TopCtrl.h"
#include "OutSink.h"
#include <Win/WinMain.h>
#include <Win/WinClass.h>
#include <Win/WinMaker.h>
#include <Win/MsgLoop.h>
#include <Handlers.h>

Out::Sink TheOutput;

int Win::Main (Win::Instance hInst, char const * cmdParam, int cmdShow)
{
	try
	{
		TheOutput.Init ("Unit Test");
		// Create top window class
		Win::Class::TopMaker topWinClass ("UnitTestClass", hInst);
		topWinClass.Register ();

		// Create Window Controller (needs access to MessagePrepro)
		Win::MessagePrepro msgPrepro;
		TopCtrl ctrl;

		// Create top window (hidden)
		Win::TopMaker topWin ("Unit Test", "UnitTestClass", hInst);
		Win::Dow::Handle appWin = topWin.Create (&ctrl);
		appWin.Show (cmdShow);
		// The main message loop
		return msgPrepro.Pump ();
	}
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
	}
	catch (...)
	{
		Win::ClearError ();
		TheOutput.Display ("Internal error: Unknown exception", Out::Error);
	}
	return 0;
}