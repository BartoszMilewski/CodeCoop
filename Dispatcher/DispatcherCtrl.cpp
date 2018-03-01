// ---------------------------------
// (c) Reliable Software 1998 - 2009
// ---------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "DispatcherCtrl.h"
#include "Commander.h"
#include "ViewMan.h"
#include "Catalog.h"
#include "AppInfo.h"
#include "OutputSink.h"
#include "Model.h"
#include "ConfigExpt.h"
#include "Lock.h"
#include "Global.h"
#include "DispatcherMsg.h"
#include "Registry.h"
#include "DispatcherParams.h"
#include "resource.h"
#include "StandaloneJoin.h"
#include "JoinProject.h"
#include "ProjectInviteDlg.h"
#include "SccProxyEx.h"
#include "DispatcherScript.h"
#include "TransportHeader.h"
#include "DispatcherIpc.h"
#include "Prompter.h"
#include "AlertMan.h"
#include "FeedbackMan.h"
#include "UserTracker.h"
#include "InviteSatDlg.h"
#include "PasswordDlg.h"
#include "PathRegistry.h"

#include <Dbg/Log.h>
#include <Dbg/Out.h>
#include <Com/Shell.h>
#include <Win/MsgLoop.h>
#include <Sys/Timer.h>
#include <Sys/Process.h>
#include <Ex/WinEx.h>
#include <Win/Mouse.h>
#include <Win/WinMain.h>
#include <StringOp.h>
#include <Ctrl/Messages.h>

// Windows WM_NOTIFY handlers

class ViewHandler : public Notify::ListViewHandler
{
public:
	ViewHandler (DispatcherController * ctrl, 
				 Commander * commander, 
				 ViewMan * viewMan, 
				 CmdVector * cmdVector)
		: Notify::ListViewHandler (viewMan->GetItemView ().GetId ()),
		  _ctrl (ctrl),
		  _commander (commander),
		  _viewMan (viewMan),
		  _cmdVector (cmdVector)
	{}

	bool OnDblClick () throw ();
	bool OnBeginLabelEdit (long & result) throw ();
	bool OnItemChanged (Win::ListView::ItemState & state) throw ();
	bool OnColumnClick (int col) throw ();

private:
	DispatcherController *	_ctrl;
	Commander *				_commander;
	ViewMan   *				_viewMan;
    CmdVector *				_cmdVector;
};

bool ViewHandler::OnDblClick () throw ()
{
	if (_viewMan->IsIn (QuarantineView))
	{
		if (_commander->can_Selection_ReleaseFromQuarantine () == Cmd::Enabled)
		{
			int cmdId = _cmdVector->Cmd2Id ("Selection_ReleaseFromQuarantine");
			Assert (cmdId != -1);
			return _ctrl->OnCommand (cmdId, false);
		}
	}
    else if (_commander->can_Selection_Details () == Cmd::Enabled)
	{
        int cmdId = _cmdVector->Cmd2Id ("Selection_Details");
        Assert (cmdId != -1);
		return _ctrl->OnCommand (cmdId, false);
	}
	return true;
}

bool ViewHandler::OnBeginLabelEdit (long & result) throw ()
{
	result = TRUE;	// No label edit in main view
	return true;
}

bool ViewHandler::OnItemChanged (Win::ListView::ItemState & state) throw ()
{
	_viewMan->UpdateToolBarState ();
	return false;
}

bool ViewHandler::OnColumnClick (int col) throw ()
{
	_viewMan->Sort (col);
	return true;
}

class ViewTabHandler : public Notify::TabHandler
{
public:
	ViewTabHandler (ViewMan * viewMan)
		: Notify::TabHandler (viewMan->GetTabView ().GetId ()), _viewMan (viewMan)
	{}

	bool OnSelChange () throw ()
	{
		_viewMan->TabChanged ();
		return true;
	}

private:
    ViewMan * _viewMan;
};

class ToolTipHandler : public Notify::ToolTipHandler
{
public:
	ToolTipHandler (ViewMan * viewMan)
		: Notify::ToolTipHandler (viewMan->GetToolBarView ().GetId ()), _viewMan (viewMan)
	{}

	bool OnNeedText (Tool::TipForCtrl * tip) throw ()
	{
		_viewMan->FillToolTip (tip); 
		return true;
	}

private:
    ViewMan *	_viewMan;
};

DispatcherController::DispatcherController (Win::MessagePrepro * msgPrepro)
  : _msgPrepro (msgPrepro),
    _mailboxTimer (MailboxProcessTimerId),
	_coopTimer (CoopTimerId),
	_maintenanceTimer (MaintenanceTimerId),
	_overallMaintenanceTimer (OverallMaintenanceTimerId),
	_restartTimer (RestartTimerId),
	_forceDispatchTimer (ForceDispatchTimerId),
	_doubleClickTimer (DoubleClickTimerId),
	_trackUserTimer (TrackUserTimerId),
	_activityTimer (ActivityTimerId),
    _isBusy (false),
	_isExecutingUserCmd (false),
	_isWaitingForDblClk (false),
	_curRetryPeriod (ProcessPeriod),
	_coopTimerPeriod (ProcessPeriod),
    _enlistmentEmailChanged (false),
	_enlistmentListChanged (false),
	_msgStartup (UM_STARTUP),
	_msgProjectChanged (UM_COOP_PROJECT_CHANGE),
	_msgProjectStateChange (UM_COOP_PROJECT_STATE_CHANGE),
	_msgChunkSizeChange (UM_COOP_CHUNK_SIZE_CHANGE),
	_msgJoinRequest (UM_COOP_JOIN_REQUEST),
	_msgCoopUpdate (UM_COOP_UPDATE),
	_msgConfigWizard (UM_CONFIG_WIZARD),
	_msgDownloadEvent (UM_DOWNLOAD_EVENT),
	_msgDownloadProgress (UM_DOWNLOAD_PROGRESS),
	_msgNotification (UM_COOP_NOTIFICATION),
	_msgDisplayAlert (UM_DISPLAY_ALERT),
	_msgActivityChange (UM_ACTIVITY_CHANGE),
	_msgShowWindow (UM_SHOW_WINDOW),
	_msgTaskbarCreated ("TaskbarCreated")
{}


