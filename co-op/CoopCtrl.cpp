//------------------------------------
//  (c) Reliable Software, 1996 - 2008
//------------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "CoopCtrl.h"
#include "Model.h"
#include "Commander.h"
#include "UiManager.h"
#include "AppInfo.h"
#include "Prompter.h"
#include "OutputSink.h"
#include "AccelTab.h"
#include "CmdLineSelection.h"
#include "Global.h"
#include "CoopMsg.h"
#include "RegKeys.h"
#include "Registry.h"
#include "PathRegistry.h"
#include "RandomUniqueName.h"
#include "PromptMode.h"
#include "CommonUserMsg.h"
#include "ProjectMarker.h"
#include "CoopBlocker.h"
#include "resource.h"

#include <Dbg/Out.h>
#include <Dbg/Log.h>
#include <Ctrl/Accelerator.h>
#include <Ctrl/ProgressDialog.h>
#include <Win/MsgLoop.h>
#include <Com/DragDrop.h>
#include <Win/Utility.h>
#include <Win/Atom.h>

extern void UnitTest () throw ();

class ExecutionStateMemento
{
public:
	ExecutionStateMemento (Model * model, FeedbackManager * feedback)
		: _busy (feedback),
		  _wasInProject (model->IsInProject ())
	{
		dbg << "    Execution State Memento: ";
		Directory & folder = model->GetDirectory ();
		// Create new selection manager for inter-process commands
		_selectMan.reset (new CmdLineSelection (*model, folder));
		// Don't watch folder changes during external command execution
		folder.SetWatchDuty (false);
		if (_wasInProject)
		{
			dbg << "- is in the project";
			_folderId = folder.GetCurrentId ();
		}
		else
		{
			dbg << "- is NOT in the project";
			_folderId = gidInvalid;
		}
		dbg << std::endl;
		// Don't prompt during external command execution
		TheOutput.SetVerbose (false);
		model->SetQuickVisit (true);
	}
	SelectionManager * GetNewSelectMan () const { return _selectMan.get (); }
	GlobalId GetFolderId () const { return _folderId; }
	bool WasInProject () const { return _wasInProject; }

private:
	BusyIndicator	_busy;
	GlobalId		_folderId;
	bool			_wasInProject;
	std::unique_ptr<SelectionManager>	_selectMan;
};

class IsEqual : public std::unary_function<FolderRequest const &, bool>
{
public:
	IsEqual (FolderRequest const & event)
		: _event (event)
	{}
	bool operator () (FolderRequest const & event) const
	{
		return event.IsEqual (_event);
	}
private:
	FolderRequest const & _event;
};

//------------------------------
// Co-op Controller
//------------------------------

CoopController::CoopController (Win::MessagePrepro & msgPrepro, int startupProjectId)
	: _msgPrepro (msgPrepro),
	  _retryTimer (RetryTimerId),
	  _needRefresh (false),
	  _isOperational (false),
	  _curRetryPeriod (0),
	  _msgStartup (UM_STARTUP),
	  _msgIpcInitiate (UM_IPC_INITIATE),
	  _msgIpcTerminate (UM_IPC_TERMINATE),
	  _msgIpcAbort (UM_IPC_ABORT),
	  _msgProjectStateChange (UM_COOP_PROJECT_STATE_CHANGE),
	  _msgAutoMergeCompleted (UM_AUTO_MERGER_COMPLETED),
	  _msgCommand (UM_COOP_COMMAND),
	  _startupProjectId (startupProjectId)
{}

CoopController::~CoopController ()
{}

