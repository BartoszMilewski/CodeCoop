#if !defined DISPATCHERCTRL_H
#define DISPATCHERCTRL_H
// ---------------------------------
// (c) Reliable Software 1998 - 2007
// ---------------------------------

#include "CmdVector.h"
#include "DispatcherCmd.h"
#include "UserTracker.h"
#include "Update.h"

#include <Win/Controller.h>
#include <Win/Message.h>
#include <Graph/Cursor.h>
#include <Sys/Timer.h>
#include <Com/Com.h>
#include <TriState.h>
#include <Com/TaskBarIcon.h>

namespace Win
{
	class MessagePrepro;
}

namespace Notify
{
	class Handler;
}

class Commander;
class ViewMan;
class Model;
class UpdateManager;
class ProcessingResult;
class FWatcher;
class JoinProjectData;
enum ViewType;

class DispatcherController : public Win::Controller, 
							 public DispatcherCmdExecutor, 
							 public Cmd::Executor,
							 public TaskbarIconNotifyHandler
{
    enum { MailboxProcessTimerId = 1, 
		   CoopTimerId,
		   ForceDispatchTimerId,
		   MaintenanceTimerId,
		   OverallMaintenanceTimerId,
		   RestartTimerId,
		   DoubleClickTimerId,
		   TrackUserTimerId,
		   ActivityTimerId
		 };

    enum { StartupPeriod			= 3 * 1000, // 3 sec
		   ProcessPeriod            = 1500,   // 1.5 sec
		   ImmediateProcessPeriod   = 100,    // 0.1 sec
		   MaxProcessPeriod         = 60 * 60 * 1000, // one hour
		   MaintenancePeriod		= 5 * 60 * 1000, // 5 min
		   OverallMaintenancePeriod = 60 * 60 * 1000, // 0ne hour
		   RestartPeriod			= 24 * 60 * 60 * 1000, // One day
		   NewMissingPeriod			= 10 * 60 * 1000, // 10 min, Revisit: make adjustable
		   BusyEmailCheckPeriod		= 5000,	  // 5 sec
		   TrackUserPeriod			= 10 * 1000, // 10 sec
		   ActivityPeriod			= 500		 // 0.5 sec
		 };
		   
public:
    DispatcherController (Win::MessagePrepro * msgPrepro);
	~DispatcherController ();
	
	Notify::Handler * GetNotifyHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ();
	Control::Handler * GetControlHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ();

	bool OnCreate (Win::CreateData const * create, bool & success) throw ();
    bool OnDestroy () throw ();
	bool OnShutdown (bool isEndOfSession, bool isLoggingOff) throw ();

	bool OnSize (int width, int height, int flag) throw ();
    bool OnClose () throw ();
	bool OnFocus (Win::Dow::Handle winPrev) throw ();
	bool OnShowWindow (bool show) throw ();

    bool OnCommand (int cmdId, bool isAccel) throw ();
	bool OnControl (Win::Dow::Handle control, unsigned id, unsigned notifyCode) throw ();
	bool OnContextMenu (Win::Dow::Handle wnd, int xPos, int yPos) throw ();
	bool OnInitPopup (Menu::Handle menu, int pos) throw ();
	bool OnMenuSelect (int id, Menu::State state, Menu::Handle menu) throw ();
	bool OnTimer (int id) throw ();
	bool OnSettingChange (Win::SystemWideFlags flags, char const * sectionName) throw ();
	bool OnUserMessage (Win::UserMessage & msg) throw ();
	bool OnRegisteredMessage (Win::Message & msg) throw ();
	bool OnInterprocessPackage (unsigned int msg, char const * package, unsigned int errCode, long & result) throw ();

	// DispatcherCmdExecutor (used by script execs)
	void PostForceDispatch () { _forceDispatchTimer.Set (ImmediateProcessPeriod); }
	void AddDispatcherLicense (std::string const & licensee, unsigned startNum, unsigned count);
	Tri::State AddAskInvitedClusterRecipient (Invitee const & invitee);
	Tri::State InviteToProject (std::string const & adminName,
								std::string const & adminEmailAddress, 
								Invitee const & invitee);
	void Restart (unsigned timeoutMs);
	
	// Cmd::Executor
	bool IsEnabled (std::string const & cmdName) const throw ();
	void ExecuteCommand (std::string const & cmdName) throw ();
	void ExecuteCommand (int cmdId) throw ();
	void DisableKeyboardAccelerators (Win::Accelerator * accel) throw () {}
	void EnableKeyboardAccelerators () throw () {}

	// TaskbarIconNotifyHandler
	void OnLButtonDown    ();
	void OnRButtonDown    ();
	void OnLButtonDblClk  ();
	void OnKeySelect      ();
	void OnBalloonUserClk ();
	void OnBalloonTimeout ();

private:
    void OnStartup ();
	void Reconfigure ();
	void MenuCommand (int cmdId);
	void RefreshUI ();

	// user message handlers
	void OnFolderChange (FWatcher * watcher);
	void OnDownloadProgress (int bytesTotal, int bytesTransferred);
	void OnDownloadEvent (UpdateManager::DownloadEvent event);
	void OnActivityChange ();
	void OnActivityTimer ();
	void SetUpdatesAlert (bool wasUserNotified);
	void ResetUpdatesAlert ();
    void OnCoopProjectChange ();
	void OnCoopUpdate (bool isConfigure);
	void OnCoopProjectStateChange (int projId, bool isNewMissing) throw ();
    void OnCoopJoinRequest (bool onlyAsObserver);
	// aux
	void OnLButtonSingleClk (); // taskbar icon helper
	void HandleAlert ();
	void JoinProject (bool onlyAsObserver);
	bool RunStandaloneJoinWizard (JoinProjectData & joinData);
	bool DisplayJoinProjectSheet (JoinProjectData & joinData) const;
	void ExecuteProjectJoin (JoinProjectData & joinData) const;
	void ResetMailboxTimer () throw ();
	void DoubleMailboxTimerPeriod ();
	ProcessingResult MailboxCheck ();
	void RefreshProjectListIfNecessary ();
	void DisplayOnTop ();
	void OpenMainWindow (ViewType view);
private:
	Win::MessagePrepro * _msgPrepro; // Dispatcher's message pump

	std::unique_ptr<Model>	     _model;
	std::unique_ptr<Commander>	 _commander;
	std::unique_ptr<CmdVector>	 _cmdVector;
    std::unique_ptr<ViewMan>		 _viewMan;
	std::unique_ptr<UpdateManager> _updateMan;
	// WM_NOTIFY notifications handlers
	std::unique_ptr<Notify::Handler>	_viewHandler;
	std::unique_ptr<Notify::Handler>	_tabHandler;
	std::unique_ptr<Notify::Handler>	_toolTipHandler;

	Win::RegisteredMessage	_msgStartup;
	Win::RegisteredMessage	_msgProjectChanged;
	Win::RegisteredMessage	_msgProjectStateChange;
	Win::RegisteredMessage	_msgChunkSizeChange;
	Win::RegisteredMessage	_msgJoinRequest;
	Win::RegisteredMessage	_msgCoopUpdate;
	Win::RegisteredMessage	_msgDownloadEvent;
	Win::RegisteredMessage	_msgDownloadProgress;
	Win::RegisteredMessage	_msgNotification;
	Win::RegisteredMessage	_msgDisplayAlert;
	Win::RegisteredMessage	_msgActivityChange;
	Win::RegisteredMessage	_msgShowWindow;
	Win::RegisteredMessage	_msgTaskbarCreated;
	Win::UserMessage		_msgConfigWizard;

	Win::Timer			_mailboxTimer;
	Win::Timer			_coopTimer;
	Win::Timer			_maintenanceTimer;
	Win::Timer			_overallMaintenanceTimer;
	Win::Timer			_restartTimer;
	Win::Timer			_forceDispatchTimer;
	Win::Timer			_doubleClickTimer;
	Win::Timer			_trackUserTimer;
	Win::Timer			_activityTimer;

	std::unique_ptr<UserTracker> _userTracker;
	bool					   _isWaitingForDblClk;

	Cursor::Hourglass   _hourglass;

	bool				_isBusy;		// true, when in the middle of arbitrary activity
										// for now: processing a timer message or executing a command
	bool				_isExecutingUserCmd;	// true, when executing a command on user action
	int					_curRetryPeriod;
	int					_coopTimerPeriod;

    bool				_enlistmentEmailChanged; // enlistment email changed by Co-Op
	bool				_enlistmentListChanged;  // enlistment list changed by Co-Op
};

#endif