DispatcherController::~DispatcherController ()
{}

Notify::Handler * DispatcherController::GetNotifyHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ()
{
	if (_viewHandler.get () && _viewHandler->IsHandlerFor (idFrom))
		return _viewHandler.get ();
	else if (_tabHandler.get () && _tabHandler->IsHandlerFor (idFrom))
		return _tabHandler.get ();
	else
		return _toolTipHandler.get ();
}

Control::Handler * DispatcherController::GetControlHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ()
{
	if (_viewMan.get ())
		return _viewMan->GetControlHandler (winFrom, idFrom);
	return 0;
}

bool DispatcherController::OnCreate (Win::CreateData const * create, bool & success) throw ()
{
	try
	{
		dbg << "Creating Controller." << std::endl;
		success = false;
#if defined (DIAGNOSTIC)
		// Uncomment this to turn on logging to the file. You may change the path.
#if 0
		FilePath userDesktopPath;
		ShellMan::VirtualDesktopFolder userDesktop;
		userDesktop.GetPath (userDesktopPath);
		if (!Dbg::TheLog.IsOn ()) 
			Dbg::TheLog.Open ("DispatcherLog.txt", userDesktopPath.GetDir ());
#endif
		// By default we log to debug monitor window 
		Dbg::TheLog.DbgMonAttach ("Dispatcher");
#endif
        ReentranceLock lock (_isBusy);
		TheAppInfo.SetWindow (_h);
		ThePrompter.Init (_h, &_isExecutingUserCmd);
		TheAlertMan.SetWindow (_h);
		TheFeedbackMan.Init (_h);
		_model.reset (new Model (_h, *_msgPrepro, *this));
		_commander.reset (new Commander (_h, _msgPrepro, *_model));
		_cmdVector.reset (new CmdVector (Cmd::Table, _commander.get ()));
		_viewMan.reset (new ViewMan (_h,
							   *_cmdVector,
							   *_model,
							   *this));
		_updateMan.reset (new UpdateManager (_h,
											 *_msgPrepro,
											 _model->GetUpdatesPath ()));
		// Pass keyboard accelerators to message preprocessor
		_viewMan->AttachAccelerator (_h, _msgPrepro);
		_viewMan->AttachMenu (_h);
		_commander->SetView (_viewMan.get ());
		// Create WM_NOTIFY handlers
		_viewHandler.reset (new ViewHandler (this, 
											 _commander.get (), 
											 _viewMan.get (), 
											 _cmdVector.get ()));
		_tabHandler.reset (new ViewTabHandler (_viewMan.get ()));
		_toolTipHandler.reset (new ToolTipHandler (_viewMan.get ()));

		_mailboxTimer.Attach (_h);
		_coopTimer.Attach (_h);
		_maintenanceTimer.Attach (_h);
		_overallMaintenanceTimer.Attach (_h);
		_restartTimer.Attach (_h);
		_forceDispatchTimer.Attach (_h);
		_doubleClickTimer.Attach (_h);
		_trackUserTimer.Attach (_h);
		_activityTimer.Attach (_h);

		_h.PostMsg (_msgStartup);

		TheOutput.SetParent (_h);
		success = true;
	}
	catch (Win::ExitException e)
	{
		TheOutput.Display (e);
	}
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
	}
	catch (std::bad_alloc bad)
	{
		Win::ClearError ();
		TheOutput.Display ("Not enough memory.\nCannot run Dispatcher.");
	}
	catch (...)
	{
		Win::ClearError ();
		TheOutput.Display ("Initialization -- Unknown Error", Out::Error); 
	}
	return true;
}

namespace UnitTest
{
	extern void Run ();
}

void DispatcherController::OnStartup ()
{
	dbg << "--> DispatcherController::OnStartup" << std::endl;
    try
    {
		if (Registry::IsFirstRun ())
		{
			// This is all we can do during the very first Dispatcher run
			dbg << "<-- DispatcherController::OnStartup - first run" << std::endl;
			TheEmail.Refresh ();// Set defaults for e-mail processing - will be used by the ConfigWizard
			_viewMan->OnStartup ();
			return;
		}

		if (!TheEmail.Refresh ())
		{
			if (TheEmail.PasswordDecryptionFailed ())
			{
				// Prompt the user for the passwords
				EmailPasswordCtrl passwordCtrl;
				if (ThePrompter.GetData (passwordCtrl))
				{
					// Store passwords in the registry and re-initialize TheEmail
					Email::RegConfig emailCfg;
					Pop3Account pop3Account (emailCfg, std::string ());
					pop3Account.SetPassword (passwordCtrl.GetPop3Password ());
					pop3Account.Save (emailCfg);
					if (passwordCtrl.UseSmtpPassword ())
					{
						SmtpAccount smtpAccount (emailCfg);
						smtpAccount.SetPassword (passwordCtrl.GetSmtpPassword ());
						smtpAccount.Save (emailCfg);
					}
					TheEmail.Refresh ();
				}
				else
				{
					throw Win::InternalException ("The e-mail configuration is incorrect: "
												"missing password.\n"
												"The dispatcher cannot continue.\n\n"
												"To restart the Dispatcher, use "
												"Start Menu>Reliable Software>Dispatcher");
				}
			}
		}
		_model->OnStartup ();
		_viewMan->OnStartup ();
		_updateMan->OnStartup ();
#if defined (COOP_LITE)
		Topology topology = _model->GetTopology ();
		if (topology.IsHub () || topology.IsSatellite ())
		{
			// Unsupported configurations - start configuration wizard
			try
			{
				Out::Answer ans = TheOutput.Prompt (
					"Code Co-op Lite has detected a LAN configuration supported only by Code Co-op Pro.\n"
					"You have to reconfigure your Dispatcher not to use LAN.\n\n"
					"Would you like to do it now?",
					Out::PromptStyle (Out::YesNo, Out::No));
				if (ans == Out::No)
					throw Win::InternalException ("Cannot continue in this configuration");
				_commander->Program_ConfigWizard ();
			}
			catch (ConfigExpt)
			{
				dbg << "Reconfigure" << std::endl;
				Reconfigure ();
			}

			topology = _model->GetTopology ();
			if (!topology.IsStandalone () && !topology.IsPeer ())
			{
				throw Win::InternalException ("This product supports only two configurations:\n\n"
					"- standalone operation on a single computer or\n"
					"- collaboration with others using e-mail (e-mail peer).");
			}
		}
#endif
		if (_model->IsConfigWithEmail () && !TheEmail.IsValid ())
		{
			try
			{
				TheOutput.Display ("The e-mail configuration is incorrect.\n"
								   "Please correct e-mail settings.");
				_commander->Program_EmailOptions ();
			}
			catch (ConfigExpt)
			{
				Reconfigure ();
			}
			if (!TheEmail.IsValid ())
				throw Win::InternalException ("The e-mail configuration is incorrect.\n"
											  "Dispatcher cannot continue.\n\n"
											  "To restart the Dispatcher, use "
											  "Start Menu>Reliable Software>Dispatcher");
		}


#if !defined (NDEBUG)
		UnitTest::Run ();
#endif

		if (_updateMan->IsAlerting ())
			SetUpdatesAlert (false);

		_mailboxTimer.Set (StartupPeriod);
		_maintenanceTimer.Set (MaintenancePeriod);
		_overallMaintenanceTimer.Set (StartupPeriod + 10000); // stagger by 10s
		_restartTimer.Set (RestartPeriod);
		_coopTimer.Set (_coopTimerPeriod);

	}
    catch (Win::ExitException e)
    {
        TheOutput.Display (e);
		dbg << "<-- DispatcherController::OnStartup - exit exception: " << Out::Sink::FormatExceptionMsg (e) << std::endl;
		_h.Destroy ();
    }
    catch (Win::Exception e)
    {
        TheOutput.Display (e);
		dbg << "<-- DispatcherController::OnStartup - exception: " << Out::Sink::FormatExceptionMsg (e) << std::endl;
		_h.Destroy ();
    }
    catch (...)
    {
		Win::ClearError ();
#if defined DEBUG
        TheOutput.Display ("Unknown error when restoring program state");
#endif
		dbg << "<-- DispatcherController::OnStartup - unknow exception" << std::endl;
		_h.Destroy ();
    }
	dbg << "<-- DispatcherController::OnStartup" << std::endl;
}