bool CoopController::OnCreate (Win::CreateData const * create, bool & success) throw ()
{
#if defined (DIAGNOSTIC)
	try
	{
 #if 0	// Uncomment this to turn on logging to the file. You may change the path.
		FilePath userDesktopPath;
		ShellMan::VirtualDesktopFolder userDesktop;
		userDesktop.GetPath (userDesktopPath);
		std::string msg("Starting the log in:\n\n");
		msg += userDesktopPath.GetDir ();
		TheOutput.Display(msg.c_str());
		if (!Dbg::TheLog.IsOn ()) 
			Dbg::TheLog.Open ("CoopLog.txt", userDesktopPath.GetDir ());
 #endif // log to file

		// By default we log to debug monitor window 
		Dbg::TheLog.DbgMonAttach ("GUI Code Co-op");
	}
	catch (Win::Exception e)
	{
		TheOutput.DisplayException(e, _h, "Code Co-op");
	}
	catch ( ... )
	{
		TheOutput.Display("Unknown error opening the log");
		// Ignore all errors
	}
#endif
	success = false;
	try
	{
		Win::ClearError ();
		TheAppInfo.SetWindow (_h);
		ThePrompter.SetWindow (_h);
		ThePrompter.AddUncriticalSect (_critSect);
		TheOutput.AddUncriticalSect (_critSect);

		// Create the model
		_model.reset (new Model);
		// Create commander
		_commander.reset (new Commander (*_model,
										 &_critSect,
										 0,			// InputSource
										 _msgPrepro,// Message preprocessor
										 _h));		// Revisit: top app window
		// Create command vector
		_cmdVector.reset (
			new CmdVector (Cmd::Table, _commander.get ()));
		// Create IPC queue
		CommandIpc::Context context (*_commander,
									 *_cmdVector,
									 _critSect, 
									 *_model); // feedback
		_ipcQueue.reset (new CommandIpc::Queue (_h, context));
		// Create the view manager
		_uiMan.reset (new UiManager (_h, *_model, *_cmdVector, *this));
		_commander->ConnectGUI (_uiMan.get (), _uiMan.get ());
		_model->SetUIFeedback (_uiMan->GetFeedback ());
		Accel::Maker accelMaker (Accel::Keys, *_cmdVector);
		_kbdAccel.reset (new Accel::Handler (_h, accelMaker.Create ()));
		_retryTimer.Attach (_h);
		EnableKeyboardAccelerators ();
		_uiMan->AttachMenu2Window (_h);

		_h.PostMsg (_msgStartup);

		TheOutput.SetParent (_h);
		success = true;
	}
	catch (Win::Exception e)
	{
		// Revisit: this doesn't work
		// after displaying the dialog, we are called with OnFocus
		// even though we are still in OnCreate. OnFocus GP-faults
		TheOutput.Display (e);
	}
	catch (std::bad_alloc)
	{
		TheOutput.Display ("Initialization failed: Out of memory");
	}
	catch (...)
	{
		Win::ClearError ();
		TheOutput.Display ("Initialization failed: Unknown Error", Out::Error);
	}
	return true;
}

bool CoopController::OnDestroy () throw ()
{
	if (_isOperational) // in case window is destroyed from withing OnCreate
	{
		try
		{
			// Debug output logging
			//Dbg::TheLog.Close ();
			_commander->SaveState ();
			_uiMan->OnDestroy ();
			// Destroy UI manager, but first zero the unique_ptr
			UiManager * tmp = _uiMan.release ();
			delete tmp;
			// Check if we have any folders to delete on the notification list 
			std::list<FolderRequest>::const_iterator iter = _folderChangeRequests.begin ();
			for (; iter != _folderChangeRequests.end (); ++iter)
			{
				if (iter->IsDeleteFolder ())
				{
					ShellMan::QuietDelete (_h, iter->GetPath ().GetDir ());
				}
			}
		}
		catch (...)
		{
			Win::ClearError ();
		}
	}
	Win::Quit ();
	return true;
}

