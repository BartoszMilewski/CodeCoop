//----------------------------------
// (c) Reliable Software 1996 - 2008
//----------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "Registry.h"
#include "PathRegistry.h"
#include "resource.h"
#include "Global.h"
#include "AppInfo.h"
#include "InputSource.h"
#include "OutputSink.h"
#include "CoopCtrl.h"
#include "CoopCaption.h"
#include "Catalog.h"
#include "ProjectMarker.h"
#include "DispatcherProxy.h"

#include <Win/WinMain.h>
#include <Win/WinClass.h>
#include <Win/WinMaker.h>
#include <Win/MsgLoop.h>
#include <Ex/WinEx.h>
#include <Sys/SysVer.h>
#include <Ctrl/Controls.h>

#include <auto_vector.h>
#include <Handlers.h>

#include <cstdlib>
#include <ctime>

void UnexpectedHandler ();

#if defined (_DEBUG)
#include "MemCheck.h"
static HeapCheck heapCheck;
#endif

int Win::Main (Win::Instance hInst, char const * cmdParam, int cmdShow)
{
	dbg << "--> Co-op Main; command line: " << cmdParam << std::endl;
	Win::UseCommonControls ();
	TheOutput.Init ("Code Co-op");
	SystemVersion osVer;
	if (osVer.IsOK ())
	{
		bool supportedVersion = (osVer.IsWin32Windows ()) ||
								(osVer.IsWinNT () && osVer.MajorVer () >= 4);
		if (!supportedVersion)
		{
			TheOutput.Display ("Unsupported Windows version", Out::Error);
			return 0;
		}
	}
	else
		return 0;	// If we can't get system version can we run ?

	UnexpectedHandlerSwitch unexpHandler (UnexpectedHandler);

	// Initialize random number generator
	std::srand (static_cast<unsigned int>(time (0)));

	int status = 0;
	try
	{
		TheOutput.Init ("Code Co-op", Registry::GetLogsFolder ());
		AppInformation::Initializer appInfoInit;
		std::unique_ptr<InputSource> inputSource (new InputSource);
		{
			InputParser parser (cmdParam, *inputSource);
			parser.Parse ();
		}

		// Are we properly configured?
		if (Registry::IsFirstRun () || Registry::IsRestoredConfiguration ())
		{
			// This is the very first Code Co-op run or the first run after restoring from
			// a backup archive. Dispatcher needs to be configured or we have to confirm the
			// restored configuration. Start Dispatcher configuration wizard
			// and quit Code Co-op.
			DispatcherProxy dispatcher;
			dispatcher.ShowConfigurationWizard ();
			return 0;
		}

		// Command-line version
		int result = 0;
		if (inputSource->StartServer () || inputSource->HasBackgroundCommands ())
		{
			TheOutput.Init ("Code Co-op Server");
			TheOutput.SetVerbose (false);
			int result = MainCmd (*inputSource, hInst);
		}

		Assert (cmdParam != 0);
		if (!inputSource->StayInGui () && cmdParam [0] != '\0')
			return result;

		TheOutput.SetVerbose (true);
		// In command-line mode, we get here only when -GUI is set
		if (inputSource->StayInGui () && inputSource->HasProjectId ())
		{
			Catalog catalog;
			// Look for GUI Code Co-op in the specified project -- window caption matters
			CoopCaption caption (catalog.GetProjectName (inputSource->GetProjectId ()),
					catalog.GetProjectSourcePath (inputSource->GetProjectId ()).GetDir ());
			Win::Dow::Handle winOtherInProj = ::FindWindow (CoopClassName, caption.str ().c_str ());
			if (!winOtherInProj.IsNull ())
			{
				// Restore GUI instance window already visiting the required project
				winOtherInProj.Restore ();
				winOtherInProj.SetForeground ();
				// Revisit: pass serialized InputSource to that process
				return 0;	// Don't start this instance
			}
		}

		Win::Class::TopMaker topWinClass (CoopClassName, hInst, ID_COOP);
		topWinClass.Style () << Win::Class::Style::RedrawOnSize;
		topWinClass.Register ();

		// Is there any running instance of Code Co-op?
		Win::Dow::Handle winOther = topWinClass.GetRunningWindow ();

		TheAppInfo.SetFirstInstanceFlag (winOther == 0);
		if (!inputSource->StayInGui ()) // Same as "not from command line
		{
			if (winOther != 0)
			{
				TheOutput.Init ("Code Co-op is already running");
				// Ask the user if he/she wants to open another project
				Out::Answer userChoice = TheOutput.Prompt (
					"Do you want to run another instance of Code Co-op\n"
					"and visit another project?");
				if (userChoice == Out::No)
				{
					// Restore previous instance window
					winOther.Restore ();
					winOther.SetForeground ();
					return 0;
				}
				else if (userChoice == Out::Cancel)
				{
					// Do not start this instance
					return 0;
				}
			}
		}

		// The ListWindow class
		Win::Class::Maker winClassText (ListWindowClassName, hInst);
		winClassText.SetResIcons (I_VIEWER);
		winClassText.Register ();

		// Create Window
		TheOutput.Init ("Code Co-op");
		ResString caption (hInst, ID_CAPTION);
		Win::TopMaker topWin (caption, CoopClassName, hInst);
		Win::MessagePrepro msgPrepro;
		CoopController coopCtrl (msgPrepro, inputSource->GetProjectId ());

		Win::Dow::Handle appWin = topWin.Create (&coopCtrl, "Code Co-op");
		coopCtrl.SetWindowPlacement (static_cast<Win::ShowCmd>(cmdShow), winOther != 0);
		if (inputSource->HasGuiCommands ())
		{
			// Post (at most one) GUI command from input source.
			inputSource->SwitchToGuiCommands ();
			InputSource::Sequencer & cmdSeq = inputSource->GetCmdSeq ();
			Assert (!cmdSeq.AtEnd ());
			dbg << "Posting GUI command: " << cmdSeq.GetCmdName () << std::endl;
			coopCtrl.PostCommand (cmdSeq.GetCmdName (),	cmdSeq.GetArguments ());
		}
		inputSource.reset (0);
		dbg << "Entering Code Co-op message loop" << std::endl;
		// The main message loop
		status = msgPrepro.Pump ();
	}
	catch (Win::Exception e)
	{
		TheOutput.Reset ();
		TheOutput.Display (e);
	}
	catch (...)
	{
		Win::ClearError ();
		TheOutput.Reset ();
		TheOutput.Display ("Internal error: Unknown exception", Out::Error);
	}
	return status;
}

void UnexpectedHandler ()
{
	dbg << "Unexpected system exception occurred during program execution." << std::endl;
	dbg << "The program will now exit." << std::endl;
	TheOutput.Display ("Unexpected system exception occurred during program execution.\n"
					   "The program will now exit.");
	terminate ();
}