bool DispatcherController::OnDestroy () throw ()
{
	if (_viewMan.get () != 0)
	{
		_viewMan->OnDestroy ();
		_viewMan.reset ();
	}
	Win::Quit ();
	return true;
}

bool DispatcherController::OnShutdown (bool isEndOfSession, bool isLoggingOff) throw ()
{
	if (isEndOfSession)
		_model->ShutDown ();
	return true;
}

bool DispatcherController::OnSettingChange (Win::SystemWideFlags flags, char const * sectionName) throw ()
{
	if (flags.IsNonClientMetrics ())
	{
		_viewMan->NonClientMetricsChanged ();
		return true;
	}
	return false;
}

bool DispatcherController::OnSize (int width, int height, int flag) throw ()
{ 
	_viewMan->Size (width, height); 
	return true;
}

bool DispatcherController::OnClose () throw ()
{
    _h.Hide ();
	_viewMan->HideWindow ();
	return true;
}

bool DispatcherController::OnFocus (Win::Dow::Handle winPrev) throw ()
{
	// OnFocus may be called before OnCreate ends;
	// this happens when some dialog (e.g. Configuration wizard)
	// is displayed from OnCreate;
	if (_viewMan.get () != 0)
		_viewMan->SetFocus ();

	return true;
}

bool DispatcherController::OnShowWindow (bool show) throw ()
{
	try
	{
		if (show)
		{
			// inform viewMan that it is to be displayed
			_viewMan->ShowWindow ();
		}
	}
	catch (Win::Exception e)
	{
		TheAlertMan.PostInfoAlert  (e);
	}
	catch (...)
	{
		Win::ClearError ();
	}

	return false;
}

bool DispatcherController::OnCommand (int cmdId, bool isAccel) throw ()
{
	if (isAccel)
	{
		// Keyboard accelerators to invisible or disabled menu items are not executed
		if (_cmdVector->Test (cmdId) != Cmd::Enabled)
			return true;
	}
	MenuCommand (cmdId);
	return true;
}

class FlagSignaller
{
public:
	FlagSignaller (bool & flag)
		: _flag (flag),
		  _originalValue (flag)
	{
		_flag = true;
	}
	~FlagSignaller ()
	{
		_flag = _originalValue;
	}
private:
	bool	 & _flag;
	bool const _originalValue;
};

void DispatcherController::MenuCommand (int cmdId)
{
    if (_isBusy)
        return;

	try
	{
		// Menu commands or accelerator
		dbg << "Executing command: " << _cmdVector->GetName (cmdId) <<std::endl;
        ReentranceLock activityLock (_isBusy);
		FlagSignaller userAction (_isExecutingUserCmd);
		Cursor::Holder working (_hourglass);
		try
		{
			_cmdVector->Execute (cmdId);
		}
		catch (ConfigExpt)
		{
			dbg << "Reconfigure" << std::endl;
			Reconfigure ();
		}
		dbg << "Executing command. Done!" << std::endl;
	}
    catch (Win::ExitException e)
    {
		TheOutput.Display (e);
        _h.Destroy ();
    }
	catch (Win::Exception e)
	{
		TheAlertMan.PostInfoAlert (e);
	}
	catch (std::bad_alloc bad)
	{
		Win::ClearError ();
		std::string msg = "Not enough memory to execute the ";
		msg += _cmdVector->GetName (cmdId);
		msg += " command.";
		TheAlertMan.PostInfoAlert (msg);
	}
	catch (...)
	{
		Win::ClearError ();
		std::string msg = "Internal Error: The ";
		msg += _cmdVector->GetName (cmdId);
		msg += " command could not execute.";
		TheAlertMan.PostInfoAlert (msg);
	}
}

bool DispatcherController::OnControl (Win::Dow::Handle control, unsigned id, unsigned notifyCode) throw ()
{
	if (ID_TOOLBARTEXT == id)
		return false;
	else
		return OnCommand (id, false);
}