void CoopController::OnStartup ()
{
	dbg << "--> CoopController::OnStartup" << std::endl;
	Assert (!_isOperational);
	if (!_commander->HasProgramExpired ())
	{
		_uiMan->OnStartup ();
		BackupMarker backupMarker;
		if (backupMarker.Exists ())
		{
			if (_model->IsQuickVisit ())
			{
				// Don't continue recovery in server mode and
				// don't allow server to run until recovery is completed.
				_h.Destroy ();
				return;
			}
			Out::Answer userChoice = TheOutput.Prompt (
				"Code Co-op hasn't been fully restored from backup.\n"
				"It can either continue project recovery or shut down.\n\n"
				"Do you want to continue?",
				Out::PromptStyle (Out::YesNo, Out::Yes, Out::Question),
				_h);
			if (userChoice == Out::No)
			{
				// Don't allow GUI Code Co-op to run until recovery is completed.
				_h.Destroy ();
				return;
			}
			// Continue recovery
			Progress::MeterDialog blockerMeter ("Restore From Backup Archive",
												_h,
												_msgPrepro,
												true);	// User can cancel
			blockerMeter.SetCaption ("Waiting for the background Code Co-op to complete its tasks.");
			CoopBlocker coopBlocker (blockerMeter.GetProgressMeter (), _h);
			if (!coopBlocker.TryToHold ())
			{	        
				// Don't allow GUI Code Co-op to run until recovery is completed.
				_h.Destroy ();
				return;
			}
			if (!_commander->DoRecovery (coopBlocker))
			{
				// Don't allow GUI Code Co-op to run until recovery is completed.
				_h.Destroy ();
				return;
			}
		}
		else if (!_model->IsQuickVisit ())
		{
			// No backup marker - check for repair list
			RepairList repairList;
			if (repairList.Exists ())
				_commander->RepairProjects ();
		}
		VerifySccSettings ();
		VerifyDifferMergerRegistry ();
		RestoreState ();
		_uiMan->RefreshPageTabs ();
		_isOperational = true;
#if !defined (NDEBUG) && !defined (BETA)
		UnitTest ();
#endif;
	}
	if (!_isOperational)
		_h.Destroy ();
	dbg << "<-- CoopController::OnStartup" << std::endl;
}

bool CoopController::OnFocus (Win::Dow::Handle winPrev) throw ()
{
	// Switch focus to view
	_uiMan->OnFocus ();
	return true;
}

bool CoopController::OnSize (int width, int height, int flag) throw ()
{
	_uiMan->OnSize (width, height); 
	return true;
}

bool CoopController::OnSettingChange (Win::SystemWideFlags flags, char const * sectionName) throw ()
{
	if (flags.IsNonClientMetrics ())
	{
		_uiMan->NonClientMetricsChanged ();
		return true;
	}
	return false;
}

Notify::Handler * CoopController::GetNotifyHandler (Win::Dow::Handle winFrom, 
													unsigned idFrom) throw ()
{
	if (_uiMan.get ())
		return _uiMan->GetNotifyHandler (winFrom, idFrom);
	return 0;
}

Control::Handler * CoopController::GetControlHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ()
{
	if (_uiMan.get ())
		return _uiMan->GetControlHandler (winFrom, idFrom);
	return 0;
}

bool CoopController::OnCommand (int cmdId, bool isAccel) throw ()
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

bool CoopController::IsEnabled (std::string const & cmdName) const throw ()
{
	return (_cmdVector->Test (cmdName.c_str ()) == Cmd::Enabled);
}

void CoopController::ExecuteCommand (std::string const & cmdName) throw ()
{
	if (IsEnabled (cmdName))
	{
		int cmdId = _cmdVector->Cmd2Id (cmdName.c_str ());
		Assert (cmdId != -1);
		MenuCommand (cmdId);
	}
}

bool CoopController::OnBrowserBackward ()
{
	ExecuteCommand ("Selection_Previous");
	return true;
}

bool CoopController::OnBrowserForward ()
{
	ExecuteCommand ("Selection_Next");
	return true;
}

void CoopController::ExecuteCommand (int cmdId) throw ()
{
	Assert (cmdId != -1);
	MenuCommand (cmdId);
}

