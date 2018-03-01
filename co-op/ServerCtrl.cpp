//------------------------------------
//  (c) Reliable Software, 2000 - 2008
//------------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "ServerCtrl.h"
#include "Commander.h"
#include "Model.h"
#include "DisplayMan.h"
#include "Global.h"
#include "PathRegistry.h"
#include "AppInfo.h"
#include "Prompter.h"
#include "OutputSink.h"
#include "CmdLineSelection.h"
#include "RandomUniqueName.h"
#include "ProjectMarker.h"

#include <Dbg/Out.h>
#include <Dbg/Log.h>
#include <Win/WinClass.h>
#include <Win/WinMaker.h>
#include <Win/MsgLoop.h>

//
// IPC server controller
//

ServerCtrl::ServerCtrl (unsigned int keepAliveTimeout, bool stayInProject)
	: _stayInProject (stayInProject),
	  _keepAliveTimeout (keepAliveTimeout),
	  _keepAliveTimer (KeepAliveTimerId),
	  _pendingMergeTimeoutCount (0),
	  _pendingMergeTimer (PendingMergeTimerId),
	  _msgIpcInitiate (UM_IPC_INITIATE),
	  _msgIpcTerminate (UM_IPC_TERMINATE),
	  _msgIpcAbort (UM_IPC_ABORT),
	  _msgAutoMergeCompleted (UM_AUTO_MERGER_COMPLETED),
	  _msgBackup (UM_COOP_BACKUP)
{
	Assert (_keepAliveTimeout != 0);
}

bool ServerCtrl::OnCreate (Win::CreateData const * create, bool & success) throw ()
{
	dbg << "ServerCtrl::OnCreate" << std::endl;
	try
	{
		// TheLog.Write ("ServerCtrl::OnCreate");
		TheAppInfo.SetWindow (_h);
		ThePrompter.SetWindow (_h);
		ThePrompter.AddUncriticalSect (_critSect);
		TheOutput.AddUncriticalSect (_critSect);

		BackupMarker backupMarker;
		if (backupMarker.Exists ())
		{
			// Don't allow server to run until recovery is completed.
			success = false;
			return success;
		}

		// Create the model
		_model.reset (new Model (true));	// Command line mode
		Win::MessagePrepro dummy;
		// Create commander
		_commander.reset (new Commander (*_model,
										 &_critSect,
										 0,			// InputSource
										 dummy,		// Message preprocessor
										 _h));		// Revisit: top app window
		// Create command vector
		_cmdVector.reset (new CmdVector (Cmd::Table, _commander.get ()));
		// Create IPC queue
		CommandIpc::Context context (*_commander,
			*_cmdVector,
			_critSect, 
			*_model); // feedback
		_ipcQueue.reset (new CommandIpc::Queue (_h, context));
		// Create blind display manager
		_displayMan.reset (new DisplayManager ());
		// Create selection manager
		_selectionMan.reset (new CmdLineSelection (*_model, _model->GetDirectory ()));
		_commander->ConnectGUI (_selectionMan.get (), _displayMan.get ());
		_model->SetUIFeedback (&_blindFeedback);
		_keepAliveTimer.Attach (_h);
		_pendingMergeTimer.Attach (_h);
		success = !_commander->HasProgramExpired ();
		SetKeepAliveTimer ();
	}
	catch (Win::Exception e)
	{
		dbg << Out::Sink::FormatExceptionMsg (e);
		success = false;
	}
	catch (...)
	{
		dbg << "Initialization--Unknown Error" << std::endl;
		Win::ClearError ();
		TheOutput.Display ("Initialization--Unknown Error", Out::Error);
		success = false;
	}
	TheOutput.SetParent (_h);
	return success;
}

bool ServerCtrl::OnDestroy () throw ()
{
	dbg << "ServerCtrl::OnDestroy" << std::endl;
	_keepAliveTimer.Kill ();
	_pendingMergeTimer.Kill ();
	return true;
}