bool DispatcherController::OnContextMenu (Win::Dow::Handle wnd, int xPos, int yPos) throw ()
{
	try
	{
		return _viewMan->DisplayContextMenu (_h, wnd, xPos, yPos);
	}
	catch (Win::Exception e)
	{
		Assert (!"Error during displaying contxt menu.");
		e;
		return false;
	}
	catch ( ... )
	{
		Assert (!"Unknown error during displaying contxt menu.");
		Win::ClearError ();
		return false;
	}
}

bool DispatcherController::OnInitPopup (Menu::Handle menu, int pos) throw ()
{
	try
	{
		_viewMan->RefreshPopup (menu, pos);
	}
	catch (...) 
	{
		Win::ClearError ();
#if defined DEBUG
		TheOutput.Display ("Internal Error: Cannot refresh popup menu.", Out::Error);
#endif
		return false;
	}
	return true;
}

bool DispatcherController::IsEnabled (std::string const & cmdName) const throw ()
{
	return (_cmdVector->Test (cmdName.c_str ()) == Cmd::Enabled);
}

void DispatcherController::ExecuteCommand (std::string const & cmdName) throw ()
{
	if (IsEnabled (cmdName))
	{
		int cmdId = _cmdVector->Cmd2Id (cmdName.c_str ());
		Assert (cmdId != -1);
		MenuCommand (cmdId);
	}
}

void DispatcherController::ExecuteCommand (int cmdId) throw ()
{
	Assert (cmdId != -1);
	MenuCommand (cmdId);
}

bool DispatcherController::OnMenuSelect (int id, Menu::State state, Menu::Handle menu) throw ()
{
	if (state.IsDismissed ())
	{
		// Menu dismissed
		_viewMan->ResetStatusText ();
	}
	else if (!state.IsSeparator () && !state.IsSysMenu () && !state.IsPopup ())
	{
		// Menu item selected
		char const * cmdHelp = _cmdVector->GetHelp (id);
		_viewMan->DisplayHelp (cmdHelp);
	}
	else
	{
		return false;
	}
	return true;
}

bool DispatcherController::OnUserMessage (Win::UserMessage & msg) throw ()
{
	bool status = true;
    try
    {
		switch (msg.GetMsg ())
		{
		case UM_FOLDER_CHANGE:
			OnFolderChange (reinterpret_cast<FWatcher *>(msg.GetLParam ()));
			break;
		case UM_TASKBAR_ICON_NOTIFY:
			OnTaskbarIconNotify (msg.GetWParam (), msg.GetLParam ());
			break;
		case UM_FINISH_SCRIPT:
			// Revisit: refresh Quarantine view based on returned flag
			_model->OnFinishScripts ();
			_viewMan->Refresh (QuarantineView);
			break;
		case UM_REFRESH_VIEW:
		case UM_SCRIPT_STATUS_CHANGE:
			RefreshUI ();
			break;
		case UM_PROXY_CONNECTED:
			try
			{
				_model->OnConnectProxy (msg.GetWParam () != 0);
			}
			catch (ConfigExpt)
			{
				Reconfigure ();
			}
			break;
		case UM_CONFIG_WIZARD:
			{
				int cmdId = _cmdVector->Cmd2Id ("Program_ConfigWizard");
				Assert (cmdId != -1);
				MenuCommand (cmdId);
			}
			break;
		default:
			status = false;
			break;
		}
	}
    catch (Win::Exception e)
    {
        TheAlertMan.PostInfoAlert (e);
    }
    catch (...)
    {
		Win::ClearError ();
#if defined DEBUG
        TheOutput.Display ("Unknown error processing user message.");
#endif
    }
	return status;
}

bool DispatcherController::OnRegisteredMessage (Win::Message & msg) throw ()
{
 	bool status = true;
	try
    {
		if (_msgProjectChanged == msg)
		{
			OnCoopProjectChange ();
		}
		else if (_msgProjectStateChange == msg)
		{
			OnCoopProjectStateChange (msg.GetWParam (), msg.GetLParam () != 0);
		}
		else if (_msgChunkSizeChange == msg)
		{
			_model->BroadcastChunkSizeChange (msg.GetWParam ());
		}
		else if (_msgJoinRequest == msg)
		{
			OnCoopJoinRequest (msg.GetWParam () != 0);
		}
		else if (_msgCoopUpdate == msg)
		{
			OnCoopUpdate (msg.GetWParam () != 0);
		}
		else if (_msgDownloadEvent == msg)
		{
			OnDownloadEvent (static_cast<UpdateManager::DownloadEvent> (msg.GetWParam ()));
		}
		else if (_msgDownloadProgress == msg)
		{
			OnDownloadProgress (msg.GetWParam (), msg.GetLParam ());
		}
		else if (_msgDisplayAlert == msg)
		{
			TheAlertMan.Display ();
			_viewMan->Refresh (AlertLogView);
		}
		else if (_msgActivityChange == msg)
		{
			OnActivityChange ();
		}
		else if (_msgShowWindow == msg)
		{
			DisplayOnTop ();
		}
		else if (_msgTaskbarCreated == msg)
		{
			// show Dispatcher taskbar icon after Explorer dies (see The Taskbar documentation)
			TheFeedbackMan.ReShowTaskbarIcon ();
		}
		else if (_msgStartup == msg)
		{
			OnStartup ();
		}
		else
			status = false;
#if defined (DIAGNOSTIC)
		Win::RegisteredMessage dbgMonStartMsg (Dbg::Monitor::UM_DBG_MON_START);
		Win::RegisteredMessage dbgMonStopMsg (Dbg::Monitor::UM_DBG_MON_STOP);
		if (msg == dbgMonStartMsg)
		{
			Dbg::TheLog.DbgMonAttach ("Dispatcher");
			status = true;
		}
		else if (msg == dbgMonStopMsg)
		{
			Dbg::TheLog.DbgMonDetach ();
			status = true;
		}
#endif
	}
    catch (Win::Exception e)
    {
        TheAlertMan.PostInfoAlert (e);
    }
    catch (...)
    {
		Win::ClearError ();
#if defined DEBUG
        TheOutput.Display ("Unknown error processing registered message.");
#endif
    }
	return status;
}