// The name of the command is pushed on the queue, followed by pairs of (argName, argValue)
// Command message is then posted; argument count passed in wParam
void CoopController::PostCommand (std::string const & cmdName, NamedValues const & namedArgs)
{
	bool addCmd = true;
	unsigned count = namedArgs.size ();
	if (_cmdQueue.size () != 0)
	{
		// Check if we already have this command in the queue
		CmdData cmdData = _cmdQueue.front ();
		if (cmdData.first == cmdName)
		{
			// Command names match - check argument list
			ArgList const & argList = cmdData.second;
			if (count == argList.size ())
			{
				// The same argument count
				ArgList::const_iterator iter = argList.begin ();
				for ( ; iter != argList.end (); ++iter)
				{
					std::string argValue = namedArgs.GetValue (iter->first);
					if (argValue != iter->second)
					{
						// Different argument values - add new command to the queue
						break;
					}
				}
				if (iter == argList.end ())
					addCmd = false;	// Identical commands - don't add to the queue
			}
		}
	}

	if (addCmd)
	{
		ArgList args;
		for (NamedValues::Iterator it = namedArgs.begin (); it != namedArgs.end (); ++it)
		{
			args.push_back (std::make_pair (it->first, it->second));
		}
		_cmdQueue.push_back (std::make_pair (cmdName, args));
		_h.PostMsg (_msgCommand);
	}
}

bool CoopController::OnInitPopup (Menu::Handle menu, int pos) throw ()
{
	try
	{
		_uiMan->RefreshPopup (menu, pos);
	}
	catch (...) 
	{
		Win::ClearError ();
		return false;
	}
	return true;
}

bool CoopController::OnMenuSelect (int id, Menu::State state, Menu::Handle menu) throw ()
{
	if (state.IsDismissed ())
	{
		// Menu dismissed
		FeedbackManager * feedbackMan = _uiMan->GetFeedback ();
		Assert (feedbackMan != 0);
		feedbackMan->Close ();
	}
	else if (!state.IsSeparator () && !state.IsSysMenu () && !state.IsPopup ())
	{
		// Menu item selected
		char const * cmdHelp = _cmdVector->GetHelp (id);
		FeedbackManager * feedbackMan = _uiMan->GetFeedback ();
		Assert (feedbackMan != 0);
		feedbackMan->SetActivity (cmdHelp);
	}
	else
	{
		return false;
	}
	return true;
}

bool CoopController::OnContextMenu (Win::Dow::Handle wnd, int xPos, int yPos) throw ()
{
	try
	{
		return _uiMan->DisplayContextMenu (_h, wnd, xPos, yPos);
	}
	catch (Win::Exception e)
	{
		Assert (!"Error during displaying context menu.");
		e;
		return false;
	}
	catch ( ... )
	{
		Assert (!"Unknown error during displaying context menu.");
		Win::ClearError ();
		return false;
	}
}

bool CoopController::OnChar (int vKey, int flags) throw ()
{
	std::string badChars (FilePath::IllegalChars);
	if (badChars.find (vKey) == std::string::npos)
		_uiMan->SelectItem (vKey);
	return true;
}

bool CoopController::OnTimer (int id) throw ()
{
	// Prevent re-entrance while executing another command
	// This is possible despite serialization, because
	// Dialog and Message Boxes peek at the message queue
	// and may call Windows Procedure recursively

    if (_retryTimer == id)
	{
		_retryTimer.Kill ();

		if (IsBusy () || _model->IsQuickVisit ())
		{
			_needRefresh = true;
			_retryTimer.Set (_curRetryPeriod);
		}
		else
		{
			BusyIndicator busy (_uiMan->GetFeedback ());
			GuiLock lock (_critSect);
			ProcessNotifications ();
		}
		return true;
	}
	return false;
}

bool CoopController::OnIpcInitiate (Win::Dow::Handle client, 
									std::string const & app, 
									std::string const & topic) throw ()
{
	if (app == CoopClassName)
	{
		TheOutput.Display ("Version mismatch between 'SccDll.dll' and 'Co-op.exe'.\n"
			"Please reinstall Code Co-op.");
	}
	return true;
}

