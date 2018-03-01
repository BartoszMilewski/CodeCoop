//----------------------------------
// (c) Reliable Software 1998 - 2008
//----------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "InputSource.h"
#include "Model.h"
#include "SelectionMan.h"
#include "DisplayMan.h"
#include "FeedbackMan.h"
#include "Commander.h"
#include "CmdVector.h"
#include "OutputSink.h"
#include "CmdLineSelection.h"
#include "ProjectData.h"
#include "Registry.h"
#include "VerificationReport.h"
#include "ServerCtrl.h"
#include "Global.h"
#include "RandomUniqueName.h"
#include "resource.h"

#include <Dbg/Out.h>
#include <Dbg/Log.h>
#include <Win/WinClass.h>
#include <Win/WinMaker.h>
#include <Win/MsgLoop.h>
#include <Sys/Synchro.h>
#include <Ex/WinEx.h>

static void StartServer (InputSource & inputSource, Win::Instance hInst);
static int ExecuteCommand (InputSource & inputSource);

int MainCmd (InputSource & inputSource, Win::Instance hInst)
{
	int retCode = 0;
	try
	{
		if (inputSource.StartServer ())
			StartServer (inputSource, hInst);
		else
			retCode = ExecuteCommand (inputSource);
	}
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
		inputSource.SetStayInGui (false);
		retCode = 1;
	}
	catch (...)
	{
		inputSource.SetStayInGui (false);
		Win::ClearError ();
		retCode = 1;
	}
	return retCode;
}

// To start Code Co-op in GUI-less server mode execute it with the following command line:
//
//	-NoGUI [conversation:"<hex number>"] [keepalivetimeout:"<decimal number>"] [stayinproject:"yes" | "no"]
//
// Optional parameters have the following meaning:
//
//	- conversation:"<hex number>"-- Windows atom specifying shared memory buffer name. Buffer should contain
//									conversation invitation. When conversation atom is provided we will post invitation
//									message to the server window.
//	- keepalivetimeout:"<decimal number>" -- after the server has been started it will wait that amount of time
//											 for conversation invitation. The timeout value is specified in miliseconds.
//											 If timeout is not specified server waits 3 seconds. Value -1 means wait
//											 forever -- use it when debugging server.
//	- stayinproject:"yes" | "no" -- after executing command in a given project Code Co-op server will stay in the
//									visited project (the default) when 'yes' is specified or option is omitted.
//									When 'no' is specified then server will leave project after executing command.
//
// Notice: option names are case sensitive.

static void StartServer (InputSource & inputSource, Win::Instance hInst)
{
#if defined (DIAGNOSTIC)
		// Uncomment this to turn on logging to the file. You may change the path.
		/*
		FilePath userDesktopPath;
		ShellMan::VirtualDesktopFolder userDesktop;
		userDesktop.GetPath (userDesktopPath);
		if (!Dbg::TheLog.IsOn ()) 
			Dbg::TheLog.Open ("ServerCoopLog.txt", userDesktopPath.GetDir ());
		*/
		// By default we log to debug monitor window 
		Dbg::TheLog.DbgMonAttach ("Server Code Co-op");
#endif

	Win::Class::TopMaker topWinClass (ServerClassName, hInst, ID_COOP);
	// Is there a running instance of server?
	Win::Dow::Handle winOther = topWinClass.GetRunningWindow ();
	if (winOther != 0)
	{
		// Revisit: now what ? Start this server instance ?
	}
	topWinClass.Register ();
	// Create top server window
	ResString caption (hInst, ID_CAPTION);
	Win::TopMaker topWin (caption, ServerClassName, hInst);

	InputSource::Sequencer & cmdSeq = inputSource.GetCmdSeq ();
	NamedValues const & args = cmdSeq.GetArguments ();
	std::string timeoutString (args.GetValue (KeepAliveTimeout));
	unsigned int timeoutMiliseconds;
	if (timeoutString.empty ())
	{
		timeoutMiliseconds = 3000;	// 3 seconds
	}
	else
	{
		std::istringstream in (timeoutString);
		in >> timeoutMiliseconds;
	}
	std::string stayInProjectString (args.GetValue (StayInProject));
	bool stayInProject;
	if (stayInProjectString.empty ())
	{
		stayInProject = true;	// The default
	}
	else
	{
		stayInProject = (stayInProjectString == "yes");
	}
	ServerCtrl serverCtrl (timeoutMiliseconds, stayInProject);
	Win::Dow::Owner serverWin (topWin.Create (&serverCtrl, "Code Co-op Server").ToNative ());
	std::string ipcBufferIdString (args.GetValue (Conversation));
	if (!ipcBufferIdString.empty ())
	{
		RandomUniqueName ipcBufferId (ipcBufferIdString);
		Win::RegisteredMessage msgInitiate (UM_IPC_INITIATE);
		msgInitiate.SetLParam (ipcBufferId.GetValue ());
		serverWin.PostMsg (msgInitiate);
	}
	dbg << "StartServer -- pumping the server" << std::endl;
	// Pump the server
	Win::MessagePrepro msgPrepro;
	msgPrepro.Pump ();
	dbg << "StartServer -- server stopped" << std::endl;
/*
	if (Dbg::TheLog.IsOn ()) Dbg::TheLog.Close ();
*/
	if (serverCtrl.StayInGui ())
	{
		inputSource.SetProjectId (serverCtrl.GetProjectId ());
		inputSource.SetStayInGui (true);
	}
}

// Pass projectId from InputSource
static int ExecuteCommand (InputSource & inputSource)
{
	InputSource::Sequencer & cmdSeq = inputSource.GetCmdSeq ();
	if (cmdSeq.AtEnd ())
		return 0;

#if defined (DIAGNOSTIC)
	// Uncomment this to turn on logging to the file. You may change the path.
/*
	FilePath userDesktopPath;
	ShellMan::VirtualDesktopFolder userDesktop;
	userDesktop.GetPath (userDesktopPath);
	if (!Dbg::TheLog.IsOn ()) 
		Dbg::TheLog.Open ("CmdLineCoopLog.txt", userDesktopPath.GetDir ());
*/
	// By default we log to debug monitor window 
	Dbg::TheLog.DbgMonAttach ("Cmd Line Code Co-op");
#endif
	// Create the model
	Model model (true); // In command line mode
	Win::MessagePrepro dummy;
	// Create commander
	Commander commander (model, 
						 0,
						 &inputSource,
						 dummy, // Message preprocessor
						 0);	// Revisit: top app window
	// Create command vector
	CmdVector cmdVector (Cmd::Table, &commander);
	// Create selection manager
	CmdLineSelection selectMan (model, model.GetDirectory ());
	// Create blind display manager
	DisplayManager displayMan;
	// Connect blind GUI
	commander.ConnectGUI (&selectMan, &displayMan);
	FeedbackManager blindFeedback;
	model.SetUIFeedback (&blindFeedback);
	if (commander.HasProgramExpired ())
		return 2;

	if (inputSource.GetProjectId () != -1)
	{
		if (!commander.VisitProject (inputSource.GetProjectId (), false)) // don't remember ID
			return 1;
	}

	// Execute commands in the project
	// Revisit: In command line mode Code Co-op doesn't have application main
	// window, so it cannot be found when we are looking for running Co-op instances.
	while (!cmdSeq.AtEnd ())
	{
		std::string const & command = cmdSeq.GetCommand (selectMan);
		int cmdId = cmdVector.Cmd2Id (command.c_str ());
		if (cmdId != -1)
			cmdVector.Execute (cmdId);

		cmdSeq.Advance ();
	}
	return 0;
}