bool DispatcherController::OnInterprocessPackage (unsigned int msg,
												  char const * package,
												  unsigned int errCode,
												  long & result) throw ()
{
	if (_msgNotification == msg)
	{
		result = 0;
		if (errCode != 0)
		{
			TheAlertMan.PostInfoAlert ("Warning: Lost notification from Code Co-op");
			result = 1;
		}
		else
		{
			unsigned char const * buf = reinterpret_cast<unsigned char const *>(package);
			try
			{
				DispatcherIpcHeader ipcHeader;
				{
					MemoryDeserializer in (buf, sizeof (DispatcherIpcHeader));
					ipcHeader.Deserialize (in, DISPATCHER_IPC_VERSION);
					if (ipcHeader.GetVersion () != DISPATCHER_IPC_VERSION)
					{
						TheAlertMan.PostInfoAlert ("Version mismatch between Code Co-op and Dispatcher");
						result = 1;
						return true;
					}
				}
				MemoryDeserializer in (buf, ipcHeader.GetSize ());
				ipcHeader.Deserialize (in, DISPATCHER_IPC_VERSION);
				TransportHeader txHdr (in);
				DispatcherScript script (in);

				_model->OnCoopNotification (txHdr, script);
				OnCoopProjectChange (); // make dispatcher refresh its project/user lists
			}
			catch (ConfigExpt)
			{
				Reconfigure ();
			}
			catch (Win::Exception e)
			{
				TheAlertMan.PostInfoAlert (e);
			}
			catch (...)
			{}
		}
		return true;
	}
	return false;
}

bool DispatcherController::OnTimer (int timerId) throw ()
{
	ProcessingResult result;
	if (_isBusy)
	{
		if (_coopTimer == timerId)
		{
			_coopTimer.Set (BusyEmailCheckPeriod);
		}
		else if (_maintenanceTimer == timerId)
		{
			_maintenanceTimer.Set (BusyEmailCheckPeriod);
		}
		else if (_overallMaintenanceTimer == timerId)
		{
			dbg << "Overall Maintenance postponed" << std::endl;
			_overallMaintenanceTimer.Set (BusyEmailCheckPeriod);
		}
		else if (_restartTimer == timerId)
		{
			_restartTimer.Set (BusyEmailCheckPeriod);
		}
		else if (_forceDispatchTimer == timerId)
		{
			_forceDispatchTimer.Set (ProcessPeriod);
		}

		dbg << "Timer: Dispatcher is busy." << std::endl;
		return true; // wait for next timer message
	}
	//// DebugOut ("Timer: ");
	ReentranceLock lock (_isBusy);
	Cursor::Holder working (_hourglass);

	try
	{
		RefreshProjectListIfNecessary ();

		if (_mailboxTimer == timerId)
		{
			// stop timer while processing mailboxes
			_mailboxTimer.Kill ();
			try
			{
				// Work done here!
				//// DebugOut ("check mailboxes\n");
				result = MailboxCheck ();
			}
			catch (ConfigExpt)
			{
				Reconfigure ();
				return true;
			}
		}
		else if (_coopTimer == timerId)
		{
			// stop timer while processing
			_coopTimer.Kill ();
			if (!_model->OnCoopTimer ())
			{
				_coopTimerPeriod *= 3;
				if (_coopTimerPeriod > 10 * 60 * 1000) // 10 minutes
					_coopTimerPeriod = 10 * 60 * 1000;
				
				_coopTimer.Set (_coopTimerPeriod);
			}
		}
		else if (_maintenanceTimer == timerId)
		{
			_maintenanceTimer.Set (MaintenancePeriod);
			_model->OnMaintenance (false);
			RefreshUI ();
		}
		else if (_overallMaintenanceTimer == timerId)
		{
			dbg << "=Overall Maintenance!=" << std::endl;
			_overallMaintenanceTimer.Set (OverallMaintenancePeriod);
			_updateMan->CheckForUpdate (false);
			_model->OnMaintenance (true);
		}
		else if (_restartTimer == timerId)
		{
			dbg << "=== Restarting Dispatcher ===" << std::endl;
			_model->ShutDown ();
			Win::CommandLine line;
			int errCode = ShellMan::Execute (Win::Dow::Handle (), 
				line.GetExePath ().c_str (), 
				"-restart");
			if (errCode != -1)
			{
				std::string msg ("Cannot restart the Dispatcher for maintenance.\n\n");
				msg += "Make sure the following path is correct:\n\n";
				msg += line.GetExePath ();
				TheAlertMan.PostInfoAlert (msg);
				_model->KeepAlive ();
			}
			else
			{
				GetWindow ().Destroy (); // terminate this instance
				return true;
			}
		}
		else if (_forceDispatchTimer == timerId)
		{
			_forceDispatchTimer.Kill ();
			_model->ClearCurrentDispatchState ();
			_model->Dispatch (false); // do not force scattering
			RefreshUI ();
		}
		else if (_doubleClickTimer == timerId)
		{
			OnLButtonSingleClk ();
		}
		else if (_trackUserTimer == timerId)
		{
			if (_userTracker.get () != 0 && _userTracker->HasBeenBusy ())
			{
				_trackUserTimer.Kill ();
				if (_updateMan->IsAlerting ())
				{
					TheAlertMan.PostUpdateAlert (_updateMan->GetNewUpdateInfo ());
				}
				_userTracker.reset ();
			}
		}
		else if (_activityTimer == timerId)
		{
			OnActivityTimer ();
		}
	}
    catch (Win::ExitException e)
    {
		TheOutput.Display (e);
       _h.Destroy ();
    }
    catch (Win::Exception e)
    {
		result.SetDispatchingDone (false);
        if (_isExecutingUserCmd)
			TheOutput.Display (e);
		else
			TheAlertMan.PostInfoAlert (e);
    }
    catch (...)
    {
		result.SetDispatchingDone (false);
		Win::ClearError ();
#if defined DEBUG
        TheOutput.Display ("Unknown error when processing timer notification");
#endif
    }

    if (!result.IsDispatchingDone ())
	{
		DoubleMailboxTimerPeriod ();
	}
	if (result.HasNewRequestsForCoop ())
	{
		// reset coop timer
		_coopTimerPeriod = ProcessPeriod;
		_coopTimer.Set (_coopTimerPeriod);
	}

	return true;
}

