//----------------------------------
// (c) Reliable Software 1999 - 2008
//----------------------------------

#include "precompiled.h"
#include "DispatcherCtrl.h"
#include "AppInfo.h"
#include "DispatcherParams.h"
#include "Registry.h"
#include "PathRegistry.h"
#include "OutputSink.h"
#include "resource.h"
#include "Global.h"
#include "DispatcherMsg.h"
#include "InputSource.h"
#include "ProjectMarker.h"

#include <Win/WinMain.h>
#include <Ctrl/Controls.h>
#include <Com/Shell.h>
#include <Win/WinClass.h>
#include <Win/WinMaker.h>
#include <Win/MsgLoop.h>
#include <Ex/WinEx.h>
#include <Handlers.h>
#include <time.h>

#if defined (_DEBUG)
#include "MemCheck.h"
static HeapCheck heapCheck;
#endif

int MainCmd (CmdInputSource & inputSource, Win::Instance hInst);
bool PostCmdLineMsg (Win::Dow::Handle targetWin, CmdInputSource const & cmdLineArgs);

void UnexpectedHandler ();

int Win::Main (Win::Instance hInst, char const * cmdParam, int cmdShow)
{
    try
    {
		Com::MainUse comUser; // must be first!
		Win::UseCommonControls ();
		UnexpectedHandlerSwitch unexpHandler (UnexpectedHandler);
		//SET::EnableExceptionTranslation ();
		// Initialize random number generator
		srand (static_cast<unsigned int>(time (0)));

		TheOutput.Init (DispatcherTitle, Registry::GetLogsFolder ());
		TheOutput.ForceForeground ();
		BackupMarker backupMarker;
		if (backupMarker.Exists ())
		{
			TheOutput.Display ("Cannot start Code Co-op Dispatcher, because the backup is in-progress.\n"
							   "Finish the backup and try starting Code Co-op Dispatcher again.");
			return 0;
		}

		Win::Mutex dispatcherMutex ("Global\\CodeCoopDispatcherMutex");
		Win::MessagePrepro msgPrepro;
		DispatcherController ctrl (&msgPrepro);
		AppInformation::Initializer appInfoInit;

		{
			// Dispatcher mutex lock scope
			Win::MutexLock lock (dispatcherMutex);

			CmdInputSource inputSource (cmdParam);
			Win::Class::TopMaker topWinClass (DispatcherClassName, hInst, ID_MAIN);

			// Note: don't change the sequence of command line argument handling!
			// MainCmd changes the inputSource!
			if (!inputSource.IsEmpty ())
			{
				// Command-line version
				int result = MainCmd (inputSource, hInst);
				if (result != -1)
					return result;
				// Command line mode continues in GUI
			}

			// Is there a running instance of this program?
			Win::Dow::Handle hwndOther = topWinClass.GetRunningWindow ();
			if (hwndOther != 0 && !inputSource.IsRestart ())
			{
				if (!PostCmdLineMsg (hwndOther, inputSource))
				{
					// No command line args generating messages - just show Dispatcher window
					// If Dispatcher is already running
					// activate the main window of the first instance.
					// It is not enough to just restore the window on screen
					// -- Dispatcher performs some specific actions 
					// when its window is displayed for the first time.
					Win::RegisteredMessage msg (UM_SHOW_WINDOW);
					hwndOther.PostMsg (msg);
				}
				return 0;
			}

			// Create Dispatcher main window
			topWinClass.Register ();
			ResString caption (hInst, ID_CAPTION);
			Win::TopMaker topWin (caption, DispatcherClassName, hInst);
			Win::Dow::Handle appWin = topWin.Maker::Create (&ctrl, DispatcherTitle);
			if (!PostCmdLineMsg (appWin, inputSource))
			{
				// No command line args generating messages.
				// Check if this is the very first run or we are restored from a backup archive.
				if (Registry::IsFirstRun () || Registry::IsRestoredConfiguration ())
				{
					// Immediately open configuration wizard
					Win::UserMessage msg (UM_CONFIG_WIZARD);
					appWin.PostMsg (msg);
				}
			}
		}

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
        TheOutput.Display ("Internal error.", Out::Error);
	}
	return 0;
}

bool PostCmdLineMsg (Win::Dow::Handle targetWin, CmdInputSource const & cmdLineArgs)
{
	bool postedMsg = false;
	for (CmdInputSource::Sequencer seq (cmdLineArgs); !seq.AtEnd (); seq.Advance ())
	{
		std::string const & cmd = seq.GetCommand ();
		if (IsNocaseEqual (cmd, "config_wizard"))
		{
			Win::UserMessage msg (UM_CONFIG_WIZARD);
			targetWin.PostMsg (msg);
			postedMsg = true;
		}
	}
	return postedMsg;
}

void UnexpectedHandler ()
{
	TheOutput.Display ("Unexpected system exception occurred during program execution.\n"
					   "The program will now exit.");
	terminate ();
}