// Warning! Locking in iffy here!
bool CoopController::OnUserMessage (Win::UserMessage & msg) throw ()
{
	if (msg.GetMsg () == UM_FOLDER_CHANGE)
	{
		OnFolderChange (reinterpret_cast<FWatcher *>(msg.GetLParam ()));
		return true;
	} 
	else if (msg.GetMsg () == UM_VSPLITTERMOVE)
	{
		_uiMan->MoveVSplitter (msg.GetLParam (), msg.GetWParam ());
	}
	else if (msg.GetMsg () == UM_HSPLITTERMOVE)
	{
		_uiMan->MoveHSplitter (msg.GetLParam (), msg.GetWParam ());
	}
	else if (msg.GetMsg () == UM_REFRESH_BROWSEWINDOW)
	{
		// This message can be posted by IPC Queue 
		// UpdateBrowseWindow may call the Model
		// Locking is necessary!
		Win::Lock lock (_critSect);
		_uiMan->UpdateBrowseWindow (reinterpret_cast<TableBrowser *>(msg.GetLParam ()));
		return true;
	}
	else if (msg.GetMsg () == UM_REFRESH_VIEWTABS)
	{
		_uiMan->RefreshPageTabs ();
		return true;
	}
	else if (msg.GetMsg () == UM_LAYOUT_CHANGE)
	{
		_uiMan->LayoutChange ();
		return true;
	}
	else if (msg.GetMsg () == UM_CLOSE_PAGE)
	{
		_uiMan->ClosePage (static_cast<ViewPage>(msg.GetWParam ()));
		return true;
	}
	return false;
}

bool CoopController::OnRegisteredMessage (Win::Message & msg) throw ()
{
	try
	{
		return ProcessRegisteredMessage (msg);
	}
	catch (Win::ExitException e)
	{
		TheOutput.Display (e);
		_h.Destroy ();
	}
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
	}
	catch (std::bad_alloc bad)
	{
		TheOutput.Display ("Internal Error: registered message processing - not enough memory.", Out::Error);
		dbg << "<-- " << "Internal Error: registered message processing - not enough memory." << std::endl;
	}
	catch ( ... )
	{
		TheOutput.Display ("Internal Error: registered message processing - execution failure.", Out::Error);
		dbg << "<-- " << "Internal Error: registered message processing - execution failure." << std::endl;
	}
	return false;
}

bool CoopController::ProcessRegisteredMessage (Win::Message & msg)
{
	dbg << "--> CoopController::ProcessRegisteredMessage" << std::endl;
	if (msg == _msgIpcInitiate)
	{
		Win::Lock lock (_critSect);
		RandomUniqueName convName (msg.GetLParam ());
		//dbg << "    IPC exchange buffer id: " << convName.GetString ().c_str () << std::endl;
		SwitchToCmdLine ();
		_ipcQueue->InitiateConv (convName.GetString ());
		dbg << "<-- CoopController::ProcessRegisteredMessage -- INVITE" << std::endl;
		return true;
	}
	else if (msg == _msgIpcTerminate)
	{
		Win::Lock lock (_critSect);
		try
		{
			if (_executionStateMemento.get () != 0 && _ipcQueue->IsIdle ())
			{
				SwitchToGUI (_ipcQueue->StayInGui ());
			}
		}
		catch ( ... )
		{
			// Since GUI is blocked solid, we must terminate
			TheOutput.SetVerbose (true);
			TheOutput.Display ("Error after processing external command. Exiting Code Co-op!");
			Win::Quit ();
		}
		dbg << "<-- CoopController::ProcessRegisteredMessage -- TERMINATE" << std::endl;
		return true;
	}
	else if (msg == _msgIpcAbort)
	{
		return true;
	}
	else if (msg == _msgProjectStateChange)
	{
		Win::Lock lock (_critSect);
		if (_uiMan->IsEmptyPage (ProjectPage))
		{
			// If this is the first project, rebuild UI
			_commander->LeaveProject ();
		}
		else
		{
			_model->OnProjectStateChange ();
			_uiMan->RefreshPageTabs ();
		}
		dbg << "<-- CoopController::ProcessRegisteredMessage -- PROJECT STATE CHANGE" << std::endl;
		return true;
	}
	else if (msg == _msgAutoMergeCompleted)
	{
		Win::Lock lock (_critSect);
		_model->OnAutoMergeCompleted (reinterpret_cast<ActiveMerger const *>(msg.GetLParam ()),
									  msg.GetWParam () != 0); 
		dbg << "<-- CoopController::ProcessRegisteredMessage -- UM_AUTO_MERGER_COMPLETED" << std::endl;
		return true;
	}
	else if (msg == _msgStartup)
	{
		OnStartup ();
		dbg << "<-- CoopController::ProcessRegisteredMessage -- UM_STARTUP" << std::endl;
		return true;
	}
	else if (msg == _msgCommand)
	{
		if (!IsBusy ())
			ExecuteQueuedCommand ();
		dbg << "<-- CoopController::ProcessRegisteredMessage -- UM_COOP_COMMAND" << std::endl;
		return true;
	}
#if defined (DIAGNOSTIC)
	Win::RegisteredMessage dbgMonStartMsg (Dbg::Monitor::UM_DBG_MON_START);
	Win::RegisteredMessage dbgMonStopMsg (Dbg::Monitor::UM_DBG_MON_STOP);
	if (msg == dbgMonStartMsg)
	{
		Dbg::TheLog.DbgMonAttach ("GUI Code Co-op");
		return true;
	}
	else if (msg == dbgMonStopMsg)
	{
		Dbg::TheLog.DbgMonDetach ();
		return true;
	}
#endif
	dbg << "<-- CoopController::ProcessRegisteredMessage" << std::endl;
	return false;
}