void DispatcherController::RefreshProjectListIfNecessary ()
{
	if (_enlistmentListChanged)
	{
		_model->EnlistmentListChanged ();
	}
	else if (_enlistmentEmailChanged) // refreshing enlistment list also updates emails
	{								  // so there is no need to update them again
		_model->EnlistmentEmailChanged ();
	}
	_enlistmentListChanged  = false;
	_enlistmentEmailChanged = false;
}

class FlagResetter
{
public:
	FlagResetter (bool & flag)
		: _flag (flag)
	{}
	~FlagResetter ()
	{
		_flag = false;
	}
private:
	bool & _flag;
};

// return true if mailboxes processed successfully
ProcessingResult DispatcherController::MailboxCheck ()
{
	FlagResetter userAction (_isExecutingUserCmd); // may be invoked in response to balloon click
	ProcessingResult result = _model->ProcessInbox (false); // do not force scattering
	RefreshUI ();
	return result;
}

void DispatcherController::OnFolderChange (FWatcher * watcher)
{
    // we are setting timer here to avoid sharing violation problem
    // it may happen that we receive the message before the application 
    // that created/changed some files in the mailbox folder closes the files
	//// DebugOut (folderPath);
	//// DebugOut ("\n");
	if (_model->OnFolderChange ())
	{
		ResetMailboxTimer ();
	}
}

void DispatcherController::OnCoopProjectStateChange (int projId, bool isNewMissing) throw ()
{
	dbg << "Dispatcher, OnCoopProjectChange, project id: " << projId << ", isNewMissing: " << isNewMissing << std::endl;
	if (isNewMissing)
	{
		Registry::UserDispatcherPrefs dispatcherPrefs;
		unsigned long delayMinutes;
		dispatcherPrefs.GetResendDelay (delayMinutes);
		// Revisit: if user selected long delay before requesting missing script and the timer is used also to some other
		// Dispatcher processing then the other processing will be disrupted.
		_overallMaintenanceTimer.Set (delayMinutes * 60 * 1000); // kick co-op after user selected delay to process resend request
	}
	if (_model->OnProjectStateChange (projId))
		RefreshUI ();
}

void DispatcherController::OnCoopProjectChange () throw ()
{ 
	// cannot directly call OnEnlistmentListChange because
	// Dispatcher may be hanging on a dialog in the middle of script processing
	// and we must not change internal data in this case
    _enlistmentListChanged = true;
	ResetMailboxTimer ();
}

void DispatcherController::JoinProject (bool onlyAsObserver)
{
	JoinProjectData joinData (_model->GetCatalog (), onlyAsObserver);
	if (!_model->IsConfigured ())
	{
		if (!RunStandaloneJoinWizard (joinData))
			return;
	}

	if (DisplayJoinProjectSheet (joinData))
	{
		ExecuteProjectJoin (joinData);
	}
}

// return false if user canceled join wizard or configuration wizard
bool DispatcherController::RunStandaloneJoinWizard (JoinProjectData & joinData)
{
	NocaseSet projects;
	_model->GetActiveProjectList (projects);

	StandaloneJoinHandlerSet hndlrSet (joinData, projects);
	if (!ThePrompter.GetWizardData (hndlrSet))
		return false;	// user canceled join 

	Assert (joinData.IsValid () || joinData.ConfigureFirst ());
	if (joinData.ConfigureFirst ())
	{
		try
		{
			_commander->Program_ConfigWizard ();
		}
		catch (ConfigExpt)
		{
			Reconfigure ();
		}
		if (!_model->IsConfigured ())
			return false; // user canceled reconfiguration
	}
	return true;
}

bool DispatcherController::DisplayJoinProjectSheet (JoinProjectData & joinData) const
{
	NocaseSet projects;
	_model->GetActiveProjectList (projects);
	NocaseMap<Transport> hubs;
	if (!joinData.IsInvitation ())
	{
		for (HubListSeq seq (_model->GetCatalog ()); !seq.AtEnd (); seq.Advance ())
		{
			std::string hubId;
			Transport transport;
			seq.GetHubEntry (hubId, transport);
			hubs [hubId] = transport;
		}
	}
	JoinProjectHndlrSet ctrlSet (joinData,
								 projects,
								 hubs,
								 _model->GetLocalRecipients (),
								 _model->GetTopology ().IsPeer ());
	return ThePrompter.GetSheetData (ctrlSet);
}

void DispatcherController::ExecuteProjectJoin (JoinProjectData & joinData) const
{
	dbg << "--> DispatcherController::ExecuteProjectJoin" << std::endl;
	Assert (joinData.IsValid ());
	if (joinData.IsRemoteAdmin ())
		_model->AddRemoteHub (joinData.GetAdminHubId (), joinData.GetAdminTransport ());

	std::string cmd (CoopExeName);	// Executable name will be stripped from the
									// command line passed to the co-op.exe
	cmd += " -GUI -Project_Join ";
	cmd += joinData.GetNamedValues ();
	dbg << "Command line: " << cmd << std::endl;
	Win::ChildProcess coop (cmd.c_str (), false);	// Don't inherit parent's handles
	FilePath installPath (Registry::GetProgramPath ());
	coop.SetAppName (installPath.GetFilePath (CoopExeName));
	coop.ShowNormal ();
	coop.Create ();
	dbg << "<-- DispatcherController::ExecuteProjectJoin" << std::endl;
}

void DispatcherController::OnCoopJoinRequest (bool onlyAsObserver)
{
	dbg << "Creating join request" << std::endl;
	DisplayOnTop ();
	JoinProject (onlyAsObserver);
	OnClose ();
	dbg << "Done with join request" << std::endl;
}

void DispatcherController::DisplayOnTop ()
{
	_viewMan->ShowWindow ();
    _h.Restore ();
	_h.SetTopMost (true);
	_h.SetFocus ();
	// Remove the top most window attribute otherwise the window will stay-on-top
	_h.SetTopMost (false);
}

void DispatcherController::OnCoopUpdate (bool isConfigure)
{
	try
	{
		if (isConfigure)
		{
			_updateMan->Configure ();
			if (!_updateMan->UsesAlerts ())
				ResetUpdatesAlert ();
		}
		else
		{
			// may be invoked by Co-op or Dispatcher
			_updateMan->CheckForUpdate (true);
		}
	}
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
	}
	catch (...)
	{
		Win::ClearError ();
#if !defined NDEBUG
		TheOutput.Display ("Internal Error: Unknown error in the update process.", Out::Error);
#endif

	}
}