bool ServerCtrl::OnTimer (int id) throw ()
{
	if (id == KeepAliveTimerId)
	{
		if (_ipcQueue->IsIdle ())
		{
			dbg << "ServerCtrl::OnTimer -- stop server" << std::endl;
			Win::Quit (0);
		}
		else
		{
			dbg << "ServerCtrl::OnTimer -- queue not idle" << std::endl;
		}
		return true;
	}
	else if (id == PendingMergeTimerId)
	{
		Win::Lock lock (_critSect);
		if (_model->HasPendingMergeRequests () && _pendingMergeTimeoutCount < 20)
		{
			dbg << "ServerCtrl::OnTimer -- waiting for pending merge requests " << std::dec << _pendingMergeTimeoutCount << std::endl;
			_pendingMergeTimer.Set (OneSecond);
			++_pendingMergeTimeoutCount;
		}
		else
		{
			dbg << "ServerCtrl::OnTimer -- NO pending merge requests" << std::endl;
			_pendingMergeTimer.Kill ();
			if (!_stayInProject)
				_commander->LeaveProject ();
			SetKeepAliveTimer ();
		}
		return true;
	}
	return false;
}

bool ServerCtrl::OnRegisteredMessage (Win::Message & msg) throw ()
{
	if (msg == _msgIpcInitiate)
	{
		dbg << "--> ServerCtrl::OnRegisteredMessage -- IPC Initiate" << std::endl;
		RandomUniqueName convName (msg.GetLParam ());
		_ipcQueue->InitiateConv (convName.GetString ());
		dbg << "<-- ServerCtrl::OnRegisteredMessage" << std::endl;
		return true;
	}
	else if (msg == _msgIpcTerminate)
	{
		OnQueueDone ();
		return true;
	}
	else if (msg == _msgIpcAbort)
	{
		// This is not a deadlock! The conversation has finished, but its thread
		// didn't die in time.
		// TheOutput.SetVerbose (true);
		// TheOutput.Display ("Background interprocess conversation deadlocked. Exiting Code Co-op!");
		Win::Quit ();
		return true;
	}
	else if (msg == _msgAutoMergeCompleted)
	{
		Win::Lock lock (_critSect);
		_model->OnAutoMergeCompleted (reinterpret_cast<ActiveMerger const *>(msg.GetLParam ()),
									  msg.GetWParam () != 0); 
		dbg << "<-- ServerCtrl::OnRegisteredMessage -- UM_AUTO_MERGER_COMPLETED; " << (msg.GetWParam () != 0 ? "NO conflicts" : "conflict detected") << std::endl;
		return true;
	}
	else if (msg == _msgBackup)
	{
		Win::Quit ();
		return true;
	}
#if defined (DIAGNOSTIC)
	Win::RegisteredMessage dbgMonStartMsg (Dbg::Monitor::UM_DBG_MON_START);
	Win::RegisteredMessage dbgMonStopMsg (Dbg::Monitor::UM_DBG_MON_STOP);
	if (msg == dbgMonStartMsg)
	{
		Dbg::TheLog.DbgMonAttach ("Server Code Co-op from Monitor");
		return true;
	}
	else if (msg == dbgMonStopMsg)
	{
		Dbg::TheLog.DbgMonDetach ();
		return true;
	}
#endif
	return false;
}

void ServerCtrl::OnQueueDone ()
{
	dbg << "--> ServerCtrl::OnQueueDone" << std::endl;
	if (!_ipcQueue->IsIdle ())
	{
		dbg << "<-- ServerCtrl::OnQueueDone - no longer idle" << std::endl;
		return;
	}

	if (_ipcQueue->StayInGui ())
	{
		dbg << "    Request to continue in GUI mode -- stopping server" << std::endl;
		// Reset keep-alive Windows timer to 0.5 second
		// This will allow the client to ask us something
		// before we quit and continue in GUI mode.
		_keepAliveTimer.Kill ();
		_keepAliveTimer.Set (OneSecond/2);
	}
	else
	{
		dbg << "    Begin wait for pending merge requests" << std::endl;
		_pendingMergeTimeoutCount = 0;
		_keepAliveTimer.Kill ();
		_pendingMergeTimer.Set (OneSecond/10);
	}
	dbg << "<-- ServerCtrl::OnQueueDone" << std::endl;
}

void ServerCtrl::SetKeepAliveTimer ()
{
	dbg << "--> ServerCtrl::SetKeepAliveTimer" << std::endl;
	// Reset keep alive timer
	if (_keepAliveTimeout != -1)
	{
		dbg << "     Reseting keep alive timer - keep alive timeout = " << std::dec << _keepAliveTimeout << std::endl;
		Assert (_keepAliveTimeout != 0);
		_keepAliveTimer.Kill ();
		_keepAliveTimer.Set (_keepAliveTimeout);
	}
	else
	{
		dbg << "    Forever waiting for the next conversation" << std::endl;
	}
	dbg << "<-- ServerCtrl::SetKeepAliveTimer" << std::endl;
}