void CoopController::ExecuteQueuedCommand ()
{
	if (_cmdQueue.empty ())
		return;
	CmdData cmdData = _cmdQueue.front ();
	_cmdQueue.pop_front ();
	ArgList const & argList = cmdData.second;
	unsigned count = argList.size ();
	for (unsigned i = 0; i < count; ++i)
	{
		ThePrompter.SetNamedValue (argList [i].first, argList [i].second);
	}
	// May recurse!
	ExecuteCommand (cmdData.first);
	ThePrompter.ClearNamedValues ();
}

bool CoopController::OnInterprocessPackage (unsigned int msg, 
											char const * package, 
											unsigned int errCode, 
											long & result) throw ()
{
	return false;
}

void CoopController::SwitchToCmdLine ()
{
	dbg << "--> CoopController::SwitchToCmdLine" << std::endl;
	if (_model->IsQuickVisit ())
	{
		dbg << "<-- CoopController::SwitchToCmdLine -- already in command line mode" << std::endl;
		return;	// Don't create memento if already in command line mode
	}

	//	Do actions that can't fail first
	_uiMan->DisableUI ();

	// Remember execution state
	_executionStateMemento.reset (new ExecutionStateMemento (_model.get (), _uiMan->GetFeedback ()));
	// Switch commander to use new selection manager
	_commander->ConnectGUI (_executionStateMemento->GetNewSelectMan (), _uiMan.get ());

	FeedbackManager * feedbackMan = _uiMan->GetFeedback ();
	Assert (feedbackMan != 0);
	feedbackMan->SetSupplementalActivity("SCC");
	dbg << "<-- CoopController::SwitchToCmdLine" << std::endl;
}

void CoopController::SwitchToGUI (bool bring2Top)
{
	dbg << "--> CoopController::SwitchToGUI" << std::endl;
	Assert (_executionStateMemento.get () != 0);

	//	None of these actions should fail

	Directory & folder = _model->GetDirectory ();
	folder.SetWatchDuty (true);
	TheOutput.SetVerbose (true);
	_model->SetQuickVisit (false);
	FeedbackManager * feedbackMan = _uiMan->GetFeedback ();
	Assert (feedbackMan != 0);
	feedbackMan->SetSupplementalActivity("");
	// Reconnect user interface
	_commander->ConnectGUI (_uiMan.get (), _uiMan.get ());

	if (_executionStateMemento->WasInProject ())
	{
		// Restore folder state
		folder.SetCurrentFolderId (_executionStateMemento->GetFolderId ());
		// Check-in area view and file view are refreshed by notifications
		_uiMan->Refresh (MailBoxPage);
	}
	else
	{
		_commander->LeaveProject ();
	}

	_executionStateMemento.reset (0);	// destroys the memento

	_uiMan->EnableUI ();

	if (bring2Top)
	{
		_h.SetForeground ();
		Win::Placement placement (_h);
		if (placement.IsMinimized ())
			_h.Restore ();
	}
	dbg << "<-- CoopController::SwitchToGUI" << std::endl;
}