void DispatcherController::OnDownloadProgress (int bytesTotal, int bytesTransferred)
{
	try
	{
		_updateMan->OnDownloadProgress (bytesTotal, bytesTransferred);
	}
	catch (Win::Exception e)
	{
		TheAlertMan.PostInfoAlert (e);
	}
	catch (...)
	{
		Win::ClearError ();
#if !defined NDEBUG
		TheOutput.Display ("Internal Error: Unknown error in the update process.", Out::Error);
#endif
	}
}

void DispatcherController::OnDownloadEvent (UpdateManager::DownloadEvent event)
{
	try
	{
		bool isNewUpdate = false;
		bool wasUserNotified = false;
		if (_updateMan->OnDownloadEvent (event, isNewUpdate, wasUserNotified))
		{
			Assert (_updateMan->UsesAlerts ());
			if (isNewUpdate)
			{
				SetUpdatesAlert (wasUserNotified);
			}
			else
			{
				ResetUpdatesAlert ();
			}
		}
	}
	catch (Win::Exception e)
	{
		TheAlertMan.PostInfoAlert (e);
	}
	catch (...)
	{
		Win::ClearError ();
#if !defined NDEBUG
		TheOutput.Display ("Internal Error: Unknown error in the update process.", Out::Error);
#endif

	}
}

void DispatcherController::OnActivityChange ()
{
	if (TheFeedbackMan.IsDispatchingActivity ())
	{
		_activityTimer.Set (ActivityPeriod);
	}
	TheFeedbackMan.ShowActivity ();
}

void DispatcherController::OnActivityTimer ()
{
	if (TheFeedbackMan.IsDispatchingActivity ())
	{
		_activityTimer.Set (ActivityPeriod);
		TheFeedbackMan.Animate ();
	}
	else
	{
		_activityTimer.Kill ();
	}
}

void DispatcherController::SetUpdatesAlert (bool wasUserNotified)
{
	Assert (_updateMan->IsAlerting ());
	TheFeedbackMan.SetUpdateReady (_updateMan->GetNewUpdateInfo ());
	if (wasUserNotified)
		return;

	// user not notified -- display balloon tip
	_userTracker.reset (new UserTracker ());
	if (_userTracker->CanTrack ())
	{
		_trackUserTimer.Set (TrackUserPeriod);
	}
	else
	{
		_userTracker.reset ();
	}
}

void DispatcherController::ResetUpdatesAlert ()
{
	_userTracker.reset ();
	_trackUserTimer.Kill ();
	TheFeedbackMan.ResetUpdateReady ();
}

// TaskbarIconNotifyHandler interface
void DispatcherController::OnLButtonDown ()
{
	// To increase responsiveness of the taskbar icon events we want to handle single left clicks.
	// Make sure it is a single click, not double click, and respond to user (display context menu)
	// Double-clicking the left mouse button actually generates a sequence of four messages:
	// WM_LBUTTONDOWN, WM_LBUTTONUP, WM_LBUTTONDBLCLK, and WM_LBUTTONUP
	// Delay interpreting WM_LBUTTONDOWN to see if WM_LBUTTONDBLCLK follows.
	// After the first WM_LBUTTONDOWN, set a timer,
	// after the timeout elapses, OnAssuredLButtonDown is called
	_isWaitingForDblClk = true;
	_doubleClickTimer.Set (Mouse::GetDoubleClickTime ());
}

void DispatcherController::OnLButtonSingleClk ()
{
	_doubleClickTimer.Kill ();
	if (_isWaitingForDblClk)
	{
		_isWaitingForDblClk = false;
		if (TheAlertMan.IsAlerting ())
		{
			HandleAlert ();
		}
		else if (_model->HasInvitation())
		{
			FlagSignaller userAction (_isExecutingUserCmd);
			_model->ReleaseInvitationFromQuarantine();
		}
		else
		{
			_viewMan->DisplayTaskbarIconMenu ();
		}
	}
}

void DispatcherController::OnRButtonDown ()
{
	_viewMan->DisplayTaskbarIconMenu ();
}

void DispatcherController::OnLButtonDblClk ()
{
	_isWaitingForDblClk = false;
	if (TheAlertMan.IsAlerting ())
	{
		HandleAlert ();
	}
	else if (_model->HasInvitation())
	{
		FlagSignaller userAction (_isExecutingUserCmd);
		_model->ReleaseInvitationFromQuarantine();
	}
	else if (_updateMan->IsAlerting ())
	{
		if (_updateMan->OnAlertOpen ())
			ResetUpdatesAlert ();
	}
	else
	{
		DisplayOnTop ();

		ViewType view = PublicInboxView;
		if (_model->HasQuarantineScripts ())
		{
			view = QuarantineView;
		}
		else if (!_model->IsAlertLogEmpty ())
		{
			view = AlertLogView;
		}
		
		_viewMan->SelectTab (view);
	}
}

void DispatcherController::OnKeySelect ()
{
	// a user selects a notify icon with the keyboard and 
	// activates it with the SPACEBAR or ENTER key
	if (_updateMan->IsAlerting ())
	{
		if (_updateMan->OnAlertOpen ())
			ResetUpdatesAlert ();
	}
	else
		_viewMan->DisplayTaskbarIconMenu ();
}

void DispatcherController::OnBalloonUserClk ()
{
	// when a user performs some mouse action on the icon during displaying balloon
	// we obtain two notifications:
	// - first BalloonUserClick
	// - then the actual mouse action (LButtonDown, etc.)
	// We don't want to handle both the actions, just the balloon click.
	// set timer here, to pass handling to OnLButtonSingleClk or OnLButtonDblClk
	_isWaitingForDblClk = true;
	_doubleClickTimer.Set (Mouse::GetDoubleClickTime ());
}

