//-----------------------------------
// (c) Reliable Software 2003 -- 2005
//-----------------------------------

#include "precompiled.h"
#include "DbgMonitor.h"
#include "DbgMonitorCtrl.h"
#include "OutputSink.h"
#include "Resource.h"

#include <Win/WinClass.h>
#include <Win/WinMaker.h>
#include <Win/MsgLoop.h>
#include <Ex/WinEx.h>
#include <Dbg/Log.h>


int RunMonitor (HINSTANCE hInst, char * cmdParam, int cmdShow)
{
	int status = 0;
	try
	{
		Win::Class::TopMaker topWinClass (Dbg::Monitor::CLASS_NAME, hInst, IDI_DBGMONITOR);
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

		// Create Window
		Win::TopMaker topWin ("Debug Output Monitor", Dbg::Monitor::CLASS_NAME, hInst);
		Win::MessagePrepro msgPrepro;
		DbgMonitorCtrl dbgMonitorCtrl;

		Win::Dow::Handle appWin = topWin.Create (&dbgMonitorCtrl, "Debug Output Monitor");
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