// Helpers

void CoopController::VerifySccSettings ()
{
	Registry::KeySccCheck sccProviderCheck;
	// Check ProviderRegKey value
	if (!sccProviderCheck.IsCoopCurrentProvider ())
	{
		// Code Co-op is not the current source code control provider
		PromptMode prompter (IDS_SCC_PROMPT);
		if (prompter.CanPrompt ())
		{
			// Ask the user if we can change the current source code control provider
			if (prompter.Prompt ())
			{
				if (Registry::CanWriteToMachineRegKey ())
				{
					Registry::KeyScc sccProviderKey;
					sccProviderKey.SetCurrentProvider (
						"Software\\Reliable Software\\Code Co-op");
				}
				else
				{
					TheOutput.Display ( 
						"You don't have write access to your computer's registry.\n"
						"Ask computer administrator to run Code Co-op, "
						"so it can setup itself as the default version control provider.");
				}
			}
		}
	}
}

void CoopController::VerifyDifferMergerRegistry ()
{
	Registry::UserDifferPrefs prefs;
	bool isOn;
	if (prefs.IsAlternativeDiffer (isOn))
	{
		if (isOn && !prefs.IsAlternativeDifferValid ())
		{
			TheOutput.Display ("Please, execute Tools>Differ to correct alternative differ settings.");
		}
	}

	if (prefs.IsAlternativeMerger (isOn))
	{
		if (isOn && !prefs.IsAlternativeMergerValid ())
		{
			TheOutput.Display ("Please, execute Tools>Merger to correct merger settings.");
		}
	}
}

void CoopController::RestoreState ()
{
	if (_startupProjectId != -1)
	{
		BusyIndicator busy (_uiMan->GetFeedback ());
		Assert (!IsBusy ());
		GuiLock lock (_critSect);
		_commander->VisitProject (_startupProjectId, false); // don't remember as "last project"
	}
	else if (TheAppInfo.IsFirstInstance () && _cmdQueue.size () == 0)
	{
		// This is the first program instance running and there are no commands posted
		BusyIndicator busy (_uiMan->GetFeedback ());
		Assert (!IsBusy ());
		GuiLock lock (_critSect);
		_commander->RestoreLastProject ();
	}
	else
	{
		// Let the user choose the project to visit
		_commander->Project_Visit ();
	}
}

void CoopController::VerifyProject () throw ()
{
	try
	{
		_model->QuietVerifyProject ();
		// Revisit: should we display verification report ?
		_uiMan->RefreshAll ();
	}
	catch (...)	
	{
		Win::ClearError ();
	}
}

void CoopController::MenuCommand (int cmdId)
{
	dbg << "--> CoopController::MenuCommand" << std::endl;
	if (IsBusy ())
		return;

	Win::ClearError ();
	std::string cmdName (_cmdVector->GetName (cmdId));
	try
	{
		BusyIndicator busy (_uiMan->GetFeedback ());
		{
			GuiLock lock (_critSect);

			{
				dbg << "    Executing command: "<< cmdName << std::endl;
				_cmdVector->Execute (cmdId);
			}

			if (_model->NeedRefresh ())
			{
				_uiMan->RefreshAll ();
				_model->NoNeedRefresh ();
			}

			if (_needRefresh)
			{
				// Process folder events received during command execution
				ProcessNotifications ();
				_needRefresh = false;
			}
			// Command execution may affect UI
			_uiMan->RefreshToolBars ();
			_uiMan->RefreshPageTabs ();
			_uiMan->Refresh (ProjectPage);
			// Command execution may generate folder events
			CheckFolderRequests ();
		}
		// May recurse
		ExecuteQueuedCommand ();
	}
	catch (Win::ExitException e)
	{
		TheOutput.Display (e);
		_h.Destroy ();
	}
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
		VerifyProject ();
	}
	catch (std::bad_alloc bad)
	{
		Win::ClearError ();
		std::string msg ("Internal Error: operation aborted - not enough memory - ");
		msg += cmdName;
		TheOutput.Display (msg.c_str (), Out::Error);
		dbg << "<-- " << msg << std::endl;
		VerifyProject ();
	}
	catch ( ... )
	{
		Win::ClearError ();
		std::string msg ("Internal Error: command execution failure - ");
		msg += cmdName;
		TheOutput.Display (msg.c_str (), Out::Error);
		dbg << "<-- " << msg << std::endl;
		VerifyProject ();
	}
	dbg << "<-- CoopController::MenuCommand" << std::endl;
}