void DispatcherController::HandleAlert ()
{
	Assert (TheAlertMan.IsAlerting ());
	bool isLowPriority = false;
	AlertMan::AlertType alert = TheAlertMan.PopAlert (isLowPriority);
	if (alert == AlertMan::Quarantine)
	{
		_model->ReleaseAllFromQuarantine ();
		_viewMan->Refresh (QuarantineView);
		_isExecutingUserCmd = true;
		_mailboxTimer.Set (ImmediateProcessPeriod);
	}
	else if (alert == AlertMan::Update)
	{
		if (_updateMan->IsAlerting ())
		{
			if (_updateMan->OnAlertOpen ())
				ResetUpdatesAlert ();
		}
	}
	else
	{
		Assert (alert == AlertMan::Info);
		OpenMainWindow (AlertLogView);
	}
}

void DispatcherController::OnBalloonTimeout ()
{
	Assert (TheAlertMan.IsAlerting ());
	bool isLowPriority = false;
	AlertMan::AlertType alert = TheAlertMan.PopAlert (isLowPriority);
	if (alert == AlertMan::Quarantine)
	{
		OpenMainWindow (QuarantineView);
	}
	else if (alert == AlertMan::Info)
	{
		if (!isLowPriority)
		{
			OpenMainWindow (AlertLogView);
		}
	}
	else
	{
		Assert (alert == AlertMan::Update);
	}
}

void DispatcherController::OpenMainWindow (ViewType view)
{
	DisplayOnTop ();
	_viewMan->SelectTab (view);
}

void DispatcherController::Reconfigure ()
{
	_model->Reconfigure (_msgPrepro);

	_curRetryPeriod = ProcessPeriod;
	_mailboxTimer.Set (ImmediateProcessPeriod);
}

void DispatcherController::DoubleMailboxTimerPeriod ()
{
	// Double the wait
	_curRetryPeriod *= 2;
	if (_curRetryPeriod > MaxProcessPeriod)
		_curRetryPeriod = MaxProcessPeriod;
    _mailboxTimer.Set (_curRetryPeriod);
}

void DispatcherController::ResetMailboxTimer () throw ()
{
	_curRetryPeriod = ProcessPeriod;
	_mailboxTimer.Set (_curRetryPeriod);
}

// DispatcherCmdExecutor interface
void DispatcherController::AddDispatcherLicense (std::string const & licensee, 
												 unsigned startNum, 
												 unsigned count)
{
	try
	{
		_model->AddDistributorLicense (licensee, startNum, count);
	}
	catch (Win::Exception const & e)
	{
		TheAlertMan.PostInfoAlert (e);
	}
}

Tri::State DispatcherController::AddAskInvitedClusterRecipient (Invitee const & invitee)
{
	Assert (_model->IsHub ());
	Assert (IsNocaseEqual (invitee.GetHubId (), _model->GetConfigData ().GetHubId ()));
	FilePath satPath = _model->FindSatellitePath (invitee.GetComputerName ());
	if (satPath.IsDirStrEmpty ())
	{
		NocaseSet offsiteSats;
		_model->GetActiveOffsiteSatList (offsiteSats);
		InviteSatData data (invitee, offsiteSats);
		InviteSatCtrl ctrl (data);
		if (ThePrompter.GetData (ctrl, "Invitation: unknown satellite path"))
		{
			if (data.IsRemove ())
				return Tri::No;
			
			satPath.Change (data.GetPath ());
		}
		else
			return Tri::Maybe;
	}
	_model->AddClusterRecipient (invitee, satPath);
	return Tri::Yes;
}

Tri::State DispatcherController::InviteToProject (
				std::string const & adminName,
				std::string const & adminEmailAddress, 
				Invitee const & invitee)
{
	Assert (IsNocaseEqual (_model->GetConfigData ().GetHubId (), invitee.GetHubId ()));
	JoinProjectData joinData (_model->GetCatalog (), invitee.IsObserver (), true); // is invitation
	joinData.GetProject ().SetProjectName (invitee.GetProjectName ());
	joinData.SetAdminHubId (adminEmailAddress);
	joinData.SetAdminTransport (Transport (adminEmailAddress));
	joinData.SetRemoteAdmin (!IsNocaseEqual (adminEmailAddress, invitee.GetHubId ()));
	MemberDescription & me = joinData.GetThisUser ();
	me.SetHubId (invitee.GetHubId ());
	me.SetName (invitee.GetUserName ());
	me.SetUserId (invitee.GetUserId ());
	joinData.SetObserver (invitee.IsObserver ());
	joinData.GetOptions ().SetAutoFullSynch (true);
	// Observer is automatically set to auto sync
	joinData.GetOptions ().SetAutoSynch (invitee.IsObserver ());

	bool canExecuteInvitation = false;
	// Check if the auto-initiation handling is enabled
	std::string autoInviteRoot;
	if (Registry::GetAutoInvitationOptions (autoInviteRoot))
	{
		// Auto-invitation handling is enabled
		// Verify auto-invite data
		FilePath projectPath (autoInviteRoot);
		projectPath.DirDown (joinData.GetProject ().GetProjectName ().c_str ());
		if (Project::Blueprint::IsRootPathWellFormed (projectPath))
		{
			if (File::Exists (projectPath.GetDir ()))
			{
				std::string uniquePath = File::CreateUniqueName (projectPath.GetDir ());
				projectPath.Change (uniquePath);
			}
			joinData.GetProject ().SetRootPath (projectPath);
			canExecuteInvitation = true;
		}
	}

	if (!canExecuteInvitation)
	{
		// Try prompting the user
		ProjectInviteCtrl ctrl (joinData, adminName);
		if (ThePrompter.GetData (ctrl, "Invitation: Path to root folder needed"))
		{
			if (ctrl.IsReject ())
				return Tri::No;

			canExecuteInvitation = true;
		}
		else
		{
			// Could not prompt the user - just posted alert
			return Tri::Maybe;
		}
	}

	Assert (canExecuteInvitation);
	_model->AddTempLocalRecipient (invitee);
	ExecuteProjectJoin (joinData);
	_maintenanceTimer.Set (5 * 1000); // make sure the feedback is displayed right away in case of failure
	return Tri::Yes;
}

void DispatcherController::Restart (unsigned timeoutMs)
{
	_restartTimer.Set (timeoutMs);
}

void DispatcherController::RefreshUI ()
{
	_viewMan->RefreshAll ();
	TheFeedbackMan.RefreshScriptCount (_model->GetIncomingCount (), _model->GetOutgoingCount ());
}
