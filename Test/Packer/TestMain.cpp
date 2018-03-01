//---------------------------------------------
// TestMain.cpp
// (c) Reliable Software 2001 -- 2003
//---------------------------------------------

#include "precompiled.h"
#include "TestMain.h"
#include "TestCtrl.h"
#include "OutputSink.h"
#include "Global.h"
#include "Resource.h"

#include <Win/WinClass.h>
#include <Win/WinMaker.h>
#include <Win/MsgLoop.h>
#include <Ex/WinEx.h>

#if defined (_DEBUG)
#include "MemCheck.h"
static HeapCheck heapCheck;
#endif

void NewHandler ();
void UnexpectedHandler ();

int TestMain (HINSTANCE hInst, char * cmdParam, int cmdShow)
{
	TheOutput.Init ("Packer Test");
	int status = 0;
	bool ignoreOtherInstances = false;
	try
	{
		Win::Class::TopMaker topWinClass ("PackerTest", hInst, IDI_PACKER_TEST);
		// Is there a running instance of this program?
		Win::Dow::Handle winOther = topWinClass.GetRunningWindow ();
		if (winOther != 0)
		{
			// Restore previous instance window
			winOther.Restore ();
			winOther.SetForeground ();
			return 0;
		}
		topWinClass.Register ();

		// List Window class
		Win::Class::TopMaker winClassText (ListWindowClassName, hInst, IDI_PACKER_TEST);
		winClassText.Register ();

		// Create Window
		Win::TopMaker topWin ("Reliable Software Packer Test Utility", "PackerTest", hInst);
		Win::MessagePrepro msgPrepro;
		TestController testCtrl (&msgPrepro);

		Win::Dow::Handle appWin = topWin.Create (&testCtrl, "Reliable Software Packer Test Utility");
		appWin.Display (cmdShow);
		// The main message loop
		status = msgPrepro.Pump ();
	}
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
	}
	catch (char const * msg)
	{
		TheOutput.Display (msg, Out::Error);
	}
	catch (...)
	{
		Win::ClearError ();
		TheOutput.Display ("Internal error: Unknown exception", Out::Error);
	}
	return status;
}