void CoopController::CheckFolderRequests ()
{
	if (_model->HasFolderRequest ())
	{
		// Transfer model's folder change requests to our list
		// and process them on timer
		bool reqAdded = false;
		std::vector<FolderRequest> & req = _model->GetFolderRequests ();
		std::vector<FolderRequest>::const_iterator iter = req.begin ();
		for (; iter != req.end (); ++iter)
		{
			typedef std::list<FolderRequest>::const_iterator Iterator;
			// Add folder event, only if it's not there
			Iterator found = std::find_if ( _folderChangeRequests.begin (),
											_folderChangeRequests.end (),
											IsEqual (*iter));
			if (found == _folderChangeRequests.end ())
			{
				_folderChangeRequests.push_back (*iter);
				reqAdded = true;
			}
		}
		req.clear ();
		if (reqAdded)
		{
			_curRetryPeriod = RetryPeriod;
			_retryTimer.Set (_curRetryPeriod);
		}
	}
}

void CoopController::OnFolderChange (FWatcher * watcher) throw ()
{
	typedef std::list<FolderRequest>::const_iterator Iter;
	try
	{
		std::string folder;
		while (_model->RetrieveFolderChange (folder))
		{
			FolderRequest newEvent (folder);
			// Remember folder change notification, only if it's not already there
			Iter iter = std::find_if (_folderChangeRequests.begin (),
									  _folderChangeRequests.end (),
									  IsEqual (newEvent));
			if (iter == _folderChangeRequests.end ())
			{
				// Transfer to our list and set the timer
				_folderChangeRequests.push_back (newEvent);
				_curRetryPeriod = RetryPeriod;
				_retryTimer.Set (_curRetryPeriod);
			}
		}
	}
	catch (...)
	{
		// ignore all exceptions during notification
		Win::ClearError ();
	}
}

void CoopController::ProcessNotifications () throw ()
{
	// Process changed folders
	try
	{
		dbg << "Processing Folder Notifications" << std::endl;
		std::list<FolderRequest>::iterator iter = _folderChangeRequests.begin ();
		while (iter != _folderChangeRequests.end ())
		{
			if (iter->IsCheckContents ())
			{
				if (_model->FolderChange (iter->GetPath ()))
					_commander->DoRefreshMailbox (true);
				// Checking folder contents may affect UI
				_uiMan->RefreshToolBars ();
				_uiMan->RefreshPageTabs ();
			}
			else
			{
				Assert (iter->IsDeleteFolder ());
				// Revisit: We are using quiet delete in order to avoid infinite
				// notification loop when folder deletion fails.  System tells us
				// only that access has been denied for a specified folder and
				// there is no way to determine the actual problem.
				ShellMan::QuietDelete (_h, iter->GetPath ().GetDir ());
			}
			// Notification processed -- remove it from the list
			// Revisit: should we check if model really processed notification ?
			iter = _folderChangeRequests.erase (iter);
		}
	}
	catch (...)
	{
		// Ignore exceptions
		Win::ClearError ();
		_needRefresh = true;
		// Double the wait
		_curRetryPeriod *= 2;
        _retryTimer.Set (_curRetryPeriod);
	}
}

// Keyboard accelerators

void CoopController::DisableKeyboardAccelerators (Win::Accelerator * accel)
{
	_msgPrepro.SetKbdAccelerator (accel);
}

void CoopController::EnableKeyboardAccelerators ()
{
	_msgPrepro.SetKbdAccelerator (_kbdAccel.get ());
}

void CoopController::SetWindowPlacement (Win::ShowCmd cmdShow, bool multipleWindows)
{
	Assert (_uiMan.get () != 0);
	_uiMan->SetWindowPlacement (cmdShow, multipleWindows);
}

